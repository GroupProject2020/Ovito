////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2018 Alexander Stukowski
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
#include <ovito/stdobj/properties/PropertyObject.h>
#include <ovito/stdobj/properties/PropertyAccess.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>
#include <ovito/core/dataset/data/AttributeDataObject.h>
#include <ovito/core/dataset/io/FileSource.h>
#include <ovito/core/dataset/animation/AnimationSettings.h>
#include "CombineDatasetsModifier.h"

namespace Ovito { namespace StdMod {

IMPLEMENT_OVITO_CLASS(CombineDatasetsModifier);
DEFINE_REFERENCE_FIELD(CombineDatasetsModifier, secondaryDataSource);
SET_PROPERTY_FIELD_LABEL(CombineDatasetsModifier, secondaryDataSource, "Secondary source");

IMPLEMENT_OVITO_CLASS(CombineDatasetsModifierDelegate);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
CombineDatasetsModifier::CombineDatasetsModifier(DataSet* dataset) : MultiDelegatingModifier(dataset)
{
	// Generate the list of delegate objects.
	createModifierDelegates(CombineDatasetsModifierDelegate::OOClass());

	// Create the file source object, which will be responsible for loading
	// and caching the data to be merged.
	OORef<FileSource> fileSource(new FileSource(dataset));

	setSecondaryDataSource(fileSource);
}

/******************************************************************************
* Modifies the input data.
******************************************************************************/
Future<PipelineFlowState> CombineDatasetsModifier::evaluate(const PipelineEvaluationRequest& request, ModifierApplication* modApp, const PipelineFlowState& input)
{
	// Get the secondary data source.
	if(!secondaryDataSource())
		throwException(tr("No dataset to be merged has been provided."));

	// Get the state.
	SharedFuture<PipelineFlowState> secondaryStateFuture = secondaryDataSource()->evaluate(request);

	// Wait for the data to become available.
	return secondaryStateFuture.then(executor(), [this, state = input, time = request.time(), modApp = OORef<ModifierApplication>(modApp)](const PipelineFlowState& secondaryState) mutable {

		// Make sure the obtained dataset is valid and ready to use.
		if(secondaryState.status().type() == PipelineStatus::Error) {
			if(FileSource* fileSource = dynamic_object_cast<FileSource>(secondaryDataSource())) {
				if(fileSource->sourceUrls().empty())
					throwException(tr("Please pick an input file to be merged."));
			}
			state.setStatus(secondaryState.status());
			return std::move(state);
		}

		if(!secondaryState)
			throwException(tr("Secondary data source has not been specified yet or is empty. Please pick an input file to be merged."));

		// Merge validity intervals of primary and secondary datasets.
		state.intersectStateValidity(secondaryState.stateValidity());

		// Merge global attributes of primary and secondary datasets.
		if(state && secondaryState) {
			for(const DataObject* obj : secondaryState.data()->objects()) {
				if(const AttributeDataObject* attribute = dynamic_object_cast<AttributeDataObject>(obj)) {
					if(state.getAttributeValue(attribute->identifier()).isNull())
						state.mutableData()->addObject(attribute);
				}
			}
		}

		// Let the delegates do their job.
		applyDelegates(state, time, modApp, { std::reference_wrapper<const PipelineFlowState>(secondaryState) });

		return std::move(state);
	});
}

/******************************************************************************
* Modifies the input data synchronously.
******************************************************************************/
void CombineDatasetsModifier::evaluateSynchronous(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state)
{
	// Get the secondary data source.
	if(!secondaryDataSource())
		return;

	// Acquire the state to be merged.
	const PipelineFlowState& secondaryState = secondaryDataSource()->evaluateSynchronous(time);
	if(!secondaryState)
		return;

	// Merge validity intervals of primary and secondary datasets.
	state.intersectStateValidity(secondaryState.stateValidity());

	// Merge global attributes of primary and secondary datasets.
	for(const DataObject* obj : secondaryState.data()->objects()) {
		if(const AttributeDataObject* attribute = dynamic_object_cast<AttributeDataObject>(obj)) {
			if(state.getAttributeValue(attribute->identifier()).isNull())
				state.addObject(attribute);
		}
	}

	// Let the delegates do their job and merge the data objects of the two datasets.
	applyDelegates(state, time, modApp, { std::reference_wrapper<const PipelineFlowState>(secondaryState) });
}

/******************************************************************************
* Is called when a RefTarget referenced by this object has generated an event.
******************************************************************************/
bool CombineDatasetsModifier::referenceEvent(RefTarget* source, const ReferenceEvent& event)
{
	if(event.type() == ReferenceEvent::AnimationFramesChanged && source == secondaryDataSource()) {
		// Propagate animation interval events from the secondary source.
		return true;
	}
	return MultiDelegatingModifier::referenceEvent(source, event);
}

/******************************************************************************
* Gets called when the data object of the node has been replaced.
******************************************************************************/
void CombineDatasetsModifier::referenceReplaced(const PropertyFieldDescriptor& field, RefTarget* oldTarget, RefTarget* newTarget)
{
	if(field == PROPERTY_FIELD(secondaryDataSource) && !isBeingLoaded()) {
		// The animation length might have changed when the secondary source has been replaced.
		notifyDependents(ReferenceEvent::AnimationFramesChanged);
	}
	MultiDelegatingModifier::referenceReplaced(field, oldTarget, newTarget);
}

/******************************************************************************
* Helper method that merges the set of element types defined for a property.
******************************************************************************/
void CombineDatasetsModifierDelegate::mergeElementTypes(PropertyObject* property1, const PropertyObject* property2, CloneHelper& cloneHelper)
{
	// Check if input properties have the right format.
	if(!property2) return;
	if(property2->elementTypes().empty()) return;
	if(property1->componentCount() != 1 || property2->componentCount() != 1) return;
	if(property1->dataType() != PropertyStorage::Int || property2->dataType() != PropertyStorage::Int) return;

	std::map<int,int> typeMap;
	for(const ElementType* type2 : property2->elementTypes()) {
		if(!type2->name().isEmpty()) {
			const ElementType* type1 = property1->elementType(type2->name());
			if(type1 == nullptr) {
				OORef<ElementType> type2clone = cloneHelper.cloneObject(type2, false);
				type2clone->setNumericId(property1->generateUniqueElementTypeId());
				property1->addElementType(type2clone);
				typeMap.insert(std::make_pair(type2->numericId(), type2clone->numericId()));
			}
			else if(type1->numericId() != type2->numericId()) {
				typeMap.insert(std::make_pair(type2->numericId(), type1->numericId()));
			}
		}
		else {
			const ElementType* type1 = property1->elementType(type2->numericId());
			if(!type1) {
				OORef<ElementType> type2clone = cloneHelper.cloneObject(type2, false);
				property1->addElementType(type2clone);
				OVITO_ASSERT(type2clone->numericId() == type2->numericId());
			}
			else if(!type1->name().isEmpty()) {
				OORef<ElementType> type2clone = cloneHelper.cloneObject(type2, false);
				type2clone->setNumericId(property1->generateUniqueElementTypeId());
				property1->addElementType(type2clone);
				typeMap.insert(std::make_pair(type2->numericId(), type2clone->numericId()));
			}
		}
	}
	// Remap particle property values.
	if(typeMap.empty() == false) {
		PropertyAccess<int> selectionArray1 = property1;
		auto p = selectionArray1.begin() + (property1->size() - property2->size());
		auto p_end = selectionArray1.end();
		for(; p != p_end; ++p) {
			auto iter = typeMap.find(*p);
			if(iter != typeMap.end()) *p = iter->second;
		}
	}
}

}	// End of namespace
}	// End of namespace
