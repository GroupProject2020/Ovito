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

#include <plugins/particles/Particles.h>
#include <plugins/particles/util/CutoffNeighborFinder.h>
#include <plugins/particles/modifier/ParticleInputHelper.h>
#include <plugins/particles/modifier/ParticleOutputHelper.h>
#include <core/dataset/pipeline/ModifierApplication.h>
#include <plugins/stdobj/simcell/SimulationCellObject.h>
#include <core/dataset/DataSetContainer.h>
#include <core/utilities/concurrent/ParallelFor.h>
#include <ptm/qcprot/polar.hpp>
#include "AtomicStrainModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

IMPLEMENT_OVITO_CLASS(AtomicStrainModifier);
DEFINE_PROPERTY_FIELD(AtomicStrainModifier, cutoff);
DEFINE_PROPERTY_FIELD(AtomicStrainModifier, calculateDeformationGradients);
DEFINE_PROPERTY_FIELD(AtomicStrainModifier, calculateStrainTensors);
DEFINE_PROPERTY_FIELD(AtomicStrainModifier, calculateNonaffineSquaredDisplacements);
DEFINE_PROPERTY_FIELD(AtomicStrainModifier, selectInvalidParticles);
DEFINE_PROPERTY_FIELD(AtomicStrainModifier, calculateStretchTensors);
DEFINE_PROPERTY_FIELD(AtomicStrainModifier, calculateRotations);
SET_PROPERTY_FIELD_LABEL(AtomicStrainModifier, cutoff, "Cutoff radius");
SET_PROPERTY_FIELD_LABEL(AtomicStrainModifier, calculateDeformationGradients, "Output deformation gradient tensors");
SET_PROPERTY_FIELD_LABEL(AtomicStrainModifier, calculateStrainTensors, "Output strain tensors");
SET_PROPERTY_FIELD_LABEL(AtomicStrainModifier, calculateNonaffineSquaredDisplacements, "Output non-affine squared displacements");
SET_PROPERTY_FIELD_LABEL(AtomicStrainModifier, selectInvalidParticles, "Select invalid particles");
SET_PROPERTY_FIELD_LABEL(AtomicStrainModifier, calculateStretchTensors, "Output stretch tensors");
SET_PROPERTY_FIELD_LABEL(AtomicStrainModifier, calculateRotations, "Output rotations");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(AtomicStrainModifier, cutoff, WorldParameterUnit, 0);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
AtomicStrainModifier::AtomicStrainModifier(DataSet* dataset) : ReferenceConfigurationModifier(dataset),
    _cutoff(3), 
	_calculateDeformationGradients(false), 
	_calculateStrainTensors(false), 
	_calculateNonaffineSquaredDisplacements(false),
	_calculateStretchTensors(false), 
	_calculateRotations(false),
    _selectInvalidParticles(true)
{
}

/******************************************************************************
* Creates and initializes a computation engine that will compute the modifier's results.
******************************************************************************/
Future<AsynchronousModifier::ComputeEnginePtr> AtomicStrainModifier::createEngineWithReference(TimePoint time, ModifierApplication* modApp, PipelineFlowState input, const PipelineFlowState& referenceState, TimeInterval validityInterval)
{
	ParticleInputHelper pih(dataset(), input);

	// Get the current particle positions.
	ParticleProperty* posProperty = pih.expectStandardProperty<ParticleProperty>(ParticleProperty::PositionProperty);

	// Get the reference particle position.
	ParticleProperty* refPosProperty = ParticleProperty::findInState(referenceState, ParticleProperty::PositionProperty);
	if(!refPosProperty)
		throwException(tr("Reference configuration does not contain particle positions."));

	// Get the simulation cells.
	SimulationCellObject* inputCell = pih.expectSimulationCell();
	SimulationCellObject* refCell = referenceState.findObject<SimulationCellObject>();
	if(!refCell)
		throwException(tr("Reference configuration does not contain simulation cell info."));

	// Vaildate the simulation cells.
	if((!inputCell->is2D() && inputCell->volume3D() < FLOATTYPE_EPSILON) || (inputCell->is2D() && inputCell->volume2D() < FLOATTYPE_EPSILON))
		throwException(tr("Simulation cell is degenerate in the deformed configuration."));
	if((!inputCell->is2D() && refCell->volume3D() < FLOATTYPE_EPSILON) || (inputCell->is2D() && refCell->volume2D() < FLOATTYPE_EPSILON))
		throwException(tr("Simulation cell is degenerate in the reference configuration."));

	// Get particle identifiers.
	ParticleProperty* identifierProperty = pih.inputStandardProperty<ParticleProperty>(ParticleProperty::IdentifierProperty);
	ParticleProperty* refIdentifierProperty = ParticleProperty::findInState(referenceState, ParticleProperty::IdentifierProperty);

	// Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
	return std::make_shared<AtomicStrainEngine>(validityInterval, posProperty->storage(), inputCell->data(), refPosProperty->storage(), refCell->data(),
			identifierProperty ? identifierProperty->storage() : nullptr, refIdentifierProperty ? refIdentifierProperty->storage() : nullptr,
			cutoff(), affineMapping(), useMinimumImageConvention(), calculateDeformationGradients(), calculateStrainTensors(),
			calculateNonaffineSquaredDisplacements(), calculateRotations(), calculateStretchTensors(), selectInvalidParticles());
}

/******************************************************************************
* Performs the actual computation. This method is executed in a worker thread.
******************************************************************************/
void AtomicStrainModifier::AtomicStrainEngine::perform()
{
	setProgressText(tr("Computing atomic displacements"));

	// First determine the mapping from particles of the reference config to particles
	// of the current config.
	if(!buildParticleMapping(false, false))
		return;

	// Compute displacement vectors of particles in the reference configuration.
	parallelForChunks(displacements()->size(), [this](size_t startIndex, size_t count) {
		Vector3* u = displacements()->dataVector3() + startIndex;
		const Point3* p0 = refPositions()->constDataPoint3() + startIndex;
		auto index = refToCurrentIndexMap().cbegin() + startIndex;
		for(; count; --count, ++u, ++p0, ++index) {
			if(*index == std::numeric_limits<size_t>::max()) {
				u->setZero();
				continue;
			}
			Point3 reduced_reference_pos = refCell().inverseMatrix() * (*p0);
			Point3 reduced_current_pos = cell().inverseMatrix() * positions()->getPoint3(*index);
			Vector3 delta = reduced_current_pos - reduced_reference_pos;
			if(useMinimumImageConvention()) {
				for(size_t k = 0; k < 3; k++) {
					if(refCell().pbcFlags()[k])
						delta[k] -= std::floor(delta[k] + FloatType(0.5));
				}
			}
			*u = refCell().matrix() * delta;
		}
	});
	if(isCanceled())
		return;

	setProgressText(tr("Computing atomic strain tensors"));
	
	// Prepare the neighbor list for the reference configuration.
	CutoffNeighborFinder neighborFinder;
	if(!neighborFinder.prepare(_cutoff, *refPositions(), refCell(), nullptr, this))
		return;

	// Perform individual strain calculation for each particle.
	parallelFor(positions()->size(), *this, [this, &neighborFinder](size_t index) {
		computeStrain(index, neighborFinder);
	});

	// Return the results of the compute engine.
	setResult(std::move(_results));
}

/******************************************************************************
* Computes the strain tensor of a single particle.
******************************************************************************/
void AtomicStrainModifier::AtomicStrainEngine::computeStrain(size_t particleIndex, CutoffNeighborFinder& neighborFinder)
{
	// Note: We do the following calculations using double precision numbers to
	// minimize numerical errors. Final results will be converted back to
	// standard precision.

	Matrix_3<double> V = Matrix_3<double>::Zero();
	Matrix_3<double> W = Matrix_3<double>::Zero();
	int numNeighbors = 0;

	// Iterate over neighbors of central particle.
	size_t particleIndexReference = currentToRefIndexMap()[particleIndex];
	FloatType sumSquaredDistance = 0;
	if(particleIndexReference != std::numeric_limits<size_t>::max()) {
		const Vector3& center_displacement = displacements()->getVector3(particleIndexReference);
		for(CutoffNeighborFinder::Query neighQuery(neighborFinder, particleIndexReference); !neighQuery.atEnd(); neighQuery.next()) {
			size_t neighborIndexCurrent = refToCurrentIndexMap()[neighQuery.current()];
			if(neighborIndexCurrent == std::numeric_limits<size_t>::max()) continue;
			const Vector3& neigh_displacement = displacements()->getVector3(neighQuery.current());
			Vector3 delta_ref = neighQuery.delta();
			Vector3 delta_cur = delta_ref + neigh_displacement - center_displacement;
			if(affineMapping() == TO_CURRENT_CELL) {
				delta_ref = refToCurTM() * delta_ref;
				delta_cur = refToCurTM() * delta_cur;
			}
			else if(affineMapping() != TO_REFERENCE_CELL) {
				delta_cur = refToCurTM() * delta_cur;
			}
			for(size_t i = 0; i < 3; i++) {
				for(size_t j = 0; j < 3; j++) {
					V(i,j) += delta_ref[j] * delta_ref[i];
					W(i,j) += delta_ref[j] * delta_cur[i];
				}
			}
			sumSquaredDistance += delta_ref.squaredLength();
			numNeighbors++;
		}
	}

	// Special handling for 2D systems.
	if(cell().is2D()) {
		// Assume plane strain.
		V(2,2) = W(2,2) = 1;
		V(0,2) = V(1,2) = V(2,0) = V(2,1) = 0;
		W(0,2) = W(1,2) = W(2,0) = W(2,1) = 0;
	}

	// Check if matrix can be inverted.
	Matrix_3<double> inverseV;
	double detThreshold = (double)sumSquaredDistance * 1e-12;
	if(numNeighbors < 2 || (!cell().is2D() && numNeighbors < 3) || !V.inverse(inverseV, detThreshold) || std::abs(W.determinant()) <= detThreshold) {
		if(_results->invalidParticles())
			_results->invalidParticles()->setInt(particleIndex, 1);
		if(_results->deformationGradients()) {
			for(Matrix_3<double>::size_type col = 0; col < 3; col++) {
				for(Matrix_3<double>::size_type row = 0; row < 3; row++) {
					_results->deformationGradients()->setFloatComponent(particleIndex, col*3+row, FloatType(0));
				}
			}
		}
		if(_results->strainTensors())
			_results->strainTensors()->setSymmetricTensor2(particleIndex, SymmetricTensor2::Zero());
        if(_results->nonaffineSquaredDisplacements())
            _results->nonaffineSquaredDisplacements()->setFloat(particleIndex, 0);
		_results->shearStrains()->setFloat(particleIndex, 0);
		_results->volumetricStrains()->setFloat(particleIndex, 0);
		if(_results->rotations())
			_results->rotations()->setQuaternion(particleIndex, Quaternion(0,0,0,0));
		if(_results->stretchTensors())
			_results->stretchTensors()->setSymmetricTensor2(particleIndex, SymmetricTensor2::Zero());
		_results->addInvalidParticle();
		return;
	}

	// Calculate deformation gradient tensor F.
	Matrix_3<double> F = W * inverseV;
	if(_results->deformationGradients()) {
		for(Matrix_3<double>::size_type col = 0; col < 3; col++) {
			for(Matrix_3<double>::size_type row = 0; row < 3; row++) {
				_results->deformationGradients()->setFloatComponent(particleIndex, col*3+row, (FloatType)F(row,col));
			}
		}
	}

	// Polar decomposition F=RU.
	if(_results->rotations() || _results->stretchTensors()) {
		Matrix_3<double> R, U;
		polar_decomposition_3x3(F.elements(), false, R.elements(), U.elements());
		if(_results->rotations()) {
			_results->rotations()->setQuaternion(particleIndex, (Quaternion)QuaternionT<double>(R));
		}
		if(_results->stretchTensors()) {
			_results->stretchTensors()->setSymmetricTensor2(particleIndex,
					SymmetricTensor2(U(0,0), U(1,1), U(2,2), U(0,1), U(0,2), U(1,2)));
		}
	}

	// Calculate strain tensor.
	SymmetricTensor2T<double> strain = (Product_AtA(F) - SymmetricTensor2T<double>::Identity()) * 0.5;
	if(_results->strainTensors())
		_results->strainTensors()->setSymmetricTensor2(particleIndex, (SymmetricTensor2)strain);

    // Calculate nonaffine displacement.
    if(_results->nonaffineSquaredDisplacements()) {
		FloatType D2min = 0;
		Matrix3 Fftype = static_cast<Matrix3>(F);

        // Again iterate over neighbor vectors of central particle.
        numNeighbors = 0;
        const Vector3& center_displacement = displacements()->getVector3(particleIndexReference);
        for(CutoffNeighborFinder::Query neighQuery(neighborFinder, particleIndexReference); !neighQuery.atEnd(); neighQuery.next()) {
			size_t neighborIndexCurrent = refToCurrentIndexMap()[neighQuery.current()];
			if(neighborIndexCurrent == std::numeric_limits<size_t>::max()) continue;
			const Vector3& neigh_displacement = displacements()->getVector3(neighQuery.current());
			Vector3 delta_ref = neighQuery.delta();
			Vector3 delta_cur = delta_ref + neigh_displacement - center_displacement;
			if(affineMapping() == TO_CURRENT_CELL) {
				delta_ref = refToCurTM() * delta_ref;
				delta_cur = refToCurTM() * delta_cur;
			}
			else if(affineMapping() != TO_REFERENCE_CELL) {
				delta_cur = refToCurTM() * delta_cur;
			}
			D2min += (Fftype * delta_ref - delta_cur).squaredLength();
		}

        _results->nonaffineSquaredDisplacements()->setFloat(particleIndex, D2min);
	}

	// Calculate von Mises shear strain.
	double xydiff = strain.xx() - strain.yy();
	double xzdiff = strain.xx() - strain.zz();
	double yzdiff = strain.yy() - strain.zz();
	double shearStrain = sqrt(strain.xy()*strain.xy() + strain.xz()*strain.xz() + strain.yz()*strain.yz() +
			(xydiff*xydiff + xzdiff*xzdiff + yzdiff*yzdiff) / 6.0);
	OVITO_ASSERT(std::isfinite(shearStrain));
	_results->shearStrains()->setFloat(particleIndex, (FloatType)shearStrain);

	// Calculate volumetric component.
	double volumetricStrain = (strain(0,0) + strain(1,1) + strain(2,2)) / 3.0;
	OVITO_ASSERT(std::isfinite(volumetricStrain));
	_results->volumetricStrains()->setFloat(particleIndex, (FloatType)volumetricStrain);

	if(_results->invalidParticles())
		_results->invalidParticles()->setInt(particleIndex, 0);
}

/******************************************************************************
* Injects the computed results of the engine into the data pipeline.
******************************************************************************/
PipelineFlowState AtomicStrainModifier::AtomicStrainResults::apply(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input)
{
	PipelineFlowState output = input;
	ParticleOutputHelper poh(modApp->dataset(), output);

	OVITO_ASSERT(shearStrains());
	if(shearStrains()->size() != poh.outputParticleCount())
		modApp->throwException(tr("Cached modifier results are obsolete, because the number of input particles has changed."));

	if(invalidParticles())
		poh.outputProperty<ParticleProperty>(invalidParticles());

	if(strainTensors())
		poh.outputProperty<ParticleProperty>(strainTensors());

	if(deformationGradients())
		poh.outputProperty<ParticleProperty>(deformationGradients());

	if(nonaffineSquaredDisplacements())
		poh.outputProperty<ParticleProperty>(nonaffineSquaredDisplacements());

	if(volumetricStrains())
		poh.outputProperty<ParticleProperty>(volumetricStrains());

	if(shearStrains())
		poh.outputProperty<ParticleProperty>(shearStrains());

	if(rotations())
		poh.outputProperty<ParticleProperty>(rotations());

	if(stretchTensors())
		poh.outputProperty<ParticleProperty>(stretchTensors());

	poh.outputAttribute(QStringLiteral("AtomicStrain.invalid_particle_count"), QVariant::fromValue(numInvalidParticles()));

	if(numInvalidParticles() != 0)
		output.setStatus(PipelineStatus(PipelineStatus::Warning, tr("Could not compute local deformation for %1 particles because of too few neighbors. Increase cutoff radius to include more neighbors.").arg(numInvalidParticles())));

	return output;
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
