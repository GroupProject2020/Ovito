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

/**
 * \file DisplayObject.h
 * \brief Contains the definition of the Ovito::DisplayObject class.
 */

#ifndef __OVITO_DISPLAY_OBJECT_H
#define __OVITO_DISPLAY_OBJECT_H

#include <core/Core.h>
#include <core/reference/RefTarget.h>
#include <core/animation/TimeInterval.h>

namespace Ovito {

class ObjectNode;				// defined in ObjectNode.h
class SceneRenderer;			// defined in SceneRenderer.h
class SceneObject;				// defined in SceneObject.h
class PipelineFlowState;		// defined in PipelineFlowState.h

/**
 * \brief Abstract base class for display object that are responsible
 *        for rendering SceneObject-derived classes in the viewports.
 */
class DisplayObject : public RefTarget
{
protected:

	/// \brief Default constructor.
	DisplayObject();

public:

	/// \brief Lets the display object render a scene object.
	///
	/// \param time The animation time at which to render the object
	/// \param sceneObject The scene object that should be rendered.
	/// \param flowState The pipeline evaluation results of the object node.
	/// \param renderer The renderer object that should be used to display the geometry.
	/// \param contextNode The object node.
	///
	/// The world transformation matrix is already set up when this method is called by the
	/// system. The object has to be rendered in the local object coordinate system.
	virtual void render(TimePoint time, SceneObject* sceneObject, const PipelineFlowState& flowState, SceneRenderer* renderer, ObjectNode* contextNode) = 0;

	/// \brief Computes the bounding box of the given scene object.
	/// \param time The animation time for which the bounding box should be computed.
	/// \param sceneObject The scene object for which to compute the bounding box.
	/// \param contextNode The scene node to which this object belongs to.
	/// \param flowState The pipeline evaluation result of the object node.
	/// \return The bounding box of the object in local object coordinates.
	virtual Box3 boundingBox(TimePoint time, SceneObject* sceneObject, ObjectNode* contextNode, const PipelineFlowState& flowState) = 0;

	/// \brief Indicates whether this object should be surrounded by a selection marker in the viewports when it is selected.
	/// \return \c true to let the system render a selection marker around the object when it is selected.
	///
	/// The default implementation returns \c true.
	virtual bool showSelectionMarker() { return true; }

private:


	Q_OBJECT
	OVITO_OBJECT
};

/**
 * This template class can be used by DisplayObject-derived classes to keep track of cached data
 * that depends on the input SceneObject.
 */
template<class... Types>
class SceneObjectCacheHelper
{
public:

	bool updateState(const Types&... args) {
		bool hasChanged = (_oldState != std::tuple<Types...>(args...));
		_oldState = std::tuple<Types...>(args...);
		return hasChanged;
	}

private:

	std::tuple<Types...> _oldState;
};

};

#endif // __OVITO_DISPLAY_OBJECT_H
