////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2018 Alexander Stukowski
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
#include <ovito/core/utilities/concurrent/Future.h>
#include <ovito/core/dataset/pipeline/PipelineStatus.h>
#include <ovito/core/dataset/pipeline/PipelineFlowState.h>
#include <ovito/core/dataset/data/TransformedDataObject.h>
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
Future<PipelineFlowState> TransformingDataVis::transformData(TimePoint time, const DataObject* dataObject, PipelineFlowState&& flowState, const PipelineFlowState& cachedState, const PipelineSceneNode* contextNode, bool breakOnError)
{
	// Check if the cache state already contains a transformed data object that we have
	// created earlier for the same input object. If yes, we can immediately return it.
	if(cachedState.data()) {
		for(const DataObject* o : cachedState.data()->objects()) {
			if(const TransformedDataObject* transformedDataObject = dynamic_object_cast<TransformedDataObject>(o)) {
				if(transformedDataObject->sourceDataObject() == dataObject && transformedDataObject->visElement() == this && transformedDataObject->visElementRevision() == revisionNumber()) {
					PipelineFlowState outputState = flowState;
					outputState.mutableData()->addObject(transformedDataObject);
					return std::move(outputState);
				}
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
	else if(breakOnError) {
		// Skip all following vis transformations once an error has occured along the pipeline.
		return std::move(flowState);
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

	// Post-process the results before returning them to the caller.
	// Turn any exception that was thrown during modifier evaluation into a valid pipeline state with an error code.
	future = future.then_future(executor(), [this, inputData = std::move(inputData)](Future<PipelineFlowState> future) mutable {
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

	// Change status to 'in progress' during long-running operations.
	if(!future.isFinished()) {
		_activeTransformationsCount++;
		setStatus(PipelineStatus::Pending);
		// Reset the pending status after the Future is fulfilled.
		future.finally(executor(), [this]() {
			OVITO_ASSERT(_activeTransformationsCount > 0);
			if(--_activeTransformationsCount == 0 && status().type() == PipelineStatus::Pending)
				setStatus(PipelineStatus::Success);
		});
	}

	return future;
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
