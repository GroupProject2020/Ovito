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

#include <plugins/stdmod/StdMod.h>
#include <core/dataset/DataSet.h>
#include <core/dataset/animation/controller/Controller.h>
#include <plugins/stdobj/util/InputHelper.h>
#include <plugins/stdobj/util/OutputHelper.h>
#include <plugins/stdobj/properties/PropertyStorage.h>
#include <plugins/stdobj/properties/PropertyObject.h>
#include "AssignColorModifier.h"

namespace Ovito { namespace StdMod {

IMPLEMENT_OVITO_CLASS(AssignColorModifier);
IMPLEMENT_OVITO_CLASS(AssignColorModifierDelegate);
DEFINE_REFERENCE_FIELD(AssignColorModifier, colorController);
DEFINE_PROPERTY_FIELD(AssignColorModifier, keepSelection);
SET_PROPERTY_FIELD_LABEL(AssignColorModifier, colorController, "Color");
SET_PROPERTY_FIELD_LABEL(AssignColorModifier, keepSelection, "Keep selection");

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
AssignColorModifier::AssignColorModifier(DataSet* dataset) : DelegatingModifier(dataset), 
	_keepSelection(true)
{
	setColorController(ControllerManager::createColorController(dataset));
	colorController()->setColorValue(0, Color(0.3f, 0.3f, 1.0f));

	// Let this modifier act on particles by default.
	createDefaultModifierDelegate(AssignColorModifierDelegate::OOClass(), QStringLiteral("ParticlesAssignColorModifierDelegate"));	
}

/******************************************************************************
* Loads the user-defined default values of this object's parameter fields from the
* application's settings store.
******************************************************************************/
void AssignColorModifier::loadUserDefaults()
{
	Modifier::loadUserDefaults();

	// In the graphical program environment, we clear the 
	// selection by default to make the assigned colors visible.
	setKeepSelection(false);
}

/******************************************************************************
* Asks the modifier for its validity interval at the given time.
******************************************************************************/
TimeInterval AssignColorModifier::modifierValidity(TimePoint time)
{
	TimeInterval interval = DelegatingModifier::modifierValidity(time);
	if(colorController()) interval.intersect(colorController()->validityInterval(time));
	return interval;
}

/******************************************************************************
* Applies the modifier operation to the data in a pipeline flow state.
******************************************************************************/
PipelineStatus AssignColorModifierDelegate::apply(Modifier* modifier, const PipelineFlowState& input, PipelineFlowState& output, TimePoint time, ModifierApplication* modApp, const std::vector<std::reference_wrapper<const PipelineFlowState>>& additionalInputs)
{
	const AssignColorModifier* mod = static_object_cast<AssignColorModifier>(modifier);
	InputHelper ih(dataset(), input);
	OutputHelper oh(dataset(), output);

	if(!mod->colorController())
		return PipelineStatus::Success;

	// Get the selection property.
	ConstPropertyPtr selProperty;
	if(PropertyObject* selPropertyObj = ih.inputStandardProperty(propertyClass(), PropertyStorage::GenericSelectionProperty)) {
		selProperty = selPropertyObj->storage();

		// Clear selection if requested.
		if(!mod->keepSelection()) {
			output.removeObject(selPropertyObj);
		}
	}
	
	// Create the color output property.
	PropertyPtr colorProperty = createOutputColorProperty(time, ih, oh, (bool)selProperty)->modifiableStorage();

	// Get modifier's parameter value.
	Color color;
	mod->colorController()->getColorValue(time, color, output.mutableStateValidity());

	if(!selProperty) {
		// Assign color to all elements.
		std::fill(colorProperty->dataColor(), colorProperty->dataColor() + colorProperty->size(), color);
	}
	else {
		// Assign color only to selected elements.
		const int* sel = selProperty->constDataInt();
		for(Color& c : colorProperty->colorRange()) {
			if(*sel++) c = color;
		}
	}

	return PipelineStatus::Success;
}

}	// End of namespace
}	// End of namespace
