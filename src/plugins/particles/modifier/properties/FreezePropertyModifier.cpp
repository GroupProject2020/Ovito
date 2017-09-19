///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2017) Alexander Stukowski
//
//  This file is part of OVITO (Open Visualization Tool).
//
//  OVITO is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  OVITO is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////

#include <plugins/particles/Particles.h>
#include <plugins/particles/modifier/ParticleInputHelper.h>
#include <plugins/particles/modifier/ParticleOutputHelper.h>
#include <core/dataset/animation/AnimationSettings.h>
#include <core/dataset/pipeline/ModifierApplication.h>
#include "FreezePropertyModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Properties)


DEFINE_PROPERTY_FIELD(FreezePropertyModifier, sourceProperty, "SourceProperty");
DEFINE_PROPERTY_FIELD(FreezePropertyModifier, destinationProperty, "DestinationProperty");
DEFINE_PROPERTY_FIELD(FreezePropertyModifier, freezeTime, "FreezeTime");
SET_PROPERTY_FIELD_LABEL(FreezePropertyModifier, sourceProperty, "Property");
SET_PROPERTY_FIELD_LABEL(FreezePropertyModifier, destinationProperty, "Destination property");
SET_PROPERTY_FIELD_LABEL(FreezePropertyModifier, freezeTime, "Freeze at frame");
SET_PROPERTY_FIELD_UNITS(FreezePropertyModifier, freezeTime, TimeParameterUnit);


DEFINE_FLAGS_REFERENCE_FIELD(FreezePropertyModifierApplication, property, "Property", ParticleProperty, PROPERTY_FIELD_NEVER_CLONE_TARGET|PROPERTY_FIELD_NO_CHANGE_MESSAGE|PROPERTY_FIELD_NO_UNDO|PROPERTY_FIELD_NO_SUB_ANIM|PROPERTY_FIELD_DONT_SAVE_RECOMPUTABLE_DATA);
DEFINE_FLAGS_REFERENCE_FIELD(FreezePropertyModifierApplication, identifiers, "Identifiers", ParticleProperty, PROPERTY_FIELD_NEVER_CLONE_TARGET|PROPERTY_FIELD_NO_CHANGE_MESSAGE|PROPERTY_FIELD_NO_UNDO|PROPERTY_FIELD_NO_SUB_ANIM|PROPERTY_FIELD_DONT_SAVE_RECOMPUTABLE_DATA);
DEFINE_FLAGS_VECTOR_REFERENCE_FIELD(FreezePropertyModifierApplication, cachedDisplayObjects, "CachedDisplayObjects", DisplayObject, PROPERTY_FIELD_NEVER_CLONE_TARGET|PROPERTY_FIELD_NO_CHANGE_MESSAGE|PROPERTY_FIELD_NO_UNDO|PROPERTY_FIELD_NO_SUB_ANIM);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
FreezePropertyModifier::FreezePropertyModifier(DataSet* dataset) : Modifier(dataset), 
	_freezeTime(0)
{



}

/******************************************************************************
* Asks the modifier whether it can be applied to the given input data.
******************************************************************************/
bool FreezePropertyModifier::OOMetaClass::isApplicableTo(const PipelineFlowState& input) const
{
	return input.findObject<ParticleProperty>() != nullptr;
}

/******************************************************************************
* Create a new modifier application that refers to this modifier instance.
******************************************************************************/
OORef<ModifierApplication> FreezePropertyModifier::createModifierApplication()
{
	OORef<ModifierApplication> modApp = new FreezePropertyModifierApplication(dataset());
	modApp->setModifier(this);
	return modApp;
}

/******************************************************************************
* This method is called by the system when the modifier is being inserted
* into a pipeline.
******************************************************************************/
void FreezePropertyModifier::initializeModifier(ModifierApplication* modApp)
{
	Modifier::initializeModifier(modApp);

	// Use the first available particle property from the input state as data source when the modifier is newly created.
	if(sourceProperty().isNull()) {
		PipelineFlowState input = modApp->evaluateInputPreliminary();
		for(DataObject* o : input.objects()) {
			if(ParticleProperty* property = dynamic_object_cast<ParticleProperty>(o)) {
				setSourceProperty(ParticlePropertyReference(property));
				setDestinationProperty(sourceProperty());
				break;
			}
		}
	}
}

