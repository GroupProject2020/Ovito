///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2016) Alexander Stukowski
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
#include "OpenGLMarkerPrimitive.h"
#include "OpenGLSceneRenderer.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Rendering) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/******************************************************************************
* Constructor.
******************************************************************************/
OpenGLMarkerPrimitive::OpenGLMarkerPrimitive(OpenGLSceneRenderer* renderer, MarkerShape shape) :
	MarkerPrimitive(shape),
	_contextGroup(QOpenGLContextGroup::currentContextGroup()),
	_shader(nullptr), _pickingShader(nullptr),
	_markerCount(-1)
{
	OVITO_ASSERT(renderer->glcontext()->shareGroup() == _contextGroup);

	// Initialize OpenGL shaders.
	if(shape == BoxShape) {
		_shader = renderer->loadShaderProgram("box_marker",
				":/openglrenderer/glsl/markers/box_lines.vs",
				":/openglrenderer/glsl/markers/marker.fs");
		_pickingShader = renderer->loadShaderProgram("box_marker_picking",
				":/openglrenderer/glsl/markers/picking/box_lines.vs",
				":/openglrenderer/glsl/markers/picking/marker.fs");	
	}
	else {
		_shader = renderer->loadShaderProgram("dot_marker", 
				":/openglrenderer/glsl/markers/marker.vs", 
				":/openglrenderer/glsl/markers/marker.fs");
		_pickingShader = renderer->loadShaderProgram("dot_marker_picking", 
				":/openglrenderer/glsl/markers/picking/marker.vs", 
				":/openglrenderer/glsl/markers/picking/marker.fs");
	}
}

/******************************************************************************
* Allocates the vertex buffer with the given number of markers.
******************************************************************************/
void OpenGLMarkerPrimitive::setCount(int markerCount)
{
	OVITO_ASSERT(QOpenGLContextGroup::currentContextGroup() == _contextGroup);

	_markerCount = markerCount;

	int verticesPerMarker = 1;
	if(markerShape() == BoxShape)
		verticesPerMarker = 24;

	// Allocate VBOs.
	_positionBuffer.create(QOpenGLBuffer::StaticDraw, _markerCount, verticesPerMarker);
	_colorBuffer.create(QOpenGLBuffer::StaticDraw, _markerCount, verticesPerMarker);
}

/******************************************************************************
* Sets the coordinates of the markers.
******************************************************************************/
void OpenGLMarkerPrimitive::setMarkerPositions(const Point3* coordinates)
{
	OVITO_ASSERT(QOpenGLContextGroup::currentContextGroup() == _contextGroup);
	_positionBuffer.fill(coordinates);
}

/******************************************************************************
* Sets the color of all markers to the given value.
******************************************************************************/
void OpenGLMarkerPrimitive::setMarkerColor(const ColorA color)
{
	OVITO_ASSERT(QOpenGLContextGroup::currentContextGroup() == _contextGroup);
	_colorBuffer.fillConstant(color);
}

/******************************************************************************
* Returns true if the geometry buffer is filled and can be rendered with the given renderer.
******************************************************************************/
bool OpenGLMarkerPrimitive::isValid(SceneRenderer* renderer)
{
	OpenGLSceneRenderer* vpRenderer = dynamic_object_cast<OpenGLSceneRenderer>(renderer);
	if(!vpRenderer) return false;
	return (_markerCount >= 0) && (_contextGroup == vpRenderer->glcontext()->shareGroup());
}

/******************************************************************************
* Renders the geometry.
******************************************************************************/
void OpenGLMarkerPrimitive::render(SceneRenderer* renderer)
{
    OVITO_REPORT_OPENGL_ERRORS();
	OVITO_ASSERT(_contextGroup == QOpenGLContextGroup::currentContextGroup());

	OpenGLSceneRenderer* vpRenderer = dynamic_object_cast<OpenGLSceneRenderer>(renderer);

	if(markerCount() <= 0 || !vpRenderer)
		return;

	vpRenderer->rebindVAO();

	// Pick the right OpenGL shader program.
	QOpenGLShaderProgram* shader = vpRenderer->isPicking() ? _pickingShader : _shader;
	if(!shader->bind())
		vpRenderer->throwException(QStringLiteral("Failed to bind OpenGL shader program."));

	if(markerShape() == DotShape) {
		OVITO_ASSERT(_positionBuffer.verticesPerElement() == 1);
		OVITO_CHECK_OPENGL(vpRenderer->glPointSize(3));
	}

	_positionBuffer.bindPositions(vpRenderer, shader);
	if(!renderer->isPicking()) {
		_colorBuffer.bindColors(vpRenderer, shader, 4);
	}
	else {
		GLint pickingBaseID = vpRenderer->registerSubObjectIDs(markerCount());
		vpRenderer->activateVertexIDs(shader, _positionBuffer.elementCount() * _positionBuffer.verticesPerElement());
		shader->setUniformValue("pickingBaseID", pickingBaseID);
	}

	if(markerShape() == DotShape) {
		OVITO_CHECK_OPENGL(shader->setUniformValue("modelview_projection_matrix", (QMatrix4x4)(vpRenderer->projParams().projectionMatrix * vpRenderer->modelViewTM())));	
		OVITO_CHECK_OPENGL(glDrawArrays(GL_POINTS, 0, markerCount()));
	}
	else if(markerShape() == BoxShape) {

		shader->setUniformValue("projection_matrix", (QMatrix4x4)renderer->projParams().projectionMatrix);
		shader->setUniformValue("viewprojection_matrix", (QMatrix4x4)(renderer->projParams().projectionMatrix * renderer->projParams().viewMatrix));
		shader->setUniformValue("model_matrix", (QMatrix4x4)vpRenderer->worldTransform());
		shader->setUniformValue("modelview_matrix", (QMatrix4x4)vpRenderer->modelViewTM());

		GLint viewportCoords[4];
		vpRenderer->glGetIntegerv(GL_VIEWPORT, viewportCoords);
		shader->setUniformValue("marker_size", 4.0f / viewportCoords[3]);
		
		static const QVector3D cubeVerts[] = {
				{-1, -1, -1}, { 1,-1,-1},
				{-1, -1,  1}, { 1,-1, 1},
				{-1, -1, -1}, {-1,-1, 1},
				{ 1, -1, -1}, { 1,-1, 1},
				{-1,  1, -1}, { 1, 1,-1},
				{-1,  1,  1}, { 1, 1, 1},
				{-1,  1, -1}, {-1, 1, 1},
				{ 1,  1, -1}, { 1, 1, 1},
				{-1, -1, -1}, {-1, 1,-1},
				{ 1, -1, -1}, { 1, 1,-1},
				{ 1, -1,  1}, { 1, 1, 1},
				{-1, -1,  1}, {-1, 1, 1}
		};
		OVITO_CHECK_OPENGL(shader->setUniformValueArray("cubeVerts", cubeVerts, 24));
		
		OVITO_CHECK_OPENGL(glDrawArrays(GL_LINES, 0, _positionBuffer.elementCount() * _positionBuffer.verticesPerElement()));
	}

	_positionBuffer.detachPositions(vpRenderer, shader);
	if(!renderer->isPicking()) {
		_colorBuffer.detachColors(vpRenderer, shader);
	}
	else {
		vpRenderer->deactivateVertexIDs(shader);
	}

	shader->release();
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
