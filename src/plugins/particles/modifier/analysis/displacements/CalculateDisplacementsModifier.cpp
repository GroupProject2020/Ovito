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
#include <core/dataset/pipeline/ModifierApplication.h>
#include <plugins/stdobj/simcell/SimulationCellObject.h>
#include <core/utilities/concurrent/ParallelFor.h>
#include "CalculateDisplacementsModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

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
Future<AsynchronousModifier::ComputeEnginePtr> CalculateDisplacementsModifier::createEngineWithReference(TimePoint time, ModifierApplication* modApp, PipelineFlowState input, const PipelineFlowState& referenceState, TimeInterval validityInterval)
{
	// Get the current particle positions.
	const ParticlesObject* particles = input.expectObject<ParticlesObject>();
	const PropertyObject* posProperty = particles->expectProperty(ParticlesObject::PositionProperty);

	// Get the reference particle position.
	const ParticlesObject* refParticles = referenceState.getObject<ParticlesObject>();
	if(!refParticles)
		throwException(tr("Reference configuration does not contain particles."));
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

	// Compute displacement vectors.
	if(affineMapping() != NO_MAPPING) {
		parallelForChunks(displacements()->size(), *task(), [this](size_t startIndex, size_t count, PromiseState& promise) {
			Vector3* u = displacements()->dataVector3() + startIndex;
			FloatType* umag = displacementMagnitudes()->dataFloat() + startIndex;
			const Point3* p = positions()->constDataPoint3() + startIndex;
			auto index = currentToRefIndexMap().cbegin() + startIndex;
			const AffineTransformation& reduced_to_absolute = (affineMapping() == TO_REFERENCE_CELL) ? refCell().matrix() : cell().matrix();
			for(; count; --count, ++u, ++umag, ++p, ++index) {
				if(promise.isCanceled()) return;
				Point3 reduced_current_pos = cell().inverseMatrix() * (*p);
				Point3 reduced_reference_pos = refCell().inverseMatrix() * refPositions()->getPoint3(*index);
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
		parallelForChunks(displacements()->size(), *task(), [this] (size_t startIndex, size_t count, PromiseState& promise) {
			Vector3* u = displacements()->dataVector3() + startIndex;
			FloatType* umag = displacementMagnitudes()->dataFloat() + startIndex;
			const Point3* p = positions()->constDataPoint3() + startIndex;
			auto index = currentToRefIndexMap().cbegin() + startIndex;
			for(; count; --count, ++u, ++umag, ++p, ++index) {
				if(promise.isCanceled()) return;
				*u = *p - refPositions()->getPoint3(*index);
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


OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
