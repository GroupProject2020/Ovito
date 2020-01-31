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
#include <ovito/core/utilities/concurrent/SharedFuture.h>
#include <ovito/core/dataset/pipeline/PipelineCache.h>
#include "PipelineObject.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Scene)

/**
 * \brief A pipeline object that maintains an output data cache.
 */
class OVITO_CORE_EXPORT CachingPipelineObject : public PipelineObject
{
	Q_OBJECT
	OVITO_CLASS(CachingPipelineObject)

public:

	/// \brief Constructor.
	CachingPipelineObject(DataSet* dataset);

	/// \brief Asks the object for the result of the data pipeline.
	virtual SharedFuture<PipelineFlowState> evaluate(const PipelineEvaluationRequest& request) override;

	/// \brief Returns the results of an immediate and preliminary evaluation of the data pipeline.
	virtual PipelineFlowState evaluatePreliminary() override { return _pipelineCache.getStaleContents(); }

	/// \brief Invalidates (and throws away) the cached pipeline state.
	/// \param keepInterval An optional time interval over which the cached data should be retained.
	void invalidatePipelineCache(TimeInterval keepInterval = TimeInterval::empty());

	/// \brief Returns the internal output cache.
	const PipelineCache& pipelineCache() const { return _pipelineCache; }

	/// \brief Returns the internal output cache.
	PipelineCache& pipelineCache() { return _pipelineCache; }

protected:

	/// \brief Asks the object for the result of the data pipeline.
	virtual Future<PipelineFlowState> evaluateInternal(const PipelineEvaluationRequest& request) = 0;

	/// \brief Decides whether a preliminary viewport update is performed after this pipeline object has been
	///        evaluated but before the rest of the pipeline is complete.
	virtual bool performPreliminaryUpdateAfterEvaluation() { return true; }

private:

	/// Cache for the data output of this pipeline object.
	PipelineCache _pipelineCache;

	/// A weak reference to the future results of an ongoing evaluation of the pipeline.
	WeakSharedFuture<PipelineFlowState> _inProgressEvalFuture;

	/// The animation time of the evaluation that is currently in progress.
	TimePoint _inProgressEvalTime = TimeNegativeInfinity();
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
