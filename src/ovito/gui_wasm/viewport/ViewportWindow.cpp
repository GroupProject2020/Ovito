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

#include <ovito/gui_wasm/GUI.h>
#include <ovito/gui_wasm/viewport/input/ViewportInputManager.h>
#include <ovito/gui_wasm/rendering/ViewportSceneRenderer.h>
#include <ovito/gui_wasm/viewport/picking/PickingSceneRenderer.h>
#include <ovito/gui_wasm/mainwin/MainWindow.h>
#include <ovito/core/viewport/Viewport.h>
#include <ovito/core/viewport/ViewportConfiguration.h>
#include <ovito/core/rendering/RenderSettings.h>
#include <ovito/core/app/Application.h>
#include "ViewportWindow.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/******************************************************************************
* Constructor.
******************************************************************************/
ViewportWindow::ViewportWindow(Viewport* owner, ViewportInputManager* inputManager, QQuickWindow* quickWindow) : QObject(quickWindow),
		_viewport(owner),
		_inputManager(inputManager),
		_quickWindow(quickWindow)
{
	// Determine OpenGL vendor string so other parts of the code can decide
	// which OpenGL features are safe to use.
	OpenGLSceneRenderer::determineOpenGLInfo();

	// Associate the viewport with this window.
	owner->setWindow(this);

	// Create the viewport renderer.
	// It is shared by all viewports of a dataset.
	for(Viewport* vp : owner->dataset()->viewportConfig()->viewports()) {
		if(vp->window() != nullptr) {
			_viewportRenderer = static_cast<ViewportWindow*>(vp->window())->_viewportRenderer;
			if(_viewportRenderer) break;
		}
	}
	if(!_viewportRenderer)
		_viewportRenderer = new ViewportSceneRenderer(owner->dataset());

	// Create the object picking renderer.
	_pickingRenderer = new PickingSceneRenderer(owner->dataset());

	// Render the viewport contents during each update of the Qt Quick window.
	connect(quickWindow, &QQuickWindow::beforeRendering, this, &ViewportWindow::renderViewport);
}

