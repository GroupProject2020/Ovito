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

namespace Ovito {

/**
 * \brief A pipeline object that maintains an output data cache.
 */
class OVITO_CORE_EXPORT CachingPipelineObject : public PipelineObject
{
	Q_OBJECT
	OVITO_CLASS(CachingPipelineObject)

public:

	/// Constructor.
	CachingPipelineObject(DataSet* dataset);

	/// Determines the time interval over which a computed pipeline state will remain valid.
	virtual TimeInterval validityInterval(const PipelineEvaluationRequest& request) const override;

	/// Asks the object for the result of the data pipeline.
	virtual SharedFuture<PipelineFlowState> evaluate(const PipelineEvaluationRequest& request) override;

	/// Asks the pipeline stage to compute the preliminary results in a synchronous fashion.
	virtual PipelineFlowState evaluateSynchronous(TimePoint time) override;

	/// Rescales the times of all animation keys from the old animation interval to the new interval.
	virtual void rescaleTime(const TimeInterval& oldAnimationInterval, const TimeInterval& newAnimationInterval) override;

	/// Returns the internal output cache.
	const PipelineCache& pipelineCache() const { return _pipelineCache; }

	/// Returns the internal output cache.
	PipelineCache& pipelineCache() { return _pipelineCache; }

protected:

	/// Is called when the value of a non-animatable property field of this RefMaker has changed.
	virtual void propertyChanged(const PropertyFieldDescriptor& field) override;

	/// Loads the class' contents from an input stream.
	virtual void loadFromStream(ObjectLoadStream& stream) override;

	/// Asks the object for the result of the data pipeline.
	virtual Future<PipelineFlowState> evaluateInternal(const PipelineEvaluationRequest& request) = 0;

	/// Lets the pipeline stage compute a preliminary result in a synchronous fashion.
	virtual PipelineFlowState evaluateInternalSynchronous(TimePoint time) { 
		return PipelineFlowState(getSourceDataCollection(), status()); 
	}

	/// Decides whether a preliminary viewport update is performed after this pipeline object has been
	/// evaluated but before the rest of the pipeline is complete.
	virtual bool performPreliminaryUpdateAfterEvaluation() { return true; }

private:

	/// Activates the precomputation of the pipeline results for all animation frames.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(bool, pipelineTrajectoryCachingEnabled, setPipelineTrajectoryCachingEnabled, PROPERTY_FIELD_NO_UNDO | PROPERTY_FIELD_NO_CHANGE_MESSAGE);

	/// Cache for the data output of this pipeline stage.
	PipelineCache _pipelineCache;

	friend class PipelineCache;
};

}	// End of namespace
