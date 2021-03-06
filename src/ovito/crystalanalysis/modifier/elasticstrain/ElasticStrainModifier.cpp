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

#include <ovito/crystalanalysis/CrystalAnalysis.h>
#include <ovito/crystalanalysis/objects/MicrostructurePhase.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/core/utilities/units/UnitsManager.h>
#include "ElasticStrainModifier.h"
#include "ElasticStrainEngine.h"

namespace Ovito { namespace CrystalAnalysis {

IMPLEMENT_OVITO_CLASS(ElasticStrainModifier);
DEFINE_PROPERTY_FIELD(ElasticStrainModifier, inputCrystalStructure);
DEFINE_PROPERTY_FIELD(ElasticStrainModifier, calculateDeformationGradients);
DEFINE_PROPERTY_FIELD(ElasticStrainModifier, calculateStrainTensors);
DEFINE_PROPERTY_FIELD(ElasticStrainModifier, latticeConstant);
DEFINE_PROPERTY_FIELD(ElasticStrainModifier, axialRatio);
DEFINE_PROPERTY_FIELD(ElasticStrainModifier, pushStrainTensorsForward);
SET_PROPERTY_FIELD_LABEL(ElasticStrainModifier, inputCrystalStructure, "Input crystal structure");
SET_PROPERTY_FIELD_LABEL(ElasticStrainModifier, calculateDeformationGradients, "Output deformation gradient tensors");
SET_PROPERTY_FIELD_LABEL(ElasticStrainModifier, calculateStrainTensors, "Output strain tensors");
SET_PROPERTY_FIELD_LABEL(ElasticStrainModifier, latticeConstant, "Lattice constant");
SET_PROPERTY_FIELD_LABEL(ElasticStrainModifier, axialRatio, "c/a ratio");
SET_PROPERTY_FIELD_LABEL(ElasticStrainModifier, pushStrainTensorsForward, "Strain tensor in spatial frame (push-forward)");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(ElasticStrainModifier, latticeConstant, WorldParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(ElasticStrainModifier, axialRatio, FloatParameterUnit, 0);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
ElasticStrainModifier::ElasticStrainModifier(DataSet* dataset) : StructureIdentificationModifier(dataset),
	_inputCrystalStructure(StructureAnalysis::LATTICE_FCC),
	_calculateDeformationGradients(false),
	_calculateStrainTensors(true),
	_latticeConstant(1),
	_axialRatio(sqrt(8.0/3.0)),
	_pushStrainTensorsForward(true)
{
	// Create the structure types.
	ParticleType::PredefinedStructureType predefTypes[] = {
			ParticleType::PredefinedStructureType::OTHER,
			ParticleType::PredefinedStructureType::FCC,
			ParticleType::PredefinedStructureType::HCP,
			ParticleType::PredefinedStructureType::BCC,
			ParticleType::PredefinedStructureType::CUBIC_DIAMOND,
			ParticleType::PredefinedStructureType::HEX_DIAMOND
	};
	OVITO_STATIC_ASSERT(sizeof(predefTypes)/sizeof(predefTypes[0]) == StructureAnalysis::NUM_LATTICE_TYPES);
	for(int id = 0; id < StructureAnalysis::NUM_LATTICE_TYPES; id++) {
		OORef<MicrostructurePhase> stype = new MicrostructurePhase(dataset);
		stype->setNumericId(id);
		stype->setDimensionality(MicrostructurePhase::Dimensionality::Volumetric);
		stype->setName(ParticleType::getPredefinedStructureTypeName(predefTypes[id]));
		stype->setColor(ParticleType::getDefaultParticleColor(ParticlesObject::StructureTypeProperty, stype->name(), id));
		addStructureType(stype);
	}
}

/******************************************************************************
* Creates and initializes a computation engine that will compute the modifier's results.
******************************************************************************/
Future<AsynchronousModifier::ComputeEnginePtr> ElasticStrainModifier::createEngine(const PipelineEvaluationRequest& request, ModifierApplication* modApp, const PipelineFlowState& input)
{
	// Get modifier inputs.
	const ParticlesObject* particles = input.expectObject<ParticlesObject>();
	particles->verifyIntegrity();
	const PropertyObject* posProperty = particles->expectProperty(ParticlesObject::PositionProperty);
	const SimulationCellObject* simCell = input.expectObject<SimulationCellObject>();
	if(simCell->is2D())
		throwException(tr("The elastic strain calculation modifier does not support 2d simulation cells."));

	// Build list of preferred crystal orientations.
	std::vector<Matrix3> preferredCrystalOrientations;
	if(inputCrystalStructure() == StructureAnalysis::LATTICE_FCC || inputCrystalStructure() == StructureAnalysis::LATTICE_BCC || inputCrystalStructure() == StructureAnalysis::LATTICE_CUBIC_DIAMOND) {
		preferredCrystalOrientations.push_back(Matrix3::Identity());
	}

	// Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
	return std::make_shared<ElasticStrainEngine>(particles, posProperty->storage(),
			simCell->data(), inputCrystalStructure(), std::move(preferredCrystalOrientations),
			calculateDeformationGradients(), calculateStrainTensors(),
			latticeConstant(), axialRatio(), pushStrainTensorsForward());
}

}	// End of namespace
}	// End of namespace
