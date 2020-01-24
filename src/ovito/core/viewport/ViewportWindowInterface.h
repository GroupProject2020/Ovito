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

#pragma once


#include <ovito/core/Core.h>

namespace Ovito { 
	class MainWindowInterface;   // Note: This class is defined in another plugin module.
}

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/**
 * \brief Abstract interface for viewport windows, which provide the connection between the 
 *        non-visual Viewport class and the GUI layer.
 */
class OVITO_CORE_EXPORT ViewportWindowInterface
{
public:

	/// Constructor which associates this window with the given viewport instance.
	ViewportWindowInterface(MainWindowInterface* mainWindow, Viewport* vp);

	/// Returns the viewport associated with this window.
	Viewport* viewport() const { return _viewport; }

	/// Returns the main window hosting this viewport window.
	MainWindowInterface* mainWindow() const { return _mainWindow; }

    /// Puts an update request event for this window on the event loop.
	virtual void renderLater() = 0;

	/// Immediately redraws the contents of this window.
	virtual void renderNow() = 0;

	/// If an update request is pending for this viewport window, immediately
	/// processes it and redraw the window contents.
	virtual void processViewportUpdate() = 0;

	/// Returns the current size of the viewport window (in device pixels).
	virtual QSize viewportWindowDeviceSize() = 0;

	/// Returns the current size of the viewport window (in device-independent pixels).
	virtual QSize viewportWindowDeviceIndependentSize() = 0;

	/// Returns the device pixel ratio of the viewport window's canvas.
	virtual qreal devicePixelRatio() = 0;

	/// Makes the viewport window delete itself.
	/// This method is automatically called by the Viewport class destructor.
	virtual void destroyViewportWindow() = 0;

	/// Renders custom GUI elements in the viewport on top of the scene.
	virtual void renderGui() = 0;

	/// Determines the object that is located under the given mouse cursor position.
	virtual ViewportPickResult pick(const QPointF& pos) = 0;

	/// Makes the OpenGL context used by the viewport window for rendering the current context.
	virtual void makeOpenGLContextCurrent() = 0;

	/// Returns the list of gizmos to render in the viewport.
	virtual const std::vector<ViewportGizmo*>& viewportGizmos() = 0;

	/// Returns whether the viewport window is currently visible on screen.
	virtual bool isVisible() const = 0;

protected:

	/// Render the axis tripod symbol in the corner of the viewport that indicates
	/// the coordinate system orientation.
	void renderOrientationIndicator(SceneRenderer* renderer);

	/// Renders the frame on top of the scene that indicates the visible rendering area.
	void renderRenderFrame(SceneRenderer* renderer);

	/// Renders the viewport caption text.
	QRectF renderViewportTitle(SceneRenderer* renderer, bool hoverState);

private:

	/// Pointer to the main window hosting this viewport window.
	MainWindowInterface* _mainWindow;

	/// The viewport associated with this window.
	Viewport* _viewport;

#ifdef OVITO_DEBUG
	/// Counts how often this viewport has been rendered during the current program session.
	int _renderDebugCounter = 0;
#endif

	/// The primitive for rendering the viewport's caption text.
	std::shared_ptr<TextPrimitive> _captionBuffer;

	/// The primitive for rendering the viewport's orientation indicator.
	std::shared_ptr<LinePrimitive> _orientationTripodGeometry;

	/// The primitive for rendering the viewport's orientation indicator labels.
	std::shared_ptr<TextPrimitive> _orientationTripodLabels[3];

	/// The primitive for rendering the frame around the visible viewport area.
	std::shared_ptr<ImagePrimitive> _renderFrameOverlay;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