/******************************************************************************
* Modifies the input data.
******************************************************************************/
Future<PipelineFlowState> FreezePropertyModifier::evaluate(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input)
{
	// Check if we already have the frozen property available.
	if(FreezePropertyModifierApplication* myModApp = dynamic_object_cast<FreezePropertyModifierApplication>(modApp)) {
		if(myModApp->hasFrozenState(freezeTime())) {
			// Perform replacement of the property in the input pipeline state.
			return evaluatePreliminary(time, modApp, std::move(input));
		}
	}
		
	// Request the frozen state from the pipeline.
	return modApp->evaluateInput(freezeTime())
		.then(executor(), [this, time, modApp = QPointer<ModifierApplication>(modApp), input = std::move(input)](const PipelineFlowState& frozenState) mutable {
			UndoSuspender noUndo(this);
			
			// Extract the input property.
			if(FreezePropertyModifierApplication* myModApp = dynamic_object_cast<FreezePropertyModifierApplication>(modApp.data())) {
				if(myModApp->modifier() == this && !sourceProperty().isNull()) {
					if(ParticleProperty* property = sourceProperty().findInState(frozenState)) {

						// Cacne the property to be frozen in the ModifierApplication.
						myModApp->updateStoredData(property, ParticleProperty::findInState(frozenState, ParticleProperty::IdentifierProperty), frozenState.stateValidity());

						// Perform the actual replacement of the property in the input pipeline state.
						return evaluatePreliminary(time, modApp, std::move(input));
					}
					else {
						throwException(tr("The particle property '%1' is not present in the input state").arg(sourceProperty().name()));
					}
				}
				myModApp->invalidateFrozenState();
			}

			return std::move(input);
		});
}

