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
	virtual PipelineFlowState evaluatePreliminary() override { return _outputCache; }

	/// \brief Marks the state in the data cache as invalid.
	void invalidatePipelineCache();

	/// \brief Returns the state currently stored input the output cache.
	const PipelineFlowState& getPipelineCache() const { return _outputCache; }

	/// \brief Replaces the state in the data cache.
	void setPipelineCache(PipelineFlowState state);
		
protected:

	/// \brief Asks the object for the result of the data pipeline.
	/// \param time Specifies at which animation time the pipeline should be evaluated.
	virtual Future<PipelineFlowState> evaluateInternal(TimePoint time) = 0;

	/// \brief Decides whether a preliminary viewport update is performed after this pipeline object has been 
	///        evaluated but before the rest of the pipeline is complete.
	virtual bool performPreliminaryUpdateAfterEvaluation() { return true; }

private:

	/// Cache for the data output of this pipeline object.
	PipelineFlowState _outputCache;

	/// A weak reference to the future results of an ongoing evaluation of the pipeline.
	WeakSharedFuture<PipelineFlowState> _inProgressEvalFuture;

	/// The animation time at which the current evaluation is in progress.
	TimePoint _inProgressEvalTime = TimeNegativeInfinity();
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace


