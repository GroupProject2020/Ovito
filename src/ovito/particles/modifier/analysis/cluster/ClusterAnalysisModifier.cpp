////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2020 Alexander Stukowski
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
#include <ovito/particles/objects/ParticleBondMap.h>
#include <ovito/stdobj/properties/PropertyAccess.h>
#include <ovito/stdobj/table/DataTable.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/utilities/units/UnitsManager.h>
#include "ClusterAnalysisModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

IMPLEMENT_OVITO_CLASS(ClusterAnalysisModifier);
DEFINE_PROPERTY_FIELD(ClusterAnalysisModifier, neighborMode);
DEFINE_PROPERTY_FIELD(ClusterAnalysisModifier, cutoff);
DEFINE_PROPERTY_FIELD(ClusterAnalysisModifier, onlySelectedParticles);
DEFINE_PROPERTY_FIELD(ClusterAnalysisModifier, sortBySize);
DEFINE_PROPERTY_FIELD(ClusterAnalysisModifier, unwrapParticleCoordinates);
DEFINE_PROPERTY_FIELD(ClusterAnalysisModifier, computeCentersOfMass);
DEFINE_PROPERTY_FIELD(ClusterAnalysisModifier, colorParticlesByCluster);
SET_PROPERTY_FIELD_LABEL(ClusterAnalysisModifier, neighborMode, "Neighbor mode");
SET_PROPERTY_FIELD_LABEL(ClusterAnalysisModifier, cutoff, "Cutoff distance");
SET_PROPERTY_FIELD_LABEL(ClusterAnalysisModifier, onlySelectedParticles, "Use only selected particles");
SET_PROPERTY_FIELD_LABEL(ClusterAnalysisModifier, sortBySize, "Sort clusters by size");
SET_PROPERTY_FIELD_LABEL(ClusterAnalysisModifier, unwrapParticleCoordinates, "Unwrap particle coordinates");
SET_PROPERTY_FIELD_LABEL(ClusterAnalysisModifier, computeCentersOfMass, "Compute centers of mass");
SET_PROPERTY_FIELD_LABEL(ClusterAnalysisModifier, colorParticlesByCluster, "Color particles by cluster");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(ClusterAnalysisModifier, cutoff, WorldParameterUnit, 0);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
ClusterAnalysisModifier::ClusterAnalysisModifier(DataSet* dataset) : AsynchronousModifier(dataset),
	_cutoff(3.2),
	_onlySelectedParticles(false),
	_sortBySize(false),
	_neighborMode(CutoffRange),
	_unwrapParticleCoordinates(false),
	_computeCentersOfMass(false),
	_colorParticlesByCluster(false)
{
}

/******************************************************************************
* Asks the modifier whether it can be applied to the given input data.
******************************************************************************/
bool ClusterAnalysisModifier::OOMetaClass::isApplicableTo(const DataCollection& input) const
{
	return input.containsObject<ParticlesObject>();
}

/******************************************************************************
* Creates and initializes a computation engine that will compute the modifier's results.
******************************************************************************/
Future<AsynchronousModifier::ComputeEnginePtr> ClusterAnalysisModifier::createEngine(const PipelineEvaluationRequest& request, ModifierApplication* modApp, const PipelineFlowState& input)
{
	// Get the current particle positions.
	const ParticlesObject* particles = input.expectObject<ParticlesObject>();
	particles->verifyIntegrity();
	const PropertyObject* posProperty = particles->expectProperty(ParticlesObject::PositionProperty);

	// Get simulation cell.
	const SimulationCellObject* inputCell = input.expectObject<SimulationCellObject>();

	// Get particle selection.
	ConstPropertyPtr selectionProperty;
	if(onlySelectedParticles())
		selectionProperty = particles->expectProperty(ParticlesObject::SelectionProperty)->storage();

	// Get the periodic image bond property if there are bonds.
	ConstPropertyPtr periodicImageBondProperty;
	if(unwrapParticleCoordinates() && particles->bonds()) {
		periodicImageBondProperty = particles->bonds()->getPropertyStorage(BondsObject::PeriodicImageProperty);
		if(!periodicImageBondProperty)
			periodicImageBondProperty = BondsObject::OOClass().createStandardStorage(particles->bonds()->elementCount(), BondsObject::PeriodicImageProperty, true);
	}

	// Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
	if(neighborMode() == CutoffRange) {
		ConstPropertyPtr bondTopology = (periodicImageBondProperty && particles->bonds()) ? particles->bonds()->getPropertyStorage(BondsObject::TopologyProperty) : nullptr;
		return std::make_shared<CutoffClusterAnalysisEngine>(particles, posProperty->storage(), inputCell->data(), 
			sortBySize(), unwrapParticleCoordinates(), computeCentersOfMass(), std::move(selectionProperty), 
			periodicImageBondProperty, std::move(bondTopology), cutoff());
	}
	else if(neighborMode() == Bonding) {
		particles->expectBonds()->verifyIntegrity();
		return std::make_shared<BondClusterAnalysisEngine>(particles, posProperty->storage(), inputCell->data(), 
			sortBySize(), unwrapParticleCoordinates(), computeCentersOfMass(), std::move(selectionProperty), 
			periodicImageBondProperty, particles->expectBondsTopology()->storage());
	}
	else {
		throwException(tr("Invalid cluster neighbor mode"));
	}
}

/******************************************************************************
* Performs the actual computation. This method is executed in a worker thread.
******************************************************************************/
void ClusterAnalysisModifier::ClusterAnalysisEngine::perform()
{
	task()->setProgressText(tr("Performing cluster analysis"));

	// Initialize.
	particleClusters()->fill<qlonglong>(-1);

	// Perform the actual clustering.
	doClustering();
	if(task()->isCanceled())
		return;

	// Wrap bonds at periodic cell boundaries after particle coordinates have been unwrapped. 
	if(_periodicImageBondProperty && _periodicImageBondProperty->size() == bondTopology()->size()) {
		OVITO_ASSERT(_unwrappedPositions);

		const std::array<bool, 3> pbcFlags = cell().pbcFlags();
		if(!pbcFlags[0] && !pbcFlags[1] && !pbcFlags[2]) {
			// No wrapping needed if there are no PBCs.
			_periodicImageBondProperty.reset(); 
		}
		else {
			ConstPropertyAccess<Point3> positionsArray(positions());
			ConstPropertyAccess<Point3> unwrappedPositionsArray(_unwrappedPositions);
			const AffineTransformation inverseSimCell = cell().inverseMatrix();
			PropertyAccess<Vector3I> pbcArray(_periodicImageBondProperty);
			Vector3I* pbcVec = pbcArray.begin();
			for(const ParticleIndexPair& bond : ConstPropertyAccess<ParticleIndexPair>(bondTopology())) {
				if(bond[0] < positionsArray.size() && bond[1] < positionsArray.size()) {
					Vector3 s1 = unwrappedPositionsArray[bond[0]] - positionsArray[bond[0]];
					Vector3 s2 = unwrappedPositionsArray[bond[1]] - positionsArray[bond[1]];
					for(size_t dim = 0; dim < 3; dim++) {
						if(pbcFlags[dim]) {
							(*pbcVec)[dim] +=
									std::lround(inverseSimCell.prodrow(s1, dim)) - std::lround(inverseSimCell.prodrow(s2, dim));
						}
					}
				}
				++pbcVec;
			}
			if(task()->isCanceled())
				return;
		}
	}

	// Determine cluster sizes.
	_clusterSizes = std::make_shared<PropertyStorage>(numClusters(), PropertyStorage::Int64, 1, 0, QStringLiteral("Cluster Size"), true, DataTable::YProperty);
	PropertyAccess<qlonglong> clusterSizeArray(_clusterSizes);
	for(auto id : ConstPropertyAccess<qlonglong>(particleClusters())) {
		if(id != 0) clusterSizeArray[id-1]++;
	}
	if(task()->isCanceled())
		return;

	// Create custer ID property.
	_clusterIds =  std::make_shared<PropertyStorage>(numClusters(), PropertyStorage::Int64, 1, 0, QStringLiteral("Cluster Identifier"), false, DataTable::XProperty);
	boost::algorithm::iota_n(PropertyAccess<qlonglong>(_clusterIds).begin(), size_t(1), _clusterIds->size());

	// Sort clusters by size.
	if(_sortBySize && numClusters() != 0) {

		// Determine new cluster ordering.
		std::vector<size_t> mapping(clusterSizeArray.size());
		std::iota(mapping.begin(), mapping.end(), size_t(0));
		std::sort(mapping.begin(), mapping.end(), [&](size_t a, size_t b) {
			return clusterSizeArray[a] > clusterSizeArray[b];
		});
		std::sort(clusterSizeArray.begin(), clusterSizeArray.end(), std::greater<>());
		setLargestClusterSize(clusterSizeArray[0]);

		// Reorder centers of mass.
		if(_computeCentersOfMass) {
			PropertyPtr oldCentersOfMass = _centersOfMass;
			PropertyStorage::makeMutable(_centersOfMass);
			oldCentersOfMass->mappedCopyTo(*_centersOfMass, mapping);
		}

		// Remap cluster IDs of particles.
		std::vector<size_t> inverseMapping(numClusters() + 1);
		inverseMapping[0] = 0;
		for(size_t i = 0; i < numClusters(); i++)
			inverseMapping[mapping[i]+1] = i+1;
		for(auto& id : PropertyAccess<qlonglong>(particleClusters()))
			id = inverseMapping[id];
	}
}

/******************************************************************************
* Performs the actual clustering algorithm.
******************************************************************************/
void ClusterAnalysisModifier::CutoffClusterAnalysisEngine::doClustering()
{
	// Prepare the neighbor finder.
	CutoffNeighborFinder neighborFinder;
	if(!neighborFinder.prepare(cutoff(), positions(), cell(), selection(), task().get()))
		return;

	size_t particleCount = positions()->size();
	task()->setProgressValue(0);
	task()->setProgressMaximum(particleCount);
	size_t progress = 0;

	PropertyAccess<qlonglong> particleClusters(this->particleClusters());
	ConstPropertyAccess<int> selectionData(selection());
	PropertyAccess<Point3> unwrappedCoordinates(_unwrappedPositions);
	PropertyAccess<Point3> comArray(_centersOfMass);

	std::deque<size_t> toProcess;
	for(size_t seedParticleIndex = 0; seedParticleIndex < particleCount; seedParticleIndex++) {

		// Skip unselected particles that are not included in the analysis.
		if(selectionData && !selectionData[seedParticleIndex]) {
			particleClusters[seedParticleIndex] = 0;
			progress++;
			continue;
		}

		// Skip particles that have already been assigned to a cluster.
		if(particleClusters[seedParticleIndex] != -1)
			continue;

		// Start a new cluster.
		setNumClusters(numClusters() + 1);
		qlonglong cluster = numClusters();
		particleClusters[seedParticleIndex] = cluster;
		Vector3 centerOfMass = Vector3::Zero();
		size_t clusterSize = 1;

		// Now recursively iterate over all neighbors of the seed particle and add them to the cluster too.
		OVITO_ASSERT(toProcess.empty());
		toProcess.push_back(seedParticleIndex);

		do {
			if(!task()->setProgressValueIntermittent(progress++))
				return;

			size_t currentParticle = toProcess.front();
			toProcess.pop_front();
			for(CutoffNeighborFinder::Query neighQuery(neighborFinder, currentParticle); !neighQuery.atEnd(); neighQuery.next()) {
				size_t neighborIndex = neighQuery.current();
				if(particleClusters[neighborIndex] == -1) {
					particleClusters[neighborIndex] = cluster;
					toProcess.push_back(neighborIndex);
					if(unwrappedCoordinates) {
						unwrappedCoordinates[neighborIndex] = unwrappedCoordinates[currentParticle] + neighQuery.delta();
						centerOfMass += unwrappedCoordinates[neighborIndex] - Point3::Origin();
						clusterSize++;
					}
				}
			}
		}
		while(toProcess.empty() == false);

		if(_centersOfMass) {
			centerOfMass += unwrappedCoordinates[seedParticleIndex] - Point3::Origin();
			_centersOfMass->grow(1);
			comArray[comArray.size() - 1] = Point3::Origin() + (centerOfMass / clusterSize);
		}
	}
}