/******************************************************************************
* Modifies the input data in an immediate, preliminary way.
******************************************************************************/
PipelineFlowState FreezePropertyModifier::evaluatePreliminary(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input)
{
	PipelineFlowState output = input;
	ParticleInputHelper pih(dataset(), input);
	ParticleOutputHelper poh(dataset(), output);
	
	if(sourceProperty().isNull()) {
		output.setStatus(PipelineStatus(PipelineStatus::Warning, tr("No source property selected.")));
		return output;
	}
	if(destinationProperty().isNull())
		throwException(tr("No output property selected."));

	// Retrieve the property values stored in the ModifierApplication.
	FreezePropertyModifierApplication* myModApp = dynamic_object_cast<FreezePropertyModifierApplication>(modApp);
	if(!myModApp || !myModApp->property())
		throwException(tr("No stored property values available."));

	// Get the particle property that will be overwritten by the stored one.
	ParticleProperty* outputProperty;
	if(destinationProperty().type() != ParticleProperty::UserProperty) {
		outputProperty = poh.outputStandardProperty<ParticleProperty>(destinationProperty().type(), true);
		if(outputProperty->dataType() != myModApp->property()->dataType()
			|| outputProperty->componentCount() != myModApp->property()->componentCount())
			throwException(tr("Types of source property and output property are not compatible. Cannot restore saved property values."));
	}
	else {
		outputProperty = poh.outputCustomProperty<ParticleProperty>(destinationProperty().name(), 
			myModApp->property()->dataType(), myModApp->property()->componentCount(),
			0, true);
	}
	OVITO_ASSERT(outputProperty->stride() == myModApp->property()->stride());
	
	// Check if particle IDs are present and if the order of particles has changed
	// since we took the snapshot of the property values.
	ParticleProperty* idProperty = pih.inputStandardProperty<ParticleProperty>(ParticleProperty::IdentifierProperty);
	if(myModApp->identifiers() && idProperty && 
			(idProperty->size() != myModApp->identifiers()->size() || 
			!std::equal(idProperty->constDataInt(), idProperty->constDataInt() + idProperty->size(), myModApp->identifiers()->constDataInt()))) {

		// Build ID-to-index map.
		std::unordered_map<int,int> idmap;
		int index = 0;
		for(int id : myModApp->identifiers()->constIntRange()) {
			if(!idmap.insert(std::make_pair(id,index)).second)
				throwException(tr("Detected duplicate particle ID %1 in saved snapshot. Cannot apply saved property values.").arg(id));
			index++;
		}

		// Copy and reorder property data.
		const int* id = idProperty->constDataInt();
		char* dest = static_cast<char*>(outputProperty->data());
		const char* src = static_cast<const char*>(myModApp->property()->constData());
		size_t stride = outputProperty->stride();
		for(size_t index = 0; index < outputProperty->size(); index++, ++id, dest += stride) {
			auto mapEntry = idmap.find(*id);
			if(mapEntry == idmap.end())
				throwException(tr("Detected new particle ID %1, which didn't exist when the snapshot was created. Cannot restore saved property values.").arg(*id));
			memcpy(dest, src + stride * mapEntry->second, stride);
		}
	}
	else {
		
		// Make sure the number of particles didn't change when no particle IDs are defined.
		if(myModApp->property()->size() != poh.outputParticleCount())
			throwException(tr("Number of input particles has changed. Cannot restore saved property values. There were %1 particles when the snapshot was created. Now there are %2.").arg(myModApp->property()->size()).arg(poh.outputParticleCount()));

		if(outputProperty->type() == myModApp->property()->type()
				&& outputProperty->name() == myModApp->property()->name()
				&& outputProperty->dataType() == myModApp->property()->dataType()) {
			// Make shallow data copy if input and output property are the same.
			outputProperty->setStorage(myModApp->property()->storage());
		}
		else {
			// Make a full data copy otherwise.
			OVITO_ASSERT(outputProperty->dataType() == myModApp->property()->dataType());
			OVITO_ASSERT(outputProperty->stride() == myModApp->property()->stride());
			OVITO_ASSERT(outputProperty->size() == myModApp->property()->size());
			memcpy(outputProperty->data(), myModApp->property()->constData(), outputProperty->stride() * outputProperty->size());
		}
	}

	// Replace display objects of output property with cached ones and cache any new display objects.
	// This is required to avoid losing the output property's display settings
	// each time the modifier is re-evaluated or when serializing the modifier application.
	QVector<DisplayObject*> currentDisplayObjs = outputProperty->displayObjects();
	// Replace with cached display objects if they are of the same class type.
	for(int i = 0; i < currentDisplayObjs.size() && i < myModApp->cachedDisplayObjects().size(); i++) {
		if(currentDisplayObjs[i]->getOOClass() == myModApp->cachedDisplayObjects()[i]->getOOClass()) {
			currentDisplayObjs[i] = myModApp->cachedDisplayObjects()[i];
		}
	}
	outputProperty->setDisplayObjects(currentDisplayObjs);
	myModApp->setCachedDisplayObjects(currentDisplayObjs);

	return output;
}

/******************************************************************************
* Makes a copy of the given source property and, optionally, of the provided
* particle identifier list, which will allow to restore the saved property
* values even if the order of particles changes.
******************************************************************************/
void FreezePropertyModifierApplication::updateStoredData(ParticleProperty* property, ParticleProperty* identifiers, TimeInterval validityInterval)
{
	CloneHelper cloneHelper;
	setProperty(cloneHelper.cloneObject(property, false));
	setIdentifiers(cloneHelper.cloneObject(identifiers, false));
	_validityInterval = validityInterval;
}

/******************************************************************************
* Is called when a RefTarget referenced by this object has generated an event.
******************************************************************************/
bool FreezePropertyModifierApplication::referenceEvent(RefTarget* source, ReferenceEvent* event)
{
	if(event->type() == ReferenceEvent::TargetChanged) {
		// Invalidate cached state.
		invalidateFrozenState();
	}
	return ModifierApplication::referenceEvent(source, event);
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
