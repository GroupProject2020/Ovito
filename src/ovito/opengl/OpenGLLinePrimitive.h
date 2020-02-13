////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2013 Alexander Stukowski
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
#include <ovito/core/rendering/LinePrimitive.h>
#include "OpenGLBuffer.h"

namespace Ovito {

/**
 * \brief This class is responsible for rendering line primitives using OpenGL.
 */
class OpenGLLinePrimitive : public LinePrimitive
{
public:

	/// Constructor.
	OpenGLLinePrimitive(OpenGLSceneRenderer* renderer);

	/// \brief Allocates a geometry buffer with the given number of vertices.
	virtual void setVertexCount(int vertexCount, FloatType lineWidth) override;

	/// \brief Returns the number of vertices stored in the buffer.
	virtual int vertexCount() const override { return _positionsBuffer.elementCount(); }

	/// \brief Sets the coordinates of the vertices.
	virtual void setVertexPositions(const Point3* coordinates) override;

	/// \brief Sets the colors of the vertices.
	virtual void setVertexColors(const ColorA* colors) override;

	/// \brief Sets the color of all vertices to the given value.
	virtual void setLineColor(const ColorA color) override;

	/// \brief Returns true if the geometry buffer is filled and can be rendered with the given renderer.
	virtual bool isValid(SceneRenderer* renderer) override;

	/// \brief Renders the geometry.
	virtual void render(SceneRenderer* renderer) override;

protected:

	/// \brief Renders the lines using GL_LINES mode.
	void renderLines(OpenGLSceneRenderer* renderer);

	/// \brief Renders the lines using polygons.
	void renderThickLines(OpenGLSceneRenderer* renderer);

private:

	/// The internal OpenGL vertex buffer that stores the vertex positions.
	OpenGLBuffer<Point_3<float>> _positionsBuffer;

	/// The internal OpenGL vertex buffer that stores the vertex colors.
	OpenGLBuffer<ColorAT<float>> _colorsBuffer;

	/// The internal OpenGL vertex buffer that stores the line segment vectors.
	OpenGLBuffer<Vector_3<float>> _vectorsBuffer;

	/// The internal OpenGL vertex buffer that stores the indices for a call to glDrawElements().
	OpenGLBuffer<GLuint> _indicesBuffer;

	/// The client-side buffer that stores the indices for a call to glDrawElements().
	std::vector<GLuint> _indicesBufferClient;

	/// The GL context group under which the GL vertex buffer has been created.
	QOpenGLContextGroup* _contextGroup;

	/// The OpenGL shader program used to render the lines.
	QOpenGLShaderProgram* _shader;

	/// The OpenGL shader program used to render the lines in picking mode.
	QOpenGLShaderProgram* _pickingShader;

	/// The OpenGL shader program used to render thick lines.
	QOpenGLShaderProgram* _thickLineShader;

	/// The OpenGL shader program used to render thick lines in picking mode.
	QOpenGLShaderProgram* _thickLinePickingShader;

	/// The width of lines in screen space.
	FloatType _lineWidth;

	/// Indicates that an index VBO is used.
	bool _useIndexVBO;
};

}	// End of namespace


