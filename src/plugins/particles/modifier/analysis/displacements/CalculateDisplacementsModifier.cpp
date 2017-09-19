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
#include <plugins/particles/modifier/ParticleInputHelper.h>
#include <plugins/particles/modifier/ParticleOutputHelper.h>
#include <core/dataset/pipeline/ModifierApplication.h>
#include <core/dataset/data/simcell/SimulationCellObject.h>
#include <core/utilities/concurrent/ParallelFor.h>
#include "CalculateDisplacementsModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

IMPLEMENT_OVITO_CLASS(CalculateDisplacementsModifier);
DEFINE_REFERENCE_FIELD(CalculateDisplacementsModifier, vectorDisplay);
SET_PROPERTY_FIELD_LABEL(CalculateDisplacementsModifier, vectorDisplay, "Vector display");

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
CalculateDisplacementsModifier::CalculateDisplacementsModifier(DataSet* dataset) : ReferenceConfigurationModifier(dataset)
{
	// Create display object for vectors.
	setVectorDisplay(new VectorDisplay(dataset));
	vectorDisplay()->setObjectTitle(tr("Displacements"));

	// Don't show vectors by default, because too many vectors can make the
	// program freeze. User has to enable the display manually.
	vectorDisplay()->setEnabled(false);

	// Configure vector display such that arrows point from the reference particle positions
	// to the current particle positions.
	vectorDisplay()->setReverseArrowDirection(false);
	vectorDisplay()->setArrowPosition(VectorDisplay::Head);
}

/*********************************************sourceFrameToAnimationTime*********************************
* Handles reference events sent by reference targets of this object.
******************************************************************************/
bool CalculateDisplacementsModifier::referenceEvent(RefTarget* source, ReferenceEvent* event)
{
	// Do not propagate messages sent by the attached display object.
	if(source == vectorDisplay())
		return false;

	return ReferenceConfigurationModifier::referenceEvent(source, event);
}

/******************************************************************************
* Creates and initializes a computation engine that will compute the modifier's results.
******************************************************************************/
Future<AsynchronousModifier::ComputeEnginePtr> CalculateDisplacementsModifier::createEngineWithReference(TimePoint time, ModifierApplication* modApp, PipelineFlowState input, const PipelineFlowState& referenceState, TimeInterval validityInterval)
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

	// Get particle identifiers.
	ParticleProperty* identifierProperty = pih.inputStandardProperty<ParticleProperty>(ParticleProperty::IdentifierProperty);
	ParticleProperty* refIdentifierProperty = ParticleProperty::findInState(referenceState, ParticleProperty::IdentifierProperty);

	// Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
	return std::make_shared<DisplacementEngine>(validityInterval, posProperty->storage(), inputCell->data(), refPosProperty->storage(), refCell->data(),
			identifierProperty ? identifierProperty->storage() : nullptr, refIdentifierProperty ? refIdentifierProperty->storage() : nullptr,
			affineMapping(), useMinimumImageConvention());
}

/******************************************************************************
* Asks the object for the result of the data pipeline.
******************************************************************************/
void CalculateDisplacementsModifier::DisplacementEngine::perform()
{
	// First determine the mapping from particles of the reference config to particles
	// of the current config.
	if(!buildParticleMapping())
		return;

	// Compute displacement vectors.
	if(affineMapping() != NO_MAPPING) {
		parallelForChunks(displacements()->size(), [this](size_t startIndex, size_t count) {
			Vector3* u = displacements()->dataVector3() + startIndex;
			FloatType* umag = displacementMagnitudes()->dataFloat() + startIndex;
			const Point3* p = positions()->constDataPoint3() + startIndex;
			auto index = currentToRefIndexMap().cbegin() + startIndex;
			for(; count; --count, ++u, ++umag, ++p, ++index) {
				Point3 ru = cell().inverseMatrix() * (*p);
				Point3 ru0 = refCell().inverseMatrix() * refPositions()->getPoint3(*index);
				Vector3 delta = ru - ru0;
				if(useMinimumImageConvention()) {
					for(size_t k = 0; k < 3; k++) {
						if(refCell().pbcFlags()[k]) {
							if(delta[k] > FloatType(0.5)) delta[k] -= FloatType(1);
							else if(delta[k] < FloatType(-0.5)) delta[k] += FloatType(1);
						}
					}
				}
				if(affineMapping() == TO_REFERENCE_CELL)
					*u = refCell().matrix() * delta;
				else
					*u = cell().matrix() * delta;
				*umag = u->length();
			}
		});
	}
	else {
		parallelForChunks(displacements()->size(), [this] (size_t startIndex, size_t count) {
			Vector3* u = displacements()->dataVector3() + startIndex;
			FloatType* umag = displacementMagnitudes()->dataFloat() + startIndex;
			const Point3* p = positions()->constDataPoint3() + startIndex;
			auto index = currentToRefIndexMap().cbegin() + startIndex;
			for(; count; --count, ++u, ++umag, ++p, ++index) {
				*u = *p - refPositions()->getPoint3(*index);
				if(useMinimumImageConvention()) {
					for(size_t k = 0; k < 3; k++) {
						if(refCell().pbcFlags()[k]) {
							if((*u + refCell().matrix().column(k)).squaredLength() < u->squaredLength())
								*u += refCell().matrix().column(k);
							else if((*u - refCell().matrix().column(k)).squaredLength() < u->squaredLength())
								*u -= refCell().matrix().column(k);
						}
					}
					*umag = u->length();
				}
			}
		});
	}

	// Return the results of the compute engine.
	setResult(std::move(_results));
}

/******************************************************************************
* Injects the computed results of the engine into the data pipeline.
******************************************************************************/
PipelineFlowState CalculateDisplacementsModifier::DisplacementResults::apply(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input)
{
	CalculateDisplacementsModifier* modifier = static_object_cast<CalculateDisplacementsModifier>(modApp->modifier());

	PipelineFlowState output = input;
	ParticleOutputHelper poh(modApp->dataset(), output);
	poh.outputProperty<ParticleProperty>(displacements())->setDisplayObject(modifier->vectorDisplay());	
	poh.outputProperty<ParticleProperty>(displacementMagnitudes());
	
	return output;
}


OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
