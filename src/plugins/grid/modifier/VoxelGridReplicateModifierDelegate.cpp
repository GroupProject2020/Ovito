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

#include <plugins/grid/Grid.h>
#include <plugins/grid/objects/VoxelGrid.h>
#include <plugins/stdobj/simcell/SimulationCellObject.h>
#include "VoxelGridReplicateModifierDelegate.h"

namespace Ovito { namespace Grid {

IMPLEMENT_OVITO_CLASS(VoxelGridReplicateModifierDelegate);

/******************************************************************************
* Determines whether this delegate can handle the given input data.
******************************************************************************/
bool VoxelGridReplicateModifierDelegate::OOMetaClass::isApplicableTo(const PipelineFlowState& input) const
{
	return input.containsObject<VoxelGrid>();
}

/******************************************************************************
* Applies the modifier operation to the data in a pipeline flow state.
******************************************************************************/
PipelineStatus VoxelGridReplicateModifierDelegate::apply(Modifier* modifier, const PipelineFlowState& input, PipelineFlowState& output, TimePoint time, ModifierApplication* modApp, const std::vector<std::reference_wrapper<const PipelineFlowState>>& additionalInputs)
{
	ReplicateModifier* mod = static_object_cast<ReplicateModifier>(modifier);

	const VoxelGrid* existingVoxelGrid = input.getObject<VoxelGrid>();
	if(!existingVoxelGrid || !existingVoxelGrid->domain())
		return PipelineStatus::Success;
	
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
		return PipelineStatus::Success;

	// Create the output copy of the input grid.
	VoxelGrid* newVoxelGrid = output.makeMutable(existingVoxelGrid);
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
	newVoxelGrid->domain()->setCellMatrix(simCell);

	// Replicate voxel property data.
	newVoxelGrid->makePropertiesMutable();
	for(PropertyObject* property : newVoxelGrid->properties()) {
		ConstPropertyPtr oldData = property->storage();
		property->resize(oldData->size() * numCopies, false);

		char* dst = (char*)property->data();
		const char* src = (const char*)oldData->constData();
		size_t stride = oldData->stride();
		for(size_t z = 0; z < shape[2]; z++) {
			size_t zs = z % oldShape[2];
			for(size_t y = 0; y < shape[1]; y++) {
				size_t ys = y % oldShape[1];
				for(size_t x = 0; x < shape[0]; x++) {
					size_t xs = x % oldShape[0];
					size_t index = xs + ys * oldShape[0] + zs * oldShape[0] * oldShape[1];
					OVITO_ASSERT(index < oldData->size());
					memcpy(dst, src + index * stride, stride);
					dst += stride;
				}
			}
		}
		OVITO_ASSERT(dst == (char*)property->data() + property->size()*property->stride());
	}

	return PipelineStatus::Success;
}

}	// End of namespace
}	// End of namespace
