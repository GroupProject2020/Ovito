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

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Scene)

/******************************************************************************
* Constructor.
******************************************************************************/
PipelineCache::PipelineCache() // NOLINT
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
SharedFuture<PipelineFlowState> PipelineCache::evaluatePipeline(const PipelineEvaluationRequest& request, CachingPipelineObject* pipelineObject, PipelineSceneNode* pipeline, bool includeVisElements)
{
	OVITO_ASSERT(QThread::currentThread() == QCoreApplication::instance()->thread());
	OVITO_ASSERT(pipeline != nullptr || pipelineObject != nullptr);
	OVITO_ASSERT(pipeline != nullptr || includeVisElements == false);
	OVITO_ASSERT_MSG(_preparingEvaluation == false, "PipelineCache::evaluatePipeline", "Function is not reentrant.");

	// Update the times for which we should keep computed pipeline outputs.
	_requestedIntervals = request.cachingIntervals();

	// Check if we can serve the request immediately using the cached state(s).
	for(const PipelineFlowState& state : _cachedStates) {
		if(state.stateValidity().contains(request.time())) {
			return Future<PipelineFlowState>::createImmediateEmplace(state);
		}
	}

	// Check if there already is an evaluation in progress that is compatible with the new request.
	for(const EvaluationInProgress& evaluation : _evaluationsInProgress) {
		if(evaluation.validityInterval.contains(request.time())) {
			SharedFuture<PipelineFlowState> future = evaluation.future.lock();
			if(future.isValid() && !future.isCanceled()) {
				return future;
			}
		}
	}
	
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
		if(!includeVisElements) {
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
	if(includeVisElements) {
		future = future.then(pipeline->executor(), [this, request, pipeline](const PipelineFlowState& state) {
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
				stateFuture = stateFuture.then(pipeline->executor(), [this](PipelineFlowState&& state) {
					cacheTransformedDataObjects(state);
					return std::move(state);
				});
			}
			return stateFuture;
		});
	}

	// Store evaluation results in this cache.
	future = future.then(pipeline ? pipeline->executor() : pipelineObject->executor(), [this, pipeline, pipelineObject, evaluation, includeVisElements](PipelineFlowState state) {
		// The pipeline should never return a state without proper validity interval.
		//OVITO_ASSERT(!state.stateValidity().isEmpty());
		//OVITO_ASSERT(!evaluation->validityInterval.isEmpty());

		// Restrict the validity of the state.
		state.intersectStateValidity(evaluation->validityInterval);

		// Let the cache decide whether the state should be stored or not.
		if(!state.stateValidity().isEmpty()) {
			if(pipeline) {
				insertState(state, pipeline);

				// Let the pipeline update its list of vis elements based on the new pipeline results.
				if(!includeVisElements)
					pipeline->updateVisElementList(state);
			}
			else {
				insertState(state, pipelineObject);

				// We also have a new preliminary state. Inform the upstream pipeline about it.
				if(pipelineObject->performPreliminaryUpdateAfterEvaluation() && Application::instance()->guiMode()) {
					if(state.stateValidity().contains(pipelineObject->dataset()->animationSettings()->time())) {
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
	future.finally(pipeline ? pipeline->executor() : pipelineObject->executor(), [this, evaluation]() {
		cleanupEvaluation(evaluation);
	});

	OVITO_ASSERT(future.isValid());
	return future;
}

/******************************************************************************
* Removes an evaluation record from the list of evaluations currently in progress.
******************************************************************************/
void PipelineCache::cleanupEvaluation(std::forward_list<EvaluationInProgress>::iterator evaluation)
{
	OVITO_ASSERT(!_evaluationsInProgress.empty());
	OVITO_ASSERT(QThread::currentThread() == QCoreApplication::instance()->thread());
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
void PipelineCache::insertState(const PipelineFlowState& state, RefTarget* ownerObject)
{
	OVITO_ASSERT(QThread::currentThread() == QCoreApplication::instance()->thread());
	OVITO_ASSERT(ownerObject != nullptr);
	OVITO_ASSERT(!ownerObject->dataset()->undoStack().isRecording());

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

	ownerObject->notifyDependents(ReferenceEvent::PipelineCacheUpdated);
}

/******************************************************************************
* Performs a synchronous evaluation of the pipeline yielding a preliminary state.
******************************************************************************/
const PipelineFlowState& PipelineCache::evaluatePipelineSynchronous(PipelineSceneNode* pipeline, TimePoint time)
{
	// First, check if we can serve the request from the asynchronous evaluation cache.
	if(const PipelineFlowState& cachedState = getAt(time)) {
		if(cachedState.stateValidity().contains(pipeline->dataset()->animationSettings()->time()))
			_synchronousState = cachedState;
		return cachedState;
	}
	else {
		// Otherwise, try to serve the request from the synchronous evaluation cache.
		if(!_synchronousState.stateValidity().contains(time)) {
			
			// If no cached results are available, re-evaluate the pipeline.
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
const PipelineFlowState& PipelineCache::evaluatePipelineStageSynchronous(CachingPipelineObject* pipelineObject, TimePoint time)
{
	// First, check if we can serve the request from the asynchronous evaluation cache.
	if(const PipelineFlowState& cachedState = getAt(time)) {
		if(cachedState.stateValidity().contains(pipelineObject->dataset()->animationSettings()->time())) {
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
	OVITO_ASSERT(QThread::currentThread() == QCoreApplication::instance()->thread());
	OVITO_ASSERT_MSG(_preparingEvaluation == false, "PipelineCache::invalidate", "Cannot invalidate cache while preparing to evaluate the pipeline.");

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

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
