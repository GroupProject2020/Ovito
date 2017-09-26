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
#include <core/dataset/data/simcell/SimulationCellObject.h>
#include <core/dataset/data/simcell/PeriodicDomainDataObject.h>
#include <core/dataset/pipeline/ModifierApplication.h>
#include <core/dataset/pipeline/InputHelper.h>
#include <core/dataset/pipeline/OutputHelper.h>
#include <core/dataset/animation/AnimationSettings.h>
#include "AffineTransformationModifier.h"

namespace Ovito { namespace StdMod {

IMPLEMENT_OVITO_CLASS(AffineTransformationModifier);
DEFINE_PROPERTY_FIELD(AffineTransformationModifier, transformationTM);
DEFINE_PROPERTY_FIELD(AffineTransformationModifier, selectionOnly);
DEFINE_PROPERTY_FIELD(AffineTransformationModifier, targetCell);
DEFINE_PROPERTY_FIELD(AffineTransformationModifier, relativeMode);
SET_PROPERTY_FIELD_LABEL(AffineTransformationModifier, transformationTM, "Transformation");
SET_PROPERTY_FIELD_LABEL(AffineTransformationModifier, selectionOnly, "Transform selected elements only");
SET_PROPERTY_FIELD_LABEL(AffineTransformationModifier, targetCell, "Target cell shape");
SET_PROPERTY_FIELD_LABEL(AffineTransformationModifier, relativeMode, "Relative transformation");

IMPLEMENT_OVITO_CLASS(AffineTransformationModifierDelegate);
IMPLEMENT_OVITO_CLASS(SimulationCellAffineTransformationModifierDelegate);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
AffineTransformationModifier::AffineTransformationModifier(DataSet* dataset) : MultiDelegatingModifier(dataset),
	_selectionOnly(false),
	_transformationTM(AffineTransformation::Identity()), 
	_targetCell(AffineTransformation::Zero()),
	_relativeMode(true)
{
	// Generate the list of delegate objects.
	createModifierDelegates(AffineTransformationModifierDelegate::OOClass());
}

/******************************************************************************
* This method is called by the system when the modifier has been inserted
* into a PipelineObject.
******************************************************************************/
void AffineTransformationModifier::initializeModifier(ModifierApplication* modApp)
{
	MultiDelegatingModifier::initializeModifier(modApp);

	// Take the simulation cell from the input object as the default destination cell geometry for absolute scaling.
	if(targetCell() == AffineTransformation::Zero()) {
		PipelineFlowState input = modApp->evaluateInputPreliminary();
		SimulationCellObject* cell = input.findObject<SimulationCellObject>();
		if(cell)
			setTargetCell(cell->cellMatrix());
	}
}

/******************************************************************************
* Modifies the input data in an immediate, preliminary way.
******************************************************************************/
PipelineFlowState AffineTransformationModifier::evaluatePreliminary(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input)
{
	// Validate parameters and input data.
	if(!relativeMode()) {
		SimulationCellObject* simCell = input.findObject<SimulationCellObject>();
		if(!simCell || simCell->cellMatrix().determinant() == 0)
			throwException(tr("Input simulation cell does not exist or is degenerate. Transformation to target cell would be singular."));
	}

	// Apply all enabled modifier delegates to the input data.
	return MultiDelegatingModifier::evaluatePreliminary(time, modApp, input);
}

/******************************************************************************
* Determines whether this delegate can handle the given input data.
******************************************************************************/
bool SimulationCellAffineTransformationModifierDelegate::OOMetaClass::isApplicableTo(const PipelineFlowState& input) const
{
	return input.findObject<SimulationCellObject>() != nullptr ||
		input.findObject<PeriodicDomainDataObject>() != nullptr;
}

/******************************************************************************
* Applies the modifier operation to the data in a pipeline flow state.
******************************************************************************/
PipelineStatus SimulationCellAffineTransformationModifierDelegate::apply(Modifier* modifier, const PipelineFlowState& input, PipelineFlowState& output, TimePoint time, ModifierApplication* modApp)
{
	AffineTransformationModifier* mod = static_object_cast<AffineTransformationModifier>(modifier);
	if(mod->selectionOnly()) 
		return PipelineStatus::Success;

	InputHelper ih(dataset(), input);
	OutputHelper oh(dataset(), output);
	
	AffineTransformation tm;
	if(mod->relativeMode())
		tm = mod->transformationTM();
	else
		tm = mod->targetCell() * ih.expectSimulationCell()->cellMatrix().inverse();
	
	// Transform SimulationCellObject.
	if(SimulationCellObject* inputCell = input.findObject<SimulationCellObject>()) {
		if(mod->relativeMode()) {
			oh.outputObject<SimulationCellObject>()->setCellMatrix(tm * inputCell->cellMatrix());
		}
		else {
			oh.outputObject<SimulationCellObject>()->setCellMatrix(mod->targetCell());
		}
	}

	// Transform the domains of PeriodicDomainDataObjects.
	for(DataObject* obj : output.objects()) {
		if(PeriodicDomainDataObject* existingObject = dynamic_object_cast<PeriodicDomainDataObject>(obj)) {
			if(existingObject->domain()) {
				PeriodicDomainDataObject* newObject = oh.cloneIfNeeded(existingObject);
				newObject->domain()->setCellMatrix(tm * existingObject->domain()->cellMatrix());
			}
		}
	}	
	
	return PipelineStatus::Success;
}

}	// End of namespace
}	// End of namespace
