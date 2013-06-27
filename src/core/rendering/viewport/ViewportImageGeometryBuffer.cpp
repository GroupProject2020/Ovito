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

#include <core/Core.h>
#include "ViewportImageGeometryBuffer.h"
#include "ViewportSceneRenderer.h"

#include <QGLWidget>

namespace Ovito {

IMPLEMENT_OVITO_OBJECT(Core, ViewportImageGeometryBuffer, ImageGeometryBuffer);

/******************************************************************************
* Constructor.
******************************************************************************/
ViewportImageGeometryBuffer::ViewportImageGeometryBuffer(ViewportSceneRenderer* renderer) :
	_renderer(renderer),
	_contextGroup(QOpenGLContextGroup::currentContextGroup()),
	_texture(0),
	_needTextureUpdate(true)
{
	OVITO_ASSERT(renderer->glcontext()->shareGroup() == _contextGroup);

	// Initialize OpenGL shader.
	_shader = renderer->loadShaderProgram("image", ":/core/glsl/image.vertex.glsl", ":/core/glsl/image.fragment.glsl");

	// Create OpenGL texture.
	glGenTextures(1, &_texture);

	// Make sure texture gets deleted again when this object is destroyed.
	attachOpenGLResources();
}

/******************************************************************************
* Destructor.
******************************************************************************/
ViewportImageGeometryBuffer::~ViewportImageGeometryBuffer()
{
	destroyOpenGLResources();
}

/******************************************************************************
* This method that takes care of freeing the shared OpenGL resources owned
* by this class.
******************************************************************************/
void ViewportImageGeometryBuffer::freeOpenGLResources()
{
	OVITO_CHECK_OPENGL(glDeleteTextures(1, &_texture));
	_texture = 0;
}

/******************************************************************************
* Returns true if the buffer is filled and can be rendered with the given renderer.
******************************************************************************/
bool ViewportImageGeometryBuffer::isValid(SceneRenderer* renderer)
{
	ViewportSceneRenderer* vpRenderer = qobject_cast<ViewportSceneRenderer*>(renderer);
	if(!vpRenderer) return false;
	return (_contextGroup == vpRenderer->glcontext()->shareGroup()) && (_texture != 0);
}

/******************************************************************************
* Renders the image in a rectangle given in window coordinates.
******************************************************************************/
void ViewportImageGeometryBuffer::renderWindow(const Point2& pos, const Vector2& size)
{
	GLint vc[4];
	glGetIntegerv(GL_VIEWPORT, vc);

	// Transform rectangle to normalized device coordinates.
	renderViewport(Point2(pos.x() / vc[2] * 2 - 1, 1 - (pos.y() + size.y()) / vc[3] * 2),
		Vector2(size.x() / vc[2] * 2, size.y() / vc[3] * 2));
}

/******************************************************************************
* Renders the image in a rectangle given in viewport coordinates.
******************************************************************************/
void ViewportImageGeometryBuffer::renderViewport(const Point2& pos, const Vector2& size)
{
	OVITO_ASSERT(_contextGroup == QOpenGLContextGroup::currentContextGroup());
	OVITO_ASSERT(_texture != 0);

	if(image().isNull())
		return;

	// Prepare texture.
	OVITO_CHECK_OPENGL(glBindTexture(GL_TEXTURE_2D, _texture));
	renderer()->glfuncs()->glActiveTexture(GL_TEXTURE0);

	if(_needTextureUpdate) {
		_needTextureUpdate = false;

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

		// Upload texture data.
		QImage textureImage = QGLWidget::convertToGLFormat(image());
		OVITO_CHECK_OPENGL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textureImage.width(), textureImage.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, textureImage.constBits()));
	}

	// Transform rectangle to normalized device coordinates.
	QVector2D corners[4] = {
			QVector2D(pos.x(), pos.y()),
			QVector2D(pos.x() + size.x(), pos.y()),
			QVector2D(pos.x(), pos.y() + size.y()),
			QVector2D(pos.x() + size.x(), pos.y() + size.y())
	};

	bool wasDepthTestEnabled = glIsEnabled(GL_DEPTH_TEST);
	bool wasBlendEnabled = glIsEnabled(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	if(!_shader->bind())
		throw Exception(tr("Failed to bind OpenGL shader."));

	_shader->setUniformValueArray("corners", corners, 4);

	OVITO_CHECK_OPENGL(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));

	_shader->release();

	// Restore old state.
	if(wasDepthTestEnabled) glEnable(GL_DEPTH_TEST);
	if(!wasBlendEnabled) glDisable(GL_BLEND);
}

};