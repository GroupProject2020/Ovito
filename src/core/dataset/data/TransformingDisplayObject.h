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
#include <core/dataset/data/DisplayObject.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Scene)

/**
 * \brief A type of DisplayObject that first transforms data into another form before
 *        rendering it. The transformation process typically occurs asynchronously. 
 */
class OVITO_CORE_EXPORT TransformingDisplayObject : public DisplayObject
{
	Q_OBJECT
	OVITO_CLASS(TransformingDisplayObject)
	
protected:

	/// \brief Constructor.
	TransformingDisplayObject(DataSet* dataset);

public:

	/// Indicates that the display object wants to transform data objects before rendering. 
	virtual bool doesPerformDataTransformation() const override { return true; }

	/// Lets the display object transform a data object in preparation for rendering.
	virtual Future<PipelineFlowState> transformData(TimePoint time, DataObject* dataObject, PipelineFlowState&& flowState, const PipelineFlowState& cachedState, ObjectNode* contextNode) override;

	/// Returns a structure that describes the current status of the display object.
	virtual PipelineStatus status() const override {
		// During an ongoing transformation process, the status of the DisplayObject is 'in progress'.
		// Otherwise the status indicates the outcome of the transformation operation.
		return _activeTransformationsCount > 0 ? PipelineStatus(PipelineStatus::Pending) : DisplayObject::status(); 
	}

	/// Returns the revision counter of this display object, which is incremented every time one of the object's parameters change.
	unsigned int revisionNumber() const { return _revisionNumber; }	

	/// Bumps up the internal revision number of this DisplayObject in order to mark
	/// all transformed data objects as outdated which have been generated so far.
	void invalidateTransformedObjects() {
		_revisionNumber++;
	}

protected:
	
	/// Lets the display object transform a data object in preparation for rendering.
	virtual Future<PipelineFlowState> transformDataImpl(TimePoint time, DataObject* dataObject, PipelineFlowState&& flowState, const PipelineFlowState& cachedState, ObjectNode* contextNode) = 0;
	
private:
	
	/// The number of data transformations that are currently in progress.
	int _activeTransformationsCount = 0;

	/// The revision counter of this display object.
	/// The counter is incremented every time one of the object's parameters change that 
	/// trigger a regeneration of the transformed data object from the input data.
	unsigned int _revisionNumber = 0;	
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
