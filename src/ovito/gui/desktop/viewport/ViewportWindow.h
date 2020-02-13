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


#include <ovito/gui/desktop/GUI.h>
#include <ovito/gui/base/rendering/ViewportSceneRenderer.h>
#include <ovito/gui/base/rendering/PickingSceneRenderer.h>
#include <ovito/core/viewport/ViewportWindowInterface.h>

namespace Ovito {

/**
 * \brief The internal render window/widget used by the Viewport class.
 */
class OVITO_GUI_EXPORT ViewportWindow : public QOpenGLWidget, public ViewportWindowInterface
{
	Q_OBJECT

public:

	/// Constructor.
	ViewportWindow(Viewport* vp, ViewportInputManager* inputManager, MainWindow* mainWindow, QWidget* parentWidget);

	/// Returns the input manager handling mouse events of the viewport (if any).
	ViewportInputManager* inputManager() const;

    /// \brief Puts an update request event for this window on the event loop.
	virtual void renderLater() override;

	/// \brief Immediately redraws the contents of this window.
	virtual void renderNow() override;

	/// If an update request is pending for this viewport window, immediately
	/// processes it and redraw the window contents.
	virtual void processViewportUpdate() override;

	/// Returns the current size of the viewport window (in device pixels).
	virtual QSize viewportWindowDeviceSize() override {
		return size() * devicePixelRatio();
	}

	/// Returns the current size of the viewport window (in device-independent pixels).
	virtual QSize viewportWindowDeviceIndependentSize() override {
		return size();
	}

	/// Returns the device pixel ratio of the viewport window's canvas.
	virtual qreal devicePixelRatio() override {
		return QOpenGLWidget::devicePixelRatioF();
	}

	/// Lets the viewport window delete itself.
	/// This is called by the Viewport class destructor.
	virtual void destroyViewportWindow() override {
		deleteLater();
	}

	/// Renders custom GUI elements in the viewport on top of the scene.
	virtual void renderGui(SceneRenderer* renderer) override;

	/// Makes the OpenGL context used by the viewport window for rendering the current context.
	virtual void makeOpenGLContextCurrent() override { makeCurrent(); }

	/// Returns the list of gizmos to render in the viewport.
	virtual const std::vector<ViewportGizmo*>& viewportGizmos() override;

	/// Returns whether the viewport window is currently visible on screen.
	virtual bool isVisible() const override { return QOpenGLWidget::isVisible(); }

	/// \brief Displays the context menu for the viewport.
	/// \param pos The position in where the context menu should be displayed.
	void showViewportMenu(const QPoint& pos = QPoint(0,0));

	/// Returns the renderer generating an offscreen image of the scene used for object picking.
	PickingSceneRenderer* pickingRenderer() const { return _pickingRenderer; }

	/// Determines the object that is located under the given mouse cursor position.
	virtual ViewportPickResult pick(const QPointF& pos) override;

protected:

	/// Is called whenever the widget needs to be painted.
	virtual void paintGL() override;

	/// Is called whenever the GL context needs to be initialized.
	virtual void initializeGL() override;

	/// Is called when the mouse cursor leaves the widget.
	virtual void leaveEvent(QEvent* event) override;

	/// Is called when the viewport becomes visible.
	virtual void showEvent(QShowEvent* event) override;

	/// Handles double click events.
	virtual void mouseDoubleClickEvent(QMouseEvent* event) override;

	/// Handles mouse press events.
	virtual void mousePressEvent(QMouseEvent* event) override;

	/// Handles mouse release events.
	virtual void mouseReleaseEvent(QMouseEvent* event) override;

	/// Handles mouse move events.
	virtual void mouseMoveEvent(QMouseEvent* event) override;

	/// Handles mouse wheel events.
	virtual void wheelEvent(QWheelEvent* event) override;

	/// Is called when the widgets looses the input focus.
	virtual void focusOutEvent(QFocusEvent* event) override;

private:

	/// Renders the contents of the viewport window.
	void renderViewport();

private:

	/// A flag that indicates that a viewport update has been requested.
	bool _updateRequested = false;

	/// The zone in the upper left corner of the viewport where
	/// the context menu can be activated by the user.
	QRectF _contextMenuArea;

	/// Indicates that the mouse cursor is currently positioned inside the
	/// viewport area that activates the viewport context menu.
	bool _cursorInContextMenuArea = false;

	/// The input manager handling mouse events of the viewport.
	QPointer<ViewportInputManager> _inputManager;

	/// This is the renderer of the interactive viewport.
	OORef<ViewportSceneRenderer> _viewportRenderer;

	/// This renderer generates an offscreen rendering of the scene that allows picking of objects.
	OORef<PickingSceneRenderer> _pickingRenderer;
};

}	// End of namespace