/******************************************************************************
* Destructor.
******************************************************************************/
ViewportWindow::~ViewportWindow()
{
	// Detach from Viewport class.
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
* Puts an update request event for this viewport on the event loop.
******************************************************************************/
void ViewportWindow::renderLater()
{
	_updateRequested = true;
	quickWindow()->update();
}

/******************************************************************************
* If an update request is pending for this viewport window, immediately
* processes it and redraw the window contents.
******************************************************************************/
void ViewportWindow::processViewportUpdate()
{
	if(_updateRequested) {
		OVITO_ASSERT_MSG(!_viewport->isRendering(), "ViewportWindow::processUpdateRequest()", "Recursive viewport repaint detected.");
		OVITO_ASSERT_MSG(!_viewport->dataset()->viewportConfig()->isRendering(), "ViewportWindow::processUpdateRequest()", "Recursive viewport repaint detected.");
//		repaint();
	}
}

/******************************************************************************
* Render the axis tripod symbol in the corner of the viewport that indicates
* the coordinate system orientation.
******************************************************************************/
void ViewportWindow::renderOrientationIndicator()
{
	const FloatType tripodSize = FloatType(80);			// device-independent pixels
	const FloatType tripodArrowSize = FloatType(0.17); 	// percentage of the above value.

	// Turn off depth-testing.
	_viewportRenderer->setDepthTestEnabled(false);

	// Setup projection matrix.
	const FloatType tripodPixelSize = tripodSize * _viewportRenderer->devicePixelRatio();
	_viewportRenderer->setRenderingViewport(0, 0, tripodPixelSize, tripodPixelSize);
	ViewProjectionParameters projParams = viewport()->projectionParams();
	projParams.projectionMatrix = Matrix4::ortho(-1.4f, 1.4f, -1.4f, 1.4f, -2, 2);
	projParams.inverseProjectionMatrix = projParams.projectionMatrix.inverse();
	projParams.viewMatrix.setIdentity();
	projParams.inverseViewMatrix.setIdentity();
	projParams.isPerspective = false;
	_viewportRenderer->setProjParams(projParams);
	_viewportRenderer->setWorldTransform(AffineTransformation::Identity());

    static const ColorA axisColors[3] = { ColorA(1, 0, 0), ColorA(0, 1, 0), ColorA(0.4f, 0.4f, 1) };
	static const QString labels[3] = { QStringLiteral("x"), QStringLiteral("y"), QStringLiteral("z") };

	// Create line buffer.
	if(!_orientationTripodGeometry || !_orientationTripodGeometry->isValid(_viewportRenderer)) {
		_orientationTripodGeometry = _viewportRenderer->createLinePrimitive();
		_orientationTripodGeometry->setVertexCount(18);
		ColorA vertexColors[18];
		for(int i = 0; i < 18; i++)
			vertexColors[i] = axisColors[i / 6];
		_orientationTripodGeometry->setVertexColors(vertexColors);
	}

	// Render arrows.
	Point3 vertices[18];
	for(int axis = 0, index = 0; axis < 3; axis++) {
		Vector3 dir = viewport()->projectionParams().viewMatrix.column(axis).normalized();
		vertices[index++] = Point3::Origin();
		vertices[index++] = Point3::Origin() + dir;
		vertices[index++] = Point3::Origin() + dir;
		vertices[index++] = Point3::Origin() + (dir + tripodArrowSize * Vector3(dir.y() - dir.x(), -dir.x() - dir.y(), dir.z()));
		vertices[index++] = Point3::Origin() + dir;
		vertices[index++] = Point3::Origin() + (dir + tripodArrowSize * Vector3(-dir.y() - dir.x(), dir.x() - dir.y(), dir.z()));
	}
	_orientationTripodGeometry->setVertexPositions(vertices);
	_orientationTripodGeometry->render(_viewportRenderer);

#if 0
	// Render x,y,z labels.
	for(int axis = 0; axis < 3; axis++) {

		// Create a rendering buffer that is responsible for rendering the text label.
		if(!_orientationTripodLabels[axis] || !_orientationTripodLabels[axis]->isValid(_viewportRenderer)) {
			_orientationTripodLabels[axis] = _viewportRenderer->createTextPrimitive();
			_orientationTripodLabels[axis]->setFont(ViewportSettings::getSettings().viewportFont());
			_orientationTripodLabels[axis]->setColor(axisColors[axis]);
			_orientationTripodLabels[axis]->setText(labels[axis]);
		}

		Point3 p = Point3::Origin() + viewport()->projectionParams().viewMatrix.column(axis).resized(1.2f);
		Point3 ndcPoint = projParams.projectionMatrix * p;
		Point2 windowPoint(( ndcPoint.x() + FloatType(1)) * tripodPixelSize / 2,
							(-ndcPoint.y() + FloatType(1)) * tripodPixelSize / 2);
		_orientationTripodLabels[axis]->renderWindow(_viewportRenderer, windowPoint, Qt::AlignHCenter | Qt::AlignVCenter);
	}
#endif

	// Restore old rendering attributes.
	_viewportRenderer->setDepthTestEnabled(true);
	_viewportRenderer->setRenderingViewport(0, 0, viewportWindowDeviceSize().width(), viewportWindowDeviceSize().height());
}

/******************************************************************************
* Renders the frame on top of the scene that indicates the visible rendering area.
******************************************************************************/
void ViewportWindow::renderRenderFrame()
{
	// Create a rendering buffer that is responsible for rendering the frame.
	if(!_renderFrameOverlay || !_renderFrameOverlay->isValid(_viewportRenderer)) {
		_renderFrameOverlay = _viewportRenderer->createImagePrimitive();
		QImage image(1, 1, QImage::Format_ARGB32);
		image.fill(0xA0A0A0A0);
		_renderFrameOverlay->setImage(image);
	}

	Box2 rect = viewport()->renderFrameRect();

	// Render rectangle borders
	_renderFrameOverlay->renderViewport(_viewportRenderer, Point2(-1,-1), Vector2(FloatType(1) + rect.minc.x(), 2));
	_renderFrameOverlay->renderViewport(_viewportRenderer, Point2(rect.maxc.x(),-1), Vector2(FloatType(1) - rect.maxc.x(), 2));
	_renderFrameOverlay->renderViewport(_viewportRenderer, Point2(rect.minc.x(),-1), Vector2(rect.width(), FloatType(1) + rect.minc.y()));
	_renderFrameOverlay->renderViewport(_viewportRenderer, Point2(rect.minc.x(),rect.maxc.y()), Vector2(rect.width(), FloatType(1) - rect.maxc.y()));
}

/******************************************************************************
* Determines the object that is visible under the given mouse cursor position.
******************************************************************************/
ViewportPickResult ViewportWindow::pick(const QPointF& pos)
{
	ViewportPickResult result;

	// Cannot perform picking while viewport is not visible or currently rendering or when updates are disabled.
	if(quickWindow()->isVisible() && !viewport()->isRendering() && !viewport()->dataset()->viewportConfig()->isSuspended()) {
		try {
			if(_pickingRenderer->isRefreshRequired()) {
				// Let the viewport do the actual rendering work.
				viewport()->renderInteractive(_pickingRenderer);
			}

			// Query which object is located at the given window position.
			const QPoint pixelPos = (pos * quickWindow()->effectiveDevicePixelRatio()).toPoint();
			const PickingSceneRenderer::ObjectRecord* objInfo;
			quint32 subobjectId;
			std::tie(objInfo, subobjectId) = _pickingRenderer->objectAtLocation(pixelPos);
			if(objInfo) {
				result.setPipelineNode(objInfo->objectNode);
				result.setPickInfo(objInfo->pickInfo);
				result.setHitLocation(_pickingRenderer->worldPositionFromLocation(pixelPos));
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
* Renders custom GUI elements in the viewport on top of the scene.
******************************************************************************/
void ViewportWindow::renderGui()
{
	if(viewport()->renderPreviewMode()) {
		// Render render frame.
		renderRenderFrame();
	}
	else {
		// Render orientation tripod.
		renderOrientationIndicator();
	}
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
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
