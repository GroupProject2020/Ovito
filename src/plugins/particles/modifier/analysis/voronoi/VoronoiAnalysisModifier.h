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
#include <plugins/particles/objects/BondsStorage.h>
#include <plugins/particles/objects/BondsDisplay.h>
#include <plugins/particles/objects/ParticleProperty.h>
#include <core/dataset/data/properties/PropertyStorage.h>
#include <core/dataset/data/simcell/SimulationCell.h>
#include <core/dataset/pipeline/AsynchronousModifier.h>
#include <core/dataset/pipeline/ModifierApplication.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

/**
 * \brief This modifier computes the atomic volume and the Voronoi indices of particles.
 */
class OVITO_PARTICLES_EXPORT VoronoiAnalysisModifier : public AsynchronousModifier
{
public:

	/// Constructor.
	Q_INVOKABLE VoronoiAnalysisModifier(DataSet* dataset);

protected:

	/// Handles reference events sent by reference targets of this object.
	virtual bool referenceEvent(RefTarget* source, ReferenceEvent* event) override;

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
	class VoronoiAnalysisResults : public ComputeEngineResults 
	{
	public:

		/// Constructor.
		VoronoiAnalysisResults(size_t particleCount, int edgeCount, bool computeIndices, bool computeBonds) :
			_coordinationNumbers(ParticleProperty::createStandardStorage(particleCount, ParticleProperty::CoordinationProperty, true)),
			_atomicVolumes(std::make_shared<PropertyStorage>(particleCount, qMetaTypeId<FloatType>(), 1, 0, QStringLiteral("Atomic Volume"), true)),
			_voronoiIndices(computeIndices ? std::make_shared<PropertyStorage>(particleCount, qMetaTypeId<int>(), edgeCount, 0, QStringLiteral("Voronoi Index"), true) : nullptr),
			_bonds(computeBonds ? std::make_shared<BondsStorage>() : nullptr) {}

		/// Injects the computed results into the data pipeline.
		virtual PipelineFlowState apply(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input) override;

		/// Returns the property storage that contains the computed coordination numbers.
		const PropertyPtr& coordinationNumbers() const { return _coordinationNumbers; }
		
		/// Returns the property storage that contains the computed atomic volumes.
		const PropertyPtr& atomicVolumes() const { return _atomicVolumes; }

		/// Returns the property storage that contains the computed Voronoi indices.
		const PropertyPtr& voronoiIndices() const { return _voronoiIndices; }
		
		/// Returns the total volume of the simulation cell computed by the modifier.
		double simulationBoxVolume() const { return _simulationBoxVolume; }

		/// Sets the total volume of the simulation cell computed by the modifier.
		void setSimulationBoxVolume(double vol) { _simulationBoxVolume = vol; }
				
		/// Returns the volume sum of all Voronoi cells computed by the modifier.
		std::atomic<double>& voronoiVolumeSum() { return _voronoiVolumeSum; }
	
		/// Returns the maximum number of edges of any Voronoi face.
		std::atomic<int>& maxFaceOrder() { return _maxFaceOrder; }

		/// Returns the computed nearest neighbor bonds.
		const BondsPtr& bonds() const { return _bonds; }

	private:

		const PropertyPtr _coordinationNumbers;
		const PropertyPtr _atomicVolumes;
		const PropertyPtr _voronoiIndices;
		const BondsPtr _bonds;

		/// The total volume of the simulation cell computed by the modifier.
		double _simulationBoxVolume = 0;
	
		/// The volume sum of all Voronoi cells.
		std::atomic<double> _voronoiVolumeSum{0.0};
		
		/// The maximum number of edges of a Voronoi face.
		std::atomic<int> _maxFaceOrder{0};
	};

	/// Computes the modifier's results.
	class VoronoiAnalysisEngine : public ComputeEngine
	{
	public:

		/// Constructor.
		VoronoiAnalysisEngine(const TimeInterval& validityInterval, ConstPropertyPtr positions, ConstPropertyPtr selection, std::vector<FloatType> radii,
							const SimulationCell& simCell,
							int edgeCount, bool computeIndices, bool computeBonds, FloatType edgeThreshold, FloatType faceThreshold, FloatType relativeFaceThreshold) :
			ComputeEngine(validityInterval),
			_positions(positions),
			_selection(std::move(selection)),
			_radii(std::move(radii)),
			_simCell(simCell),
			_edgeThreshold(edgeThreshold),
			_faceThreshold(faceThreshold),
			_relativeFaceThreshold(relativeFaceThreshold),
			_results(std::make_shared<VoronoiAnalysisResults>(positions->size(), edgeCount, computeIndices, computeBonds)) {}
			
		/// Computes the modifier's results.
		virtual void perform() override;

		const SimulationCell& simCell() const { return _simCell; }
		const ConstPropertyPtr& positions() const { return _positions; }
		const ConstPropertyPtr selection() const { return _selection; }
		
	private:

		const FloatType _edgeThreshold;
		const FloatType _faceThreshold;
		const FloatType _relativeFaceThreshold;
		const SimulationCell _simCell;
		std::vector<FloatType> _radii;
		const ConstPropertyPtr _positions;
		const ConstPropertyPtr _selection;
		std::shared_ptr<VoronoiAnalysisResults> _results;
	};

	/// Controls whether the modifier takes into account only selected particles.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, onlySelected, setOnlySelected);

	/// Controls whether the modifier takes into account particle radii.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, useRadii, setUseRadii);

	/// Controls whether the modifier computes Voronoi indices.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, computeIndices, setComputeIndices);

	/// Controls up to which edge count Voronoi indices are being computed.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, edgeCount, setEdgeCount);

	/// The minimum length for an edge to be counted.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, edgeThreshold, setEdgeThreshold);

	/// The minimum area for a face to be counted.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, faceThreshold, setFaceThreshold);

	/// The minimum area for a face to be counted relative to the total polyhedron surface.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, relativeFaceThreshold, setRelativeFaceThreshold);

	/// Controls whether the modifier output nearest neighbor bonds.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, computeBonds, setComputeBonds);

	/// The display object for rendering the bonds generated by the modifier.
	DECLARE_MODIFIABLE_REFERENCE_FIELD(BondsDisplay, bondsDisplay, setBondsDisplay);

private:

	Q_OBJECT
	OVITO_CLASS

	Q_CLASSINFO("DisplayName", "Voronoi analysis");
	Q_CLASSINFO("ModifierCategory", "Analysis");
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace


