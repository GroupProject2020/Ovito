///////////////////////////////////////////////////////////////////////////////
// 
//  Copyright (2018) Alexander Stukowski
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
#include <core/utilities/concurrent/Future.h>
#include <core/dataset/pipeline/PipelineStatus.h>
#include <core/dataset/pipeline/PipelineFlowState.h>
#include <core/dataset/data/TransformedDataObject.h>
#include "TransformingDataVis.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Scene)

IMPLEMENT_OVITO_CLASS(TransformingDataVis);

/******************************************************************************
* Constructor.
******************************************************************************/
TransformingDataVis::TransformingDataVis(DataSet* dataset) : DataVis(dataset)
{
}

/******************************************************************************
* Lets the vis element transform a data object in preparation for rendering.
******************************************************************************/
Future<PipelineFlowState> TransformingDataVis::transformData(TimePoint time, const DataObject* dataObject, PipelineFlowState&& flowState, const PipelineFlowState& cachedState, const PipelineSceneNode* contextNode) 
{
	// Check if the cache state already contains a transformed data object that we have
	// created earlier for the same input object. If yes, we can immediately return it.
	for(const DataObject* o : cachedState.objects()) {
		if(const TransformedDataObject* transformedDataObject = dynamic_object_cast<TransformedDataObject>(o)) {
			if(transformedDataObject->sourceDataObject() == dataObject && transformedDataObject->visElement() == this && transformedDataObject->visElementRevision() == revisionNumber()) {
				flowState.addObject(transformedDataObject);
				return std::move(flowState);
			}
		}
	}
	
	// We don't want to create any undo records while performing the data transformation.
	UndoSuspender noUndo(this);

	// Clear the status of the input unless it is an error.
	if(flowState.status().type() != PipelineStatus::Error) {
		OVITO_ASSERT(flowState.status().type() != PipelineStatus::Pending);
		flowState.setStatus(PipelineStatus());
	}

	// Make a copy of the input state. We might need it later when an error occurs.
	PipelineFlowState inputData = flowState;
			
	Future<PipelineFlowState> future;
	try {
		// Let the transforming vis element do its job.
		future = transformDataImpl(time, dataObject, std::move(flowState), cachedState, contextNode);
	}
	catch(...) {
		future = Future<PipelineFlowState>::createFailed(std::current_exception());
	}

	// Change status to 'in progress' during long-running operations.
	if(!future.isFinished()) {
		if(_activeTransformationsCount++ == 0)
			notifyDependents(ReferenceEvent::ObjectStatusChanged);
		// Reset the pending status after the Future is fulfilled.
		future.finally(executor(), [this]() {
			OVITO_ASSERT(_activeTransformationsCount > 0);
			if(--_activeTransformationsCount == 0)
				notifyDependents(ReferenceEvent::ObjectStatusChanged);
		});
	}
	
	// Post-process the results before returning them to the caller. 
	// Turn any exception that was thrown during modifier evaluation into a valid pipeline state with an error code.
	return future.then_future(executor(), [this, inputData = std::move(inputData)](Future<PipelineFlowState> future) mutable {
		try {
			try {
				PipelineFlowState state = future.result();
				if(inputData.status().type() != PipelineStatus::Error)
					setStatus(state.status());
				else
					setStatus(PipelineStatus());
				return state;
			}
			catch(const Exception&) {
				throw;
			}
			catch(const std::bad_alloc&) {
				throwException(tr("Not enough memory."));
			}
			catch(const std::exception& ex) {
				qWarning() << "WARNING: Visual element" << this << "has thrown a non-standard exception:" << ex.what();
				OVITO_ASSERT(false);
				throwException(tr("Exception: %1").arg(QString::fromLatin1(ex.what())));
			}
		}
		catch(Exception& ex) {
			setStatus(PipelineStatus(PipelineStatus::Error, ex.messages().join(QChar('\n'))));
			ex.prependGeneralMessage(tr("Visual element '%1' reported:").arg(objectTitle()));
			inputData.setStatus(PipelineStatus(PipelineStatus::Error, ex.messages().join(QChar(' '))));
			return std::move(inputData);
		}
		catch(...) {
			OVITO_ASSERT_MSG(false, "TransformingDataVis::transformData()", "Caught an unexpected exception type during data transformation.");
			PipelineStatus status(PipelineStatus::Error, tr("Unknown exception caught during data object transformation '%1'.").arg(objectTitle()));
			setStatus(status);
			inputData.setStatus(status);
			return std::move(inputData);
		}
	});
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
