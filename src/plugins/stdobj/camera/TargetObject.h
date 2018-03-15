///////////////////////////////////////////////////////////////////////////////
// 
//  Copyright (2013) Alexander Stukowski
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


#include <plugins/stdobj/StdObj.h>
#include <core/dataset/data/DataObject.h>
#include <core/dataset/data/VersionedDataObjectRef.h>
#include <core/dataset/data/DataVis.h>
#include <core/rendering/LinePrimitive.h>

namespace Ovito { namespace StdObj {

/**
 * A simple helper object that serves as direction target for camera and light objects.
 */
class OVITO_STDOBJ_EXPORT TargetObject : public DataObject
{
	Q_OBJECT
	OVITO_CLASS(TargetObject)
	Q_CLASSINFO("DisplayName", "Target");

public:

	/// Constructor.
	Q_INVOKABLE TargetObject(DataSet* dataset);
};

/**
 * \brief A visual element rendering target objects in the interactive viewports.
 */
class OVITO_STDOBJ_EXPORT TargetVis : public DataVis
{
	Q_OBJECT
	OVITO_CLASS(TargetVis)
	Q_CLASSINFO("DisplayName", "Target icon");
	
public:

	/// \brief Constructor.
	Q_INVOKABLE TargetVis(DataSet* dataset) : DataVis(dataset) {}

	/// \brief Lets the vis element render a data object.
	virtual void render(TimePoint time, DataObject* dataObject, const PipelineFlowState& flowState, SceneRenderer* renderer, PipelineSceneNode* contextNode) override;

	/// \brief Computes the bounding box of the object.
	virtual Box3 boundingBox(TimePoint time, DataObject* dataObject, PipelineSceneNode* contextNode, const PipelineFlowState& flowState, TimeInterval& validityInterval) override;
};

}	// End of namespace
}	// End of namespace
