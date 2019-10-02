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

#include <ovito/grid/Grid.h>
#include <ovito/grid/objects/VoxelGrid.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include "VoxelGridAffineTransformationModifierDelegate.h"

namespace Ovito { namespace Grid {

IMPLEMENT_OVITO_CLASS(VoxelGridAffineTransformationModifierDelegate);

/******************************************************************************
* Indicates which data objects in the given input data collection the modifier 
* delegate is able to operate on.
******************************************************************************/
QVector<DataObjectReference> VoxelGridAffineTransformationModifierDelegate::OOMetaClass::getApplicableObjects(const DataCollection& input) const 
{
	if(input.containsObject<VoxelGrid>())
		return { DataObjectReference(&VoxelGrid::OOClass()) };
	return {};
}

/******************************************************************************
* Applies the modifier operation to the data in a pipeline flow state.
******************************************************************************/
PipelineStatus VoxelGridAffineTransformationModifierDelegate::apply(Modifier* modifier, PipelineFlowState& state, TimePoint time, ModifierApplication* modApp, const std::vector<std::reference_wrapper<const PipelineFlowState>>& additionalInputs)
{
	// Transform the domains of PeriodicDomainDataObjects.
	for(const DataObject* obj : state.data()->objects()) {
		if(const VoxelGrid* existingObject = dynamic_object_cast<VoxelGrid>(obj)) {
			if(existingObject->domain()) {

				// Determine transformation matrix.
				AffineTransformationModifier* mod = static_object_cast<AffineTransformationModifier>(modifier);
				AffineTransformation tm;
				if(mod->relativeMode())
					tm = mod->transformationTM();
				else
					tm = mod->targetCell() * state.expectObject<SimulationCellObject>()->cellMatrix().inverse();

				VoxelGrid* newObject = state.makeMutable(existingObject);
				newObject->mutableDomain()->setCellMatrix(tm * existingObject->domain()->cellMatrix());
			}
		}
	}

	return PipelineStatus::Success;
}

}	// End of namespace
}	// End of namespace
