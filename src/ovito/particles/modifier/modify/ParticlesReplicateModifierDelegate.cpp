////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2017 Alexander Stukowski
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

#include <ovito/particles/Particles.h>
#include <ovito/particles/objects/BondsObject.h>
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>
#include "ParticlesReplicateModifierDelegate.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Modify)

IMPLEMENT_OVITO_CLASS(ParticlesReplicateModifierDelegate);

/******************************************************************************
* Indicates which data objects in the given input data collection the modifier
* delegate is able to operate on.
******************************************************************************/
QVector<DataObjectReference> ParticlesReplicateModifierDelegate::OOMetaClass::getApplicableObjects(const DataCollection& input) const
{
	if(input.containsObject<ParticlesObject>())
		return { DataObjectReference(&ParticlesObject::OOClass()) };
	return {};
}

/******************************************************************************
* Applies the modifier operation to the data in a pipeline flow state.
******************************************************************************/
PipelineStatus ParticlesReplicateModifierDelegate::apply(Modifier* modifier, PipelineFlowState& state, TimePoint time, ModifierApplication* modApp, const std::vector<std::reference_wrapper<const PipelineFlowState>>& additionalInputs)
{
	ReplicateModifier* mod = static_object_cast<ReplicateModifier>(modifier);
	const ParticlesObject* inputParticles = state.getObject<ParticlesObject>();

	std::array<int,3> nPBC;
	nPBC[0] = std::max(mod->numImagesX(),1);
	nPBC[1] = std::max(mod->numImagesY(),1);
	nPBC[2] = std::max(mod->numImagesZ(),1);

	// Calculate new number of particles.
	size_t numCopies = nPBC[0] * nPBC[1] * nPBC[2];
	if(numCopies <= 1 || !inputParticles || inputParticles->elementCount() == 0)
		return PipelineStatus::Success;

	// Extend particle property arrays.
	size_t oldParticleCount = inputParticles->elementCount();
	size_t newParticleCount = oldParticleCount * numCopies;

	const AffineTransformation& simCell = state.expectObject<SimulationCellObject>()->cellMatrix();

	// Ensure that the particles can be modified.
	ParticlesObject* outputParticles = state.makeMutable(inputParticles);
	outputParticles->replicate(numCopies);

	// Replicate particle property values.
	Box3I newImages = mod->replicaRange();
	for(PropertyObject* property : outputParticles->properties()) {
		OVITO_ASSERT(property->size() == newParticleCount);

		// Shift particle positions by the periodicity vector.
		if(property->type() == ParticlesObject::PositionProperty) {
			Point3* p = property->data<Point3>();
			for(int imageX = newImages.minc.x(); imageX <= newImages.maxc.x(); imageX++) {
				for(int imageY = newImages.minc.y(); imageY <= newImages.maxc.y(); imageY++) {
					for(int imageZ = newImages.minc.z(); imageZ <= newImages.maxc.z(); imageZ++) {
						if(imageX != 0 || imageY != 0 || imageZ != 0) {
							const Vector3 imageDelta = simCell * Vector3(imageX, imageY, imageZ);
							for(size_t i = 0; i < oldParticleCount; i++)
								*p++ += imageDelta;
						}
						else {
							p += oldParticleCount;
						}
					}
				}
			}
		}

		// Assign unique IDs to duplicated particles.
		if(mod->uniqueIdentifiers() && (property->type() == ParticlesObject::IdentifierProperty || property->type() == ParticlesObject::MoleculeProperty)) {
			auto minmax = std::minmax_element(property->cdata<qlonglong>(), property->cdata<qlonglong>() + oldParticleCount);
			auto minID = *minmax.first;
			auto maxID = *minmax.second;
			for(size_t c = 1; c < numCopies; c++) {
				auto offset = (maxID - minID + 1) * c;
				for(auto id = property->data<qlonglong>() + c * oldParticleCount, id_end = id + oldParticleCount; id != id_end; ++id)
					*id += offset;
			}
		}
	}

	// Replicate bonds.
	if(outputParticles->bonds()) {
		if(ConstPropertyPtr oldTopology = outputParticles->bonds()->getPropertyStorage(BondsObject::TopologyProperty)) {

			size_t oldBondCount = oldTopology->size();
			size_t newBondCount = oldBondCount * numCopies;

			ConstPropertyPtr oldPeriodicImages = outputParticles->bonds()->getPropertyStorage(BondsObject::PeriodicImageProperty);

			// Replicate bond property values.
			outputParticles->makeBondsMutable();
			outputParticles->bonds()->makePropertiesMutable();
			outputParticles->bonds()->replicate(numCopies);
			for(PropertyObject* property : outputParticles->bonds()->properties()) {
				OVITO_ASSERT(property->size() == newBondCount);

				size_t destinationIndex = 0;
				Point3I image;

				// Special handling for the topology property.
				if(property->type() == BondsObject::TopologyProperty) {
					for(image[0] = newImages.minc.x(); image[0] <= newImages.maxc.x(); image[0]++) {
						for(image[1] = newImages.minc.y(); image[1] <= newImages.maxc.y(); image[1]++) {
							for(image[2] = newImages.minc.z(); image[2] <= newImages.maxc.z(); image[2]++) {
								for(size_t bindex = 0; bindex < oldBondCount; bindex++, destinationIndex++) {
									Point3I newImage;
									for(size_t dim = 0; dim < 3; dim++) {
										int i = image[dim] + (oldPeriodicImages ? oldPeriodicImages->get<int>(bindex, dim) : 0) - newImages.minc[dim];
										newImage[dim] = SimulationCell::modulo(i, nPBC[dim]) + newImages.minc[dim];
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
									property->set<qlonglong>(destinationIndex, 0, property->get<qlonglong>(destinationIndex, 0) + imageIndex1 * oldParticleCount);
									property->set<qlonglong>(destinationIndex, 1, property->get<qlonglong>(destinationIndex, 1) + imageIndex2 * oldParticleCount);
									OVITO_ASSERT(property->get<qlonglong>(destinationIndex, 0) < newParticleCount);
									OVITO_ASSERT(property->get<qlonglong>(destinationIndex, 1) < newParticleCount);
								}
							}
						}
					}
				}
				else if(property->type() == BondsObject::PeriodicImageProperty) {
					// Special handling for the PBC shift vector property.
					OVITO_ASSERT(oldPeriodicImages);
					for(image[0] = newImages.minc.x(); image[0] <= newImages.maxc.x(); image[0]++) {
						for(image[1] = newImages.minc.y(); image[1] <= newImages.maxc.y(); image[1]++) {
							for(image[2] = newImages.minc.z(); image[2] <= newImages.maxc.z(); image[2]++) {
								for(size_t bindex = 0; bindex < oldBondCount; bindex++, destinationIndex++) {
									Vector3I newShift;
									for(size_t dim = 0; dim < 3; dim++) {
										int i = image[dim] + oldPeriodicImages->get<int>(bindex, dim) - newImages.minc[dim];
										newShift[dim] = i >= 0 ? (i / nPBC[dim]) : ((i-nPBC[dim]+1) / nPBC[dim]);
										if(!mod->adjustBoxSize())
											newShift[dim] *= nPBC[dim];
									}
									property->set<Vector3I>(destinationIndex, newShift);
								}
							}
						}
					}
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
