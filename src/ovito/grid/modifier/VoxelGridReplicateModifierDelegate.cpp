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

#include <ovito/grid/Grid.h>
#include <ovito/grid/objects/VoxelGrid.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include "VoxelGridReplicateModifierDelegate.h"

namespace Ovito { namespace Grid {

IMPLEMENT_OVITO_CLASS(VoxelGridReplicateModifierDelegate);

/******************************************************************************
* Indicates which data objects in the given input data collection the modifier
* delegate is able to operate on.
******************************************************************************/
QVector<DataObjectReference> VoxelGridReplicateModifierDelegate::OOMetaClass::getApplicableObjects(const DataCollection& input) const
{
	if(input.containsObject<VoxelGrid>())
		return { DataObjectReference(&VoxelGrid::OOClass()) };
	return {};
}

/******************************************************************************
* Applies the modifier operation to the data in a pipeline flow state.
******************************************************************************/
PipelineStatus VoxelGridReplicateModifierDelegate::apply(Modifier* modifier, PipelineFlowState& state, TimePoint time, ModifierApplication* modApp, const std::vector<std::reference_wrapper<const PipelineFlowState>>& additionalInputs)
{
	ReplicateModifier* mod = static_object_cast<ReplicateModifier>(modifier);

	for(const DataObject* obj : state.data()->objects()) {
		if(const VoxelGrid* existingVoxelGrid = dynamic_object_cast<VoxelGrid>(obj)) {

			if(!existingVoxelGrid->domain())
				continue;

			std::array<int,3> nPBC;
			Box3I newImages = mod->replicaRange();
			nPBC[0] = std::max(mod->numImagesX(),1);
			nPBC[1] = std::max(mod->numImagesY(),1);
			nPBC[2] = std::max(mod->numImagesZ(),1);
			if(existingVoxelGrid->domain()->is2D()) {
				nPBC[2] = 1;
				newImages.minc.z() = newImages.maxc.z() = 0;
			}

			size_t numCopies = (size_t)nPBC[0] * (size_t)nPBC[1] * (size_t)nPBC[2];
			if(numCopies <= 1)
				continue;

			// Create the output copy of the input grid.
			VoxelGrid* newVoxelGrid = state.makeMutable(existingVoxelGrid);
			const VoxelGrid::GridDimensions oldShape = existingVoxelGrid->shape();
			VoxelGrid::GridDimensions shape = oldShape;
			shape[0] *= nPBC[0];
			shape[1] *= nPBC[1];
			shape[2] *= nPBC[2];
			newVoxelGrid->setShape(shape);

			// Extend the periodic domain the grid is embedded in.
			AffineTransformation simCell = existingVoxelGrid->domain()->cellMatrix();
			simCell.translation() += (FloatType)newImages.minc.x() * simCell.column(0);
			simCell.translation() += (FloatType)newImages.minc.y() * simCell.column(1);
			simCell.translation() += (FloatType)newImages.minc.z() * simCell.column(2);
			simCell.column(0) *= (newImages.sizeX() + 1);
			simCell.column(1) *= (newImages.sizeY() + 1);
			simCell.column(2) *= (newImages.sizeZ() + 1);
			newVoxelGrid->mutableDomain()->setCellMatrix(simCell);

			// Replicate voxel property data.
			newVoxelGrid->replicate(numCopies, false);

			// We cannot rely on the replicate() method above to duplicate the data in the property
			// arrays, because for three-dimensional voxel grids, the storage order of voxel data matters.
			// The following loop takes care of replicating the the property values the right way.
			for(PropertyObject* property : newVoxelGrid->properties()) {
				// First, copy the original property data to a temporary buffer so that
				// it doesn't get destroyed while we are rewriting it to the replicated property array.
				size_t stride = property->stride();
				char* dst = static_cast<char*>(property->data<void>());
				std::vector<char> buffer(dst, dst + stride * existingVoxelGrid->elementCount());
				const char* src = buffer.data();
				for(size_t z = 0; z < shape[2]; z++) {
					size_t zs = z % oldShape[2];
					for(size_t y = 0; y < shape[1]; y++) {
						size_t ys = y % oldShape[1];
						for(size_t x = 0; x < shape[0]; x++) {
							size_t xs = x % oldShape[0];
							size_t index = existingVoxelGrid->voxelIndex(xs, ys, zs);
							std::memcpy(dst, src + index * stride, stride);
							dst += stride;
						}
					}
				}
				OVITO_ASSERT(dst == static_cast<char*>(property->data<void>()) + property->size()*property->stride());
			}
		}
	}

	return PipelineStatus::Success;
}

}	// End of namespace
}	// End of namespace
