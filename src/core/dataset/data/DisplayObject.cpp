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

#include <core/Core.h>
#include <core/dataset/pipeline/PipelineFlowState.h>
#include <core/utilities/concurrent/Future.h>
#include "DisplayObject.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Scene)

IMPLEMENT_OVITO_CLASS(DisplayObject);
DEFINE_PROPERTY_FIELD(DisplayObject, isEnabled);
DEFINE_PROPERTY_FIELD(DisplayObject, title);
SET_PROPERTY_FIELD_LABEL(DisplayObject, isEnabled, "Enabled");
SET_PROPERTY_FIELD_CHANGE_EVENT(DisplayObject, isEnabled, ReferenceEvent::TargetEnabledOrDisabled);
SET_PROPERTY_FIELD_LABEL(DisplayObject, title, "Name");
SET_PROPERTY_FIELD_CHANGE_EVENT(DisplayObject, title, ReferenceEvent::TitleChanged);

/******************************************************************************
* Constructor.
******************************************************************************/
DisplayObject::DisplayObject(DataSet* dataset) : RefTarget(dataset), _isEnabled(true)
{
}

/******************************************************************************
* Lets the display object transform a data object in preparation for rendering.
******************************************************************************/
Future<PipelineFlowState> DisplayObject::transformData(TimePoint time, DataObject* dataObject, PipelineFlowState&& flowState, const PipelineFlowState& cachedState, ObjectNode* contextNode) 
{
	return std::move(flowState);
}

/******************************************************************************
* Sets the current status of the display object.
******************************************************************************/
void DisplayObject::setStatus(const PipelineStatus& status) 
{
	if(status != _status) {
		_status = status; 
		notifyDependents(ReferenceEvent::ObjectStatusChanged);
	}
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
