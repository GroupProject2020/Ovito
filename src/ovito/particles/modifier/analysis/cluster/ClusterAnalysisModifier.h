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

#pragma once


#include <ovito/particles/Particles.h>
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/particles/util/CutoffNeighborFinder.h>
#include <ovito/particles/util/ParticleOrderingFingerprint.h>
#include <ovito/particles/objects/BondsObject.h>
#include <ovito/stdobj/simcell/SimulationCell.h>
#include <ovito/core/dataset/pipeline/AsynchronousModifier.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

/**
 * \brief This modifier builds clusters of particles.
 */
class OVITO_PARTICLES_EXPORT ClusterAnalysisModifier : public AsynchronousModifier
{
	/// Give this modifier class its own metaclass.
	class ClusterAnalysisModifierClass : public AsynchronousModifier::OOMetaClass
	{
	public:

		/// Inherit constructor from base metaclass.
		using AsynchronousModifier::OOMetaClass::OOMetaClass;

		/// Asks the metaclass whether the modifier can be applied to the given input data.
		virtual bool isApplicableTo(const DataCollection& input) const override;
	};

	Q_OBJECT
	OVITO_CLASS_META(ClusterAnalysisModifier, ClusterAnalysisModifierClass)

	Q_CLASSINFO("DisplayName", "Cluster analysis");
	Q_CLASSINFO("ModifierCategory", "Analysis");

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

private:

	/// Computes the modifier's results.
	class ClusterAnalysisEngine : public ComputeEngine
	{
	public:

		/// Constructor.
		ClusterAnalysisEngine(ParticleOrderingFingerprint fingerprint, ConstPropertyPtr positions, const SimulationCell& simCell, bool sortBySize, bool unwrapParticleCoordinates, bool computeCentersOfMass, ConstPropertyPtr selection) :
			_positions(positions),
			_simCell(simCell),
			_sortBySize(sortBySize),
			_unwrapParticleCoordinates(unwrapParticleCoordinates),
			_computeCentersOfMass(computeCentersOfMass),
			_unwrappedPositions((unwrapParticleCoordinates || computeCentersOfMass) ? std::make_shared<PropertyStorage>(*positions) : nullptr),
			_centersOfMass(computeCentersOfMass ? std::make_shared<PropertyStorage>(1, PropertyStorage::Float, 3, 0, QStringLiteral("Center of Mass"), true, 
				0, QStringList() << QStringLiteral("X") << QStringLiteral("Y") << QStringLiteral("Z")) : nullptr),
			_selection(std::move(selection)),
			_particleClusters(ParticlesObject::OOClass().createStandardStorage(fingerprint.particleCount(), ParticlesObject::ClusterProperty, false)),
			_inputFingerprint(std::move(fingerprint)) {}

		/// This method is called by the system after the computation was successfully completed.
		virtual void cleanup() override {
			_positions.reset();
			_selection.reset();
			if(!_unwrapParticleCoordinates) 
				_unwrappedPositions.reset();
			ComputeEngine::cleanup();
		}

		/// Computes the modifier's results.
		virtual void perform() override;

		/// Injects the computed results into the data pipeline.
		virtual void emitResults(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state) override;

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
		const bool _unwrapParticleCoordinates;
		const bool _computeCentersOfMass;
		ConstPropertyPtr _positions;
		ConstPropertyPtr _selection;
		size_t _numClusters = 0;
		size_t _largestClusterSize = 0;
		const PropertyPtr _particleClusters;
		PropertyPtr _clusterSizes;
		PropertyPtr _unwrappedPositions;
		PropertyPtr _centersOfMass;
		ParticleOrderingFingerprint _inputFingerprint;
	};

	/// Computes the modifier's results.
	class CutoffClusterAnalysisEngine : public ClusterAnalysisEngine
	{
	public:

		/// Constructor.
		CutoffClusterAnalysisEngine(ParticleOrderingFingerprint fingerprint, ConstPropertyPtr positions, const SimulationCell& simCell, bool sortBySize, bool unwrapParticleCoordinates, bool computeCentersOfMass, ConstPropertyPtr selection, FloatType cutoff) :
			ClusterAnalysisEngine(std::move(fingerprint), std::move(positions), simCell, sortBySize, unwrapParticleCoordinates, computeCentersOfMass, std::move(selection)),
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
		BondClusterAnalysisEngine(ParticleOrderingFingerprint fingerprint, ConstPropertyPtr positions, const SimulationCell& simCell, bool sortBySize, bool unwrapParticleCoordinates, bool computeCentersOfMass, ConstPropertyPtr selection, ConstPropertyPtr bondTopology) :
			ClusterAnalysisEngine(std::move(fingerprint), std::move(positions), simCell, sortBySize, unwrapParticleCoordinates, computeCentersOfMass, std::move(selection)),
			_bondTopology(std::move(bondTopology)) {}

		/// This method is called by the system after the computation was successfully completed.
		virtual void cleanup() override {
			_bondTopology.reset();
			ClusterAnalysisEngine::cleanup();
		}

		/// Performs the actual clustering algorithm.
		virtual void doClustering() override;

		/// Returns the list of input bonds.
		const ConstPropertyPtr& bondTopology() const { return _bondTopology; }

	private:

		ConstPropertyPtr _bondTopology;
		PropertyPtr _periodicImageProperty;
	};

	/// The neighbor mode.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(NeighborMode, neighborMode, setNeighborMode, PROPERTY_FIELD_MEMORIZE);

	/// The cutoff radius for the distance-based neighbor criterion.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, cutoff, setCutoff, PROPERTY_FIELD_MEMORIZE);

	/// Controls whether analysis should take into account only selected particles.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, onlySelectedParticles, setOnlySelectedParticles);

	/// Controls the sorting of cluster IDs by cluster size.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(bool, sortBySize, setSortBySize, PROPERTY_FIELD_MEMORIZE);

	/// Controls the unwrapping of the particle coordinates that make up a cluster.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(bool, unwrapParticleCoordinates, setUnwrapParticleCoordinates, PROPERTY_FIELD_MEMORIZE);

	/// Controls the computation of cluster centers of mass.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(bool, computeCentersOfMass, setComputeCentersOfMass, PROPERTY_FIELD_MEMORIZE);
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
