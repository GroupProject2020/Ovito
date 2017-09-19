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
* Marks the state in the data cache as invalid.
******************************************************************************/
void CachingPipelineObject::invalidatePipelineCache()
{
//	qDebug() << "CachingPipelineObject::invalidatePipelineCache() this=" << this;
	_outputCache.setStateValidity(TimeInterval::empty());
	_inProgressEvalFuture.reset();
	_inProgressEvalTime = TimeNegativeInfinity();
}

/******************************************************************************
* Replaces the state in the data cache.
******************************************************************************/
void CachingPipelineObject::setPipelineCache(PipelineFlowState state)
{
	_outputCache = std::move(state);
	if(performPreliminaryUpdateAfterEvaluation()) {
//		qDebug() << "CachingPipelineObject::setPipelineCache: Notifying dependents of pipeline object" << this << " about preliminary state";
		notifyDependents(ReferenceEvent::PreliminaryStateAvailable);
	}
}

/******************************************************************************
* Asks the object for the result of the data pipeline.
******************************************************************************/
SharedFuture<PipelineFlowState> CachingPipelineObject::evaluate(TimePoint time)
{
//	qDebug() << "CachingPipelineObject::evaluate: current cache:" << _outputCache.stateValidity() << "this=" << this << "time=" << time;

	// Check if we can immediately serve the request from the internal cache.
	if(_outputCache.stateValidity().contains(time))
		return _outputCache;

	// Check if there is already an evaluation in progress whose shared future we can return to the caller.
	if(_inProgressEvalTime == time) {
		SharedFuture<PipelineFlowState> sharedFuture = _inProgressEvalFuture.lock();
		if(sharedFuture.isValid() && !sharedFuture.isCanceled()) {
//			qDebug() << "CachingPipelineObject::evaluate: returning existing future (is finished=" << sharedFuture.isFinished() << ")";
			return sharedFuture;
		}
	}

	// Let the subclass perform the actual pipeline evaluation.
	Future<PipelineFlowState> stateFuture = evaluateInternal(time);

	if(time == dataset()->animationSettings()->time()) {
		// Cache the results before returning them to the caller.
		stateFuture = stateFuture.then(executor(), [this](PipelineFlowState&& state) {
//			qDebug() << "CachingPipelineObject::evaluate: Filling cache of pipeline object" << this << "; new validity:" << state.stateValidity();
			setPipelineCache(state);
			return std::move(state);
		});
	}

	// Keep a weak reference to the future to be able to serve several simultaneous requests.
	SharedFuture<PipelineFlowState> sharedFuture(std::move(stateFuture));
	_inProgressEvalFuture = sharedFuture;
	_inProgressEvalTime = time;

//	qDebug() << "CachingPipelineObject::evaluate: returning promise:" << sharedFuture.sharedState().get() << "this=" << this << "time=" << _inProgressEvalTime;

	return sharedFuture;
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
