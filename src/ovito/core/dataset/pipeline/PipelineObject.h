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

#pragma once


#include <ovito/core/Core.h>
#include <ovito/core/oo/RefTarget.h>
#include <ovito/core/utilities/concurrent/Future.h>
#include <ovito/core/dataset/pipeline/PipelineFlowState.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Scene)

/**
 * \brief This is the base class for objects that constitute a data pipeline.
 */
class OVITO_CORE_EXPORT PipelineObject : public RefTarget
{
	Q_OBJECT
	OVITO_CLASS(PipelineObject)

public:

	/// \brief Constructor.
	explicit PipelineObject(DataSet* dataset);

	/// \brief Determines the time interval over which a computed pipeline state will remain valid.
	virtual TimeInterval validityInterval(const PipelineEvaluationRequest& request) const { return TimeInterval::infinite(); }

	/// \brief Asks the pipeline stage to compute the results.
	virtual SharedFuture<PipelineFlowState> evaluate(const PipelineEvaluationRequest& request) = 0;

	/// \brief Asks the pipeline stage to compute the preliminary results in a synchronous fashion.
	virtual PipelineFlowState evaluateSynchronous(TimePoint time) { return {}; }

	/// \brief Returns a list of pipeline nodes that have this object in their pipeline.
	/// \param onlyScenePipelines If true, pipelines which are currently not part of the scene are ignored.
	QSet<PipelineSceneNode*> pipelines(bool onlyScenePipelines) const;

	/// \brief Determines whether the data pipeline branches above this pipeline object,
	///        i.e. whether this pipeline object has multiple dependents, all using this pipeline
	///        object as input.
	///
	/// \param onlyScenePipelines If true, branches to pipelines which are currently not part of the scene are ignored.
	bool isPipelineBranch(bool onlyScenePipelines) const;

	/// \brief Sets the current status of the pipeline object.
	void setStatus(const PipelineStatus& status);

	/// \brief Returns the current status of the pipeline object.
	virtual PipelineStatus status() const { return _status; }

	/// \brief Returns the number of animation frames this pipeline object can provide.
	virtual int numberOfSourceFrames() const { return 1; }

	/// \brief Given an animation time, computes the source frame to show.
	virtual int animationTimeToSourceFrame(TimePoint time) const;

	/// \brief Given a source frame index, returns the animation time at which it is shown.
	virtual TimePoint sourceFrameToAnimationTime(int frame) const;

	/// \brief Returns the human-readable labels associated with the animation frames (e.g. the simulation timestep numbers).
	virtual QMap<int, QString> animationFrameLabels() const { return {}; }

	/// Returns the data collection that is managed by this object (if it is a data source).
	/// The returned data collection will be displayed under the data source in the pipeline editor.
	virtual DataCollection* getSourceDataCollection() const { return nullptr; }

private:

	/// The current status of this pipeline object.
	PipelineStatus _status;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
