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
#include <core/dataset/data/simcell/SimulationCell.h>
#include <core/dataset/pipeline/AsynchronousModifier.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

/**
 * \brief Calculates the centro-symmetry parameter (CSP) for particles.
 */
class OVITO_PARTICLES_EXPORT CentroSymmetryModifier : public AsynchronousModifier
{
public:

	/// The maximum number of neighbors that can be taken into account to compute the CSP.
	enum { MAX_CSP_NEIGHBORS = 32 };

public:

	/// Constructor.
	Q_INVOKABLE CentroSymmetryModifier(DataSet* dataset);

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
		
protected:

	/// Creates a computation engine that will compute the modifier's results.
	virtual Future<ComputeEnginePtr> createEngine(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input) override;
		
	/// Computes the centrosymmetry parameter of a single particle.
	static FloatType computeCSP(NearestNeighborFinder& neighList, size_t particleIndex);

private:

	/// Stores the modifier's results.
	class CentroSymmetryResults : public ComputeEngineResults 
	{
	public:

		/// Constructor.
		CentroSymmetryResults(size_t particleCount) :
			_csp(ParticleProperty::createStandardStorage(particleCount, ParticleProperty::CentroSymmetryProperty, false)) {}

		/// Injects the computed results into the data pipeline.
		virtual PipelineFlowState apply(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input) override;

		/// Returns the property storage that contains the computed per-particle CSP values.
		const PropertyPtr& csp() const { return _csp; }		

	private:

		const PropertyPtr _csp;		
	};
	
	/// Computes the modifier's results.
	class CentroSymmetryEngine : public ComputeEngine
	{
	public:

		/// Constructor.
		CentroSymmetryEngine(const TimeInterval& validityInterval, ConstPropertyPtr positions, const SimulationCell& simCell, int nneighbors) :
			ComputeEngine(validityInterval),
			_nneighbors(nneighbors),
			_positions(positions),
			_simCell(simCell),
			_results(std::make_shared<CentroSymmetryResults>(positions->size())) {}

		/// Computes the modifier's results.
		virtual void perform() override;

		/// Returns the property storage that contains the input particle positions.
		const ConstPropertyPtr& positions() const { return _positions; }

		/// Returns the simulation cell data.
		const SimulationCell& cell() const { return _simCell; }

	private:

		const int _nneighbors;
		const SimulationCell _simCell;
		const ConstPropertyPtr _positions;
		std::shared_ptr<CentroSymmetryResults> _results;
	};

	/// Specifies the number of nearest neighbors to take into account when computing the CSP.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, numNeighbors, setNumNeighbors);

	Q_OBJECT
	OVITO_CLASS

	Q_CLASSINFO("DisplayName", "Centrosymmetry parameter");
	Q_CLASSINFO("ModifierCategory", "Analysis");
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace


