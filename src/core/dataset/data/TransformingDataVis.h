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

#pragma once


#include <core/Core.h>
#include <core/dataset/data/DataVis.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Scene)

/**
 * \brief A type of DataVis that first transforms data into another form before
 *        rendering it. The transformation process typically happens asynchronously. 
 */
class OVITO_CORE_EXPORT TransformingDataVis : public DataVis
{
	Q_OBJECT
	OVITO_CLASS(TransformingDataVis)
	
protected:

	/// \brief Constructor.
	TransformingDataVis(DataSet* dataset);

public:

	/// Lets the vis element transform a data object in preparation for rendering.
	Future<PipelineFlowState> transformData(TimePoint time, const DataObject* dataObject, PipelineFlowState&& flowState, const PipelineFlowState& cachedState, const PipelineSceneNode* contextNode, bool breakOnError);

	/// Returns a structure that describes the current status of the vis element.
	virtual PipelineStatus status() const override {
		// During an ongoing transformation process, the status of the DataVis is 'in progress'.
		// Otherwise the status indicates the outcome of the transformation operation.
		return _activeTransformationsCount > 0 ? PipelineStatus(PipelineStatus::Pending) : DataVis::status(); 
	}

	/// Returns the revision counter of this vis element, which is incremented each time one of its parameters changes.
	unsigned int revisionNumber() const { return _revisionNumber; }	

	/// Bumps up the internal revision number of this DataVis in order to mark
	/// all transformed data objects as outdated which have been generated so far.
	void invalidateTransformedObjects() {
		_revisionNumber++;
	}

protected:
	
	/// Lets the vis element transform a data object in preparation for rendering.
	virtual Future<PipelineFlowState> transformDataImpl(TimePoint time, const DataObject* dataObject, PipelineFlowState&& flowState, const PipelineFlowState& cachedState, const PipelineSceneNode* contextNode) = 0;
	
private:
	
	/// The number of data transformations that are currently in progress.
	int _activeTransformationsCount = 0;

	/// The revision counter of this element.
	/// The counter is incremented every time one of the object's parameters changes that 
	/// trigger a regeneration of the transformed data object from the input data.
	unsigned int _revisionNumber = 0;	
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
