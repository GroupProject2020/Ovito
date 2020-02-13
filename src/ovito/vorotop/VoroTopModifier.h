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

#pragma once


#include <ovito/vorotop/VoroTopPlugin.h>
#include <ovito/stdobj/properties/PropertyStorage.h>
#include <ovito/particles/modifier/analysis/StructureIdentificationModifier.h>
#include "Filter.h"

namespace voro {
	class voronoicell_neighbor;	// Defined by Voro++
}

namespace Ovito { namespace VoroTop {

/**
 * \brief This analysis modifier performs the Voronoi topology analysis developed by Emanuel A. Lazar.
 */
class OVITO_VOROTOP_EXPORT VoroTopModifier : public StructureIdentificationModifier
{
	Q_OBJECT
	OVITO_CLASS(VoroTopModifier)

	Q_CLASSINFO("DisplayName", "VoroTop analysis");
	Q_CLASSINFO("ModifierCategory", "Structure identification");

public:

	/// Constructor.
	Q_INVOKABLE VoroTopModifier(DataSet* dataset);

	/// Loads a new filter definition into the modifier.
	bool loadFilterDefinition(const QString& filepath, AsyncOperation&& operation);

	/// Returns the VoroTop filter definition cached from the last analysis run.
	const std::shared_ptr<Filter>& filter() const { return _filter; }

protected:

	/// Is called when the value of a property of this object has changed.
	virtual void propertyChanged(const PropertyFieldDescriptor& field) override;

	/// Creates a computation engine that will compute the modifier's results.
	virtual Future<ComputeEnginePtr> createEngine(const PipelineEvaluationRequest& request, ModifierApplication* modApp, const PipelineFlowState& input) override;

private:

	/// Compute engine that performs the actual analysis in a background thread.
	class VoroTopAnalysisEngine : public StructureIdentificationEngine
	{
	public:

		/// Constructor.
		VoroTopAnalysisEngine(ParticleOrderingFingerprint fingerprint, const TimeInterval& validityInterval, ConstPropertyPtr positions, ConstPropertyPtr selection,
							std::vector<FloatType> radii, const SimulationCell& simCell, const QString& filterFile, std::shared_ptr<Filter> filter, QVector<bool> typesToIdentify) :
			StructureIdentificationEngine(std::move(fingerprint), std::move(positions), simCell, std::move(typesToIdentify), std::move(selection)),
			_filterFile(filterFile),
			_filter(std::move(filter)),
			_radii(std::move(radii)) {}

		/// This method is called by the system after the computation was successfully completed.
		virtual void cleanup() override {
			decltype(_radii){}.swap(_radii);
			StructureIdentificationEngine::cleanup();
		}

		/// Computes the modifier's results and stores them in this object for later retrieval.
		virtual void perform() override;

		/// Injects the computed results into the data pipeline.
		virtual void emitResults(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state) override;

		/// Processes a single Voronoi cell.
		int processCell(voro::voronoicell_neighbor& vcell);

		/// Returns the VoroTop filter definition.
		const std::shared_ptr<Filter>& filter() const { return _filter; }

	private:

		/// The path of the external file containing the filter definition.
		QString _filterFile;

		/// The VoroTop filter definition.
		std::shared_ptr<Filter> _filter;

		/// The per-particle radii.
		std::vector<FloatType> _radii;
	};

private:

	/// Controls whether the weighted Voronoi tessellation is computed, which takes into account particle radii.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, useRadii, setUseRadii);

	/// The external file path of the loaded filter file.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(QString, filterFile, setFilterFile);

	/// The VoroTop filter definition cached from the last analysis run.
	std::shared_ptr<Filter> _filter;
};

}	// End of namespace
}	// End of namespace
