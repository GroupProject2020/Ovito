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
#include <plugins/grid/objects/VoxelProperty.h>
#include <plugins/stdobj/simcell/SimulationCellObject.h>
#include <plugins/stdobj/util/InputHelper.h>
#include <plugins/stdobj/util/OutputHelper.h>
#include <core/dataset/DataSet.h>
#include "VoxelGridReplicateModifierDelegate.h"

namespace Ovito { namespace Grid {

IMPLEMENT_OVITO_CLASS(VoxelGridReplicateModifierDelegate);

/******************************************************************************
* Determines whether this delegate can handle the given input data.
******************************************************************************/
bool VoxelGridReplicateModifierDelegate::OOMetaClass::isApplicableTo(const PipelineFlowState& input) const
{
	return input.findObject<VoxelGrid>() != nullptr;
}

/******************************************************************************
* Applies the modifier operation to the data in a pipeline flow state.
******************************************************************************/
PipelineStatus VoxelGridReplicateModifierDelegate::apply(Modifier* modifier, const PipelineFlowState& input, PipelineFlowState& output, TimePoint time, ModifierApplication* modApp, const std::vector<std::reference_wrapper<const PipelineFlowState>>& additionalInputs)
{
	ReplicateModifier* mod = static_object_cast<ReplicateModifier>(modifier);

	OORef<VoxelGrid> existingVoxelGrid = input.findObject<VoxelGrid>();
	if(!existingVoxelGrid || !existingVoxelGrid->domain())
		return PipelineStatus::Success;
	
	std::array<int,3> nPBC;
	nPBC[0] = std::max(mod->numImagesX(),1);
	nPBC[1] = std::max(mod->numImagesY(),1);
	nPBC[2] = std::max(mod->numImagesZ(),1);

	size_t numCopies = nPBC[0] * nPBC[1] * nPBC[2];
	if(numCopies <= 1)
		return PipelineStatus::Success;

	Box3I newImages = mod->replicaRange();

	InputHelper ih(dataset(), input);
	OutputHelper oh(dataset(), output);

	// Create the output copy of the input grid.
	VoxelGrid* newVoxelGrid = oh.cloneIfNeeded(existingVoxelGrid.get());
	std::vector<size_t> shape = existingVoxelGrid->shape();
	if(shape.size() != 3)
		throwException(tr("Can replicate only 3-dimensional data grids."));
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
	for(DataObject* obj : output.objects()) {
		if(OORef<VoxelProperty> existingProperty = dynamic_object_cast<VoxelProperty>(obj)) {
			VoxelProperty* newProperty = oh.cloneIfNeeded(existingProperty.get());
			newProperty->resize(newProperty->size() * numCopies, false);

			char* dst = (char*)newProperty->data();
			for(size_t z = 0; z < shape[2]; z++) {
				size_t zs = z % existingVoxelGrid->shape()[2];
				for(size_t y = 0; y < shape[1]; y++) {
					size_t ys = y % existingVoxelGrid->shape()[1];
					for(size_t x = 0; x < shape[0]; x++) {
						size_t xs = x % existingVoxelGrid->shape()[0];
						size_t index = xs + ys * existingVoxelGrid->shape()[1] + zs * existingVoxelGrid->shape()[0] * existingVoxelGrid->shape()[1];
						memcpy(dst, (char*)existingProperty->constData() + index * existingProperty->stride(), newProperty->stride());
						dst += newProperty->stride();
					}
				}
			}
		}
	}

	return PipelineStatus::Success;
}

}	// End of namespace
}	// End of namespace
