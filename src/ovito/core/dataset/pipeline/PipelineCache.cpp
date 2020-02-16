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
#include <ovito/core/dataset/pipeline/PipelineCache.h>
#include <ovito/core/dataset/pipeline/CachingPipelineObject.h>
#include <ovito/core/dataset/scene/PipelineSceneNode.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/dataset/data/TransformingDataVis.h>
#include <ovito/core/dataset/data/TransformedDataObject.h>
#include <ovito/core/app/Application.h>
#include <ovito/core/utilities/concurrent/Future.h>

namespace Ovito {

/******************************************************************************
* Constructor.
******************************************************************************/
PipelineCache::PipelineCache(RefTarget* owner, bool includeVisElements) : _ownerObject(owner), _includeVisElements(includeVisElements)
{
}

/******************************************************************************
* Destructor.
******************************************************************************/
PipelineCache::~PipelineCache() // NOLINT
{
}

/******************************************************************************
* Starts a pipeline evaluation or returns a reference to an existing evaluation 
* that is currently in progress. 
******************************************************************************/
SharedFuture<PipelineFlowState> PipelineCache::evaluatePipeline(const PipelineEvaluationRequest& request)
{
	OVITO_ASSERT(!QCoreApplication::instance() || QThread::currentThread() == QCoreApplication::instance()->thread());
	OVITO_ASSERT_MSG(_preparingEvaluation == false, "PipelineCache::evaluatePipeline", "Function is not reentrant.");

	// Update the times for which we should keep computed pipeline outputs.
	if(!_precomputeAllFrames)
		_requestedIntervals = request.cachingIntervals();
	else
		_requestedIntervals.add(TimeInterval::infinite());

	// Check if we can serve the request immediately using the cached state(s).
	for(const PipelineFlowState& state : _cachedStates) {
		if(state.stateValidity().contains(request.time())) {
			startFramePrecomputation();
			return Future<PipelineFlowState>::createImmediateEmplace(state);
		}
	}

	// Check if there already is an evaluation in progress that is compatible with the new request.
	for(const EvaluationInProgress& evaluation : _evaluationsInProgress) {
		if(evaluation.validityInterval.contains(request.time())) {
			SharedFuture<PipelineFlowState> future = evaluation.future.lock();
			if(future.isValid() && !future.isCanceled()) {
				startFramePrecomputation();
				return future;
			}
		}
	}
	
	CachingPipelineObject* pipelineObject = dynamic_object_cast<CachingPipelineObject>(ownerObject());
	PipelineSceneNode* pipeline = !pipelineObject ? dynamic_object_cast<PipelineSceneNode>(ownerObject()) : nullptr;
	OVITO_ASSERT(pipeline != nullptr || pipelineObject != nullptr);
	OVITO_ASSERT(pipeline != nullptr || _includeVisElements == false);

	SharedFuture<PipelineFlowState> future;
	TimeInterval preliminaryValidityInterval;

#ifdef OVITO_DEBUG
	// This flag is set here to detect unexpected calls to invalidate().
	_preparingEvaluation = true; 
#endif

	if(!pipelineObject) {
		// Without a pipeline data source, the results will be an empty data collection.
		if(!pipeline->dataProvider())
			return Future<PipelineFlowState>::createImmediateEmplace(nullptr, PipelineStatus::Success);
		
		preliminaryValidityInterval = pipeline->dataProvider()->validityInterval(request);
		if(!_includeVisElements) {
			// When requesting the pipeline output without the effect of visualization elements, 
			// delegate the evaluation to the head node of the pipeline.
			future = pipeline->dataProvider()->evaluate(request);
		}
		else {
			// When requesting the pipeline output with the effect of visualization elements, 
			// delegate the evaluation to the pipeline's other cache.
			future = pipeline->evaluatePipeline(request);
		}
	}
	else {
		preliminaryValidityInterval = pipelineObject->validityInterval(request);
		future = pipelineObject->evaluateInternal(request);
	}

	// Pre-register the evaluation operation.
	_evaluationsInProgress.push_front({ preliminaryValidityInterval });
	auto evaluation = _evaluationsInProgress.begin();
	OVITO_ASSERT(!evaluation->validityInterval.isEmpty());

	// When requesting the pipeline output with the effect of visualization elements, 
	// let the visualization elements operate on the data collection.
	if(_includeVisElements) {
		future = future.then(ownerObject()->executor(), [this, request, pipeline](const PipelineFlowState& state) {
			// Give every visualization element the opportunity to apply an asynchronous data transformation.
			Future<PipelineFlowState> stateFuture;
			if(state) {
				for(const auto& dataObj : state.data()->objects()) {
					for(DataVis* vis : dataObj->visElements()) {
						if(TransformingDataVis* transformingVis = dynamic_object_cast<TransformingDataVis>(vis)) {
							if(transformingVis->isEnabled()) {
								if(!stateFuture.isValid()) {
									stateFuture = transformingVis->transformData(request, dataObj, PipelineFlowState(state), _cachedTransformedDataObjects);
								}
								else {
									OORef<PipelineSceneNode> pipelineRef{pipeline}; // Used to keep the pipeline object alive.
									stateFuture = stateFuture.then(transformingVis->executor(), [request, dataObj, transformingVis, this, pipeline = std::move(pipelineRef)](PipelineFlowState&& state) {
										return transformingVis->transformData(request, dataObj, std::move(state), _cachedTransformedDataObjects);
									});
								}
							}
						}
					}
				}
			}
			if(!stateFuture.isValid()) {
				_cachedTransformedDataObjects.clear();
				stateFuture = Future<PipelineFlowState>::createImmediate(state);
			}
			else {
				// Cache the transformed data objects created by transforming visualization elements.
				stateFuture = stateFuture.then(ownerObject()->executor(), [this](PipelineFlowState&& state) {
					cacheTransformedDataObjects(state);
					return std::move(state);
				});
			}
			return stateFuture;
		});
	}

	// Store evaluation results in this cache.
	future = future.then(ownerObject()->executor(), [this, pipeline, pipelineObject, evaluation](PipelineFlowState state) {

		// Restrict the validity of the state.
		state.intersectStateValidity(evaluation->validityInterval);

		if(!state.stateValidity().isEmpty()) {
			// Let the cache decide whether the state should be stored or not.
			insertState(state);
			if(pipeline) {
				// Let the pipeline update its list of vis elements based on the new pipeline results.
				if(!_includeVisElements)
					pipeline->updateVisElementList(state);
			}
			else {
				// We also have a new preliminary state. Inform the upstream pipeline about it.
				if(pipelineObject->performPreliminaryUpdateAfterEvaluation() && Application::instance()->guiMode()) {
					if(state.stateValidity().contains(pipelineObject->dataset()->animationSettings()->time())) {
						// Adopt the newly computed state as the current synchronous cache state.
						_synchronousState = state;
						_synchronousState.setStateValidity(TimeInterval::infinite());
						pipelineObject->notifyDependents(ReferenceEvent::PreliminaryStateAvailable);
					}
				}
			}
		}

		// Return state to caller.
		return std::move(state);
	});

	// Keep a weak reference to the future.
	evaluation->future = future;

#ifdef OVITO_DEBUG
	// From now on, it is okay again to call invalidate().
	_preparingEvaluation = false; 
#endif

	// Remove evaluation record from the list of ongoing evaluations once it is finished (successfully or not).
	future.finally(ownerObject()->executor(), [this, evaluation](const TaskPtr&) {
		cleanupEvaluation(evaluation);
	});

	// Start the process of caching the pipeline results for all animation frames.
	startFramePrecomputation();

	OVITO_ASSERT(future.isValid());
	return future;
}

/******************************************************************************
* Removes an evaluation record from the list of evaluations currently in progress.
******************************************************************************/
void PipelineCache::cleanupEvaluation(std::forward_list<EvaluationInProgress>::iterator evaluation)
{
	OVITO_ASSERT(!_evaluationsInProgress.empty());
	OVITO_ASSERT(!QCoreApplication::instance() || QThread::currentThread() == QCoreApplication::instance()->thread());
	for(auto iter = _evaluationsInProgress.before_begin(), next = iter++; next != _evaluationsInProgress.end(); iter = next++) {
		if(next == evaluation) {
			_evaluationsInProgress.erase_after(iter);
			return;
		}
	}
	OVITO_ASSERT(false);
}

/******************************************************************************
* Inserts (or may reject) a pipeline state into the cache. 
******************************************************************************/
void PipelineCache::insertState(const PipelineFlowState& state)
{
	OVITO_ASSERT(!QCoreApplication::instance() || QThread::currentThread() == QCoreApplication::instance()->thread());
	OVITO_ASSERT(!ownerObject()->dataset()->undoStack().isRecording());

	// Keep only cached states that overlap with the previously requested time interval.
	// Throw away existing cached states that do not overlap with the requested time intervals,
	// or which *do* overlap with the newly computed state and are now outdated.
	_cachedStates.erase(std::remove_if(_cachedStates.begin(), _cachedStates.end(), [&](const PipelineFlowState& cachedState) {
		if(cachedState.stateValidity().overlap(state.stateValidity()))
			return true;
		return !std::any_of(_requestedIntervals.cbegin(), _requestedIntervals.cend(), 
			std::bind(&TimeInterval::overlap, cachedState.stateValidity(), std::placeholders::_1));
	}), _cachedStates.end());

	// Decide whether to store the newly computed state in the cache or not. 
	// To keep it, its validity interval must overlap with one of the requested time intervals.
	if(std::any_of(_requestedIntervals.cbegin(), _requestedIntervals.cend(), 
			std::bind(&TimeInterval::overlap, state.stateValidity(), std::placeholders::_1))) {
		_cachedStates.push_back(state);
	}

	ownerObject()->notifyDependents(ReferenceEvent::PipelineCacheUpdated);
}

/******************************************************************************
* Performs a synchronous evaluation of the pipeline yielding a preliminary state.
******************************************************************************/
const PipelineFlowState& PipelineCache::evaluatePipelineSynchronous(TimePoint time)
{
	// First, check if we can serve the request from the asynchronous evaluation cache.
	if(const PipelineFlowState& cachedState = getAt(time)) {
		if(cachedState.stateValidity().contains(ownerObject()->dataset()->animationSettings()->time()))
			_synchronousState = cachedState;
		return cachedState;
	}
	else {
		// Otherwise, try to serve the request from the synchronous evaluation cache.
		if(!_synchronousState.stateValidity().contains(time)) {
			
			// If no cached results are available, re-evaluate the pipeline.
			PipelineSceneNode* pipeline = static_object_cast<PipelineSceneNode>(ownerObject());
			if(pipeline->dataProvider()) {
				// Adopt new state produced by the pipeline if it is not empty. 
				// Otherwise stick with the old state from our own cache.
				UndoSuspender noUndo(pipeline);
				if(PipelineFlowState newPreliminaryState = pipeline->dataProvider()->evaluateSynchronous(time)) {
					_synchronousState = std::move(newPreliminaryState);

					// Add the transformed data objects cached from the last pipeline evaluation.
					if(_synchronousState) {
						for(const auto& obj : _cachedTransformedDataObjects)
							_synchronousState.addObject(obj);
					}
				}
			}
			else {
				_synchronousState.reset();
			}

			// The preliminary state cache is time-independent.
			_synchronousState.setStateValidity(TimeInterval::infinite());
		}
		return _synchronousState;
	}
}

/******************************************************************************
* Performs a synchronous evaluation of a pipeline page yielding a preliminary state.
******************************************************************************/
const PipelineFlowState& PipelineCache::evaluatePipelineStageSynchronous(TimePoint time)
{
	// First, check if we can serve the request from the asynchronous evaluation cache.
	if(const PipelineFlowState& cachedState = getAt(time)) {
		if(cachedState.stateValidity().contains(ownerObject()->dataset()->animationSettings()->time())) {
			_synchronousState = cachedState;
		}
		return cachedState;
	}
	else {
		// Otherwise, try to serve the request from the synchronous evaluation cache.
		if(!_synchronousState.stateValidity().contains(time)) {
			// If no cached results are available, re-evaluate the pipeline.
			// Adopt new state produced by the pipeline if it is not empty. 
			// Otherwise stick with the old state from our own cache.
			CachingPipelineObject* pipelineObject = static_object_cast<CachingPipelineObject>(ownerObject());
			UndoSuspender noUndo(pipelineObject);
			if(PipelineFlowState newState = pipelineObject->evaluateInternalSynchronous(time)) {
				_synchronousState = std::move(newState);
			}

			// The preliminary state cache is time-independent.
			_synchronousState.setStateValidity(TimeInterval::infinite());
		}
	}
	return _synchronousState;
}

/******************************************************************************
* Marks the contents of the cache as outdated and throws away data that is no longer needed.
******************************************************************************/
void PipelineCache::invalidate(TimeInterval keepInterval, bool resetSynchronousCache)
{
	OVITO_ASSERT(!QCoreApplication::instance() || QThread::currentThread() == QCoreApplication::instance()->thread());
	OVITO_ASSERT_MSG(_preparingEvaluation == false, "PipelineCache::invalidate", "Cannot invalidate cache while preparing to evaluate the pipeline.");

	// Interrupt frame precomputation, which might be in progress.
	_precomputeFramesOperation.reset();

	// Reduce the validity of ongoing evaluations.
	for(EvaluationInProgress& evaluation : _evaluationsInProgress) {
		evaluation.validityInterval.intersect(keepInterval);
	}

	// Reduce the validity of the cached states. Throw away states that became completely invalid.
	for(PipelineFlowState& state : _cachedStates) {
		state.intersectStateValidity(keepInterval);
		if(state.stateValidity().isEmpty()) {
			state.reset();
		}
	}

	// Reduce the validity interval of the synchronous state cache.
	_synchronousState.intersectStateValidity(keepInterval);
	if(resetSynchronousCache && _synchronousState.stateValidity().isEmpty())
		_synchronousState.reset();

	if(resetSynchronousCache)
		_cachedTransformedDataObjects.clear();
}

/******************************************************************************
* Special method used by the FileSource class to replace the contents of the pipeline
* cache with a data collection modified by the user.
******************************************************************************/
void PipelineCache::overrideCache(DataCollection* dataCollection)
{
	OVITO_ASSERT(dataCollection != nullptr);

	// Interrupt frame precomputation, which might be in progress.
	_precomputeFramesOperation.reset();

	// Reduce the validity of the cached states to the current animation time. 
	// Throw away states that became completely invalid.
	// Replace the contents of the cache with the given data collection.
	TimeInterval keepInterval(dataCollection->dataset()->animationSettings()->time());
	for(PipelineFlowState& state : _cachedStates) {
		state.intersectStateValidity(keepInterval);
		if(state.stateValidity().isEmpty()) {
			state.reset();
		}
		else {
			state.setData(dataCollection);
		}
	}

	_synchronousState.setData(dataCollection);
}

/******************************************************************************
* Looks up the pipeline state for the given animation time.
******************************************************************************/
const PipelineFlowState& PipelineCache::getAt(TimePoint time) const
{
	for(const PipelineFlowState& state : _cachedStates) {
		if(state.stateValidity().contains(time))
			return state;
	}
	static const PipelineFlowState emptyState;
	return emptyState;
}

/******************************************************************************
* Populates the internal cache with transformed data objects generated by 
* transforming visual elements.
******************************************************************************/
void PipelineCache::cacheTransformedDataObjects(const PipelineFlowState& state)
{
	_cachedTransformedDataObjects.clear();
	if(state.data()) {
		for(const DataObject* o : state.data()->objects()) {
			if(const TransformedDataObject* transformedDataObject = dynamic_object_cast<TransformedDataObject>(o)) {
				_cachedTransformedDataObjects.push_back(transformedDataObject);
			}
		}
	}
}

/******************************************************************************
* Enables or disables the precomputation and caching of all frames of the animation.
******************************************************************************/
void PipelineCache::setPrecomputeAllFrames(bool enable)
{
	if(enable != _precomputeAllFrames) {
		_precomputeAllFrames = enable;
		if(!_precomputeAllFrames) {
			// Interrupt the precomputation process if it is currently in progress.
			_precomputeFramesOperation.reset();
			// Throw away all precomputed data (except current frame) to reduce memory footprint.
			invalidate(TimeInterval(ownerObject()->dataset()->animationSettings()->time()));
		}
	}
}

/******************************************************************************
* Starts the process of caching the pipeline results for all animation frames.
******************************************************************************/
void PipelineCache::startFramePrecomputation()
{
	// Start the animation frame precomputation process if it has been activated.
	if(_precomputeAllFrames && !_precomputeFramesOperation.isValid()) {
		// Create the async operation object that manages the frame precomputation.
		_precomputeFramesOperation = Promise<>::createAsynchronousOperation(ownerObject()->dataset()->taskManager());

		// Determine the number of frames that need to be precomputed.
		PipelineObject* pipelineObject = dynamic_object_cast<PipelineObject>(ownerObject());
		if(!pipelineObject)
			pipelineObject = static_object_cast<PipelineSceneNode>(ownerObject())->dataProvider();
		if(pipelineObject)
			_precomputeFramesOperation.setProgressMaximum(pipelineObject->numberOfSourceFrames());

		// Automatically reset the async operation object and the current frame precomputation when the 
		// task gets canceled by the system.
		_precomputeFramesOperation.finally(ownerObject()->executor(), false, [this](const TaskPtr&) {
			_precomputeFrameFuture.reset();
			_precomputeFramesOperation.reset();
		});

		// Compute the first frame of the trajectory.
		precomputeNextAnimationFrame();
	}
}

/******************************************************************************
* Requests the next frame from the pipeline that needs to be precomputed.
******************************************************************************/
void PipelineCache::precomputeNextAnimationFrame()
{
	OVITO_ASSERT(_precomputeFramesOperation.isValid());
	OVITO_ASSERT(!_precomputeFramesOperation.isCanceled());

	// Determine the total number of animation frames.
	PipelineObject* pipelineObject = dynamic_object_cast<PipelineObject>(ownerObject());
	if(!pipelineObject)
		pipelineObject = static_object_cast<PipelineSceneNode>(ownerObject())->dataProvider();
	int numSourceFrames = pipelineObject ? pipelineObject->numberOfSourceFrames() : 0;

	// Determine what is the next animation frame that needs to be precomputed.
	int nextFrame = 0;
	TimePoint nextFrameTime;
	while(nextFrame < numSourceFrames) {
		nextFrameTime = pipelineObject->sourceFrameToAnimationTime(nextFrame);
		const PipelineFlowState& state = getAt(nextFrameTime);
		if(!state) break;
		do {
			nextFrameTime = pipelineObject->sourceFrameToAnimationTime(++nextFrame);
		}
		while(state.stateValidity().contains(nextFrameTime) && nextFrame < numSourceFrames);
	}
	_precomputeFramesOperation.setProgressValue(nextFrame);
	_precomputeFramesOperation.setProgressText(CachingPipelineObject::tr("Caching trajectory (%1 frames remaining)").arg(numSourceFrames - nextFrame));
	if(nextFrame >= numSourceFrames) {
		// Precomputation of trajectory frames is complete.
		_precomputeFramesOperation.setFinished();
		OVITO_ASSERT(!_precomputeFrameFuture.isValid());
		return;
	}

	// Request the next frame from the input trajectory.
	_precomputeFrameFuture = evaluatePipeline(PipelineEvaluationRequest(nextFrameTime));

	// Wait until input frame is ready.
	_precomputeFrameFuture.finally(ownerObject()->executor(), true, [this](const TaskPtr& task) {
		try {
			// If the pipeline evaluation has been canceled for some reason, we interrupt the precomputation process.
			if(!_precomputeFramesOperation.isValid() || _precomputeFramesOperation.isFinished() || task->isCanceled()) {
				_precomputeFramesOperation.reset();
				OVITO_ASSERT(!_precomputeFrameFuture.isValid());
				return;
			}
			OVITO_ASSERT(_precomputeFrameFuture.isValid());

			// Store the computed frame in the pipeline cache.
			insertState(_precomputeFrameFuture.result());

			// Schedule the pipeline evaluation at the next frame.
			precomputeNextAnimationFrame();
		}
		catch(const Exception&) {
			// In case of an error during pipeline evaluation or the unwrapping calculation, 
			// abort the operation.
			_precomputeFramesOperation.setFinished();
		}
	});
}

}	// End of namespace
