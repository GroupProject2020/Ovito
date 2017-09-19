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

#pragma once


#include <plugins/particles/Particles.h>
#include <plugins/particles/objects/ParticleProperty.h>
#include <plugins/particles/util/CutoffNeighborFinder.h>
#include <plugins/particles/objects/BondsStorage.h>
#include <core/dataset/data/simcell/SimulationCell.h>
#include <core/dataset/pipeline/AsynchronousModifier.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

/**
 * \brief This modifier builds clusters of particles.
 */
class OVITO_PARTICLES_EXPORT ClusterAnalysisModifier : public AsynchronousModifier
{
public:

	enum NeighborMode {
		CutoffRange,	///< Treats particles as neighbors which are within a certain distance.
		Bonding,		///< Treats particles as neighbors which are connected by a bond.
	};
	Q_ENUMS(NeighborMode);

	/// Constructor.
	Q_INVOKABLE ClusterAnalysisModifier(DataSet* dataset);

protected:

	/// Creates a computation engine that will compute the modifier's results.
	virtual Future<ComputeEnginePtr> createEngine(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input) override;

public:
	
	/// Give this modifier class its own metaclass.
	class OOMetaClass : public AsynchronousModifier::OOMetaClass 
	{
	public:

		/// Inherit constructor from base metaclass.
		using AsynchronousModifier::OOMetaClass::OOMetaClass;

		/// Asks the metaclass whether the modifier can be applied to the given input data.
		virtual bool isApplicableTo(const PipelineFlowState& input) const override;
	};

private:

	/// Stores the modifier's results.
	class ClusterAnalysisResults : public ComputeEngineResults 
	{
	public:

		/// Constructor.
		ClusterAnalysisResults(size_t particleCount) :
			_particleClusters(ParticleProperty::createStandardStorage(particleCount, ParticleProperty::ClusterProperty, false)) {}

		/// Injects the computed results into the data pipeline.
		virtual PipelineFlowState apply(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input) override;

		/// Returns the property storage that contains the computed cluster number of each particle.
		const PropertyPtr& particleClusters() const { return _particleClusters; }
			
		/// Returns the number of clusters.
		size_t numClusters() const { return _numClusters; }

		/// Sets the number of clusters.
		void setNumClusters(size_t num) { _numClusters = num; }
				
		/// Returns the size of the largest cluster.
		size_t largestClusterSize() const { return _largestClusterSize; }

		/// Sets the size of the largest cluster.
		void setLargestClusterSize(size_t size) { _largestClusterSize = size; }
				
	private:

		size_t _numClusters = 0;
		size_t _largestClusterSize = 0;
		const PropertyPtr _particleClusters;
	};

	/// Computes the modifier's results.
	class ClusterAnalysisEngine : public ComputeEngine
	{
	public:

		/// Constructor.
		ClusterAnalysisEngine(const TimeInterval& validityInterval, ConstPropertyPtr positions, const SimulationCell& simCell, bool sortBySize, ConstPropertyPtr selection) :
			ComputeEngine(validityInterval),
			_positions(positions), _simCell(simCell), 
			_sortBySize(sortBySize),
			_selection(std::move(selection)),
			_results(std::make_shared<ClusterAnalysisResults>(positions->size())) {}

		/// Computes the modifier's results.
		virtual void perform() override;

		/// Performs the actual clustering algorithm.
		virtual void doClustering() = 0;

		/// Returns the property storage that contains the input particle positions.
		const ConstPropertyPtr& positions() const { return _positions; }

		/// Returns the simulation cell data.
		const SimulationCell& cell() const { return _simCell; }

		/// Returns the property storage that contains the particle selection (optional).
		const ConstPropertyPtr& selection() const { return _selection; }

	protected:

		const SimulationCell _simCell;
		const bool _sortBySize;
		const ConstPropertyPtr _positions;
		const ConstPropertyPtr _selection;
		std::shared_ptr<ClusterAnalysisResults> _results;
	};

	/// Computes the modifier's results.
	class CutoffClusterAnalysisEngine : public ClusterAnalysisEngine
	{
	public:

		/// Constructor.
		CutoffClusterAnalysisEngine(const TimeInterval& validityInterval, ConstPropertyPtr positions, const SimulationCell& simCell, bool sortBySize, ConstPropertyPtr selection, FloatType cutoff) :
			ClusterAnalysisEngine(validityInterval, std::move(positions), simCell, sortBySize, std::move(selection)),
			_cutoff(cutoff) {}

		/// Performs the actual clustering algorithm.
		virtual void doClustering() override;

		/// Returns the cutoff radius.
		FloatType cutoff() const { return _cutoff; }

	private:

		const FloatType _cutoff;
	};

	/// Computes the modifier's results.
	class BondClusterAnalysisEngine : public ClusterAnalysisEngine
	{
	public:

		/// Constructor.
		BondClusterAnalysisEngine(const TimeInterval& validityInterval, ConstPropertyPtr positions, const SimulationCell& simCell, bool sortBySize, ConstPropertyPtr selection, ConstBondsPtr bonds) :
			ClusterAnalysisEngine(validityInterval, std::move(positions), simCell, sortBySize, std::move(selection)),
			_bonds(std::move(bonds)) {}

		/// Performs the actual clustering algorithm.
		virtual void doClustering() override;

		/// Returns the list of input bonds.
		const ConstBondsPtr& bonds() const { return _bonds; }

	private:

		const ConstBondsPtr _bonds;
	};

	/// The neighbor mode.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(NeighborMode, neighborMode, setNeighborMode);

	/// The cutoff radius for the distance-based neighbor criterion.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, cutoff, setCutoff);

	/// Controls whether analysis should take into account only selected particles.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, onlySelectedParticles, setOnlySelectedParticles);

	/// Controls the sorting of cluster IDs by cluster size.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, sortBySize, setSortBySize);

	Q_OBJECT
	OVITO_CLASS

	Q_CLASSINFO("DisplayName", "Cluster analysis");
	Q_CLASSINFO("ModifierCategory", "Analysis");
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace


