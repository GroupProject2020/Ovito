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
#include <core/dataset/DataSet.h>
#include <core/dataset/pipeline/PipelineObject.h>
#include <core/dataset/pipeline/ModifierApplication.h>
#include <core/dataset/animation/AnimationSettings.h>
#include <core/dataset/scene/PipelineSceneNode.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Scene)

IMPLEMENT_OVITO_CLASS(PipelineObject);

/******************************************************************************
* Constructor.
******************************************************************************/
PipelineObject::PipelineObject(DataSet* dataset) : RefTarget(dataset)
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
			if(modApp->input() == this && modApp->pipelines(onlyScenePipelines).empty() == false)
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
* Sets the current status of the modifier.
******************************************************************************/
void PipelineObject::setStatus(const PipelineStatus& status) 
{
	if(status != _status) {
		_status = status; 
		notifyDependents(ReferenceEvent::ObjectStatusChanged);
	}
}

/******************************************************************************
* Given an animation time, computes the source frame to show.
******************************************************************************/
int PipelineObject::animationTimeToSourceFrame(TimePoint time) const
{
	return dataset()->animationSettings()->timeToFrame(time);
}

/******************************************************************************
* Given a source frame index, returns the animation time at which it is shown.
******************************************************************************/
TimePoint PipelineObject::sourceFrameToAnimationTime(int frame) const
{
	return dataset()->animationSettings()->frameToTime(frame);
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
