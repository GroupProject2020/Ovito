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

#include <ovito/gui/web/GUIWeb.h>
#include <ovito/gui/web/mainwin/MainWindow.h>
#include <ovito/gui/base/viewport/ViewportInputManager.h>
#include <ovito/gui/base/rendering/ViewportSceneRenderer.h>
#include <ovito/gui/base/rendering/PickingSceneRenderer.h>
#include <ovito/core/viewport/Viewport.h>
#include <ovito/core/viewport/ViewportConfiguration.h>
#include <ovito/core/rendering/RenderSettings.h>
#include <ovito/core/app/Application.h>
#include "ViewportWindow.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/******************************************************************************
* Constructor.
******************************************************************************/
ViewportWindow::ViewportWindow(Viewport* vp, ViewportInputManager* inputManager, MainWindow* mainWindow, QQuickItem* parentItem) : 
	QQuickFramebufferObject(parentItem),
	ViewportWindowInterface(mainWindow, vp),
	_inputManager(inputManager)
{
	// Show the FBO contents upside down.
	setMirrorVertically(true);

	// Determine OpenGL vendor string so other parts of the code can decide
	// which OpenGL features are safe to use.
	OpenGLSceneRenderer::determineOpenGLInfo();

	// Create the viewport renderer.
	// It is shared by all viewports of a dataset.
	for(Viewport* vp : vp->dataset()->viewportConfig()->viewports()) {
		if(vp->window() != nullptr) {
			_viewportRenderer = static_cast<ViewportWindow*>(vp->window())->_viewportRenderer;
			if(_viewportRenderer) break;
		}
	}
	if(!_viewportRenderer)
		_viewportRenderer = new ViewportSceneRenderer(vp->dataset());

	// Create the object picking renderer.
	_pickingRenderer = new PickingSceneRenderer(vp->dataset());

	// Receive mouse input events.
	setAcceptedMouseButtons(Qt::AllButtons);
//	setAcceptHoverEvents(true);
}

/******************************************************************************
* Destructor.
******************************************************************************/
ViewportWindow::~ViewportWindow()
{
	// Detach from Viewport instance.
	if(viewport())
		viewport()->setWindow(nullptr);
}

/******************************************************************************
* Returns the input manager handling mouse events of the viewport (if any).
******************************************************************************/
ViewportInputManager* ViewportWindow::inputManager() const
{
	return _inputManager.data();
}

/******************************************************************************
* Create the renderer used to render into the FBO.
******************************************************************************/
QQuickFramebufferObject::Renderer* ViewportWindow::createRenderer() const
{
	return new Renderer(const_cast<ViewportWindow*>(this));
}

/******************************************************************************
* Puts an update request event for this viewport on the event loop.
******************************************************************************/
void ViewportWindow::renderLater()
{
	_updateRequested = true;
	update();
}

/******************************************************************************
* If an update request is pending for this viewport window, immediately
* processes it and redraw the window contents.
******************************************************************************/
void ViewportWindow::processViewportUpdate()
{
	if(_updateRequested) {
		OVITO_ASSERT_MSG(!viewport()->isRendering(), "ViewportWindow::processUpdateRequest()", "Recursive viewport repaint detected.");
		OVITO_ASSERT_MSG(!viewport()->dataset()->viewportConfig()->isRendering(), "ViewportWindow::processUpdateRequest()", "Recursive viewport repaint detected.");
//		repaint();
	}
}

/******************************************************************************
* Handles double click events.
******************************************************************************/
void ViewportWindow::mouseDoubleClickEvent(QMouseEvent* event)
{
	if(inputManager()) {
		if(ViewportInputMode* mode = inputManager()->activeMode()) {
			try {
				mode->mouseDoubleClickEvent(this, event);
			}
			catch(const Exception& ex) {
				qWarning() << "Uncaught exception in viewport mouse event handler:";
				ex.logError();
			}
		}
	}
}

/******************************************************************************
* Handles mouse press events.
******************************************************************************/
void ViewportWindow::mousePressEvent(QMouseEvent* event)
{
	viewport()->dataset()->viewportConfig()->setActiveViewport(viewport());

	if(inputManager()) {
		if(ViewportInputMode* mode = inputManager()->activeMode()) {
			try {
				mode->mousePressEvent(this, event);
			}
			catch(const Exception& ex) {
				qWarning() << "Uncaught exception in viewport mouse event handler:";
				ex.logError();
			}
		}
	}
}

/******************************************************************************
* Handles mouse release events.
******************************************************************************/
void ViewportWindow::mouseReleaseEvent(QMouseEvent* event)
{
	if(inputManager()) {
		if(ViewportInputMode* mode = inputManager()->activeMode()) {
			try {
				mode->mouseReleaseEvent(this, event);
			}
			catch(const Exception& ex) {
				qWarning() << "Uncaught exception in viewport mouse event handler:";
				ex.logError();
			}
		}
	}
}

