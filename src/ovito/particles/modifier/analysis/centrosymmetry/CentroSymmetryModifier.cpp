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
#include <ovito/particles/util/NearestNeighborFinder.h>
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/stdobj/properties/PropertyAccess.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/core/utilities/concurrent/ParallelFor.h>
#include <ovito/core/utilities/units/UnitsManager.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>
#include "CentroSymmetryModifier.h"

namespace Ovito { namespace Particles {

IMPLEMENT_OVITO_CLASS(CentroSymmetryModifier);
DEFINE_PROPERTY_FIELD(CentroSymmetryModifier, numNeighbors);
SET_PROPERTY_FIELD_LABEL(CentroSymmetryModifier, numNeighbors, "Number of neighbors");
SET_PROPERTY_FIELD_UNITS_AND_RANGE(CentroSymmetryModifier, numNeighbors, IntegerParameterUnit, 2, CentroSymmetryModifier::MAX_CSP_NEIGHBORS);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
CentroSymmetryModifier::CentroSymmetryModifier(DataSet* dataset) : AsynchronousModifier(dataset),
	_numNeighbors(12)
{
}

/******************************************************************************
* Asks the modifier whether it can be applied to the given input data.
******************************************************************************/
bool CentroSymmetryModifier::OOMetaClass::isApplicableTo(const DataCollection& input) const
{
	return input.containsObject<ParticlesObject>();
}

/******************************************************************************
* Creates and initializes a computation engine that will compute the modifier's results.
******************************************************************************/
Future<AsynchronousModifier::ComputeEnginePtr> CentroSymmetryModifier::createEngine(const PipelineEvaluationRequest& request, ModifierApplication* modApp, const PipelineFlowState& input)
{
	// Get modifier input.
	const ParticlesObject* particles = input.expectObject<ParticlesObject>();
	particles->verifyIntegrity();
	const PropertyObject* posProperty = particles->expectProperty(ParticlesObject::PositionProperty);
	const SimulationCellObject* simCell = input.expectObject<SimulationCellObject>();

	if(numNeighbors() < 2)
		throwException(tr("The number of neighbors to take into account in the centrosymmetry calculation is invalid. It must be at least 2."));

	if(numNeighbors() % 2)
		throwException(tr("The number of neighbors to take into account in the centrosymmetry calculation must be a positive and even integer."));

	// Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
	return std::make_shared<CentroSymmetryEngine>(particles, posProperty->storage(), simCell->data(), numNeighbors());
}

/******************************************************************************
* Performs the actual computation. This method is executed in a worker thread.
******************************************************************************/
void CentroSymmetryModifier::CentroSymmetryEngine::perform()
{
	setProgressText(tr("Computing centrosymmetry parameters"));

	// Prepare the neighbor list.
	NearestNeighborFinder neighFinder(_nneighbors);
	if(!neighFinder.prepare(positions(), cell(), {}, this)) {
		return;
	}

	// Output storage.
	PropertyAccess<FloatType> output(csp());

	// Perform analysis on each particle.
	parallelFor(positions()->size(), *this, [&](size_t index) {
		output[index] = computeCSP(neighFinder, index);
	});

	// Release data that is no longer needed.
	_positions.reset();
}

/******************************************************************************
* Computes the centrosymmetry parameter of a single particle.
******************************************************************************/
FloatType CentroSymmetryModifier::computeCSP(NearestNeighborFinder& neighFinder, size_t particleIndex)
{
	// Find k nearest neighbor of current atom.
	NearestNeighborFinder::Query<MAX_CSP_NEIGHBORS> neighQuery(neighFinder);
	neighQuery.findNeighbors(particleIndex);

	int numNN = neighQuery.results().size();

    // R = Ri + Rj for each of npairs i,j pairs among numNN neighbors.
	FloatType pairs[MAX_CSP_NEIGHBORS*MAX_CSP_NEIGHBORS/2];
	FloatType* p = pairs;
	for(auto ij = neighQuery.results().begin(); ij != neighQuery.results().end(); ++ij) {
		for(auto ik = ij + 1; ik != neighQuery.results().end(); ++ik) {
			*p++ = (ik->delta + ij->delta).squaredLength();
		}
	}

    // Find NN/2 smallest pair distances from the list.
	std::partial_sort(pairs, pairs + (numNN/2), p);

    // Centrosymmetry = sum of numNN/2 smallest squared values.
    return std::accumulate(pairs, pairs + (numNN/2), FloatType(0), std::plus<FloatType>());
}

/******************************************************************************
* Injects the computed results of the engine into the data pipeline.
******************************************************************************/
void CentroSymmetryModifier::CentroSymmetryEngine::emitResults(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state)
{
	ParticlesObject* particles = state.expectMutableObject<ParticlesObject>();

	if(_inputFingerprint.hasChanged(particles))
		modApp->throwException(tr("Cached modifier results are obsolete, because the number or the storage order of input particles has changed."));

	OVITO_ASSERT(csp()->size() == particles->elementCount());
	particles->createProperty(csp());
}

}	// End of namespace
}	// End of namespace
