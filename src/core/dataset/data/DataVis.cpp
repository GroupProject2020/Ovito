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
#include <core/dataset/scene/PipelineSceneNode.h>
#include <core/utilities/concurrent/Future.h>
#include "DataVis.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Scene)

IMPLEMENT_OVITO_CLASS(DataVis);
DEFINE_PROPERTY_FIELD(DataVis, isEnabled);
DEFINE_PROPERTY_FIELD(DataVis, title);
DEFINE_PROPERTY_FIELD(DataVis, status);
SET_PROPERTY_FIELD_LABEL(DataVis, isEnabled, "Enabled");
SET_PROPERTY_FIELD_LABEL(DataVis, title, "Name");
SET_PROPERTY_FIELD_LABEL(DataVis, status, "Status");
SET_PROPERTY_FIELD_CHANGE_EVENT(DataVis, isEnabled, ReferenceEvent::TargetEnabledOrDisabled);
SET_PROPERTY_FIELD_CHANGE_EVENT(DataVis, title, ReferenceEvent::TitleChanged);
SET_PROPERTY_FIELD_CHANGE_EVENT(DataVis, status, ReferenceEvent::ObjectStatusChanged);

/******************************************************************************
* Constructor.
******************************************************************************/
DataVis::DataVis(DataSet* dataset) : RefTarget(dataset), _isEnabled(true)
{
}

/******************************************************************************
* Returns all pipeline nodes whose pipeline produced this visualization element.
******************************************************************************/
QSet<PipelineSceneNode*> DataVis::pipelines(bool onlyScenePipelines) const
{
	QSet<PipelineSceneNode*> pipelineList;
	for(RefMaker* dependent : this->dependents()) {
		if(PipelineSceneNode* pipeline = dynamic_object_cast<PipelineSceneNode>(dependent)) {
            if(pipeline->visElements().contains(const_cast<DataVis*>(this))) {
				if(!onlyScenePipelines || pipeline->isInScene())
		    		pipelineList.insert(pipeline);
			}
		}
	}
	return pipelineList;
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
