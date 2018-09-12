///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2018) Alexander Stukowski
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

#include <plugins/stdmod/StdMod.h>
#include <plugins/stdobj/properties/PropertyObject.h>
#include <plugins/stdobj/properties/PropertyContainer.h>
#include <core/dataset/pipeline/ModifierApplication.h>
#include <core/dataset/DataSet.h>
#include "ManualSelectionModifier.h"

namespace Ovito { namespace StdMod {

IMPLEMENT_OVITO_CLASS(ManualSelectionModifier);

IMPLEMENT_OVITO_CLASS(ManualSelectionModifierApplication);
SET_MODIFIER_APPLICATION_TYPE(ManualSelectionModifier, ManualSelectionModifierApplication);
DEFINE_REFERENCE_FIELD(ManualSelectionModifierApplication, selectionSet);
SET_PROPERTY_FIELD_LABEL(ManualSelectionModifierApplication, selectionSet, "Element selection set");

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
ManualSelectionModifier::ManualSelectionModifier(DataSet* dataset) : GenericPropertyModifier(dataset)
{
	// Operate on particles by default.
	setDefaultSubject(QStringLiteral("Particles"), QStringLiteral("ParticlesObject"));
}

/******************************************************************************
* This method is called by the system when the modifier has been inserted
* into a pipeline.
******************************************************************************/
void ManualSelectionModifier::initializeModifier(ModifierApplication* modApp)
{
	Modifier::initializeModifier(modApp);

	// Take a snapshot of the existing selection state at the time the modifier is created.
	if(!getSelectionSet(modApp, false)) {
		resetSelection(modApp, modApp->evaluateInputPreliminary());
	}
}

/******************************************************************************
* Is called when the value of a property of this object has changed.
******************************************************************************/
void ManualSelectionModifier::propertyChanged(const PropertyFieldDescriptor& field)
{
	// Whenever the subject of this modifier is changed, reset the selection.
	if(field == PROPERTY_FIELD(GenericPropertyModifier::subject) && !isBeingLoaded()) {
		for(ModifierApplication* modApp : modifierApplications()) {
			resetSelection(modApp, modApp->evaluateInputPreliminary());
		}
	}
	GenericPropertyModifier::propertyChanged(field);
}

/******************************************************************************
* Modifies the input data in an immediate, preliminary way.
******************************************************************************/
void ManualSelectionModifier::evaluatePreliminary(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state)
{
	// Retrieve the selection stored in the modifier application.
	ElementSelectionSet* selectionSet = getSelectionSet(modApp, false);
	if(!selectionSet)
		throwException(tr("No stored selection set available. Please reset the selection state."));

	if(subject()) {
		PropertyContainer* container = state.expectMutableLeafObject(subject());
		
		PipelineStatus status = selectionSet->applySelection(
				container->createProperty(PropertyStorage::GenericSelectionProperty),
				container->getProperty(PropertyStorage::GenericIdentifierProperty));
		
		state.setStatus(std::move(status));
	}
}

/******************************************************************************
* Returns the selection set object stored in the ModifierApplication, or, if
* it does not exist, creates one.
******************************************************************************/
ElementSelectionSet* ManualSelectionModifier::getSelectionSet(ModifierApplication* modApp, bool createIfNotExist)
{
	ManualSelectionModifierApplication* myModApp = dynamic_object_cast<ManualSelectionModifierApplication>(modApp);
	if(!myModApp)
		throwException(tr("Manual selection modifier is not associated with a ManualSelectionModifierApplication."));

	ElementSelectionSet* selectionSet = myModApp->selectionSet();
	if(!selectionSet && createIfNotExist)
		myModApp->setSelectionSet(selectionSet = new ElementSelectionSet(dataset()));

	return selectionSet;
}

/******************************************************************************
* Adopts the selection state from the modifier's input.
******************************************************************************/
void ManualSelectionModifier::resetSelection(ModifierApplication* modApp, const PipelineFlowState& state)
{
	if(subject()) {
		const PropertyContainer* container = state.expectLeafObject(subject());
		getSelectionSet(modApp, true)->resetSelection(container);
	}
}

/******************************************************************************
* Selects all elements.
******************************************************************************/
void ManualSelectionModifier::selectAll(ModifierApplication* modApp, const PipelineFlowState& state)
{
	if(subject()) {
		const PropertyContainer* container = state.expectLeafObject(subject());
		getSelectionSet(modApp, true)->selectAll(container);
	}
}

/******************************************************************************
* Deselects all elements.
******************************************************************************/
void ManualSelectionModifier::clearSelection(ModifierApplication* modApp, const PipelineFlowState& state)
{
	if(subject()) {
		const PropertyContainer* container = state.expectLeafObject(subject());
		getSelectionSet(modApp, true)->clearSelection(container);
	}
}

/******************************************************************************
* Toggles the selection state of a single element.
******************************************************************************/
void ManualSelectionModifier::toggleElementSelection(ModifierApplication* modApp, const PipelineFlowState& state, size_t elementIndex)
{
	ElementSelectionSet* selectionSet = getSelectionSet(modApp, false);
	if(!selectionSet)
		throwException(tr("No stored selection set available. Please reset the selection state."));
	if(subject()) {
		const PropertyContainer* container = state.expectLeafObject(subject());
		selectionSet->toggleElement(container, elementIndex);
	}
}

/******************************************************************************
* Replaces the selection.
******************************************************************************/
void ManualSelectionModifier::setSelection(ModifierApplication* modApp, const PipelineFlowState& state, const boost::dynamic_bitset<>& selection, ElementSelectionSet::SelectionMode mode)
{
	if(subject()) {
		const PropertyContainer* container = state.expectLeafObject(subject());
		getSelectionSet(modApp, true)->setSelection(container, selection, mode);
	}
}

}	// End of namespace
}	// End of namespace
