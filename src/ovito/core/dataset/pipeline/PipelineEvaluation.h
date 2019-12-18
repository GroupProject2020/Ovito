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

#pragma once


#include <ovito/core/Core.h>
#include <ovito/core/dataset/animation/TimeInterval.h>
#include <ovito/core/dataset/pipeline/PipelineFlowState.h>
#include <ovito/core/utilities/concurrent/SharedFuture.h>
#include <ovito/core/utilities/concurrent/ThreadSafeTask.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Scene)

/**
 * \brief This class holds the parameters for an evaluation request of a data pipeline.
 */
class OVITO_CORE_EXPORT PipelineEvaluationRequest
{
public:

	/// Constructs a pipeline evaluation object for the given animation time.
	explicit PipelineEvaluationRequest(TimePoint time = 0, bool breakOnError = false) : _time(time), _breakOnError(breakOnError) {}

	/// Constructs a pipeline evaluation object for the given animation time and inherits all other settings from another request object.
	explicit PipelineEvaluationRequest(TimePoint time, const PipelineEvaluationRequest& other) : _time(time), _breakOnError(other.breakOnError()) {}

	/// Returns the animation time at which the pipeline is being evaluated.
	TimePoint time() const { return _time; }

	/// Returns whether the pipeline system should stop the evaluation as soon as a first error occurs in one of the modifiers.
	bool breakOnError() const { return _breakOnError; }

private:

	/// The animation time at which the pipeline is being evaluated.
	TimePoint _time = 0;

	/// Makes the pipeline system stop the evaluation as soon as a first error occurs in one of the modifiers.
	bool _breakOnError = false;
};

/**
 * \brief This helper class manages the evaluation of a PipelineSceneNode.
 */
class OVITO_CORE_EXPORT PipelineEvaluationFuture : public SharedFuture<PipelineFlowState>
{
public:

	/// Constructs a pipeline evaluation object for the given animation time.
	explicit PipelineEvaluationFuture(TimePoint time = 0, bool breakOnError = false) : _request(time, breakOnError) {}

	/// Resets the state of the pipeline evaluation.
	void reset(TimePoint time = 0);

	/// Starts executing the pipeline evaluation.
	void execute(PipelineSceneNode* pipeline, bool generateVisualElements = false);

	/// Returns the animation time at which the pipeline is being evaluated.
	TimePoint time() const { return _request.time(); }

	/// Returns the pipeline that is being evaluated.
	const QPointer<PipelineSceneNode>& pipeline() const { return _pipeline; }

private:

	/// The request that triggered the pipeline evaluation.
	PipelineEvaluationRequest _request;

	/// The pipeline that is currently being evaluated.
	QPointer<PipelineSceneNode> _pipeline;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
