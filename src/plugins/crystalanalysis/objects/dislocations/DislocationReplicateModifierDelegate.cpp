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

#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include <plugins/crystalanalysis/objects/dislocations/DislocationNetworkObject.h>
#include <plugins/stdobj/simcell/SimulationCellObject.h>
#include <plugins/stdobj/util/InputHelper.h>
#include <plugins/stdobj/util/OutputHelper.h>
#include <core/dataset/DataSet.h>
#include <core/dataset/pipeline/ModifierApplication.h>
#include "DislocationReplicateModifierDelegate.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

IMPLEMENT_OVITO_CLASS(DislocationReplicateModifierDelegate);

/******************************************************************************
* Determines whether this delegate can handle the given input data.
******************************************************************************/
bool DislocationReplicateModifierDelegate::OOMetaClass::isApplicableTo(const PipelineFlowState& input) const
{
	return input.findObjectOfType<DislocationNetworkObject>() != nullptr;
}

/******************************************************************************
* Applies the modifier operation to the data in a pipeline flow state.
******************************************************************************/
PipelineStatus DislocationReplicateModifierDelegate::apply(Modifier* modifier, const PipelineFlowState& input, PipelineFlowState& output, TimePoint time, ModifierApplication* modApp, const std::vector<std::reference_wrapper<const PipelineFlowState>>& additionalInputs)
{
	ReplicateModifier* mod = static_object_cast<ReplicateModifier>(modifier);
	
	std::array<int,3> nPBC;
	nPBC[0] = std::max(mod->numImagesX(),1);
	nPBC[1] = std::max(mod->numImagesY(),1);
	nPBC[2] = std::max(mod->numImagesZ(),1);

	size_t numCopies = nPBC[0] * nPBC[1] * nPBC[2];
	if(numCopies <= 1)
		return PipelineStatus::Success;

	Box3I newImages = mod->replicaRange();

	InputHelper ih(dataset(), input);
	OutputHelper oh(dataset(), output, modApp);

	for(DataObject* obj : output.objects()) {
		if(DislocationNetworkObject* existingDislocations = dynamic_object_cast<DislocationNetworkObject>(obj)) {
			// For replication, a domain is required.
			if(!existingDislocations->domain()) continue;
			AffineTransformation simCell = existingDislocations->domain()->cellMatrix();
			AffineTransformation inverseSimCell;
			if(!simCell.inverse(inverseSimCell))
				continue;

			// Create the output copy of the input dislocation object.
			DislocationNetworkObject* newDislocations = oh.cloneIfNeeded(existingDislocations);
			std::shared_ptr<DislocationNetwork> dislocations = newDislocations->modifiableStorage();
			
			// Shift existing vertices so that they form the first image at grid position (0,0,0).
			const Vector3 imageDelta = simCell * Vector3(newImages.minc.x(), newImages.minc.y(), newImages.minc.z());
			if(!imageDelta.isZero()) {
				for(DislocationSegment* segment : dislocations->segments()) {
					for(Point3& p : segment->line)
						p += imageDelta;
				}
			}

			// Replicate lines.
			size_t oldSegmentCount = dislocations->segments().size();
			for(int imageX = 0; imageX < nPBC[0]; imageX++) {
				for(int imageY = 0; imageY < nPBC[1]; imageY++) {
					for(int imageZ = 0; imageZ < nPBC[2]; imageZ++) {
						if(imageX == 0 && imageY == 0 && imageZ == 0) continue;
						// Shift vertex positions by the periodicity vector.
						const Vector3 imageDelta = simCell * Vector3(imageX, imageY, imageZ);
						for(size_t i = 0; i < oldSegmentCount; i++) {
							DislocationSegment* oldSegment = dislocations->segments()[i];
							DislocationSegment* newSegment = dislocations->createSegment(oldSegment->burgersVector);
							newSegment->line = oldSegment->line;
							newSegment->coreSize = oldSegment->coreSize;
							for(Point3& p : newSegment->line)
								p += imageDelta;
						}
						// TODO: Replicate nodal connectivity.
					}
				}
			}
			OVITO_ASSERT(dislocations->segments().size() == oldSegmentCount * numCopies);
			
			// Extend the periodic domain the surface is embedded in.
			simCell.translation() += (FloatType)newImages.minc.x() * simCell.column(0);
			simCell.translation() += (FloatType)newImages.minc.y() * simCell.column(1);
			simCell.translation() += (FloatType)newImages.minc.z() * simCell.column(2);
			simCell.column(0) *= (newImages.sizeX() + 1);
			simCell.column(1) *= (newImages.sizeY() + 1);
			simCell.column(2) *= (newImages.sizeZ() + 1);
			newDislocations->domain()->setCellMatrix(simCell);			
		}
	}

	return PipelineStatus::Success;
}

}	// End of namespace
}	// End of namespace
}	// End of namespace
