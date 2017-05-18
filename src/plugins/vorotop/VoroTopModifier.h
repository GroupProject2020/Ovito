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


#include <plugins/vorotop/VoroTopPlugin.h>
#include <plugins/particles/data/ParticleProperty.h>
#include <plugins/particles/modifier/analysis/StructureIdentificationModifier.h>
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
public:

	/// Constructor.
	Q_INVOKABLE VoroTopModifier(DataSet* dataset);

	/// Loads a new filter definition into the modifier.
	void loadFilterDefinition(const QString& filepath);

	/// Returns the VoroTop filter definition cached from the last analysis run.
	const std::shared_ptr<Filter>& filter() const { return _filter; }

protected:

	/// Is called when the value of a property of this object has changed.
	virtual void propertyChanged(const PropertyFieldDescriptor& field) override;

	/// Creates a computation engine that will compute the modifier's results.
	virtual std::shared_ptr<ComputeEngine> createEngine(TimePoint time, TimeInterval validityInterval) override;

	/// Unpacks the results of the computation engine and stores them in the modifier.
	virtual void transferComputationResults(ComputeEngine* engine) override;

	/// Lets the modifier insert the cached computation results into the modification pipeline.
	virtual PipelineStatus applyComputationResults(TimePoint time, TimeInterval& validityInterval) override;

private:

	/// Compute engine that performs the actual analysis in a background thread.
	class VoroTopAnalysisEngine : public StructureIdentificationEngine
	{
	public:

		/// Constructor.
		VoroTopAnalysisEngine(const TimeInterval& validityInterval, ParticleProperty* positions, ParticleProperty* selection,
							std::vector<FloatType>&& radii, const SimulationCell& simCell, const QString& filterFile, const std::shared_ptr<Filter>& filter, const QVector<bool>& typesToIdentify) :
			StructureIdentificationEngine(validityInterval, positions, simCell, typesToIdentify, selection),
			_filterFile(filterFile),
			_filter(filter),
			_radii(std::move(radii)) {}

		/// Computes the modifier's results and stores them in this object for later retrieval.
		virtual void perform() override;

		/// Processes a single Voronoi cell.
		void processCell(voro::voronoicell_neighbor& vcell, size_t particleIndex, QMutex* mutex);

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

	Q_OBJECT
	OVITO_OBJECT

	Q_CLASSINFO("DisplayName", "VoroTop analysis");
	Q_CLASSINFO("ModifierCategory", "Analysis");
};

}	// End of namespace
}	// End of namespace
