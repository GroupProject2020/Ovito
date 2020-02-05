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
#include <ovito/core/dataset/animation/AnimationSettings.h>
#include <ovito/core/dataset/pipeline/CachingPipelineObject.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Scene)

IMPLEMENT_OVITO_CLASS(CachingPipelineObject);

/******************************************************************************
* Constructor.
******************************************************************************/
CachingPipelineObject::CachingPipelineObject(DataSet* dataset) : PipelineObject(dataset)
{
}

/******************************************************************************
* Determines the time interval over which a computed pipeline state will remain valid.
******************************************************************************/
TimeInterval CachingPipelineObject::validityInterval(const PipelineEvaluationRequest& request) const
{
	TimeInterval iv = PipelineObject::validityInterval(request);

	// If the requested frame is available in the cache, restrict the returned validity interval to 
	// the validity interval of the cached state. Otherwise assume that a new pipeline computation 
	// will be performed and let the sub-class determine the actual validity interval.
	const PipelineFlowState& state = pipelineCache().getAt(request.time());
	if(state.stateValidity().contains(request.time()))
		iv.intersect(state.stateValidity());
	
	return iv;
}

/******************************************************************************
* Asks the object for the result of the data pipeline.
******************************************************************************/
SharedFuture<PipelineFlowState> CachingPipelineObject::evaluate(const PipelineEvaluationRequest& request)
{
	return pipelineCache().evaluatePipelineStage(request, this);
}

/******************************************************************************
* Returns the results of an immediate and preliminary evaluation of the data pipeline.
******************************************************************************/
PipelineFlowState CachingPipelineObject::evaluateSynchronous()
{
	TimePoint time = dataset()->animationSettings()->time();
	return pipelineCache().evaluatePipelineStageSynchronous(this, time);
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
