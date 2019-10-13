////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2018 Alexander Stukowski
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

#pragma once


#include <ovito/gui/GUI.h>

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
