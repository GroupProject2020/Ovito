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

#include <ovito/core/Core.h>
#include <ovito/core/dataset/scene/PipelineSceneNode.h>
#include "DataVis.h"

namespace Ovito {

IMPLEMENT_OVITO_CLASS(DataVis);

/******************************************************************************
* Constructor.
******************************************************************************/
DataVis::DataVis(DataSet* dataset) : ActiveObject(dataset)
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

}	// End of namespace