/******************************************************************************
* Performs the actual clustering algorithm.
******************************************************************************/
void ClusterAnalysisModifier::BondClusterAnalysisEngine::doClustering()
{
	size_t particleCount = positions()->size();
	task()->setProgressValue(0);
	task()->setProgressMaximum(particleCount);
	size_t progress = 0;

	// Prepare particle bond map.
	ParticleBondMap bondMap(bondTopology());

	PropertyAccess<qlonglong> particleClusters(this->particleClusters());
	ConstPropertyAccess<int> selectionData(this->selection());
	ConstPropertyAccess<ParticleIndexPair> bondTopology(this->bondTopology());
	PropertyAccess<Point3> unwrappedCoordinates(_unwrappedPositions);
	PropertyAccess<Point3> comArray(_centersOfMass);

	std::deque<size_t> toProcess;
	for(size_t seedParticleIndex = 0; seedParticleIndex < particleCount; seedParticleIndex++) {

		// Skip unselected particles that are not included in the analysis.
		if(selectionData && !selectionData[seedParticleIndex]) {
			particleClusters[seedParticleIndex] = 0;
			progress++;
			continue;
		}

		// Skip particles that have already been assigned to a cluster.
		if(particleClusters[seedParticleIndex] != -1)
			continue;

		// Start a new cluster.
		setNumClusters(numClusters() + 1);
		qlonglong cluster = numClusters();
		particleClusters[seedParticleIndex] = cluster;
		Vector3 centerOfMass = Vector3::Zero();
		size_t clusterSize = 1;

		// Now recursively iterate over all neighbors of the seed particle and add them to the cluster too.
		OVITO_ASSERT(toProcess.empty());
		toProcess.push_back(seedParticleIndex);

		do {
			if(!task()->setProgressValueIntermittent(progress++))
				return;

			size_t currentParticle = toProcess.front();
			toProcess.pop_front();

			// Iterate over all bonds of the current particle.
			for(size_t neighborBondIndex : bondMap.bondIndicesOfParticle(currentParticle)) {
				OVITO_ASSERT(bondTopology[neighborBondIndex][0] == currentParticle || bondTopology[neighborBondIndex][1] == currentParticle);
				size_t neighborIndex = bondTopology[neighborBondIndex][0];
				if(neighborIndex == currentParticle)
					neighborIndex = bondTopology[neighborBondIndex][1];
				if(neighborIndex >= particleCount)
					continue;
				if(particleClusters[neighborIndex] != -1)
					continue;
				if(selectionData && !selectionData[neighborIndex])
					continue;

				particleClusters[neighborIndex] = cluster;
				toProcess.push_back(neighborIndex);

				if(unwrappedCoordinates) {
					Vector3 delta = cell().wrapVector(unwrappedCoordinates[neighborIndex] - unwrappedCoordinates[currentParticle]);
					unwrappedCoordinates[neighborIndex] = unwrappedCoordinates[currentParticle] + delta;
					centerOfMass += unwrappedCoordinates[neighborIndex] - Point3::Origin();
					clusterSize++;
				}
			}
		}
		while(toProcess.empty() == false);

		if(_centersOfMass) {
			centerOfMass += unwrappedCoordinates[seedParticleIndex] - Point3::Origin();
			_centersOfMass->grow(1);
			comArray[comArray.size() - 1] = Point3::Origin() + (centerOfMass / clusterSize);
		}
	}
}

/******************************************************************************
* Injects the computed results of the engine into the data pipeline.
******************************************************************************/
void ClusterAnalysisModifier::ClusterAnalysisEngine::emitResults(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state)
{
	ClusterAnalysisModifier* modifier = static_object_cast<ClusterAnalysisModifier>(modApp->modifier());
	ParticlesObject* particles = state.expectMutableObject<ParticlesObject>();

	if(_inputFingerprint.hasChanged(particles))
		modApp->throwException(tr("Cached modifier results are obsolete, because the number or the storage order of input particles has changed."));

	// Output the cluster assignment.
	particles->createProperty(particleClusters());

	// Give clusters a random color.
	if(modifier->colorParticlesByCluster()) {
		// Assign random colors to clusters.
		std::vector<Color> clusterColors(numClusters() + 1);
		std::default_random_engine rng(1);
		std::uniform_real_distribution<FloatType> uniform_dist(0, 1);
		boost::generate(clusterColors, [&]() { return Color::fromHSV(uniform_dist(rng), 1.0 - uniform_dist(rng) * 0.4, 1.0 - uniform_dist(rng) * 0.3); });
		// Special color for particles not part of any cluster:
		clusterColors[0] = Color(0.8, 0.8, 0.8);

		// Assign colors to particles according to the clusters they belong to.
		PropertyAccess<Color> colorsArray = particles->createProperty(ParticlesObject::ColorProperty, false);
		boost::transform(ConstPropertyAccess<qlonglong>(particleClusters()), colorsArray.begin(), [&](qlonglong cluster) { 
			OVITO_ASSERT(cluster >= 0 && cluster < clusterColors.size());
			return clusterColors[cluster];
		});
	}

	// Output unwrapped particle coordinates.
	if(modifier->unwrapParticleCoordinates() && _unwrappedPositions) {
		particles->createProperty(_unwrappedPositions);

		// Correct the PBC flags of the bonds if particles have been unwrapped.
		if(particles->bonds() && _periodicImageBondProperty && _periodicImageBondProperty->size() == particles->bonds()->elementCount()) {
			particles->makeBondsMutable()->createProperty(_periodicImageBondProperty);
		}
	}

	state.addAttribute(QStringLiteral("ClusterAnalysis.cluster_count"), QVariant::fromValue(numClusters()), modApp);
	if(modifier->sortBySize())
		state.addAttribute(QStringLiteral("ClusterAnalysis.largest_size"), QVariant::fromValue(largestClusterSize()), modApp);

	// Output a data table with the cluster list.
	DataTable* table = state.createObject<DataTable>(QStringLiteral("clusters"), modApp, DataTable::Scatter, tr("Cluster list"), _clusterSizes, _clusterIds);

	// Output centers of mass.
	if(modifier->computeCentersOfMass() && _centersOfMass)
		table->createProperty(_centersOfMass);

	state.setStatus(PipelineStatus(PipelineStatus::Success, tr("Found %n cluster(s).", "", numClusters())));
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
