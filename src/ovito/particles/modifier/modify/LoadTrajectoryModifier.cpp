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

#include <ovito/particles/Particles.h>
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/particles/objects/BondsObject.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/stdobj/properties/PropertyAccess.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>
#include <ovito/core/dataset/io/FileSource.h>
#include <ovito/core/dataset/animation/AnimationSettings.h>
#include <ovito/core/dataset/data/AttributeDataObject.h>
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
Future<PipelineFlowState> LoadTrajectoryModifier::evaluate(const PipelineEvaluationRequest& request, ModifierApplication* modApp, const PipelineFlowState& input)
{
	OVITO_ASSERT(input);

	// Get the trajectory data source.
	if(!trajectorySource())
		throwException(tr("No trajectory data source has been set."));

	// Get the trajectory frame.
	SharedFuture<PipelineFlowState> trajStateFuture = trajectorySource()->evaluate(request);

	// Wait for the data to become available.
	return trajStateFuture.then(modApp->executor(), [state = input, modApp](const PipelineFlowState& trajState) mutable {

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

		if(!trajState)
			modApp->throwException(tr("Data source has not been specified yet or is empty. Please pick a trajectory file."));

		// Merge validity intervals of topology and trajectory datasets.
		state.intersectStateValidity(trajState.stateValidity());

		// Get the current particle positions.
		const ParticlesObject* trajectoryParticles = trajState.getObject<ParticlesObject>();
		if(!trajectoryParticles)
			modApp->throwException(tr("Trajectory dataset does not contain any particle positions."));
		trajectoryParticles->verifyIntegrity();
		ConstPropertyAccess<Point3> trajectoryPosProperty = trajectoryParticles->expectProperty(ParticlesObject::PositionProperty);

		// Get the positions from the topology dataset.
		ParticlesObject* particles = state.expectMutableObject<ParticlesObject>();
		particles->verifyIntegrity();
		const PropertyObject* posProperty = particles->expectProperty(ParticlesObject::PositionProperty);

		// Build particle-to-particle index map.
		std::vector<size_t> indexToIndexMap(particles->elementCount());
		ConstPropertyAccess<qlonglong> identifierProperty = particles->getProperty(ParticlesObject::IdentifierProperty);
		ConstPropertyAccess<qlonglong> trajIdentifierProperty = trajectoryParticles->getProperty(ParticlesObject::IdentifierProperty);
		if(identifierProperty && trajIdentifierProperty) {

			// Build map of particle identifiers in trajectory dataset.
			std::map<qlonglong, size_t> refMap;
			size_t index = 0;
			for(qlonglong id : trajIdentifierProperty) {
				if(refMap.insert(std::make_pair(id, index++)).second == false)
					modApp->throwException(tr("Particles with duplicate identifiers detected in trajectory data."));
			}

			// Check for duplicate identifiers in topology dataset.
			std::vector<size_t> idSet(identifierProperty.cbegin(), identifierProperty.cend());
			boost::sort(idSet);
			if(boost::adjacent_find(idSet) != idSet.cend())
				modApp->throwException(tr("Particles with duplicate identifiers detected in topology dataset."));

			// Build index map.
			const qlonglong* id = identifierProperty.cbegin();
			for(auto& mappedIndex : indexToIndexMap) {
				auto iter = refMap.find(*id);
				if(iter == refMap.end())
					modApp->throwException(tr("Particle id %1 from topology dataset not found in trajectory dataset.").arg(*id));
				mappedIndex = iter->second;
				++id;
			}
		}
		else {
			// Topology dataset and trajectory data must contain the same number of particles.
			if(posProperty->size() != trajectoryPosProperty.size()) {
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
			property->mappedCopyTo(outputProperty, indexToIndexMap);
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
				ConstPropertyAccess<Point3> outputPosProperty = particles->expectProperty(ParticlesObject::PositionProperty);
				AffineTransformation inverseSimCell = simCell.inverse();

				BondsObject* bonds = particles->makeBondsMutable();
				if(ConstPropertyAccess<ParticleIndexPair> topologyProperty = bonds->getProperty(BondsObject::TopologyProperty)) {
					PropertyAccess<Vector3I> periodicImageProperty = bonds->createProperty(BondsObject::PeriodicImageProperty, true);

					// Wrap bonds crossing a periodic boundary by resetting their PBC shift vectors.
					SimulationCell cell = trajectoryCell->data();
					for(size_t bondIndex = 0; bondIndex < topologyProperty.size(); bondIndex++) {
						size_t particleIndex1 = topologyProperty[bondIndex][0];
						size_t particleIndex2 = topologyProperty[bondIndex][1];
						if(particleIndex1 >= outputPosProperty.size() || particleIndex2 >= outputPosProperty.size())
							continue;
						const Point3& p1 = outputPosProperty[particleIndex1];
						const Point3& p2 = outputPosProperty[particleIndex2];
						Vector3 delta = p1 - p2;
						for(int dim = 0; dim < 3; dim++) {
							if(pbc[dim]) {
								periodicImageProperty[bondIndex][dim] = (int)std::floor(inverseSimCell.prodrow(delta, dim) + FloatType(0.5));
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

/******************************************************************************
* Is called when a RefTarget referenced by this object has generated an event.
******************************************************************************/
bool LoadTrajectoryModifier::referenceEvent(RefTarget* source, const ReferenceEvent& event)
{
	if(event.type() == ReferenceEvent::AnimationFramesChanged && source == trajectorySource()) {
		// Propagate animation interval events from the trajectory source.
		return true;
	}
	return Modifier::referenceEvent(source, event);
}

/******************************************************************************
* Gets called when the data object of the node has been replaced.
******************************************************************************/
void LoadTrajectoryModifier::referenceReplaced(const PropertyFieldDescriptor& field, RefTarget* oldTarget, RefTarget* newTarget)
{
	if(field == PROPERTY_FIELD(trajectorySource) && !isBeingLoaded()) {
		// The animation length might have changed when the trajectory source has been replaced.
		notifyDependents(ReferenceEvent::AnimationFramesChanged);
	}
	Modifier::referenceReplaced(field, oldTarget, newTarget);
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
