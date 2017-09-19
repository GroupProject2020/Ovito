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
#include <core/dataset/data/properties/PropertyStorage.h>
#include <plugins/particles/util/NearestNeighborFinder.h>
#include <core/dataset/data/simcell/SimulationCell.h>
#include <plugins/particles/modifier/analysis/ReferenceConfigurationModifier.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

/**
 * \brief Performs the Wigner-Seitz cell analysis to identify point defects in crystals.
 */
class OVITO_PARTICLES_EXPORT WignerSeitzAnalysisModifier : public ReferenceConfigurationModifier
{
	Q_OBJECT
	OVITO_CLASS(WignerSeitzAnalysisModifier)

	Q_CLASSINFO("DisplayName", "Wigner-Seitz defect analysis");
	Q_CLASSINFO("ModifierCategory", "Analysis");

public:

	/// Constructor.
	Q_INVOKABLE WignerSeitzAnalysisModifier(DataSet* dataset);

protected:

	/// Creates a computation engine that will compute the modifier's results.
	virtual Future<ComputeEnginePtr> createEngineWithReference(TimePoint time, ModifierApplication* modApp, PipelineFlowState input, const PipelineFlowState& referenceState, TimeInterval validityInterval) override;
	
private:

	/// Stores the modifier's results.
	class WignerSeitzAnalysisResults : public ComputeEngineResults 
	{
	public:

		/// Constructor.
		WignerSeitzAnalysisResults(PipelineFlowState referenceState) : _referenceState(std::move(referenceState)) {}

		/// Injects the computed results into the data pipeline.
		virtual PipelineFlowState apply(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input) override;

		/// Returns the number of vacant sites found during the last analysis run.
		size_t vacancyCount() const { return _vacancyCount; }

		/// Increments the number of vacant sites found during the last analysis run.
		void incrementVacancyCount(size_t n = 1) { _vacancyCount += n; }
		
		/// Returns the number of interstitial atoms found during the last analysis run.
		size_t interstitialCount() const { return _interstitialCount; }	

		/// Increments the number of interstitial atoms found during the last analysis run.
		void incrementInterstitialCount(size_t n = 1) { _interstitialCount += n; }

		/// Returns the property storage that contains the computed occupancies.
		const PropertyPtr& occupancyNumbers() const { return _occupancyNumbers; }		

		/// Replaces the property storage for the computed occupancies.
		void setOccupancyNumbers(PropertyPtr prop) { _occupancyNumbers = std::move(prop); }		

		/// Returns the reference state.
		const PipelineFlowState& referenceState() const { return _referenceState; }
		
	private:

		const PipelineFlowState _referenceState;
		PropertyPtr _occupancyNumbers;
		size_t _vacancyCount = 0;
		size_t _interstitialCount = 0;
	};

	/// Computes the modifier's results.
	class WignerSeitzAnalysisEngine : public RefConfigEngineBase
	{
	public:

		/// Constructor.
		WignerSeitzAnalysisEngine(const TimeInterval& validityInterval, std::shared_ptr<WignerSeitzAnalysisResults> results, ConstPropertyPtr positions, const SimulationCell& simCell,
				ConstPropertyPtr refPositions, const SimulationCell& simCellRef, AffineMappingType affineMapping,
				ConstPropertyPtr typeProperty, int ptypeMinId, int ptypeMaxId) :
			RefConfigEngineBase(validityInterval, std::move(positions), simCell, std::move(refPositions), simCellRef,
				nullptr, nullptr, affineMapping, false),
			_typeProperty(std::move(typeProperty)), _ptypeMinId(ptypeMinId), _ptypeMaxId(ptypeMaxId),
			_results(std::move(results)) {}

		/// Computes the modifier's results.
		virtual void perform() override;

		/// Returns the property storage that contains the particle types.
		const ConstPropertyPtr& particleTypes() const { return _typeProperty; }

	private:

		const ConstPropertyPtr _typeProperty;
		int _ptypeMinId;
		int _ptypeMaxId;
		std::shared_ptr<WignerSeitzAnalysisResults> _results;
	};

	/// Enable per-type occupancy numbers.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(bool, perTypeOccupancy, setPerTypeOccupancy, PROPERTY_FIELD_MEMORIZE)
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace


