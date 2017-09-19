///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2015) Alexander Stukowski
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
#include <plugins/particles/util/NearestNeighborFinder.h>
#include <plugins/particles/objects/BondsObject.h>
#include <plugins/particles/modifier/ParticleInputHelper.h>
#include <plugins/particles/modifier/ParticleOutputHelper.h>
#include <core/utilities/concurrent/ParallelFor.h>
#include <core/utilities/units/UnitsManager.h>
#include <core/dataset/pipeline/ModifierApplication.h>
#include <core/dataset/data/simcell/SimulationCellObject.h>
#include "ExpandSelectionModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Selection)

IMPLEMENT_OVITO_CLASS(ExpandSelectionModifier);
DEFINE_PROPERTY_FIELD(ExpandSelectionModifier, mode);
DEFINE_PROPERTY_FIELD(ExpandSelectionModifier, cutoffRange);
DEFINE_PROPERTY_FIELD(ExpandSelectionModifier, numNearestNeighbors);
DEFINE_PROPERTY_FIELD(ExpandSelectionModifier, numberOfIterations);
SET_PROPERTY_FIELD_LABEL(ExpandSelectionModifier, mode, "Mode");
SET_PROPERTY_FIELD_LABEL(ExpandSelectionModifier, cutoffRange, "Cutoff distance");
SET_PROPERTY_FIELD_LABEL(ExpandSelectionModifier, numNearestNeighbors, "N");
SET_PROPERTY_FIELD_LABEL(ExpandSelectionModifier, numberOfIterations, "Number of iterations");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(ExpandSelectionModifier, cutoffRange, WorldParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_RANGE(ExpandSelectionModifier, numNearestNeighbors, IntegerParameterUnit, 1, ExpandSelectionModifier::MAX_NEAREST_NEIGHBORS);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(ExpandSelectionModifier, numberOfIterations, IntegerParameterUnit, 1);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
ExpandSelectionModifier::ExpandSelectionModifier(DataSet* dataset) : AsynchronousModifier(dataset),
	_mode(CutoffRange), 
	_cutoffRange(3.2), 
	_numNearestNeighbors(1), 
	_numberOfIterations(1)
{
}

/******************************************************************************
* Asks the modifier whether it can be applied to the given input data.
******************************************************************************/
bool ExpandSelectionModifier::OOMetaClass::isApplicableTo(const PipelineFlowState& input) const
{
	return input.findObject<ParticleProperty>() != nullptr;
}

/******************************************************************************
* Creates and initializes a computation engine that will compute the 
* modifier's results.
******************************************************************************/
Future<AsynchronousModifier::ComputeEnginePtr> ExpandSelectionModifier::createEngine(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input)
{
	ParticleInputHelper pih(dataset(), input);

	// Get the current positions.
	ParticleProperty* posProperty = pih.expectStandardProperty<ParticleProperty>(ParticleProperty::PositionProperty);

	// Get the current selection.
	ParticleProperty* inputSelection = pih.expectStandardProperty<ParticleProperty>(ParticleProperty::SelectionProperty);

	// Get simulation cell.
	SimulationCellObject* inputCell = pih.expectSimulationCell();

	// Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
	if(mode() == CutoffRange) {
		return std::make_shared<ExpandSelectionCutoffEngine>(input.stateValidity(), posProperty->storage(), inputCell->data(), inputSelection->storage(), numberOfIterations(), cutoffRange());
	}
	else if(mode() == NearestNeighbors) {
		return std::make_shared<ExpandSelectionNearestEngine>(input.stateValidity(), posProperty->storage(), inputCell->data(), inputSelection->storage(), numberOfIterations(), numNearestNeighbors());
	}
	else if(mode() == BondedNeighbors) {
		BondsObject* bonds = pih.expectBonds();
		return std::make_shared<ExpandSelectionBondedEngine>(input.stateValidity(), posProperty->storage(), inputCell->data(), inputSelection->storage(), numberOfIterations(), bonds->storage());
	}
	else {
		throwException(tr("Invalid selection expansion mode."));
	}
}

/******************************************************************************
* Performs the actual computation. This method is executed in a worker thread.
******************************************************************************/
void ExpandSelectionModifier::ExpandSelectionEngine::perform()
{
	setProgressText(tr("Expanding particle selection"));

	results()->setNumSelectedParticlesInput(_inputSelection->size() - std::count(_inputSelection->constDataInt(), _inputSelection->constDataInt() + _inputSelection->size(), 0));

	beginProgressSubSteps(_numIterations);
	for(int i = 0; i < _numIterations; i++) {
		if(i != 0) {
			_inputSelection = results()->outputSelection();
			results()->setOutputSelection(std::make_shared<PropertyStorage>(*inputSelection()));
			nextProgressSubStep();
		}
		expandSelection();
		if(isCanceled()) return;
	}
	endProgressSubSteps();

	results()->setNumSelectedParticlesOutput(results()->outputSelection()->size() - std::count(results()->outputSelection()->constDataInt(), results()->outputSelection()->constDataInt() + _inputSelection->size(), 0));
}

/******************************************************************************
* Performs one iteration of the selection expansion.
******************************************************************************/
void ExpandSelectionModifier::ExpandSelectionNearestEngine::expandSelection()
{
	if(_numNearestNeighbors > MAX_NEAREST_NEIGHBORS)
		throw Exception(tr("Invalid parameter. The expand selection modifier can expand the selection only to the %1 nearest neighbors of particles. This limit is set at compile time.").arg(MAX_NEAREST_NEIGHBORS));

	// Prepare the neighbor list.
	NearestNeighborFinder neighFinder(_numNearestNeighbors);
	if(!neighFinder.prepare(*positions(), simCell(), nullptr, this))
		return;

	OVITO_ASSERT(inputSelection()->constDataInt() != results()->outputSelection()->dataInt());
	parallelFor(positions()->size(), *this, [&neighFinder, this](size_t index) {
		if(!inputSelection()->getInt(index)) return;

		NearestNeighborFinder::Query<MAX_NEAREST_NEIGHBORS> neighQuery(neighFinder);
		neighQuery.findNeighbors(index);
		OVITO_ASSERT(neighQuery.results().size() <= _numNearestNeighbors);

		for(auto n = neighQuery.results().begin(); n != neighQuery.results().end(); ++n) {
			results()->outputSelection()->setInt(n->index, 1);
		}
	});
}

/******************************************************************************
* Performs one iteration of the selection expansion.
******************************************************************************/
void ExpandSelectionModifier::ExpandSelectionBondedEngine::expandSelection()
{
	OVITO_ASSERT(inputSelection()->constDataInt() != results()->outputSelection()->dataInt());

	unsigned int particleCount = inputSelection()->size();
	parallelFor(_bonds->size(), *this, [this,particleCount](size_t index) {

		const Bond& bond = (*_bonds)[index];
		if(bond.index1 >= particleCount || bond.index2 >= particleCount)
			return;
		if(inputSelection()->getInt(bond.index1))
			results()->outputSelection()->setInt(bond.index2, 1);
		if(inputSelection()->getInt(bond.index2))
			results()->outputSelection()->setInt(bond.index1, 1);
	});
}

/******************************************************************************
* Performs one iteration of the selection expansion.
******************************************************************************/
void ExpandSelectionModifier::ExpandSelectionCutoffEngine::expandSelection()
{
	// Prepare the neighbor list.
	CutoffNeighborFinder neighborListBuilder;
	if(!neighborListBuilder.prepare(_cutoffRange, *positions(), simCell(), nullptr, this))
		return;

	OVITO_ASSERT(inputSelection()->constDataInt() != results()->outputSelection()->dataInt());
	parallelFor(positions()->size(), *this, [&neighborListBuilder, this](size_t index) {
		if(!inputSelection()->getInt(index)) return;
		for(CutoffNeighborFinder::Query neighQuery(neighborListBuilder, index); !neighQuery.atEnd(); neighQuery.next()) {
			results()->outputSelection()->setInt(neighQuery.current(), 1);
		}
	});
}

/******************************************************************************
* Injects the computed results of the engine into the data pipeline.
******************************************************************************/
PipelineFlowState ExpandSelectionModifier::ExpandSelectionResults::apply(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input)
{
	PipelineFlowState output = input;
	ParticleOutputHelper poh(modApp->dataset(), output);

	poh.outputProperty<ParticleProperty>(outputSelection());

	QString msg = tr("Added %1 particles to selection.\n"
			"Old selection count was: %2\n"
			"New selection count is: %3")
					.arg(numSelectedParticlesOutput() - numSelectedParticlesInput())
					.arg(numSelectedParticlesInput())
					.arg(numSelectedParticlesOutput());
	output.setStatus(PipelineStatus(PipelineStatus::Success, std::move(msg)));

	return output;
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
