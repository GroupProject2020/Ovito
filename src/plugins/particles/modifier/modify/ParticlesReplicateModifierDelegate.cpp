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

#include <plugins/particles/Particles.h>
#include <plugins/particles/modifier/ParticleInputHelper.h>
#include <plugins/particles/modifier/ParticleOutputHelper.h>
#include <plugins/particles/objects/BondsObject.h>
#include <plugins/particles/objects/BondProperty.h>
#include <plugins/particles/objects/ParticleProperty.h>
#include <core/dataset/data/simcell/SimulationCellObject.h>
#include <core/dataset/DataSet.h>
#include "ParticlesReplicateModifierDelegate.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Modify)

IMPLEMENT_OVITO_CLASS(ParticlesReplicateModifierDelegate);

/******************************************************************************
* Determines whether this delegate can handle the given input data.
******************************************************************************/
bool ParticlesReplicateModifierDelegate::OOMetaClass::isApplicableTo(const PipelineFlowState& input) const
{
	return input.findObject<ParticleProperty>() != nullptr;
}

/******************************************************************************
* Applies the modifier operation to the data in a pipeline flow state.
******************************************************************************/
PipelineStatus ParticlesReplicateModifierDelegate::apply(Modifier* modifier, const PipelineFlowState& input, PipelineFlowState& output, TimePoint time, ModifierApplication* modApp)
{
	ReplicateModifier* mod = static_object_cast<ReplicateModifier>(modifier);
	ParticleInputHelper pih(dataset(), input);
	ParticleOutputHelper poh(dataset(), output);
	
	std::array<int,3> nPBC;
	nPBC[0] = std::max(mod->numImagesX(),1);
	nPBC[1] = std::max(mod->numImagesY(),1);
	nPBC[2] = std::max(mod->numImagesZ(),1);

	// Calculate new number of particles.
	size_t numCopies = nPBC[0] * nPBC[1] * nPBC[2];
	if(numCopies <= 1 || pih.inputParticleCount() == 0)
		return PipelineStatus::Success;

	Box3I newImages = mod->replicaRange();

	// Enlarge particle property arrays.
	size_t oldParticleCount = pih.inputParticleCount();
	size_t newParticleCount = oldParticleCount * numCopies;

	poh.setOutputParticleCount(newParticleCount);
	AffineTransformation simCell = pih.expectSimulationCell()->cellMatrix();

	// Replicate particle property values.
	for(DataObject* obj : output.objects()) {
		OORef<ParticleProperty> existingProperty = dynamic_object_cast<ParticleProperty>(obj);
		if(!existingProperty) continue;

		// Create modifiable copy.
		ParticleProperty* newProperty = poh.cloneIfNeeded(existingProperty.get());
		newProperty->resize(newParticleCount, false);

		size_t destinationIndex = 0;

		for(int imageX = newImages.minc.x(); imageX <= newImages.maxc.x(); imageX++) {
			for(int imageY = newImages.minc.y(); imageY <= newImages.maxc.y(); imageY++) {
				for(int imageZ = newImages.minc.z(); imageZ <= newImages.maxc.z(); imageZ++) {

					// Duplicate property data.
					memcpy((char*)newProperty->data() + (destinationIndex * newProperty->stride()),
							existingProperty->constData(), newProperty->stride() * oldParticleCount);

					if(newProperty->type() == ParticleProperty::PositionProperty && (imageX != 0 || imageY != 0 || imageZ != 0)) {
						// Shift particle positions by the periodicity vector.
						const Vector3 imageDelta = simCell * Vector3(imageX, imageY, imageZ);

						const Point3* pend = newProperty->dataPoint3() + (destinationIndex + oldParticleCount);
						for(Point3* p = newProperty->dataPoint3() + destinationIndex; p != pend; ++p)
							*p += imageDelta;
					}

					destinationIndex += oldParticleCount;
				}
			}
		}

		// Assign unique IDs to duplicated particle.
		if(mod->uniqueIdentifiers() && newProperty->type() == ParticleProperty::IdentifierProperty) {
			auto minmax = std::minmax_element(newProperty->constDataInt(), newProperty->constDataInt() + oldParticleCount);
			int minID = *minmax.first;
			int maxID = *minmax.second;
			for(size_t c = 1; c < numCopies; c++) {
				int offset = (maxID - minID + 1) * c;
				for(auto id = newProperty->dataInt() + c * oldParticleCount, id_end = id + oldParticleCount; id != id_end; ++id)
					*id += offset;
			}
		}
	}

	// Replicate bonds.
	size_t oldBondCount = 0;
	size_t newBondCount = 0;
	for(DataObject* obj : output.objects()) {
		OORef<BondsObject> existingBonds = dynamic_object_cast<BondsObject>(obj);
		if(!existingBonds)
			continue;

		// Create modifiable copy.
		BondsObject* newBonds = poh.cloneIfNeeded(existingBonds.get());

		// Duplicate bonds and adjust particle indices and PBC shift vectors as needed.
		// Some bonds may no longer cross periodic boundaries.
		oldBondCount = newBonds->storage()->size();
		newBondCount = oldBondCount * numCopies;
		newBonds->modifiableStorage()->resize(newBondCount);
		auto outBond = newBonds->modifiableStorage()->begin();
		Point3I image;
		for(image[0] = newImages.minc.x(); image[0] <= newImages.maxc.x(); image[0]++) {
			for(image[1] = newImages.minc.y(); image[1] <= newImages.maxc.y(); image[1]++) {
				for(image[2] = newImages.minc.z(); image[2] <= newImages.maxc.z(); image[2]++) {
					auto inBond = existingBonds->storage()->cbegin();
					for(size_t bindex = 0; bindex < oldBondCount; bindex++, ++inBond, ++outBond) {
						Point3I newImage;
						Vector_3<int8_t> newShift;
						for(size_t dim = 0; dim < 3; dim++) {
							int i = image[dim] + (int)inBond->pbcShift[dim] - newImages.minc[dim];
							newImage[dim] = SimulationCell::modulo(i, nPBC[dim]) + newImages.minc[dim];
							newShift[dim] = i >= 0 ? (i / nPBC[dim]) : ((i-nPBC[dim]+1) / nPBC[dim]);
							if(!mod->adjustBoxSize())
								newShift[dim] *= nPBC[dim];
						}
						OVITO_ASSERT(newImage.x() >= newImages.minc.x() && newImage.x() <= newImages.maxc.x());
						OVITO_ASSERT(newImage.y() >= newImages.minc.y() && newImage.y() <= newImages.maxc.y());
						OVITO_ASSERT(newImage.z() >= newImages.minc.z() && newImage.z() <= newImages.maxc.z());
						size_t imageIndex1 =   ((image.x()-newImages.minc.x()) * nPBC[1] * nPBC[2])
										 	 + ((image.y()-newImages.minc.y()) * nPBC[2])
											 +  (image.z()-newImages.minc.z());
						size_t imageIndex2 =   ((newImage.x()-newImages.minc.x()) * nPBC[1] * nPBC[2])
										 	 + ((newImage.y()-newImages.minc.y()) * nPBC[2])
											 +  (newImage.z()-newImages.minc.z());
						outBond->pbcShift = newShift;
						outBond->index1 = inBond->index1 + imageIndex1 * oldParticleCount;
						outBond->index2 = inBond->index2 + imageIndex2 * oldParticleCount;
						OVITO_ASSERT(outBond->index1 < newParticleCount);
						OVITO_ASSERT(outBond->index2 < newParticleCount);
					}
				}
			}
		}
	}

	// Replicate bond property values.
	for(DataObject* obj : output.objects()) {
		OORef<BondProperty> existingProperty = dynamic_object_cast<BondProperty>(obj);
		if(!existingProperty || existingProperty->size() != oldBondCount)
			continue;

		// Create modifiable copy.
		BondProperty* newProperty = poh.cloneIfNeeded(existingProperty.get());
		newProperty->resize(newBondCount, false);

		size_t destinationIndex = 0;
		for(int imageX = newImages.minc.x(); imageX <= newImages.maxc.x(); imageX++) {
			for(int imageY = newImages.minc.y(); imageY <= newImages.maxc.y(); imageY++) {
				for(int imageZ = newImages.minc.z(); imageZ <= newImages.maxc.z(); imageZ++) {

					// Duplicate property data.
					memcpy((char*)newProperty->data() + (destinationIndex * newProperty->stride()),
							existingProperty->constData(), newProperty->stride() * oldBondCount);

					destinationIndex += oldBondCount;
				}
			}
		}
	}

	return PipelineStatus::Success;
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
