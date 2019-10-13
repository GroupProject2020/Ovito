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
