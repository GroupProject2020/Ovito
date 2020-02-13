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


#include <ovito/crystalanalysis/CrystalAnalysis.h>
#include <ovito/particles/modifier/analysis/StructureIdentificationModifier.h>
#include <ovito/crystalanalysis/modifier/dxa/StructureAnalysis.h>

namespace Ovito { namespace CrystalAnalysis {

/*
 * Extracts dislocation lines from a crystal.
 */
class OVITO_CRYSTALANALYSIS_EXPORT ElasticStrainModifier : public StructureIdentificationModifier
{
	Q_OBJECT
	OVITO_CLASS(ElasticStrainModifier)

	Q_CLASSINFO("DisplayName", "Elastic strain calculation");
	Q_CLASSINFO("ModifierCategory", "Analysis");

public:

	/// Constructor.
	Q_INVOKABLE ElasticStrainModifier(DataSet* dataset);

protected:

	/// Creates a computation engine that will compute the modifier's results.
	virtual Future<ComputeEnginePtr> createEngine(const PipelineEvaluationRequest& request, ModifierApplication* modApp, const PipelineFlowState& input) override;

private:

	/// The type of crystal to be analyzed.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(StructureAnalysis::LatticeStructureType, inputCrystalStructure, setInputCrystalStructure, PROPERTY_FIELD_MEMORIZE);

	/// Controls whether atomic deformation gradient tensors should be computed and stored.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(bool, calculateDeformationGradients, setCalculateDeformationGradients, PROPERTY_FIELD_MEMORIZE);

	/// Controls whether atomic strain tensors should be computed and stored.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(bool, calculateStrainTensors, setCalculateStrainTensors, PROPERTY_FIELD_MEMORIZE);

	/// Controls whether the calculated strain tensors should be pushed forward to the spatial reference frame.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(bool, pushStrainTensorsForward, setPushStrainTensorsForward, PROPERTY_FIELD_MEMORIZE);

	/// The lattice parameter of ideal crystal.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, latticeConstant, setLatticeConstant, PROPERTY_FIELD_MEMORIZE);

	/// The c/a ratio of the ideal crystal.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, axialRatio, setAxialRatio, PROPERTY_FIELD_MEMORIZE);
};

}	// End of namespace
}	// End of namespace
