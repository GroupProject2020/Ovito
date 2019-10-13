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
#include <ovito/crystalanalysis/objects/ClusterGraphObject.h>
#include <ovito/core/utilities/concurrent/ParallelFor.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>
#include <ovito/core/dataset/DataSet.h>
#include "ElasticStrainEngine.h"
#include "ElasticStrainModifier.h"

namespace Ovito { namespace CrystalAnalysis {

/******************************************************************************
* Constructor.
******************************************************************************/
ElasticStrainEngine::ElasticStrainEngine(
		ParticleOrderingFingerprint fingerprint,
		ConstPropertyPtr positions, const SimulationCell& simCell,
		int inputCrystalStructure, std::vector<Matrix3> preferredCrystalOrientations,
		bool calculateDeformationGradients, bool calculateStrainTensors,
		FloatType latticeConstant, FloatType caRatio, bool pushStrainTensorsForward) :
	StructureIdentificationModifier::StructureIdentificationEngine(std::move(fingerprint), positions, simCell, {}),
	_structureAnalysis(std::make_unique<StructureAnalysis>(positions, simCell, (StructureAnalysis::LatticeStructureType)inputCrystalStructure, selection(), structures(), std::move(preferredCrystalOrientations))),
	_inputCrystalStructure(inputCrystalStructure),
	_latticeConstant(latticeConstant),
	_pushStrainTensorsForward(pushStrainTensorsForward),
	_volumetricStrains(std::make_shared<PropertyStorage>(positions->size(), PropertyStorage::Float, 1, 0, QStringLiteral("Volumetric Strain"), false)),
	_strainTensors(calculateStrainTensors ? ParticlesObject::OOClass().createStandardStorage(positions->size(), ParticlesObject::ElasticStrainTensorProperty, false) : nullptr),
	_deformationGradients(calculateDeformationGradients ? ParticlesObject::OOClass().createStandardStorage(positions->size(), ParticlesObject::ElasticDeformationGradientProperty, false) : nullptr)
{
	setAtomClusters(_structureAnalysis->atomClusters());
	if(inputCrystalStructure == StructureAnalysis::LATTICE_FCC || inputCrystalStructure == StructureAnalysis::LATTICE_BCC || inputCrystalStructure == StructureAnalysis::LATTICE_CUBIC_DIAMOND) {
		// Cubic crystal structures always have a c/a ratio of one.
		_axialScaling = 1;
	}
	else {
		// Convert to internal units.
		_latticeConstant *= sqrt(2.0);
		_axialScaling = caRatio / sqrt(8.0/3.0);
	}
}

/******************************************************************************
* Performs the actual analysis. This method is executed in a worker thread.
******************************************************************************/
void ElasticStrainEngine::perform()
{
	task()->setProgressText(ElasticStrainModifier::tr("Calculating elastic strain tensors"));

	task()->beginProgressSubStepsWithWeights({ 35, 6, 1, 1, 20 });
	if(!_structureAnalysis->identifyStructures(*task()))
		return;

	task()->nextProgressSubStep();
	if(!_structureAnalysis->buildClusters(*task()))
		return;

	task()->nextProgressSubStep();
	if(!_structureAnalysis->connectClusters(*task()))
		return;

	task()->nextProgressSubStep();
	if(!_structureAnalysis->formSuperClusters(*task()))
		return;

	task()->nextProgressSubStep();
	parallelFor(positions()->size(), *task(), [this](size_t particleIndex) {

		Cluster* localCluster = _structureAnalysis->atomCluster(particleIndex);
		if(localCluster->id != 0) {

			// The shape of the ideal unit cell.
			Matrix3 idealUnitCellTM(_latticeConstant, 0, 0,
									0, _latticeConstant, 0,
									0, 0, _latticeConstant * _axialScaling);

			// If the cluster is a defect (stacking fault), find the parent crystal cluster.
			Cluster* parentCluster = nullptr;
			if(localCluster->parentTransition != nullptr) {
				parentCluster = localCluster->parentTransition->cluster2;
				idealUnitCellTM = idealUnitCellTM * localCluster->parentTransition->tm;
			}
			else if(localCluster->structure == _inputCrystalStructure) {
				parentCluster = localCluster;
			}

			if(parentCluster != nullptr) {
				OVITO_ASSERT(parentCluster->structure == _inputCrystalStructure);

				// For calculating the cluster orientation.
				Matrix_3<double> orientationV = Matrix_3<double>::Zero();
				Matrix_3<double> orientationW = Matrix_3<double>::Zero();

				int numneigh = _structureAnalysis->numberOfNeighbors(particleIndex);
				for(int n = 0; n < numneigh; n++) {
					int neighborAtomIndex = _structureAnalysis->getNeighbor(particleIndex, n);
					// Add vector pair to matrices for computing the elastic deformation gradient.
					Vector3 latticeVector = idealUnitCellTM * _structureAnalysis->neighborLatticeVector(particleIndex, n);
					const Vector3& spatialVector = cell().wrapVector(positions()->getPoint3(neighborAtomIndex) - positions()->getPoint3(particleIndex));
					for(size_t i = 0; i < 3; i++) {
						for(size_t j = 0; j < 3; j++) {
							orientationV(i,j) += (double)(latticeVector[j] * latticeVector[i]);
							orientationW(i,j) += (double)(latticeVector[j] * spatialVector[i]);
						}
					}
				}

				// Calculate deformation gradient tensor.
				Matrix_3<double> elasticF = orientationW * orientationV.inverse();
				if(deformationGradients()) {
					for(size_t col = 0; col < 3; col++) {
						for(size_t row = 0; row < 3; row++) {
							deformationGradients()->setFloatComponent(particleIndex, col*3+row, (FloatType)elasticF(row,col));
						}
					}
				}

				// Calculate strain tensor.
				SymmetricTensor2T<double> elasticStrain;
				if(!_pushStrainTensorsForward) {
					// Compute Green strain tensor in material frame.
					elasticStrain = (Product_AtA(elasticF) - SymmetricTensor2T<double>::Identity()) * 0.5;
				}
				else {
					// Compute Euler strain tensor in spatial frame.
					Matrix_3<double> inverseF;
					if(!elasticF.inverse(inverseF))
						throw Exception(ElasticStrainModifier::tr("Cannot compute strain tensor in spatial reference frame, because the elastic deformation gradient at atom index %1 is singular.").arg(particleIndex+1));
					elasticStrain = (SymmetricTensor2T<double>::Identity() - Product_AtA(inverseF)) * 0.5;
				}

				// Store strain tensor in output property.
				if(strainTensors()) {
					strainTensors()->setSymmetricTensor2(particleIndex, (SymmetricTensor2)elasticStrain);
				}

				// Calculate volumetric strain component.
				double volumetricStrain = (elasticStrain(0,0) + elasticStrain(1,1) + elasticStrain(2,2)) / 3.0;
				OVITO_ASSERT(std::isfinite(volumetricStrain));
				volumetricStrains()->setFloat(particleIndex, (FloatType)volumetricStrain);

				return;
			}
		}

		// Mark atom as invalid.
		volumetricStrains()->setFloat(particleIndex, 0);
		if(strainTensors()) {
			for(size_t component = 0; component < 6; component++)
				strainTensors()->setFloatComponent(particleIndex, component, 0);
		}
		if(deformationGradients()) {
			for(size_t component = 0; component < 9; component++)
				deformationGradients()->setFloatComponent(particleIndex, component, 0);
		}
	});

	task()->endProgressSubSteps();
}

/******************************************************************************
* Injects the computed results of the engine into the data pipeline.
******************************************************************************/
void ElasticStrainEngine::emitResults(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state)
{
	ElasticStrainModifier* modifier = static_object_cast<ElasticStrainModifier>(modApp->modifier());

	StructureIdentificationEngine::emitResults(time, modApp, state);

	// Output cluster graph.
	ClusterGraphObject* clusterGraphObj = state.createObject<ClusterGraphObject>(modApp);
	clusterGraphObj->setStorage(clusterGraph());

	// Output particle properties.
	ParticlesObject* particles = state.expectMutableObject<ParticlesObject>();
	particles->createProperty(atomClusters());
	if(modifier->calculateStrainTensors() && strainTensors())
		particles->createProperty(strainTensors());

	if(modifier->calculateDeformationGradients() && deformationGradients())
		particles->createProperty(deformationGradients());

	if(volumetricStrains())
		particles->createProperty(volumetricStrains());
}

}	// End of namespace
}	// End of namespace
