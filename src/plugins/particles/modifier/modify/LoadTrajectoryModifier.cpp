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
#include <plugins/particles/objects/ParticlesObject.h>
#include <plugins/particles/objects/BondsObject.h>
#include <core/dataset/pipeline/ModifierApplication.h>
#include <plugins/stdobj/simcell/SimulationCellObject.h>
#include <core/dataset/io/FileSource.h>
#include <core/dataset/animation/AnimationSettings.h>
#include <core/dataset/data/AttributeDataObject.h>
#include "LoadTrajectoryModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Modify)

IMPLEMENT_OVITO_CLASS(LoadTrajectoryModifier);
DEFINE_REFERENCE_FIELD(LoadTrajectoryModifier, trajectorySource);
SET_PROPERTY_FIELD_LABEL(LoadTrajectoryModifier, trajectorySource, "Trajectory source");

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
LoadTrajectoryModifier::LoadTrajectoryModifier(DataSet* dataset) : Modifier(dataset)
{
	// Create the file source object, which will be responsible for loading
	// and caching the trajectory data.
	OORef<FileSource> fileSource(new FileSource(dataset));

	// Enable automatic adjustment of animation length for the trajectory source object.
	fileSource->setAdjustAnimationIntervalEnabled(true);

	setTrajectorySource(fileSource);
}

/******************************************************************************
* Asks the modifier whether it can be applied to the given input data.
******************************************************************************/
bool LoadTrajectoryModifier::OOMetaClass::isApplicableTo(const DataCollection& input) const
{
	return input.containsObject<ParticlesObject>();
}

