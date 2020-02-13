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
#include "OpenGLTextPrimitive.h"
#include "OpenGLSceneRenderer.h"

namespace Ovito {

/******************************************************************************
* Constructor.
******************************************************************************/
OpenGLTextPrimitive::OpenGLTextPrimitive(OpenGLSceneRenderer* renderer) :
	_contextGroup(QOpenGLContextGroup::currentContextGroup()),
	_needImageUpdate(true),
	_imageBuffer(renderer->createImagePrimitive())
{
}

/******************************************************************************
* Returns true if the buffer is filled and can be rendered with the given renderer.
******************************************************************************/
bool OpenGLTextPrimitive::isValid(SceneRenderer* renderer)
{
	return _imageBuffer->isValid(renderer);
}

/******************************************************************************
* Renders the text string at the given location given in normalized
* viewport coordinates ([-1,+1] range).
******************************************************************************/
void OpenGLTextPrimitive::renderViewport(SceneRenderer* renderer, const Point2& pos, int alignment)
{
	OpenGLSceneRenderer* vpRenderer = dynamic_object_cast<OpenGLSceneRenderer>(renderer);
	GLint vc[4];
	vpRenderer->glGetIntegerv(GL_VIEWPORT, vc);

	Point2 windowPos((pos.x() + 1.0) * vc[2] / 2, (-pos.y() + 1.0) * vc[3] / 2);
	renderWindow(renderer, windowPos, alignment);
}

/******************************************************************************
* Renders the text string at the given 2D window (device pixel) coordinates.
******************************************************************************/
void OpenGLTextPrimitive::renderWindow(SceneRenderer* renderer, const Point2& pos, int alignment)
{
	if(text().isEmpty() || renderer->isPicking())
		return;

	OpenGLSceneRenderer* vpRenderer = static_object_cast<OpenGLSceneRenderer>(renderer);
    OVITO_REPORT_OPENGL_ERRORS(vpRenderer);
	if(_needImageUpdate) {
		_needImageUpdate = false;

		// Measure text size.
		QRect rect;
		qreal devicePixelRatio = vpRenderer->devicePixelRatio();
		{
			QImage textureImage(1, 1, QImage::Format_RGB32);
			textureImage.setDevicePixelRatio(devicePixelRatio);
			QPainter painter(&textureImage);
			painter.setFont(font());
			rect = painter.boundingRect(QRect(), Qt::AlignLeft | Qt::AlignTop, text());
		}

		// Generate texture image.
		QImage textureImage((rect.width() * devicePixelRatio)+1, (rect.height() * devicePixelRatio)+1, QImage::Format_ARGB32_Premultiplied);
		textureImage.setDevicePixelRatio(devicePixelRatio);
		textureImage.fill((QColor)backgroundColor());
		{
			QPainter painter(&textureImage);
			painter.setFont(font());
			painter.setPen((QColor)color());
			painter.drawText(rect, Qt::AlignLeft | Qt::AlignTop, text());
		}
		_textOffset = rect.topLeft();
		//textureImage.save(QString("%1.png").arg(text()));

		_imageBuffer->setImage(textureImage);
	}
    OVITO_REPORT_OPENGL_ERRORS(vpRenderer);

	Point2 alignedPos = pos;
	Vector2 size = Vector2(_imageBuffer->image().width(), _imageBuffer->image().height()) * (FloatType)vpRenderer->antialiasingLevelInternal();
	if(alignment & Qt::AlignRight) alignedPos.x() += -size.x();
	else if(alignment & Qt::AlignHCenter) alignedPos.x() += -size.x() / 2;
	if(alignment & Qt::AlignBottom) alignedPos.y() += -size.y();
	else if(alignment & Qt::AlignVCenter) alignedPos.y() += -size.y() / 2;
	_imageBuffer->renderWindow(renderer, alignedPos, size);
}

}	// End of namespace
