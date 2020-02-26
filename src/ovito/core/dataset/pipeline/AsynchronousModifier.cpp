////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2020 Alexander Stukowski
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

#include <ovito/core/Core.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/dataset/DataSetContainer.h>
#include <ovito/core/dataset/pipeline/AsynchronousModifierApplication.h>
#include "AsynchronousModifier.h"

#ifdef Q_OS_LINUX
	#include <malloc.h>
#endif

namespace Ovito {

IMPLEMENT_OVITO_CLASS(AsynchronousModifier);

// Export this class template specialization from the DLL under Windows.
template class Future<AsynchronousModifier::ComputeEnginePtr>;

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
AsynchronousModifier::AsynchronousModifier(DataSet* dataset) : Modifier(dataset)
{
}

/******************************************************************************
* Asks the object for the result of the data pipeline.
******************************************************************************/
Future<PipelineFlowState> AsynchronousModifier::evaluate(const PipelineEvaluationRequest& request, ModifierApplication* modApp, const PipelineFlowState& input)
{
	// Check if there are existing computation results stored in the ModifierApplication that can be re-used.
	if(AsynchronousModifierApplication* asyncModApp = dynamic_object_cast<AsynchronousModifierApplication>(modApp)) {
		const AsynchronousModifier::ComputeEnginePtr& lastResults = asyncModApp->lastComputeResults();
		if(lastResults && lastResults->validityInterval().contains(request.time())) {
			// Re-use the computation results and apply them to the input data.
			UndoSuspender noUndo(this);
			PipelineFlowState output = input;
			lastResults->emitResults(request.time(), modApp, output);
			output.intersectStateValidity(lastResults->validityInterval());
			return output;
		}
	}

	// Let the subclass create the computation engine based on the input data.
	return createEngine(request, modApp, input)
		.then(executor(), [this, time = request.time(), input = input, modApp = QPointer<ModifierApplication>(modApp)](ComputeEnginePtr engine) mutable {
			// Execute the engine in a worker thread.
			// Collect results from the engine in the UI thread once it has finished running.
			return dataset()->taskManager().runTaskAsync(engine)
				.then(executor(), [this, time, modApp, state = std::move(input), engine]() mutable {
					if(modApp && modApp->modifier() == this) {

						// Keep a copy of the results in the ModifierApplication for later.
						if(AsynchronousModifierApplication* asyncModApp = dynamic_object_cast<AsynchronousModifierApplication>(modApp.data())) {
							TimeInterval iv = engine->validityInterval();
							iv.intersect(state.stateValidity());
							engine->setValidityInterval(iv);
							asyncModApp->setLastComputeResults(engine);
						}

						// Apply the computed results to the input data.
						engine->emitResults(time, modApp, state);
						state.intersectStateValidity(engine->validityInterval());
					}
					return std::move(state);
				});
		});
}

/******************************************************************************
* Modifies the input data synchronously.
******************************************************************************/
void AsynchronousModifier::evaluateSynchronous(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state)
{
	// If results are still available from the last pipeline evaluation, apply them to the input data.
	if(AsynchronousModifierApplication* asyncModApp = dynamic_object_cast<AsynchronousModifierApplication>(modApp)) {
		if(const AsynchronousModifier::ComputeEnginePtr& lastResults = asyncModApp->lastComputeResults()) {
			UndoSuspender noUndo(this);
			lastResults->emitResults(time, modApp, state);
			state.intersectStateValidity(lastResults->validityInterval());
		}
	}
	Modifier::evaluateSynchronous(time, modApp, state);
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

#ifdef Q_OS_LINUX
/******************************************************************************
* Destructor.
******************************************************************************/
AsynchronousModifier::ComputeEngine::~ComputeEngine()
{
	// Some compute engines allocate a considerable amount of memory in small chunks,
	// which is sometimes not released back to the OS by the C memory allocator.
	// This call to malloc_trim() will explicitly trigger an attempt to release free memory
	// at the top of the heap.
	::malloc_trim(0);
}
#endif

}	// End of namespace
