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
#include <core/dataset/animation/AnimationSettings.h>
#include <core/dataset/pipeline/CachingPipelineObject.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Scene)

IMPLEMENT_OVITO_CLASS(CachingPipelineObject);

/******************************************************************************
* Constructor.
******************************************************************************/
CachingPipelineObject::CachingPipelineObject(DataSet* dataset) : PipelineObject(dataset)
{
}

/******************************************************************************
* Throws away the cached pipeline state.
******************************************************************************/
void CachingPipelineObject::invalidatePipelineCache(TimeInterval keepInterval)
{
	// Reduce the cache validity to the interval to be kept.
	_pipelineCache.invalidate(false, keepInterval);

	// Abort any pipeline evaluation currently in progress unless it 
	// falls inside the time interval that should be kept.
	if(!keepInterval.contains(_inProgressEvalTime)) {
		_inProgressEvalFuture.reset();
		_inProgressEvalTime = TimeNegativeInfinity();
	}
}

/******************************************************************************
* Asks the object for the result of the data pipeline.
******************************************************************************/
SharedFuture<PipelineFlowState> CachingPipelineObject::evaluate(TimePoint time, bool breakOnError)
{
	// Check if we can immediately serve the request from the internal cache.
	if(_pipelineCache.contains(time))
		return _pipelineCache.getAt(time);

	// Check if there is already an evaluation in progress whose shared future we can return to the caller.
	if(_inProgressEvalTime == time) {
		SharedFuture<PipelineFlowState> sharedFuture = _inProgressEvalFuture.lock();
		if(sharedFuture.isValid() && !sharedFuture.isCanceled()) {
			return sharedFuture;
		}
	}

	// Let the subclass perform the actual pipeline evaluation.
	Future<PipelineFlowState> stateFuture = evaluateInternal(time, breakOnError);

	// Cache the results in our local pipeline cache.
	if(_pipelineCache.insert(stateFuture, time, this)) {
		// If the cache was updated, we also have a new preliminary state.
		// Inform the pipeline about it.
		if(performPreliminaryUpdateAfterEvaluation() && time == dataset()->animationSettings()->time()) {
			stateFuture = stateFuture.then(executor(), [this](PipelineFlowState&& state) {
				notifyDependents(ReferenceEvent::PreliminaryStateAvailable);
				return std::move(state);
			});
		}
	}
	OVITO_ASSERT(stateFuture.isValid());

	// Keep a weak reference to the future to be able to serve several simultaneous requests.
	SharedFuture<PipelineFlowState> sharedFuture(std::move(stateFuture));
	_inProgressEvalFuture = sharedFuture;
	_inProgressEvalTime = time;

	return sharedFuture;
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
