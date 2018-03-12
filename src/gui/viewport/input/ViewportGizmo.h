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


#include <gui/GUI.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(ViewportInput)

/**
 * \brief Abstract base class for viewport gizmos that display additional content in the 
 *        interactive viewports.
 *
 * The ViewportInputManager keeps a list of active ViewportGizmo objects.
 */
class OVITO_GUI_EXPORT ViewportGizmo
{
public:

	/// \brief Lets the input mode render its 3d overlay content in a viewport.
	/// \param vp The viewport into which the mode should render its specific overlay content.
	/// \param renderer The renderer that should be used to display the overlay.
	///
	/// This method is called by the system every time the viewports are redrawn and this input
	/// mode is on the input mode stack.
	///
	/// The default implementation of this method does nothing. 
	virtual void renderOverlay3D(Viewport* vp, ViewportSceneRenderer* renderer) {}

	/// \brief Lets the input mode render its 2d overlay content in a viewport.
	/// \param vp The viewport into which the mode should render its specific overlay content.
	/// \param renderer The renderer that should be used to display the overlay.
	///
	/// This method is called by the system every time the viewports are redrawn and this input
	/// mode is on the input mode stack.
	///
	/// The default implementation of this method does nothing. 
	virtual void renderOverlay2D(Viewport* vp, ViewportSceneRenderer* renderer) {}
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
