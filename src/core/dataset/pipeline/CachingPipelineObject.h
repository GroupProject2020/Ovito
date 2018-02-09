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

#pragma once


#include <core/Core.h>
#include <core/utilities/concurrent/SharedFuture.h>
#include <core/dataset/pipeline/PipelineCache.h>
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
	/// \param time Specifies at which animation time the pipeline should be evaluated.
	virtual SharedFuture<PipelineFlowState> evaluate(TimePoint time) override;

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
	/// \param time Specifies at which animation time the pipeline should be evaluated.
	virtual Future<PipelineFlowState> evaluateInternal(TimePoint time) = 0;

	/// \brief Decides whether a preliminary viewport update is performed after this pipeline object has been 
	///        evaluated but before the rest of the pipeline is complete.
	virtual bool performPreliminaryUpdateAfterEvaluation() { return true; }

private:

	/// Cache for the data output of this pipeline object.
	PipelineCache _pipelineCache;

	/// A weak reference to the future results of an ongoing evaluation of the pipeline.
	WeakSharedFuture<PipelineFlowState> _inProgressEvalFuture;

	/// The animation time at which the current evaluation is in progress.
	TimePoint _inProgressEvalTime = TimeNegativeInfinity();
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace


