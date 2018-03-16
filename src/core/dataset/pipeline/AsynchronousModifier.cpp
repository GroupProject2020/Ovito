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

#include <core/Core.h>
#include <core/dataset/DataSet.h>
#include <core/dataset/DataSetContainer.h>
#include <core/dataset/pipeline/AsynchronousModifierApplication.h>
#include "AsynchronousModifier.h"

#ifdef Q_OS_LINUX
	#include <malloc.h>
#endif

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Scene)

IMPLEMENT_OVITO_CLASS(AsynchronousModifier);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
AsynchronousModifier::AsynchronousModifier(DataSet* dataset) : Modifier(dataset)
{
}

/******************************************************************************
* Asks the object for the result of the data pipeline.
******************************************************************************/
Future<PipelineFlowState> AsynchronousModifier::evaluate(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input)
{
	// Check if there are existing computation results stored in the ModifierApplication that can be re-used.
	if(AsynchronousModifierApplication* asyncModApp = dynamic_object_cast<AsynchronousModifierApplication>(modApp)) {
		const AsynchronousModifier::ComputeEngineResultsPtr& lastResults = asyncModApp->lastComputeResults();
		if(lastResults && lastResults->validityInterval().contains(time)) {
			// Re-use the computation results and apply them to the input data.
			UndoSuspender noUndo(this);
			PipelineFlowState resultState = lastResults->apply(time, modApp, std::move(input));
			resultState.mutableStateValidity().intersect(lastResults->validityInterval());
			return resultState;
		}
	}

	// Let the subclass create the computation engine based on the input data.
	Future<ComputeEnginePtr> engineFuture = createEngine(time, modApp, input);
	return engineFuture.then(executor(), [this, time, input = input, modApp = QPointer<ModifierApplication>(modApp)](const ComputeEnginePtr& engine) mutable {
			
			// Execute the engine in a worker thread.
			// Collect results from the engine in the UI thread once it has finished running.
			return dataset()->container()->taskManager().runTaskAsync(engine)
				.then(executor(), [this, time, modApp, input = std::move(input)](const ComputeEngineResultsPtr& results) mutable {
					if(modApp && modApp->modifier() == this) {
						
						// Keep a copy of the results in the ModifierApplication for later.
						if(AsynchronousModifierApplication* asyncModApp = dynamic_object_cast<AsynchronousModifierApplication>(modApp.data())) {
							TimeInterval iv = results->validityInterval();
							iv.intersect(input.stateValidity());
							results->setValidityInterval(iv);
							asyncModApp->setLastComputeResults(results);
						}

						UndoSuspender noUndo(this);

						// Apply the computed results to the input data.
						PipelineFlowState resultState = results->apply(time, modApp, std::move(input));
						resultState.mutableStateValidity().intersect(results->validityInterval());
						return resultState;
					}
					else return std::move(input);
				});
		});
}

/******************************************************************************
* Modifies the input data in an immediate, preliminary way.
******************************************************************************/
PipelineFlowState AsynchronousModifier::evaluatePreliminary(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input)
{
	// If results are still available from the last pipeline evaluation, apply them to the input data.
	if(AsynchronousModifierApplication* asyncModApp = dynamic_object_cast<AsynchronousModifierApplication>(modApp)) {
		if(const AsynchronousModifier::ComputeEngineResultsPtr& lastResults = asyncModApp->lastComputeResults()) {
			PipelineFlowState resultState = lastResults->apply(time, modApp, input);
			resultState.mutableStateValidity().intersect(lastResults->validityInterval());
			return resultState;
		}
	}
	return Modifier::evaluatePreliminary(time, modApp, input);
}

/******************************************************************************
* Saves the class' contents to the given stream.
******************************************************************************/
void AsynchronousModifier::saveToStream(ObjectSaveStream& stream, bool excludeRecomputableData)
{
	Modifier::saveToStream(stream, excludeRecomputableData);
	stream.beginChunk(0x02);
	// Chunk reserved for future use.
	stream.endChunk();
}

/******************************************************************************
* Loads the class' contents from the given stream.
******************************************************************************/
void AsynchronousModifier::loadFromStream(ObjectLoadStream& stream)
{
	Modifier::loadFromStream(stream);
	stream.expectChunk(0x02);
	stream.closeChunk();
}

/******************************************************************************
* Destructor of compute engine.
******************************************************************************/
AsynchronousModifier::ComputeEngine::~ComputeEngine()
{
#ifdef Q_OS_LINUX
	// Some compute engines allocate a considerable amount of memory in small chunks,
	// which is sometimes not released back to the OS by the C memory allocator.
	// This call to malloc_trim() will explicitly trigger an attempt to release free memory
	// at the top of the heap.
	::malloc_trim(0);
#endif
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace