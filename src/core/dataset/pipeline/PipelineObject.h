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
#include <core/oo/RefTarget.h>
#include <core/utilities/concurrent/Future.h>
#include <core/dataset/pipeline/PipelineFlowState.h>

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
	PipelineObject(DataSet* dataset);

	/// \brief Asks the object for the result of the data pipeline.
	/// \param time Specifies at which animation time the pipeline should be evaluated.
	virtual SharedFuture<PipelineFlowState> evaluate(TimePoint time) = 0;

	/// \brief Returns the results of an immediate and preliminary evaluation of the data pipeline.
	virtual PipelineFlowState evaluatePreliminary() { return {}; }

	/// \brief Returns a list of object nodes that have this object in their pipeline.
	QSet<PipelineSceneNode*> dependentNodes(bool skipRemovedNodes = false) const;

	/// \brief Sets the current status of the pipeline object.
	void setStatus(const PipelineStatus& status);

	/// \brief Returns the current status of the pipeline object.
	virtual PipelineStatus status() const { return _status; }
	
	/// \brief Given an animation time, computes the source frame to show.
	virtual int animationTimeToSourceFrame(TimePoint time) const;
	
	/// \brief Given a source frame index, returns the animation time at which it is shown.
	virtual TimePoint sourceFrameToAnimationTime(int frame) const;
		
private:

	/// The current status of this pipeline object.
	PipelineStatus _status;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace


