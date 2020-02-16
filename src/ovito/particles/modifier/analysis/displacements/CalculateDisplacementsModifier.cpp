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

#include <ovito/particles/Particles.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/stdobj/properties/PropertyAccess.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>
#include <ovito/core/utilities/concurrent/ParallelFor.h>
#include "CalculateDisplacementsModifier.h"

namespace Ovito { namespace Particles {

IMPLEMENT_OVITO_CLASS(CalculateDisplacementsModifier);
DEFINE_REFERENCE_FIELD(CalculateDisplacementsModifier, vectorVis);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
CalculateDisplacementsModifier::CalculateDisplacementsModifier(DataSet* dataset) : ReferenceConfigurationModifier(dataset)
{
	// Create vis element for vectors.
	setVectorVis(new VectorVis(dataset));
	vectorVis()->setObjectTitle(tr("Displacements"));

	// Don't show vectors by default, because too many vectors can make the
	// program freeze. User has to enable the display manually.
	vectorVis()->setEnabled(false);

	// Configure vector display such that arrows point from the reference particle positions
	// to the current particle positions.
	vectorVis()->setReverseArrowDirection(false);
	vectorVis()->setArrowPosition(VectorVis::Head);
}

/******************************************************************************
* Creates and initializes a computation engine that will compute the modifier's results.
******************************************************************************/
Future<AsynchronousModifier::ComputeEnginePtr> CalculateDisplacementsModifier::createEngineInternal(const PipelineEvaluationRequest& request, ModifierApplication* modApp, PipelineFlowState input, const PipelineFlowState& referenceState, TimeInterval validityInterval)
{
	// Get the current particle positions.
	const ParticlesObject* particles = input.expectObject<ParticlesObject>();
	particles->verifyIntegrity();
	const PropertyObject* posProperty = particles->expectProperty(ParticlesObject::PositionProperty);

	// Get the reference particle position.
	const ParticlesObject* refParticles = referenceState.getObject<ParticlesObject>();
	if(!refParticles)
		throwException(tr("Reference configuration does not contain particles."));
	refParticles->verifyIntegrity();
	const PropertyObject* refPosProperty = refParticles->expectProperty(ParticlesObject::PositionProperty);

	// Get the simulation cells.
	const SimulationCellObject* inputCell = input.expectObject<SimulationCellObject>();
	const SimulationCellObject* refCell = referenceState.getObject<SimulationCellObject>();
	if(!refCell)
		throwException(tr("Reference configuration does not contain simulation cell info."));

	// Get particle identifiers.
	ConstPropertyPtr identifierProperty = particles->getPropertyStorage(ParticlesObject::IdentifierProperty);
	ConstPropertyPtr refIdentifierProperty = refParticles->getPropertyStorage(ParticlesObject::IdentifierProperty);

	// Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
	return std::make_shared<DisplacementEngine>(validityInterval, posProperty->storage(), inputCell->data(),
			particles, refPosProperty->storage(), refCell->data(),
			std::move(identifierProperty), std::move(refIdentifierProperty),
			affineMapping(), useMinimumImageConvention());
}

/******************************************************************************
* Asks the object for the result of the data pipeline.
******************************************************************************/
void CalculateDisplacementsModifier::DisplacementEngine::perform()
{
	// First determine the mapping from particles of the reference config to particles
	// of the current config.
	if(!buildParticleMapping(true, false))
		return;

	PropertyAccess<Vector3> displacementsArray(displacements());
	PropertyAccess<FloatType> displacementMagnitudesArray(displacementMagnitudes());
	ConstPropertyAccess<Point3> positionsArray(positions());
	ConstPropertyAccess<Point3> refPositionsArray(refPositions());

	// Compute displacement vectors.
	if(affineMapping() != NO_MAPPING) {
		parallelForChunks(displacements()->size(), *this, [&](size_t startIndex, size_t count, Task& task) {
			Vector3* u = displacementsArray.begin() + startIndex;
			FloatType* umag = displacementMagnitudesArray.begin() + startIndex;
			const Point3* p = positionsArray.cbegin() + startIndex;
			auto index = currentToRefIndexMap().cbegin() + startIndex;
			const AffineTransformation& reduced_to_absolute = (affineMapping() == TO_REFERENCE_CELL) ? refCell().matrix() : cell().matrix();
			for(; count; --count, ++u, ++umag, ++p, ++index) {
				if(task.isCanceled()) return;
				Point3 reduced_current_pos = cell().inverseMatrix() * (*p);
				Point3 reduced_reference_pos = refCell().inverseMatrix() * refPositionsArray[*index];
				Vector3 delta = reduced_current_pos - reduced_reference_pos;
				if(useMinimumImageConvention()) {
					for(size_t k = 0; k < 3; k++) {
						if(refCell().pbcFlags()[k])
							delta[k] -= std::floor(delta[k] + FloatType(0.5));
					}
				}
				*u = reduced_to_absolute * delta;
				*umag = u->length();
			}
		});
	}
	else {
		parallelForChunks(displacements()->size(), *this, [&] (size_t startIndex, size_t count, Task& task) {
			Vector3* u = displacementsArray.begin() + startIndex;
			FloatType* umag = displacementMagnitudesArray.begin() + startIndex;
			const Point3* p = positionsArray.cbegin() + startIndex;
			auto index = currentToRefIndexMap().cbegin() + startIndex;
			for(; count; --count, ++u, ++umag, ++p, ++index) {
				if(task.isCanceled()) return;
				*u = *p - refPositionsArray[*index];
				if(useMinimumImageConvention()) {
					for(size_t k = 0; k < 3; k++) {
						if(refCell().pbcFlags()[k]) {
							while((*u + refCell().matrix().column(k)).squaredLength() < u->squaredLength())
								*u += refCell().matrix().column(k);

							while((*u - refCell().matrix().column(k)).squaredLength() < u->squaredLength())
								*u -= refCell().matrix().column(k);
						}
					}
				}
				*umag = u->length();
			}
		});
	}

	// Release data that is no longer needed.
	releaseWorkingData();
}

/******************************************************************************
* Injects the computed results of the engine into the data pipeline.
******************************************************************************/
void CalculateDisplacementsModifier::DisplacementEngine::emitResults(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state)
{
	CalculateDisplacementsModifier* modifier = static_object_cast<CalculateDisplacementsModifier>(modApp->modifier());

	ParticlesObject* particles = state.expectMutableObject<ParticlesObject>();

	if(_inputFingerprint.hasChanged(particles))
		modApp->throwException(tr("Cached modifier results are obsolete, because the number or the storage order of input particles has changed."));

	particles->createProperty(displacements())->setVisElement(modifier->vectorVis());
	particles->createProperty(displacementMagnitudes());
}

}	// End of namespace
}	// End of namespace
