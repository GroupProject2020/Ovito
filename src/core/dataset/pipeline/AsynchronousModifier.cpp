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
* Create a new modifier application that refers to this modifier instance.
******************************************************************************/
OORef<ModifierApplication> AsynchronousModifier::createModifierApplication()
{
	OORef<ModifierApplication> modApp = new AsynchronousModifierApplication(dataset());
	modApp->setModifier(this);
	return modApp;
}

/******************************************************************************
* Asks the object for the result of the data pipeline.
******************************************************************************/
Future<PipelineFlowState> AsynchronousModifier::evaluate(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input)
{
	// Let the subclass create the computation engine based on the input data.
	Future<ComputeEnginePtr> engineFuture = createEngine(time, modApp, input);
	return engineFuture.then(executor(), [this,time,input = input,modApp = QPointer<ModifierApplication>(modApp)](const ComputeEnginePtr& engine) mutable {
//			qDebug() << "AsynchronousModifier::evaluate: create compute engine" << engine.get();
			
			// Execute the engine in a worker thread.
			// Collect results from the engine in the UI thread once it has finished running.
			return dataset()->container()->taskManager().runTaskAsync(engine)
				.then(executor(), [this,time,modApp,input = std::move(input)](const ComputeEngineResultsPtr& results) mutable {
					if(modApp && modApp->modifier() == this) {
//						qDebug() << "AsynchronousModifier::evaluate: compute engine completed:" << results.get() << "modifier=" << this;
						
						// Keep a copy of the results in the ModifierApplication for later.
						if(AsynchronousModifierApplication* asyncModApp = dynamic_object_cast<AsynchronousModifierApplication>(modApp.data()))
							asyncModApp->setLastComputeResults(results);

						UndoSuspender noUndo(this);

						// Apply the computed results to the input data.
						return results->apply(time, modApp, std::move(input));
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
		if(asyncModApp->lastComputeResults())
			return asyncModApp->lastComputeResults()->apply(time, modApp, input);
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
	// For future use.
	stream.endChunk();
}

/******************************************************************************
* Loads the class' contents from the given stream.
******************************************************************************/
void AsynchronousModifier::loadFromStream(ObjectLoadStream& stream)
{
	Modifier::loadFromStream(stream);
	stream.expectChunkRange(0, 2);
	// For future use.
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