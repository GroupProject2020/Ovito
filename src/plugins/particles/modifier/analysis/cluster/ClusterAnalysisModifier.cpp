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
#include <plugins/particles/objects/BondsObject.h>
#include <plugins/particles/modifier/ParticleInputHelper.h>
#include <plugins/particles/modifier/ParticleOutputHelper.h>
#include <core/dataset/pipeline/ModifierApplication.h>
#include <plugins/stdobj/simcell/SimulationCellObject.h>
#include "ClusterAnalysisModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

IMPLEMENT_OVITO_CLASS(ClusterAnalysisModifier);
DEFINE_PROPERTY_FIELD(ClusterAnalysisModifier, neighborMode);
DEFINE_PROPERTY_FIELD(ClusterAnalysisModifier, cutoff);
DEFINE_PROPERTY_FIELD(ClusterAnalysisModifier, onlySelectedParticles);
DEFINE_PROPERTY_FIELD(ClusterAnalysisModifier, sortBySize);
SET_PROPERTY_FIELD_LABEL(ClusterAnalysisModifier, neighborMode, "Neighbor mode");
SET_PROPERTY_FIELD_LABEL(ClusterAnalysisModifier, cutoff, "Cutoff distance");
SET_PROPERTY_FIELD_LABEL(ClusterAnalysisModifier, onlySelectedParticles, "Use only selected particles");
SET_PROPERTY_FIELD_LABEL(ClusterAnalysisModifier, sortBySize, "Sort clusters by size");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(ClusterAnalysisModifier, cutoff, WorldParameterUnit, 0);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
ClusterAnalysisModifier::ClusterAnalysisModifier(DataSet* dataset) : AsynchronousModifier(dataset),
	_cutoff(3.2), 
	_onlySelectedParticles(false), 
	_sortBySize(false), 
	_neighborMode(CutoffRange)
{
}

/******************************************************************************
* Asks the modifier whether it can be applied to the given input data.
******************************************************************************/
bool ClusterAnalysisModifier::OOMetaClass::isApplicableTo(const PipelineFlowState& input) const
{
	return input.findObject<ParticleProperty>() != nullptr;
}

/******************************************************************************
* Creates and initializes a computation engine that will compute the modifier's results.
******************************************************************************/
Future<AsynchronousModifier::ComputeEnginePtr> ClusterAnalysisModifier::createEngine(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input)
{
	// Get the current particle positions.
	ParticleInputHelper pih(dataset(), input);
	ParticleProperty* posProperty = pih.expectStandardProperty<ParticleProperty>(ParticleProperty::PositionProperty);

	// Get simulation cell.
	SimulationCellObject* inputCell = pih.expectSimulationCell();

	// Get particle selection.
	ConstPropertyPtr selectionProperty;
	if(onlySelectedParticles())
		selectionProperty = pih.expectStandardProperty<ParticleProperty>(ParticleProperty::SelectionProperty)->storage();

	// Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
	if(neighborMode() == CutoffRange) {
		return std::make_shared<CutoffClusterAnalysisEngine>(posProperty->storage(), inputCell->data(), sortBySize(), std::move(selectionProperty), cutoff());
	}
	else if(neighborMode() == Bonding) {
		BondsObject* bonds = pih.expectBonds();
		return std::make_shared<BondClusterAnalysisEngine>(posProperty->storage(), inputCell->data(), sortBySize(), std::move(selectionProperty), bonds->storage());
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
	setProgressText(tr("Performing cluster analysis"));

	// Initialize.
	std::fill(_results->particleClusters()->dataInt64(), _results->particleClusters()->dataInt64() + _results->particleClusters()->size(), -1);
	
	// Perform the actual clustering.
	doClustering();
	if(isCanceled())
		return;

	// Sort clusters by size.
	if(_sortBySize && _results->numClusters() != 0) {

		// Determine cluster sizes.
		std::vector<size_t> clusterSizes(_results->numClusters() + 1, 0);
		for(auto id : _results->particleClusters()->constInt64Range()) {
			clusterSizes[id]++;
		}

		// Sort clusters by size.
		std::vector<size_t> mapping(_results->numClusters() + 1);
		std::iota(mapping.begin(), mapping.end(), size_t(0));
		std::sort(mapping.begin() + 1, mapping.end(), [&clusterSizes](size_t a, size_t b) {
			return clusterSizes[a] > clusterSizes[b];
		});
		_results->setLargestClusterSize(clusterSizes[mapping[1]]);
		clusterSizes.clear();
		clusterSizes.shrink_to_fit();

		// Remap cluster IDs.
		std::vector<size_t> inverseMapping(_results->numClusters() + 1);
		for(size_t i = 0; i <= _results->numClusters(); i++)
			inverseMapping[mapping[i]] = i;
		for(auto& id : _results->particleClusters()->int64Range())
			id = inverseMapping[id];
	}

	// Return the results of the compute engine.
	setResult(std::move(_results));	
}

/******************************************************************************
* Performs the actual clustering algorithm.
******************************************************************************/
void ClusterAnalysisModifier::CutoffClusterAnalysisEngine::doClustering()
{
	// Prepare the neighbor finder.
	CutoffNeighborFinder neighborFinder;
	if(!neighborFinder.prepare(cutoff(), *positions(), cell(), selection().get(), this))
		return;

	size_t particleCount = positions()->size();
	setProgressValue(0);
	setProgressMaximum(particleCount);

	PropertyStorage& particleClusters = *_results->particleClusters();
			
	std::deque<size_t> toProcess;
	for(size_t seedParticleIndex = 0; seedParticleIndex < particleCount; seedParticleIndex++) {

		// Skip unselected particles that are not included in the analysis.
		if(selection() && !selection()->getInt(seedParticleIndex)) {
			particleClusters.setInt64(seedParticleIndex, 0);
			continue;
		}

		// Skip particles that have already been assigned to a cluster.
		if(particleClusters.getInt64(seedParticleIndex) != -1)
			continue;

		// Start a new cluster.
		_results->setNumClusters(_results->numClusters() + 1);
		qlonglong cluster = _results->numClusters();
		particleClusters.setInt64(seedParticleIndex, cluster);

		// Now recursively iterate over all neighbors of the seed particle and add them to the cluster too.
		OVITO_ASSERT(toProcess.empty());
		toProcess.push_back(seedParticleIndex);

		do {
			incrementProgressValue();
			if(isCanceled())
				return;

			size_t currentParticle = toProcess.front();
			toProcess.pop_front();
			for(CutoffNeighborFinder::Query neighQuery(neighborFinder, currentParticle); !neighQuery.atEnd(); neighQuery.next()) {
				size_t neighborIndex = neighQuery.current();
				if(particleClusters.getInt64(neighborIndex) == -1) {
					particleClusters.setInt64(neighborIndex, cluster);
					toProcess.push_back(neighborIndex);
				}
			}
		}
		while(toProcess.empty() == false);
	}
}

/******************************************************************************
* Performs the actual clustering algorithm.
******************************************************************************/
void ClusterAnalysisModifier::BondClusterAnalysisEngine::doClustering()
{
	size_t particleCount = positions()->size();
	setProgressValue(0);
	setProgressMaximum(particleCount);

	// Prepare particle bond map.
	ParticleBondMap bondMap(*bonds());

	PropertyStorage& particleClusters = *_results->particleClusters();
	
	std::deque<size_t> toProcess;
	for(size_t seedParticleIndex = 0; seedParticleIndex < particleCount; seedParticleIndex++) {

		// Skip unselected particles that are not included in the analysis.
		if(selection() && !selection()->getInt(seedParticleIndex)) {
			particleClusters.setInt64(seedParticleIndex, 0);
			continue;
		}

		// Skip particles that have already been assigned to a cluster.
		if(particleClusters.getInt64(seedParticleIndex) != -1)
			continue;

		// Start a new cluster.
		_results->setNumClusters(_results->numClusters() + 1);
		qlonglong cluster = _results->numClusters();
		particleClusters.setInt64(seedParticleIndex, cluster);

		// Now recursively iterate over all neighbors of the seed particle and add them to the cluster too.
		OVITO_ASSERT(toProcess.empty());
		toProcess.push_back(seedParticleIndex);

		do {
			incrementProgressValue();
			if(isCanceled())
				return;

			size_t currentParticle = toProcess.front();
			toProcess.pop_front();

			// Iterate over all bonds of the current particle.
			for(size_t neighborBondIndex : bondMap.bondIndicesOfParticle(currentParticle)) {
				const Bond& neighborBond = (*bonds())[neighborBondIndex];
				OVITO_ASSERT(neighborBond.index1 == currentParticle || neighborBond.index2 == currentParticle);
				size_t neighborIndex = (neighborBond.index1 == currentParticle) ? neighborBond.index2 : neighborBond.index1;
				if(neighborIndex >= particleCount)
					continue;
				if(particleClusters.getInt64(neighborIndex) != -1) 
					continue;
				if(selection() && !selection()->getInt(neighborIndex))
					continue;

				particleClusters.setInt64(neighborIndex, cluster);
				toProcess.push_back(neighborIndex);
			}
		}
		while(toProcess.empty() == false);
	}
}


/******************************************************************************
* Injects the computed results of the engine into the data pipeline.
******************************************************************************/
PipelineFlowState ClusterAnalysisModifier::ClusterAnalysisResults::apply(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input)
{
	ClusterAnalysisModifier* modifier = static_object_cast<ClusterAnalysisModifier>(modApp->modifier());
	
	PipelineFlowState output = input;
	ParticleOutputHelper poh(modApp->dataset(), output);
	if(particleClusters()->size() != poh.outputParticleCount())
		modApp->throwException(tr("Cached modifier results are obsolete, because the number of input particles has changed."));
	poh.outputProperty<ParticleProperty>(particleClusters());

	output.attributes().insert(QStringLiteral("ClusterAnalysis.cluster_count"), QVariant::fromValue(numClusters()));
	if(modifier->sortBySize())
		output.attributes().insert(QStringLiteral("ClusterAnalysis.largest_size"), QVariant::fromValue(largestClusterSize()));

	output.setStatus(PipelineStatus(PipelineStatus::Success, tr("Found %n cluster(s)", "", numClusters())));
	return output;
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