/******************************************************************************
* Handles mouse move events.
******************************************************************************/
void ViewportWindow::mouseMoveEvent(QMouseEvent* event)
{
	if(_contextMenuArea.contains(event->localPos()) && !_cursorInContextMenuArea) {
		_cursorInContextMenuArea = true;
		viewport()->updateViewport();
	}
	else if(!_contextMenuArea.contains(event->localPos()) && _cursorInContextMenuArea) {
		_cursorInContextMenuArea = false;
		viewport()->updateViewport();
	}

	if(inputManager()) {
		if(ViewportInputMode* mode = inputManager()->activeMode()) {
			try {
				mode->mouseMoveEvent(this, event);
			}
			catch(const Exception& ex) {
				qWarning() << "Uncaught exception in viewport mouse event handler:";
				ex.logError();
			}
		}
	}
}

/******************************************************************************
* Handles mouse wheel events.
******************************************************************************/
void ViewportWindow::wheelEvent(QWheelEvent* event)
{
	if(inputManager()) {
		if(ViewportInputMode* mode = inputManager()->activeMode()) {
			try {
				mode->wheelEvent(this, event);
			}
			catch(const Exception& ex) {
				qWarning() << "Uncaught exception in viewport mouse event handler:";
				ex.logError();
			}
		}
	}
}

/******************************************************************************
* Returns the list of gizmos to render in the viewport.
******************************************************************************/
const std::vector<ViewportGizmo*>& ViewportWindow::viewportGizmos()
{
	return inputManager()->viewportGizmos();
}

/******************************************************************************
* Determines the object that is visible under the given mouse cursor position.
******************************************************************************/
ViewportPickResult ViewportWindow::pick(const QPointF& pos)
{
	ViewportPickResult result;

	// Cannot perform picking while viewport is not visible or currently rendering or when updates are disabled.
	if(isVisible() && pickingRenderer() && !viewport()->isRendering() && !viewport()->dataset()->viewportConfig()->isSuspended()) {
		try {
			if(pickingRenderer()->isRefreshRequired()) {
				// Let the viewport do the actual rendering work.
				viewport()->renderInteractive(pickingRenderer());
			}

			// Query which object is located at the given window position.
			const QPoint pixelPos = (pos * devicePixelRatio()).toPoint();
			const PickingSceneRenderer::ObjectRecord* objInfo;
			quint32 subobjectId;
			std::tie(objInfo, subobjectId) = pickingRenderer()->objectAtLocation(pixelPos);
			if(objInfo) {
				result.setPipelineNode(objInfo->objectNode);
				result.setPickInfo(objInfo->pickInfo);
				result.setHitLocation(pickingRenderer()->worldPositionFromLocation(pixelPos));
				result.setSubobjectId(subobjectId);
			}
		}
		catch(const Exception& ex) {
			ex.reportError();
		}
	}

	return result;
}

/******************************************************************************
* Immediately redraws the contents of this window.
******************************************************************************/
void ViewportWindow::renderNow()
{
}

/******************************************************************************
* Renders the contents of the viewport window.
******************************************************************************/
void ViewportWindow::renderViewport()
{
	_updateRequested = false;

	// Do not re-enter rendering function of the same viewport.
	if(!viewport() || viewport()->isRendering())
		return;

	// Invalidate picking buffer every time the visible contents of the viewport change.
	_pickingRenderer->reset();

	// Dpn't render anything if viewport updates are currently suspended. 
	if(viewport()->dataset()->viewportConfig()->isSuspended()) {
		return;
	}

	try {
		// Let the Viewport class do the actual rendering work.
		viewport()->renderInteractive(_viewportRenderer);
	}
	catch(Exception& ex) {
		if(ex.context() == nullptr) ex.setContext(viewport()->dataset());
		ex.prependGeneralMessage(tr("An unexpected error occurred while rendering the viewport contents. The program will quit."));
		viewport()->dataset()->viewportConfig()->suspendViewportUpdates();
		ex.reportError();
	}

	// Reset the OpenGL context back to its default state expected by Qt Quick.
	window()->resetOpenGLState();
}

/******************************************************************************
* Renders custom GUI elements in the viewport on top of the scene.
******************************************************************************/
void ViewportWindow::renderGui(SceneRenderer* renderer)
{
	if(viewport()->renderPreviewMode()) {
		// Render render frame.
		renderRenderFrame(renderer);
	}
	else {
		// Render orientation tripod.
		renderOrientationIndicator(renderer);
	}

	// Render viewport caption.
	_contextMenuArea = renderViewportTitle(renderer, _cursorInContextMenuArea);
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
