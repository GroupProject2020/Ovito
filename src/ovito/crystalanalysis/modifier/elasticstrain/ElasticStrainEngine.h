///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2019) Alexander Stukowski
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


#include <ovito/crystalanalysis/CrystalAnalysis.h>
#include <ovito/particles/modifier/analysis/StructureIdentificationModifier.h>
#include <ovito/crystalanalysis/modifier/dxa/StructureAnalysis.h>

namespace Ovito { namespace CrystalAnalysis {

/*
 * Computation engine of the ElasticStrainModifier, which performs the actual strain tensor calculation.
 */
class ElasticStrainEngine : public StructureIdentificationModifier::StructureIdentificationEngine
{
public:

	/// Constructor.
	ElasticStrainEngine(ParticleOrderingFingerprint fingerprint,
			ConstPropertyPtr positions, const SimulationCell& simCell,
			int inputCrystalStructure, std::vector<Matrix3> preferredCrystalOrientations,
			bool calculateDeformationGradients, bool calculateStrainTensors,
			FloatType latticeConstant, FloatType caRatio, bool pushStrainTensorsForward);

	/// This method is called by the system after the computation was successfully completed.
	virtual void cleanup() override {
		_structureAnalysis.reset();
		StructureIdentificationEngine::cleanup();
	}

	/// Computes the modifier's results and stores them in this object for later retrieval.
	virtual void perform() override;

	/// Injects the computed results into the data pipeline.
	virtual void emitResults(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state) override;

	/// Returns the array of atom cluster IDs.
	const PropertyPtr& atomClusters() const { return _atomClusters; }

	/// Assigns the array of atom cluster IDs.
	void setAtomClusters(PropertyPtr prop) { _atomClusters = std::move(prop); }

	/// Returns the created cluster graph.
	const std::shared_ptr<ClusterGraph>& clusterGraph() { return _clusterGraph; }

	/// Returns the property storage that contains the computed per-particle volumetric strain values.
	const PropertyPtr& volumetricStrains() const { return _volumetricStrains; }

	/// Returns the property storage that contains the computed per-particle strain tensors.
	const PropertyPtr& strainTensors() const { return _strainTensors; }

	/// Returns the property storage that contains the computed per-particle deformation gradient tensors.
	const PropertyPtr& deformationGradients() const { return _deformationGradients; }

private:

	const int _inputCrystalStructure;
	FloatType _latticeConstant;
	FloatType _axialScaling;
	const bool _pushStrainTensorsForward;
	std::unique_ptr<StructureAnalysis> _structureAnalysis;

	/// This stores the cached atom-to-cluster assignments computed by the modifier.
	PropertyPtr _atomClusters;

	/// This stores the cached cluster graph computed by the modifier.
	const std::shared_ptr<ClusterGraph> _clusterGraph;

	/// This stores the cached results of the modifier.
	const PropertyPtr _volumetricStrains;

	/// This stores the cached results of the modifier.
	const PropertyPtr _strainTensors;

	/// This stores the cached results of the modifier.
	const PropertyPtr _deformationGradients;
};

}	// End of namespace
}	// End of namespace
