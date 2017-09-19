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


#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include <plugins/particles/modifier/analysis/StructureIdentificationModifier.h>
#include <plugins/crystalanalysis/objects/patterns/PatternCatalog.h>
#include <plugins/crystalanalysis/modifier/dxa/StructureAnalysis.h>

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

/*
 * Extracts dislocation lines from a crystal.
 */
class OVITO_CRYSTALANALYSIS_EXPORT ElasticStrainModifier : public StructureIdentificationModifier
{
public:

	/// Constructor.
	Q_INVOKABLE ElasticStrainModifier(DataSet* dataset);

protected:

	/// Creates a computation engine that will compute the modifier's results.
	virtual Future<ComputeEnginePtr> createEngine(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input) override;

private:

	/// The type of crystal to be analyzed.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(StructureAnalysis::LatticeStructureType, inputCrystalStructure, setInputCrystalStructure);

	/// Controls whether atomic deformation gradient tensors should be computed and stored.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, calculateDeformationGradients, setCalculateDeformationGradients);

	/// Controls whether atomic strain tensors should be computed and stored.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, calculateStrainTensors, setCalculateStrainTensors);

	/// Controls whether the calculated strain tensors should be pushed forward to the spatial reference frame.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, pushStrainTensorsForward, setPushStrainTensorsForward);

	/// The lattice parameter of ideal crystal.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, latticeConstant, setLatticeConstant);

	/// The c/a ratio of the ideal crystal.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, axialRatio, setAxialRatio);

	/// The catalog of structure patterns.
	DECLARE_MODIFIABLE_REFERENCE_FIELD(PatternCatalog, patternCatalog, setPatternCatalog);

	Q_OBJECT
	OVITO_CLASS

	Q_CLASSINFO("DisplayName", "Elastic strain calculation");
	Q_CLASSINFO("ModifierCategory", "Analysis");
};

}	// End of namespace
}	// End of namespace
}	// End of namespace


