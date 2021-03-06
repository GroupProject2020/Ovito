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
#include <ovito/core/rendering/ImagePrimitive.h>
#include "OpenGLTexture.h"
#include "OpenGLSceneRenderer.h"
#include "OpenGLBuffer.h"
#include <QOpenGLShaderProgram>

namespace Ovito {

/**
 * \brief Buffer object that stores an image to be rendered in the viewports.
 */
class OpenGLImagePrimitive : public ImagePrimitive
{
public:

	/// Constructor.
	OpenGLImagePrimitive(OpenGLSceneRenderer* renderer);

	/// \brief Sets the image to be rendered.
	virtual void setImage(const QImage& image) override {
		_needTextureUpdate = true;
		ImagePrimitive::setImage(image);
	}

	/// \brief Returns true if the geometry buffer is filled and can be rendered with the given renderer.
	virtual bool isValid(SceneRenderer* renderer) override;

	/// \brief Renders the image in a rectangle given in pixel coordinates.
	virtual void renderWindow(SceneRenderer* renderer, const Point2& pos, const Vector2& size) override;

	/// \brief Renders the image in a rectangle given in viewport coordinates.
	virtual void renderViewport(SceneRenderer* renderer, const Point2& pos, const Vector2& size) override;

private:

	/// Converts the QImage into the unnamed format expected by OpenGL functions such as glTexImage2D().
	static QImage convertToGLFormat(const QImage& img);

	/// The GL context group under which the GL vertex buffer has been created.
	QOpenGLContextGroup* _contextGroup;

	/// The OpenGL shader program used to render the image.
	QOpenGLShaderProgram* _shader;

	/// The OpenGL vertex buffer that stores the vertex positions.
	OpenGLBuffer<Point_3<GLfloat>> _vertexBuffer;

	/// The OpenGL texture that is used for rendering the image.
	OpenGLTexture _texture;

	/// Indicates that the texture needs to be updated.
	bool _needTextureUpdate = true;
};

}	// End of namespace
