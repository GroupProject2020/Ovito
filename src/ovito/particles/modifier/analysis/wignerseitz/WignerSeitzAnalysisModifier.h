////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2017 Alexander Stukowski
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
#include <ovito/stdobj/properties/PropertyStorage.h>
#include <ovito/particles/util/NearestNeighborFinder.h>
#include <ovito/stdobj/simcell/SimulationCell.h>
#include <ovito/particles/modifier/analysis/ReferenceConfigurationModifier.h>

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

	/// Computes the modifier's results.
	class WignerSeitzAnalysisEngine : public RefConfigEngineBase
	{
	public:

		/// Constructor.
		WignerSeitzAnalysisEngine(const TimeInterval& validityInterval, ConstPropertyPtr positions, const SimulationCell& simCell,
				PipelineFlowState referenceState, ConstPropertyPtr refPositions, const SimulationCell& simCellRef, AffineMappingType affineMapping,
				ConstPropertyPtr typeProperty, int ptypeMinId, int ptypeMaxId, ConstPropertyPtr referenceTypeProperty, ConstPropertyPtr referenceIdentifierProperty) :
			RefConfigEngineBase(validityInterval, std::move(positions), simCell, std::move(refPositions), simCellRef,
				nullptr, nullptr, affineMapping, false),
			_typeProperty(std::move(typeProperty)),
			_ptypeMinId(ptypeMinId), _ptypeMaxId(ptypeMaxId),
			_referenceTypeProperty(std::move(referenceTypeProperty)),
			_referenceIdentifierProperty(std::move(referenceIdentifierProperty)),
			_referenceState(std::move(referenceState)) {}

		/// This method is called by the system after the computation was successfully completed.
		virtual void cleanup() override {
			_typeProperty.reset();
			_referenceTypeProperty.reset();
			_referenceIdentifierProperty.reset();
			RefConfigEngineBase::cleanup();
		}

		/// Computes the modifier's results.
		virtual void perform() override;

		/// Injects the computed results into the data pipeline.
		virtual void emitResults(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state) override;

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

		/// Returns the property storage that contains the type of site each atom has been assigned to.
		const PropertyPtr& siteTypes() const { return _siteTypes; }

		/// Replaces the property storage for the type of the site each atom has been assigned to.
		void setSiteTypes(PropertyPtr prop) { _siteTypes = std::move(prop); }

		/// Returns the property storage that contains the index of the site each atom has been assigned to.
		const PropertyPtr& siteIndices() const { return _siteIndices; }

		/// Replaces the property storage for the index of the site each atom has been assigned to.
		void setSiteIndices(PropertyPtr prop) { _siteIndices = std::move(prop); }

		/// Returns the property storage that contains the identifier of the site each atom has been assigned to.
		const PropertyPtr& siteIdentifiers() const { return _siteIdentifiers; }

		/// Replaces the property storage for the indeitifier of the site each atom has been assigned to.
		void setSiteIdentifiers(PropertyPtr prop) { _siteIdentifiers = std::move(prop); }

		/// Returns the reference state.
		const PipelineFlowState& referenceState() const { return _referenceState; }

		/// Returns the property storage that contains the particle types.
		const ConstPropertyPtr& particleTypes() const { return _typeProperty; }

	private:

		ConstPropertyPtr _typeProperty;
		ConstPropertyPtr _referenceTypeProperty;
		ConstPropertyPtr _referenceIdentifierProperty;
		int _ptypeMinId;
		int _ptypeMaxId;
		const PipelineFlowState _referenceState;
		PropertyPtr _occupancyNumbers;
		PropertyPtr _siteTypes;
		PropertyPtr _siteIndices;
		PropertyPtr _siteIdentifiers;
		size_t _vacancyCount = 0;
		size_t _interstitialCount = 0;
	};

	/// Enables per-type occupancy numbers.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(bool, perTypeOccupancy, setPerTypeOccupancy, PROPERTY_FIELD_MEMORIZE)

	/// Enables output of displaced atomic configuration instead of reference configuration.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(bool, outputCurrentConfig, setOutputCurrentConfig, PROPERTY_FIELD_MEMORIZE)
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace


