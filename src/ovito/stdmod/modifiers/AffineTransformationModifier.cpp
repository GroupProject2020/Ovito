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

#include <ovito/stdmod/StdMod.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/stdobj/simcell/PeriodicDomainDataObject.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>
#include <ovito/core/dataset/animation/AnimationSettings.h>
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
		const PipelineFlowState& input = modApp->evaluateInputPreliminary();
		if(const SimulationCellObject* cell = input.getObject<SimulationCellObject>())
			setTargetCell(cell->cellMatrix());
	}
}

/******************************************************************************
* Modifies the input data in an immediate, preliminary way.
******************************************************************************/
void AffineTransformationModifier::evaluatePreliminary(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state)
{
	// Validate parameters and input data.
	if(!relativeMode()) {
		const SimulationCellObject* simCell = state.getObject<SimulationCellObject>();
		if(!simCell || simCell->cellMatrix().determinant() == 0)
			throwException(tr("Input simulation cell does not exist or is degenerate. Transformation to target cell would be singular."));
	}

	// Apply all enabled modifier delegates to the input data.
	MultiDelegatingModifier::evaluatePreliminary(time, modApp, state);
}

/******************************************************************************
* Asks the metaclass which data objects in the given input data collection the 
* modifier delegate can operate on.
******************************************************************************/
QVector<DataObjectReference> SimulationCellAffineTransformationModifierDelegate::OOMetaClass::getApplicableObjects(const DataCollection& input) const
{
	if(input.containsObject<SimulationCellObject>())
		return { DataObjectReference(&SimulationCellObject::OOClass()) };
	if(input.containsObject<PeriodicDomainDataObject>())
		return { DataObjectReference(&PeriodicDomainDataObject::OOClass()) };
	return {};
}

/******************************************************************************
* Applies the modifier operation to the data in a pipeline flow state.
******************************************************************************/
PipelineStatus SimulationCellAffineTransformationModifierDelegate::apply(Modifier* modifier, PipelineFlowState& state, TimePoint time, ModifierApplication* modApp, const std::vector<std::reference_wrapper<const PipelineFlowState>>& additionalInputs)
{
	AffineTransformationModifier* mod = static_object_cast<AffineTransformationModifier>(modifier);
	if(mod->selectionOnly())
		return PipelineStatus::Success;

	AffineTransformation tm;
	if(mod->relativeMode())
		tm = mod->transformationTM();
	else
		tm = mod->targetCell() * state.expectObject<SimulationCellObject>()->cellMatrix().inverse();

	// Transform the SimulationCellObject.
	if(const SimulationCellObject* inputCell = state.getObject<SimulationCellObject>()) {
		SimulationCellObject* outputCell = state.makeMutable(inputCell);
		outputCell->setCellMatrix(mod->relativeMode() ? (tm * inputCell->cellMatrix()) : mod->targetCell());
	}

	// Transform the domains of PeriodicDomainDataObjects.
	for(const DataObject* obj : state.data()->objects()) {
		if(const PeriodicDomainDataObject* existingObject = dynamic_object_cast<PeriodicDomainDataObject>(obj)) {
			if(existingObject->domain()) {
				PeriodicDomainDataObject* newObject = state.makeMutable(existingObject);
				newObject->mutableDomain()->setCellMatrix(tm * existingObject->domain()->cellMatrix());
			}
		}
	}

	return PipelineStatus::Success;
}

}	// End of namespace
}	// End of namespace
