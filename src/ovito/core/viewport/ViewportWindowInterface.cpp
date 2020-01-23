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
#include <ovito/core/viewport/Viewport.h>
#include <ovito/core/viewport/ViewportWindowInterface.h>
#include <ovito/core/rendering/SceneRenderer.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/******************************************************************************
* Constructor which associates this window with the given viewport instance.
******************************************************************************/
ViewportWindowInterface::ViewportWindowInterface(MainWindowInterface* mainWindow, Viewport* vp) : 
	_mainWindow(mainWindow),
	_viewport(vp)
{
	OVITO_ASSERT(mainWindow);
	OVITO_ASSERT(vp);

	// Associate the viewport with this window.
	vp->setWindow(this);
}

/******************************************************************************
* Render the axis tripod symbol in the corner of the viewport that indicates
* the coordinate system orientation.
******************************************************************************/
void ViewportWindowInterface::renderOrientationIndicator(SceneRenderer* renderer)
{
	const FloatType tripodSize = 80.0;			// device-independent pixels
	const FloatType tripodArrowSize = 0.17; 	// percentage of the above value.

	// Set up projection matrix.
	QSize imageSize = renderer->outputSize();
	const FloatType tripodPixelSize = tripodSize * renderer->devicePixelRatio();
	Matrix4 viewportScalingTM = Matrix4::Identity();
	viewportScalingTM(0,0) = tripodPixelSize / imageSize.width();
	viewportScalingTM(1,1) = tripodPixelSize / imageSize.height();
	viewportScalingTM(0,3) = -1.0 + viewportScalingTM(0,0);
	viewportScalingTM(1,3) = -1.0 + viewportScalingTM(1,1);
	ViewProjectionParameters projParams = viewport()->projectionParams();
	projParams.projectionMatrix = viewportScalingTM * Matrix4::ortho(-1.4, 1.4, -1.4, 1.4, -2.0, 2.0);
	projParams.inverseProjectionMatrix = projParams.projectionMatrix.inverse();
	projParams.viewMatrix.setIdentity();
	projParams.inverseViewMatrix.setIdentity();
	projParams.isPerspective = false;
	renderer->setProjParams(projParams);
	renderer->setWorldTransform(AffineTransformation::Identity());

	// Turn off depth-testing.
	renderer->setDepthTestEnabled(false);

    static const ColorA axisColors[3] = { ColorA(1.0, 0.0, 0.0), ColorA(0.0, 1.0, 0.0), ColorA(0.4, 0.4, 1.0) };
	static const QString labels[3] = { QStringLiteral("x"), QStringLiteral("y"), QStringLiteral("z") };

	// Create line buffer.
	if(!_orientationTripodGeometry || !_orientationTripodGeometry->isValid(renderer)) {
		_orientationTripodGeometry = renderer->createLinePrimitive();
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
	_orientationTripodGeometry->render(renderer);

#if 0
	// Render x,y,z labels.
	for(int axis = 0; axis < 3; axis++) {

		// Create a rendering buffer that is responsible for rendering the text label.
		if(!_orientationTripodLabels[axis] || !_orientationTripodLabels[axis]->isValid(renderer)) {
			_orientationTripodLabels[axis] = renderer->createTextPrimitive();
			_orientationTripodLabels[axis]->setFont(ViewportSettings::getSettings().viewportFont());
			_orientationTripodLabels[axis]->setColor(axisColors[axis]);
			_orientationTripodLabels[axis]->setText(labels[axis]);
		}

		Point3 p = Point3::Origin() + viewport()->projectionParams().viewMatrix.column(axis).resized(1.2f);
		Point3 ndcPoint = projParams.projectionMatrix * p;
		Point2 windowPoint(( ndcPoint.x() + FloatType(1)) * tripodPixelSize / 2,
							(-ndcPoint.y() + FloatType(1)) * tripodPixelSize / 2);
		_orientationTripodLabels[axis]->renderWindow(renderer, windowPoint, Qt::AlignHCenter | Qt::AlignVCenter);
	}
#endif

	// Restore old rendering attributes.
	renderer->setDepthTestEnabled(true);
}

/******************************************************************************
* Renders the frame on top of the scene that indicates the visible rendering area.
******************************************************************************/
void ViewportWindowInterface::renderRenderFrame(SceneRenderer* renderer)
{
	// Create a rendering buffer that is responsible for rendering the frame.
	if(!_renderFrameOverlay || !_renderFrameOverlay->isValid(renderer)) {
		_renderFrameOverlay = renderer->createImagePrimitive();
		QImage image(1, 1, QImage::Format_ARGB32);
		image.fill(0xA0A0A0A0);
		_renderFrameOverlay->setImage(image);
	}

	Box2 rect = viewport()->renderFrameRect();

	// Render rectangle borders
	_renderFrameOverlay->renderViewport(renderer, Point2(-1,-1), Vector2(FloatType(1) + rect.minc.x(), 2));
	_renderFrameOverlay->renderViewport(renderer, Point2(rect.maxc.x(),-1), Vector2(FloatType(1) - rect.maxc.x(), 2));
	_renderFrameOverlay->renderViewport(renderer, Point2(rect.minc.x(),-1), Vector2(rect.width(), FloatType(1) + rect.minc.y()));
	_renderFrameOverlay->renderViewport(renderer, Point2(rect.minc.x(),rect.maxc.y()), Vector2(rect.width(), FloatType(1) - rect.maxc.y()));
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
