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
#include <plugins/particles/objects/VectorDisplay.h>
#include <plugins/particles/objects/ParticleProperty.h>
#include <plugins/particles/modifier/analysis/ReferenceConfigurationModifier.h>
#include <core/dataset/data/simcell/SimulationCell.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

/**
 * \brief Calculates the per-particle displacement vectors based on a reference configuration.
 */
class OVITO_PARTICLES_EXPORT CalculateDisplacementsModifier : public ReferenceConfigurationModifier
{
public:

	/// Constructor.
	Q_INVOKABLE CalculateDisplacementsModifier(DataSet* dataset);

protected:

	/// Creates a computation engine that will compute the modifier's results.
	virtual Future<ComputeEnginePtr> createEngineWithReference(TimePoint time, ModifierApplication* modApp, PipelineFlowState input, const PipelineFlowState& referenceState, TimeInterval validityInterval) override;
	
	/// Handles reference events sent by reference targets of this object.
	virtual bool referenceEvent(RefTarget* source, ReferenceEvent* event) override;

private:

	/// Stores the modifier's results.
	class DisplacementResults : public ComputeEngineResults 
	{
	public:

		/// Constructor.
		DisplacementResults(size_t particleCount) : 
			_displacements(ParticleProperty::createStandardStorage(particleCount, ParticleProperty::DisplacementProperty, false)),
			_displacementMagnitudes(ParticleProperty::createStandardStorage(particleCount, ParticleProperty::DisplacementMagnitudeProperty, false)) {}

		/// Injects the computed results into the data pipeline.
		virtual PipelineFlowState apply(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input) override;

		/// Returns the property storage that contains the computed displacement vectors.
		const PropertyPtr& displacements() const { return _displacements; }
		
		/// Returns the property storage that contains the computed displacement vector magnitudes.
		const PropertyPtr& displacementMagnitudes() const { return _displacementMagnitudes; }
		
	private:

		const PropertyPtr _displacements;
		const PropertyPtr _displacementMagnitudes;
	};

	/// Computes the modifier's results.
	class DisplacementEngine : public RefConfigEngineBase
	{
	public:

		/// Constructor.
		DisplacementEngine(const TimeInterval& validityInterval, ConstPropertyPtr positions, const SimulationCell& simCell,
				ConstPropertyPtr refPositions, const SimulationCell& simCellRef,
				ConstPropertyPtr identifiers, ConstPropertyPtr refIdentifiers,
				AffineMappingType affineMapping, bool useMinimumImageConvention) :
			RefConfigEngineBase(validityInterval, positions, simCell, std::move(refPositions), simCellRef,
				std::move(identifiers), std::move(refIdentifiers), affineMapping, useMinimumImageConvention),
			_results(std::make_shared<DisplacementResults>(positions->size())) {}

		/// Computes the modifier's results.
		virtual void perform() override;

		/// Returns the property storage that contains the computed displacement vectors.
		const PropertyPtr& displacements() const { return _results->displacements(); }
		
		/// Returns the property storage that contains the computed displacement vector magnitudes.
		const PropertyPtr& displacementMagnitudes() const { return _results->displacementMagnitudes(); }
		
	private:

		std::shared_ptr<DisplacementResults> _results;
	};

	/// The vector display object for rendering the displacement vectors.
	DECLARE_MODIFIABLE_REFERENCE_FIELD(VectorDisplay, vectorDisplay, setVectorDisplay);
	
	Q_OBJECT
	OVITO_CLASS

	Q_CLASSINFO("DisplayName", "Displacement vectors");
	Q_CLASSINFO("ModifierCategory", "Analysis");
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
