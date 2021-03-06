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
#include "OpenGLLinePrimitive.h"
#include "OpenGLSceneRenderer.h"

namespace Ovito {

/******************************************************************************
* Constructor.
******************************************************************************/
OpenGLLinePrimitive::OpenGLLinePrimitive(OpenGLSceneRenderer* renderer) :
	_contextGroup(QOpenGLContextGroup::currentContextGroup()),
	_indicesBuffer(QOpenGLBuffer::IndexBuffer)
{
	OVITO_ASSERT(renderer->glcontext()->shareGroup() == _contextGroup);

	// Initialize OpenGL shaders.
	_shader = renderer->loadShaderProgram("line", ":/openglrenderer/glsl/lines/line.vs", ":/openglrenderer/glsl/lines/line.fs");
	_pickingShader = renderer->loadShaderProgram("line.picking", ":/openglrenderer/glsl/lines/picking/line.vs", ":/openglrenderer/glsl/lines/picking/line.fs");
	_thickLineShader = renderer->loadShaderProgram("thick_line", ":/openglrenderer/glsl/lines/thick_line.vs", ":/openglrenderer/glsl/lines/line.fs");
	_thickLinePickingShader = renderer->loadShaderProgram("thick_line.picking", ":/openglrenderer/glsl/lines/picking/thick_line.vs", ":/openglrenderer/glsl/lines/picking/line.fs");

	// Use VBO to store glDrawElements() indices only on a real core profile implementation.
	_useIndexVBO = (renderer->glformat().profile() == QSurfaceFormat::CoreProfile);

	// Standard line width.
	_lineWidth = renderer->devicePixelRatio();
}

/******************************************************************************
* Allocates a vertex buffer with the given number of vertices.
******************************************************************************/
void OpenGLLinePrimitive::setVertexCount(int vertexCount, FloatType lineWidth)
{
	OVITO_ASSERT(vertexCount >= 0);
	OVITO_ASSERT((vertexCount & 1) == 0);
	OVITO_ASSERT(vertexCount < std::numeric_limits<int>::max() / sizeof(ColorAT<float>));
	OVITO_ASSERT(QOpenGLContextGroup::currentContextGroup() == _contextGroup);
	OVITO_ASSERT(lineWidth >= 0);

	if(lineWidth != 0)
		_lineWidth = lineWidth;

	if(_lineWidth == 1) {
		_positionsBuffer.create(QOpenGLBuffer::StaticDraw, vertexCount);
		_colorsBuffer.create(QOpenGLBuffer::StaticDraw, vertexCount);
	}
	else {
		_positionsBuffer.create(QOpenGLBuffer::StaticDraw, vertexCount, 2);
		_colorsBuffer.create(QOpenGLBuffer::StaticDraw, vertexCount, 2);
		_vectorsBuffer.create(QOpenGLBuffer::StaticDraw, vertexCount, 2);
		GLuint* indices;
		if(_useIndexVBO) {
			_indicesBuffer.create(QOpenGLBuffer::StaticDraw, vertexCount * 6 / 2);
			indices = _indicesBuffer.map();
		}
		else {
			_indicesBufferClient.resize(vertexCount * 6 / 2);
			indices = _indicesBufferClient.data();
		}
		for(int i = 0; i < vertexCount; i += 2, indices += 6) {
			indices[0] = i * 2;
			indices[1] = i * 2 + 1;
			indices[2] = i * 2 + 2;
			indices[3] = i * 2;
			indices[4] = i * 2 + 2;
			indices[5] = i * 2 + 3;
		}
		if(_useIndexVBO) _indicesBuffer.unmap();
	}
}

/******************************************************************************
* Sets the coordinates of the vertices.
******************************************************************************/
void OpenGLLinePrimitive::setVertexPositions(const Point3* coordinates)
{
	OVITO_ASSERT(QOpenGLContextGroup::currentContextGroup() == _contextGroup);
	_positionsBuffer.fill(coordinates);

	if(_lineWidth != 1) {
		Vector_3<float>* vectors = _vectorsBuffer.map();
		Vector_3<float>* vectors_end = vectors + _vectorsBuffer.elementCount() * _vectorsBuffer.verticesPerElement();
		for(; vectors != vectors_end; vectors += 4, coordinates += 2) {
			vectors[3] = vectors[0] = (Vector_3<float>)(coordinates[1] - coordinates[0]);
			vectors[1] = vectors[2] = -vectors[0];
		}
		_vectorsBuffer.unmap();
	}
}

/******************************************************************************
* Sets the colors of the vertices.
******************************************************************************/
void OpenGLLinePrimitive::setVertexColors(const ColorA* colors)
{
	OVITO_ASSERT(QOpenGLContextGroup::currentContextGroup() == _contextGroup);
	_colorsBuffer.fill(colors);
}

/******************************************************************************
* Sets the color of all vertices to the given value.
******************************************************************************/
void OpenGLLinePrimitive::setLineColor(const ColorA color)
{
	OVITO_ASSERT(QOpenGLContextGroup::currentContextGroup() == _contextGroup);
	_colorsBuffer.fillConstant(color);
}

/******************************************************************************
* Returns true if the geometry buffer is filled and can be rendered with the given renderer.
******************************************************************************/
bool OpenGLLinePrimitive::isValid(SceneRenderer* renderer)
{
	OpenGLSceneRenderer* vpRenderer = qobject_cast<OpenGLSceneRenderer*>(renderer);
	if(!vpRenderer) return false;
	return _positionsBuffer.isCreated() && (_contextGroup == vpRenderer->glcontext()->shareGroup());
}

/******************************************************************************
* Renders the geometry.
******************************************************************************/
void OpenGLLinePrimitive::render(SceneRenderer* renderer)
{
	OVITO_ASSERT(_contextGroup == QOpenGLContextGroup::currentContextGroup());
	OpenGLSceneRenderer* vpRenderer = dynamic_object_cast<OpenGLSceneRenderer>(renderer);

	if(vertexCount() <= 0 || !vpRenderer)
		return;

	vpRenderer->rebindVAO();

	if(_lineWidth == 1)
		renderLines(vpRenderer);
	else
		renderThickLines(vpRenderer);
}

/******************************************************************************
* Renders the lines using GL_LINES mode.
******************************************************************************/
void OpenGLLinePrimitive::renderLines(OpenGLSceneRenderer* renderer)
{
	QOpenGLShaderProgram* shader;
	if(!renderer->isPicking())
		shader = _shader;
	else
		shader = _pickingShader;

	if(!shader->bind()) {
		OVITO_ASSERT(false);
		renderer->throwException(QStringLiteral("Failed to bind OpenGL shader."));
	}

	OVITO_CHECK_OPENGL(renderer, shader->setUniformValue("modelview_projection_matrix",
			(QMatrix4x4)(renderer->projParams().projectionMatrix * renderer->modelViewTM())));

	_positionsBuffer.bindPositions(renderer, shader);
	if(!renderer->isPicking()) {
		_colorsBuffer.bindColors(renderer, shader, 4);
	}
	else {
		OVITO_REPORT_OPENGL_ERRORS(renderer);
		shader->setUniformValue("pickingBaseID", (GLint)renderer->registerSubObjectIDs(vertexCount() / 2));
		OVITO_REPORT_OPENGL_ERRORS(renderer);
		renderer->activateVertexIDs(shader, _positionsBuffer.elementCount() * _positionsBuffer.verticesPerElement());
	}

	OVITO_REPORT_OPENGL_ERRORS(renderer);
	OVITO_CHECK_OPENGL(renderer, renderer->glDrawArrays(GL_LINES, 0, _positionsBuffer.elementCount() * _positionsBuffer.verticesPerElement()));

	_positionsBuffer.detachPositions(renderer, shader);
	if(!renderer->isPicking()) {
		_colorsBuffer.detachColors(renderer, shader);
	}
	else {
		renderer->deactivateVertexIDs(shader);
	}
	shader->release();

	OVITO_REPORT_OPENGL_ERRORS(renderer);
}

/******************************************************************************
* Renders the lines using polygons.
******************************************************************************/
void OpenGLLinePrimitive::renderThickLines(OpenGLSceneRenderer* renderer)
{
	QOpenGLShaderProgram* shader;
	if(!renderer->isPicking())
		shader = _thickLineShader;
	else
		shader = _thickLinePickingShader;

	if(!shader->bind()) {
		OVITO_ASSERT(false);
		renderer->throwException(QStringLiteral("Failed to bind OpenGL shader."));
	}

	OVITO_CHECK_OPENGL(renderer, shader->setUniformValue("modelview_matrix", (QMatrix4x4)renderer->modelViewTM()));
	OVITO_CHECK_OPENGL(renderer, shader->setUniformValue("projection_matrix", (QMatrix4x4)renderer->projParams().projectionMatrix));

	_positionsBuffer.bindPositions(renderer, shader);
	if(!renderer->isPicking()) {
		_colorsBuffer.bindColors(renderer, shader, 4);
	}
	else {
		OVITO_REPORT_OPENGL_ERRORS(renderer);
		shader->setUniformValue("pickingBaseID", (GLint)renderer->registerSubObjectIDs(vertexCount() / 2));
		OVITO_REPORT_OPENGL_ERRORS(renderer);
		renderer->activateVertexIDs(shader, _positionsBuffer.elementCount() * _positionsBuffer.verticesPerElement());
	}

	GLint viewportCoords[4];
	renderer->glGetIntegerv(GL_VIEWPORT, viewportCoords);
	FloatType param = renderer->projParams().projectionMatrix(1,1) * viewportCoords[3];
	shader->setUniformValue("line_width", GLfloat(_lineWidth / param));
	shader->setUniformValue("is_perspective", renderer->projParams().isPerspective);

	OVITO_REPORT_OPENGL_ERRORS(renderer);
	_vectorsBuffer.bind(renderer, shader, "vector", GL_FLOAT, 0, 3);

	if(_useIndexVBO) {
		_indicesBuffer.oglBuffer().bind();
		OVITO_CHECK_OPENGL(renderer, renderer->glDrawElements(GL_TRIANGLES, _indicesBuffer.elementCount(), GL_UNSIGNED_INT, nullptr));
		_indicesBuffer.oglBuffer().release();
	}
	else {
		OVITO_CHECK_OPENGL(renderer, renderer->glDrawElements(GL_TRIANGLES, _indicesBufferClient.size(), GL_UNSIGNED_INT, _indicesBufferClient.data()));
	}

	_positionsBuffer.detachPositions(renderer, shader);
	if(!renderer->isPicking()) {
		_colorsBuffer.detachColors(renderer, shader);
	}
	else {
		renderer->deactivateVertexIDs(shader);
	}
	_vectorsBuffer.detach(renderer, shader, "vector");
	shader->release();

	OVITO_REPORT_OPENGL_ERRORS(renderer);
}

}	// End of namespace