/******************************************************************************
* Modifies the input data.
******************************************************************************/
Future<PipelineFlowState> LoadTrajectoryModifier::evaluate(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input)
{
	// Get the trajectory data source.
	if(!trajectorySource())
		throwException(tr("No trajectory data has been provided."));

	// Get the trajectory frame.
	SharedFuture<PipelineFlowState> trajStateFuture = trajectorySource()->evaluate(time);
	
	// Wait for the data to become available.
	return trajStateFuture.then(modApp->executor(), [state = input, modApp](const PipelineFlowState& trajState) mutable {
		UndoSuspender noUndo(modApp);
		
		// Make sure the obtained configuration is valid and ready to use.
		if(trajState.status().type() == PipelineStatus::Error) {
			if(LoadTrajectoryModifier* trajModifier = dynamic_object_cast<LoadTrajectoryModifier>(modApp->modifier())) {
				if(FileSource* fileSource = dynamic_object_cast<FileSource>(trajModifier->trajectorySource())) {
					if(fileSource->sourceUrls().empty())
						modApp->throwException(tr("Please pick the input file containing the trajectories."));
				}
			}
			state.setStatus(trajState.status());
			return std::move(state);
		}

		if(trajState.isEmpty())
			modApp->throwException(tr("Data source has not been specified yet or is empty. Please pick a trajectory file."));

		// Merge validity intervals of topology and trajectory datasets.
		state.intersectStateValidity(trajState.stateValidity());

		// Get the current particle positions.
		const ParticlesObject* trajectoryParticles = trajState.getObject<ParticlesObject>();
		if(!trajectoryParticles)
			modApp->throwException(tr("Trajectory dataset does not contain any particle positions."));
		const PropertyObject* trajectoryPosProperty = trajectoryParticles->expectProperty(ParticlesObject::PositionProperty);

		// Get the positions from the topology dataset.
		ParticlesObject* particles = state.expectMutableObject<ParticlesObject>();
		const PropertyObject* posProperty = particles->expectProperty(ParticlesObject::PositionProperty);

		// Build particle-to-particle index map.
		std::vector<size_t> indexToIndexMap(particles->elementCount());
		const PropertyObject* identifierProperty = particles->getProperty(ParticlesObject::IdentifierProperty);
		const PropertyObject* trajIdentifierProperty = trajectoryParticles->getProperty(ParticlesObject::IdentifierProperty);
		if(identifierProperty && trajIdentifierProperty) {

			// Build map of particle identifiers in trajectory dataset.
			std::map<qlonglong, size_t> refMap;
			size_t index = 0;
			auto id = trajIdentifierProperty->constDataInt64();
			auto id_end = id + trajIdentifierProperty->size();
			for(; id != id_end; ++id, ++index) {
				if(refMap.insert(std::make_pair(*id, index)).second == false)
					modApp->throwException(tr("Particles with duplicate identifiers detected in trajectory data."));
			}

			// Check for duplicate identifiers in topology dataset.
			std::vector<size_t> idSet(identifierProperty->constDataInt64(), identifierProperty->constDataInt64() + identifierProperty->size());
			std::sort(idSet.begin(), idSet.end());
			if(std::adjacent_find(idSet.begin(), idSet.end()) != idSet.end())
				modApp->throwException(tr("Particles with duplicate identifiers detected in topology dataset."));

			// Build index map.
			index = 0;
			id = identifierProperty->constDataInt64();
			for(auto& mappedIndex : indexToIndexMap) {
				auto iter = refMap.find(*id);
				if(iter == refMap.end())
					modApp->throwException(tr("Particle id %1 from topology dataset not found in trajectory dataset.").arg(*id));
				mappedIndex = iter->second;
				index++;
				++id;
			}
		}
		else {
			// Topology dataset and trajectory data must contain the same number of particles.
			if(posProperty->size() != trajectoryPosProperty->size()) {
				modApp->throwException(tr("Cannot apply trajectories to current particle dataset. Numbers of particles in the trajectory file and in the topology file do not match."));
			}

			// When particle identifiers are not available, use trivial 1-to-1 mapping.
			std::iota(indexToIndexMap.begin(), indexToIndexMap.end(), size_t(0));
		}

		// Transfer particle properties from the trajectory file.
		for(const PropertyObject* property : trajectoryParticles->properties()) {
			if(property->type() == ParticlesObject::IdentifierProperty)
				continue;

			// Get or create the output particle property.
			PropertyObject* outputProperty;
			if(property->type() != ParticlesObject::UserProperty) {
				outputProperty = particles->createProperty(property->type(), true);
				if(outputProperty->dataType() != property->dataType()
					|| outputProperty->componentCount() != property->componentCount())
					continue; // Types of source property and output property are not compatible.
			}
			else {
				outputProperty = particles->createProperty(property->name(), 
					property->dataType(), property->componentCount(),
					0, true);
			}
			OVITO_ASSERT(outputProperty->stride() == property->stride());

			// Copy and reorder property data.
			std::vector<size_t>::const_iterator idx = indexToIndexMap.cbegin();
			char* dest = static_cast<char*>(outputProperty->data());
			const char* src = static_cast<const char*>(property->constData());
			size_t stride = outputProperty->stride();
			for(size_t index = 0; index < outputProperty->size(); index++, ++idx, dest += stride) {
				memcpy(dest, src + stride * (*idx), stride);
			}
		}

		// Transfer box geometry.
		const SimulationCellObject* topologyCell = state.getObject<SimulationCellObject>();
		const SimulationCellObject* trajectoryCell = trajState.getObject<SimulationCellObject>();
		if(topologyCell && trajectoryCell) {
			SimulationCellObject* outputCell = state.makeMutable(topologyCell);
			outputCell->setCellMatrix(trajectoryCell->cellMatrix());
			const AffineTransformation& simCell = trajectoryCell->cellMatrix();

			// Trajectories of atoms may cross periodic boundaries and if atomic positions are
			// stored in wrapped coordinates, then it becomes necessary to fix bonds using the minimum image convention.
			std::array<bool, 3> pbc = topologyCell->pbcFlags();
			if((pbc[0] || pbc[1] || pbc[2]) && particles->bonds() && std::abs(simCell.determinant()) > FLOATTYPE_EPSILON) {
				const PropertyObject* outputPosProperty = particles->expectProperty(ParticlesObject::PositionProperty);
				AffineTransformation inverseSimCell = simCell.inverse();

				if(ConstPropertyPtr topologyProperty = particles->bonds()->getPropertyStorage(BondsObject::TopologyProperty)) {
					BondsObject* bonds = particles->makeBondsMutable();
					PropertyObject* periodicImageProperty = bonds->createProperty(BondsObject::PeriodicImageProperty, true);

					// Wrap bonds crossing a periodic boundary by resetting their PBC shift vectors.
					SimulationCell cell = trajectoryCell->data();
					for(size_t bondIndex = 0; bondIndex < topologyProperty->size(); bondIndex++) {
						size_t particleIndex1 = topologyProperty->getInt64Component(bondIndex, 0);
						size_t particleIndex2 = topologyProperty->getInt64Component(bondIndex, 1);
						if(particleIndex1 >= outputPosProperty->size() || particleIndex2 >= outputPosProperty->size())
							continue;
						const Point3& p1 = outputPosProperty->getPoint3(particleIndex1);
						const Point3& p2 = outputPosProperty->getPoint3(particleIndex2);
						Vector3 delta = p1 - p2; 
						for(int dim = 0; dim < 3; dim++) {
							if(pbc[dim]) {
								periodicImageProperty->setIntComponent(bondIndex, dim, (int)floor(inverseSimCell.prodrow(delta, dim) + FloatType(0.5)));
							}
						}
					}
				}
			}
		}

		// Merge attributes of topology and trajectory datasets.
		// If there is a naming collision, attributes from the trajectory dataset override those from the topology dataset. 
		for(const DataObject* obj : trajState.data()->objects()) {
			if(const AttributeDataObject* attribute = dynamic_object_cast<AttributeDataObject>(obj)) {
				const AttributeDataObject* existingAttribute = nullptr; 
				for(const DataObject* obj2 : state.data()->objects()) {
					if(const AttributeDataObject* attribute2 = dynamic_object_cast<AttributeDataObject>(obj2)) {
						if(attribute2->identifier() == attribute->identifier()) {
							existingAttribute = attribute2;
							break;
						}
					}
				}
				if(existingAttribute)
					state.mutableData()->replaceObject(existingAttribute, attribute);
				else
					state.addObject(attribute);
			}
		}

		return std::move(state);
	});
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
