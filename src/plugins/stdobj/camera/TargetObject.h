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
#include <core/dataset/data/DisplayObject.h>
#include <core/rendering/LinePrimitive.h>

namespace Ovito { namespace StdObj {

/**
 * A simple helper object that serves as direction target for camera and light objects.
 */
class OVITO_STDOBJ_EXPORT TargetObject : public DataObject
{
	Q_OBJECT
	OVITO_CLASS(TargetObject)
	
public:

	/// Constructor.
	Q_INVOKABLE TargetObject(DataSet* dataset);

	/// \brief Returns the title of this object.
	virtual QString objectTitle() override { return tr("Target"); }
};

/**
 * \brief A scene display object for target objects.
 */
class OVITO_STDOBJ_EXPORT TargetDisplayObject : public DisplayObject
{
	OVITO_CLASS(TargetDisplayObject)
	Q_OBJECT
	
public:

	/// \brief Constructor.
	Q_INVOKABLE TargetDisplayObject(DataSet* dataset) : DisplayObject(dataset) {}

	/// \brief Lets the display object render a data object.
	virtual void render(TimePoint time, DataObject* dataObject, const PipelineFlowState& flowState, SceneRenderer* renderer, ObjectNode* contextNode) override;

	/// \brief Computes the bounding box of the object.
	virtual Box3 boundingBox(TimePoint time, DataObject* dataObject, ObjectNode* contextNode, const PipelineFlowState& flowState, TimeInterval& validityInterval) override;

	/// \brief Returns the title of this object.
	virtual QString objectTitle() override { return tr("Target icon"); }

protected:

	/// The buffered geometry used to render the icon.
	std::shared_ptr<LinePrimitive> _icon;

	/// The icon geometry to be rendered in object picking mode.
	std::shared_ptr<LinePrimitive> _pickingIcon;

	/// This helper structure is used to detect any changes in the input data
	/// that require updating the geometry buffer.
	SceneObjectCacheHelper<
		VersionedDataObjectRef,		// Scene object + revision number
		Color						// Display color
		> _geometryCacheHelper;
};

}	// End of namespace
}	// End of namespace
