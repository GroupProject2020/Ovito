////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2019 Alexander Stukowski
//
//  This file is part of OVITO (Open Visualization Tool).
//
//  OVITO is free software; you can redistribute it and/or modify it either under the
//  terms of the GNU General Public License version 3 as published by the Free Software
//  Foundation (the "GPL") or, at your option, under the terms of the MIT License.
//  If you do not alter this notice, a recipient may use your version of this
//  file under either the GPL or the MIT License.
//
//  You should have received a copy of the GPL along with this program in a
//  file LICENSE.GPL.txt.  You should have received a copy of the MIT License along
//  with this program in a file LICENSE.MIT.txt
//
//  This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND,
//  either express or implied. See the GPL or the MIT License for the specific language
//  governing rights and limitations.
//
////////////////////////////////////////////////////////////////////////////////////////

#include <ovito/stdmod/StdMod.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/dataset/UndoStack.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>
#include <ovito/stdobj/properties/PropertyObject.h>
#include <ovito/stdobj/properties/PropertyContainer.h>
#include <ovito/stdobj/properties/PropertyAccess.h>
#include <ovito/core/app/Application.h>
#include "SelectTypeModifier.h"

namespace Ovito { namespace StdMod {

IMPLEMENT_OVITO_CLASS(SelectTypeModifier);
DEFINE_PROPERTY_FIELD(SelectTypeModifier, sourceProperty);
DEFINE_PROPERTY_FIELD(SelectTypeModifier, selectedTypeIDs);
DEFINE_PROPERTY_FIELD(SelectTypeModifier, selectedTypeNames);
SET_PROPERTY_FIELD_LABEL(SelectTypeModifier, sourceProperty, "Property");
SET_PROPERTY_FIELD_LABEL(SelectTypeModifier, selectedTypeIDs, "Selected type IDs");
SET_PROPERTY_FIELD_LABEL(SelectTypeModifier, selectedTypeNames, "Selected type names");

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
SelectTypeModifier::SelectTypeModifier(DataSet* dataset) : GenericPropertyModifier(dataset)
{
	// Operate on particles by default.
	setDefaultSubject(QStringLiteral("Particles"), QStringLiteral("ParticlesObject"));
}

/******************************************************************************
* This method is called by the system when the modifier has been inserted
* into a pipeline.
******************************************************************************/
void SelectTypeModifier::initializeModifier(ModifierApplication* modApp)
{
	GenericPropertyModifier::initializeModifier(modApp);

	if(sourceProperty().isNull() && subject()) {

		// When the modifier is first inserted, automatically select the most recently added
		// typed property (in GUI mode) or the canonical type property (in script mode).
		const PipelineFlowState& input = modApp->evaluateInputSynchronous(dataset()->animationSettings()->time());
		if(const PropertyContainer* container = input.getLeafObject(subject())) {
			PropertyReference bestProperty;
			for(PropertyObject* property : container->properties()) {
				if(property->elementTypes().empty() == false && property->componentCount() == 1 && property->dataType() == PropertyStorage::Int) {
					if(Application::instance()->executionContext() == Application::ExecutionContext::Interactive || property->type() == PropertyStorage::GenericTypeProperty) {
						bestProperty = PropertyReference(subject().dataClass(), property);
					}
				}
			}
			if(!bestProperty.isNull())
				setSourceProperty(bestProperty);
		}
	}
}

/******************************************************************************
* Is called when the value of a property of this object has changed.
******************************************************************************/
void SelectTypeModifier::propertyChanged(const PropertyFieldDescriptor& field)
{
	// Whenever the selected property class of this modifier is changed, update the source property reference accordingly.
	if(field == PROPERTY_FIELD(GenericPropertyModifier::subject) && !isBeingLoaded() && !dataset()->undoStack().isUndoingOrRedoing()) {
		setSourceProperty(sourceProperty().convertToContainerClass(subject().dataClass()));
	}
	GenericPropertyModifier::propertyChanged(field);
}

/******************************************************************************
* Modifies the input data synchronously.
******************************************************************************/
void SelectTypeModifier::evaluateSynchronous(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state)
{
	if(!subject())
		throwException(tr("No input element type selected."));
	if(sourceProperty().isNull())
		throwException(tr("No input property selected."));

	// Check if the source property is the right kind of property.
	if(sourceProperty().containerClass() != subject().dataClass())
		throwException(tr("Modifier was set to operate on '%1', but the selected input is a '%2' property.")
			.arg(subject().dataClass()->pythonName()).arg(sourceProperty().containerClass()->propertyClassDisplayName()));

	PropertyContainer* container = state.expectMutableLeafObject(subject());
	container->verifyIntegrity();

	// Get the input property.
	const PropertyObject* typePropertyObject = sourceProperty().findInContainer(container);
	if(!typePropertyObject)
		throwException(tr("The selected input property '%1' is not present.").arg(sourceProperty().name()));
	if(typePropertyObject->componentCount() != 1)
		throwException(tr("The input property '%1' has the wrong number of components. Must be a scalar property.").arg(typePropertyObject->name()));
	if(typePropertyObject->dataType() != PropertyStorage::Int)
		throwException(tr("The input property '%1' has the wrong data type. Must be an integer property.").arg(typePropertyObject->name()));
	ConstPropertyAccess<int> typeProperty = typePropertyObject;

	// Create the selection property.
	PropertyAccess<int> selProperty = container->createProperty(PropertyStorage::GenericSelectionProperty);

	// Counts the number of selected elements.
	size_t nSelected = 0;

	// Generate set of numeric type IDs to select.
	QSet<int> idsToSelect = selectedTypeIDs();
	// Convert type names to numeric IDs.
	for(const QString& typeName : selectedTypeNames()) {
		if(const ElementType* t = typePropertyObject->elementType(typeName))
			idsToSelect.insert(t->numericId());
		else {
			bool found = false;
			for(const ElementType* t : typePropertyObject->elementTypes()) {
				if(t->nameOrNumericId() == typeName) {
					found = true;
					idsToSelect.insert(t->numericId());
					break;
				}
			}
			if(!found)
				throwException(tr("Type '%1' does not exist in the type list of property '%2'.").arg(typeName).arg(typePropertyObject->name()));
		}
	}

	boost::transform(typeProperty, selProperty.begin(), [&](int type) {
		if(idsToSelect.contains(type)) {
			nSelected++;
			return 1;
		}
		return 0;
	});

	state.addAttribute(QStringLiteral("SelectType.num_selected"), QVariant::fromValue(nSelected), modApp);

	QString statusMessage = tr("%1 out of %2 %3 selected (%4%)")
		.arg(nSelected)
		.arg(typeProperty.size())
		.arg(container->getOOMetaClass().elementDescriptionName())
		.arg((FloatType)nSelected * 100 / std::max((size_t)1,typeProperty.size()), 0, 'f', 1);

	state.setStatus(PipelineStatus(PipelineStatus::Success, std::move(statusMessage)));
}

}	// End of namespace
}	// End of namespace
