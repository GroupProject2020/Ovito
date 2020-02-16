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
#include <ovito/core/dataset/pipeline/PipelineObject.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>
#include <ovito/core/dataset/animation/AnimationSettings.h>
#include <ovito/core/dataset/scene/PipelineSceneNode.h>
#include <ovito/core/utilities/concurrent/TaskManager.h>

namespace Ovito {

IMPLEMENT_OVITO_CLASS(PipelineObject);

/******************************************************************************
* Constructor.
******************************************************************************/
PipelineObject::PipelineObject(DataSet* dataset) : ActiveObject(dataset)
{
}

/******************************************************************************
* Returns a list of pipeline nodes that have this object in their pipeline.
******************************************************************************/
QSet<PipelineSceneNode*> PipelineObject::pipelines(bool onlyScenePipelines) const
{
	QSet<PipelineSceneNode*> pipelineList;
	for(RefMaker* dependent : this->dependents()) {
		if(PipelineObject* pobj = dynamic_object_cast<PipelineObject>(dependent)) {
			pipelineList.unite(pobj->pipelines(onlyScenePipelines));
		}
		else if(PipelineSceneNode* pipeline = dynamic_object_cast<PipelineSceneNode>(dependent)) {
            if(pipeline->dataProvider() == this) {
				if(!onlyScenePipelines || pipeline->isInScene())
		    		pipelineList.insert(pipeline);
			}
		}
	}
	return pipelineList;
}

/******************************************************************************
* Determines whether the data pipeline branches above this pipeline object,
* i.e. whether this pipeline object has multiple dependents, all using this pipeline
* object as input.
******************************************************************************/
bool PipelineObject::isPipelineBranch(bool onlyScenePipelines) const
{
	int pipelineCount = 0;
	for(RefMaker* dependent : this->dependents()) {
		if(ModifierApplication* modApp = dynamic_object_cast<ModifierApplication>(dependent)) {
			if(modApp->input() == this && !modApp->pipelines(onlyScenePipelines).empty())
				pipelineCount++;
		}
		else if(PipelineSceneNode* pipeline = dynamic_object_cast<PipelineSceneNode>(dependent)) {
            if(pipeline->dataProvider() == this) {
				if(!onlyScenePipelines || pipeline->isInScene())
		    		pipelineCount++;
			}
		}
	}
	return pipelineCount > 1;
}

/******************************************************************************
* Given an animation time, computes the source frame to show.
******************************************************************************/
int PipelineObject::animationTimeToSourceFrame(TimePoint time) const
{
	OVITO_ASSERT(time != TimeNegativeInfinity());
	OVITO_ASSERT(time != TimePositiveInfinity());
	return dataset()->animationSettings()->timeToFrame(time);
}

/******************************************************************************
* Given a source frame index, returns the animation time at which it is shown.
******************************************************************************/
TimePoint PipelineObject::sourceFrameToAnimationTime(int frame) const
{
	return dataset()->animationSettings()->frameToTime(frame);
}

/******************************************************************************
* Asks the pipeline stage to compute the results for several animation times.
******************************************************************************/
Future<std::vector<PipelineFlowState>> PipelineObject::evaluateMultiple(const PipelineEvaluationRequest& request, std::vector<TimePoint> times)
{
	class MultiEvaluationTask : public Task 
	{
	public:

		/// Constructor.
		MultiEvaluationTask(PipelineObject* pipelineObject, const PipelineEvaluationRequest& request, std::vector<TimePoint>&& animationTimes) : 
			Task(Task::Started, &pipelineObject->dataset()->taskManager()),
			_pipelineObj(pipelineObject),
			_request(request),
			_pipelineStates(animationTimes.size()),
			_animationTimes(std::move(animationTimes)) {}

		/// Creates a future that returns the results of this asynchronous task to the caller.
		Future<std::vector<PipelineFlowState>> future() {
			return Future<std::vector<PipelineFlowState>>::createFromTask(shared_from_this(), _pipelineStates);
		}

		/// Is called when this task gets canceled by the system.
		virtual void cancel() noexcept override {
			_frameFuture.reset(); // Cancel pipeline evaluation that is currently in progress.
			Task::cancel();
			setFinished();
		}

		/// Requests the next frame from the pipeline.
		void go() {
			if(isCanceled()) return;
			if(!_animationTimes.empty()) {
				_request.setTime(_animationTimes.back());
				_animationTimes.pop_back();
				_frameFuture = _pipelineObj->evaluate(_request);
				_frameFuture.finally(_pipelineObj->executor(), true, 
					std::bind(&MultiEvaluationTask::processNextFrame, static_pointer_cast<MultiEvaluationTask>(shared_from_this()), std::placeholders::_1));
			}
			else {
#ifdef OVITO_DEBUG
        		this->_resultSet = true;
#endif	
				setFinished();
			}
		}

		/// Is called by the system when the next frame becomes available.
		void processNextFrame(const TaskPtr& task) {
			try {
				if(!isCanceled() && !task->isCanceled()) {
					OVITO_ASSERT(_frameFuture.isValid());
					_pipelineStates[_animationTimes.size()] = _frameFuture.result();
					_frameFuture.reset();
					go();
				}
				else cancel();
			}
			catch(...) {
				captureException();
				setFinished();
			}
		}

	private:

		/// The results of this asynchronous task.
		std::vector<PipelineFlowState> _pipelineStates;

		// Inputs of this asynchronous task.
		std::vector<TimePoint> _animationTimes;
		PipelineEvaluationRequest _request;
		SharedFuture<PipelineFlowState> _frameFuture;
		PipelineObject* _pipelineObj;
	};

	// Create the asynchronous task object and start fetching the first frame.
	std::shared_ptr<MultiEvaluationTask> task = std::make_shared<MultiEvaluationTask>(this, request, std::move(times));
	task->go();
	return task->future();
}

}	// End of namespace
