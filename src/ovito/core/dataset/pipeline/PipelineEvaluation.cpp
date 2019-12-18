////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2019 Alexander Stukowski
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
#include <ovito/core/dataset/pipeline/PipelineEvaluation.h>
#include <ovito/core/dataset/scene/PipelineSceneNode.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Scene)

/******************************************************************************
* Starts executing the pipeline evaluation.
******************************************************************************/
void PipelineEvaluationFuture::execute(PipelineSceneNode* pipeline, bool generateVisualElements)
{
    // The pipeline evaluation can not be started again while still in progress.
    OVITO_ASSERT(!isValid() || isFinished());
    // Need a valid pipeline as input.
    OVITO_ASSERT(pipeline != nullptr);

    // Let the pipeline do the heavy work.
    _pipeline = pipeline;
    if(!generateVisualElements)
        static_cast<SharedFuture<PipelineFlowState>&>(*this) = pipeline->evaluatePipeline(_request);
    else
        static_cast<SharedFuture<PipelineFlowState>&>(*this) = pipeline->evaluateRenderingPipeline(_request);
}

/******************************************************************************
* Starts executing the pipeline evaluation.
******************************************************************************/
void PipelineEvaluationFuture::reset(TimePoint time)
{
    SharedFuture<PipelineFlowState>::reset();
    _request = PipelineEvaluationRequest(time);
    _pipeline.clear();
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
