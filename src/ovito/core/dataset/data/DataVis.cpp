////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2017 Alexander Stukowski
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

#include <ovito/core/Core.h>
#include <ovito/core/dataset/pipeline/PipelineFlowState.h>
#include <ovito/core/dataset/scene/PipelineSceneNode.h>
#include <ovito/core/utilities/concurrent/Future.h>
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
