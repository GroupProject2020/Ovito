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
#include "OpenGLArrowPrimitive.h"
#include "OpenGLSceneRenderer.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Rendering) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/******************************************************************************
* Constructor.
******************************************************************************/
OpenGLArrowPrimitive::OpenGLArrowPrimitive(OpenGLSceneRenderer* renderer, ArrowPrimitive::Shape shape, ShadingMode shadingMode, RenderingQuality renderingQuality, bool translucentElements) :
	ArrowPrimitive(shape, shadingMode, renderingQuality, translucentElements),
	_contextGroup(QOpenGLContextGroup::currentContextGroup()),
	_usingGeometryShader(renderer->useGeometryShaders())
{
	OVITO_ASSERT(renderer->glcontext()->shareGroup() == _contextGroup);

	// Initialize OpenGL shaders.
	if(shadingMode == NormalShading) {
		if(renderingQuality == HighQuality && shape == CylinderShape) {
			if(!_usingGeometryShader) {
				_shader = renderer->loadShaderProgram(
						"cylinder_raytraced",
						":/openglrenderer/glsl/cylinder/cylinder_raytraced_tri.vs",
						":/openglrenderer/glsl/cylinder/cylinder_raytraced.fs");
				_pickingShader = renderer->loadShaderProgram(
						"cylinder_raytraced_picking",
						":/openglrenderer/glsl/cylinder/picking/cylinder_raytraced_tri.vs",
						":/openglrenderer/glsl/cylinder/picking/cylinder_raytraced.fs");
			}
			else {
				_shader = renderer->loadShaderProgram(
						"cylinder_geomshader_raytraced",
						":/openglrenderer/glsl/cylinder/cylinder_raytraced.vs",
						":/openglrenderer/glsl/cylinder/cylinder_raytraced.fs",
						":/openglrenderer/glsl/cylinder/cylinder_raytraced.gs");
				_pickingShader = renderer->loadShaderProgram(
						"cylinder_geomshader_raytraced_picking",
						":/openglrenderer/glsl/cylinder/picking/cylinder_raytraced.vs",
						":/openglrenderer/glsl/cylinder/picking/cylinder_raytraced.fs",
						":/openglrenderer/glsl/cylinder/picking/cylinder_raytraced.gs");
			}
		}
		else {
			_shader = renderer->loadShaderProgram(
					"arrow_shaded",
					":/openglrenderer/glsl/arrows/shaded.vs",
					":/openglrenderer/glsl/arrows/shaded.fs");
			_pickingShader = renderer->loadShaderProgram(
					"arrow_shaded_picking",
					":/openglrenderer/glsl/arrows/picking/shaded.vs",
					":/openglrenderer/glsl/arrows/picking/shaded.fs");
		}
	}
	else if(shadingMode == FlatShading) {
		if(!_usingGeometryShader || shape != CylinderShape) {
			_shader = renderer->loadShaderProgram(
					"arrow_flat",
					":/openglrenderer/glsl/arrows/flat_tri.vs",
					":/openglrenderer/glsl/arrows/flat.fs");
			_pickingShader = renderer->loadShaderProgram(
					"arrow_flat_picking",
					":/openglrenderer/glsl/arrows/picking/flat_tri.vs",
					":/openglrenderer/glsl/arrows/picking/flat.fs");
		}
		else {
			_shader = renderer->loadShaderProgram(
					"cylinder_geomshader_flat",
					":/openglrenderer/glsl/arrows/flat.vs",
					":/openglrenderer/glsl/arrows/flat.fs",
					":/openglrenderer/glsl/cylinder/flat.gs");
			_pickingShader = renderer->loadShaderProgram(
					"cylinder_geomshader_flat_picking",
					":/openglrenderer/glsl/arrows/picking/flat.vs",
					":/openglrenderer/glsl/arrows/picking/flat.fs",
					":/openglrenderer/glsl/cylinder/picking/flat.gs");
		}
	}

	OVITO_ASSERT(_shader != nullptr);
	OVITO_ASSERT(_pickingShader != nullptr);
}

/******************************************************************************
* Allocates a particle buffer with the given number of elements.
******************************************************************************/
void OpenGLArrowPrimitive::startSetElements(int elementCount)
{
	OVITO_ASSERT(elementCount >= 0);
	OVITO_ASSERT(QOpenGLContextGroup::currentContextGroup() == _contextGroup);
	OVITO_ASSERT(_mappedChunkIndex == -1);
	_verticesWithNormals.clear();
	_verticesWithElementInfo.clear();

	_elementCount = elementCount;
	bool renderMesh = true;

	// Determine the number of triangle strips and triangle fans required to render N primitives.
	int stripsPerElement;
	int fansPerElement;
	int verticesPerStrip;
	int verticesPerFan;
	if(shadingMode() == NormalShading) {
		verticesPerStrip = _cylinderSegments * 2 + 2;
		verticesPerFan = _cylinderSegments;
		if(shape() == ArrowShape) {
			stripsPerElement = 2;
			fansPerElement = 2;
		}
		else {
			stripsPerElement = 1;
			fansPerElement = 2;
			if(renderingQuality() == HighQuality) {
				if(_usingGeometryShader) {
					verticesPerStrip = 1;
					stripsPerElement = 1;
				}
				else {
					verticesPerStrip = 14;
				}
				fansPerElement = verticesPerFan = 0;
				renderMesh = false;
			}
		}
	}
	else if(shadingMode() == FlatShading) {
		fansPerElement = 1;
		stripsPerElement = 0;
		verticesPerStrip = 0;
		if(shape() == ArrowShape)
			verticesPerFan = 7;
		else
			verticesPerFan = 4;
		if(_usingGeometryShader && shape() == CylinderShape) {
			verticesPerFan = 1;
		}
		renderMesh = false;
	}
	else OVITO_ASSERT(false);

	// Determine the VBO chunk size.
	_verticesPerElement = stripsPerElement * verticesPerStrip + fansPerElement * verticesPerFan;
	int bytesPerVertex = renderMesh ? sizeof(VertexWithNormal) : sizeof(VertexWithElementInfo);
	_chunkSize = std::min(_maxVBOSize / _verticesPerElement / bytesPerVertex, _elementCount);

	// Allocate VBOs.
	for(int i = _elementCount; i > 0; i -= _chunkSize) {
		if(renderMesh) {
			OpenGLBuffer<VertexWithNormal> buffer;
			buffer.create(QOpenGLBuffer::StaticDraw, std::min(i, _chunkSize), _verticesPerElement);
			_verticesWithNormals.push_back(buffer);
		}
		else {
			OpenGLBuffer<VertexWithElementInfo> buffer;
			buffer.create(QOpenGLBuffer::StaticDraw, std::min(i, _chunkSize), _verticesPerElement);
			_verticesWithElementInfo.push_back(buffer);
		}
	}

#ifndef Q_OS_WASM
	// Prepare arrays to be passed to the glMultiDrawArrays() function.
	_stripPrimitiveVertexStarts.resize(_chunkSize * stripsPerElement);
	_stripPrimitiveVertexCounts.resize(_chunkSize * stripsPerElement);
	_fanPrimitiveVertexStarts.resize(_chunkSize * fansPerElement);
	_fanPrimitiveVertexCounts.resize(_chunkSize * fansPerElement);
	std::fill(_stripPrimitiveVertexCounts.begin(), _stripPrimitiveVertexCounts.end(), verticesPerStrip);
	std::fill(_fanPrimitiveVertexCounts.begin(), _fanPrimitiveVertexCounts.end(), verticesPerFan);
	auto ps_strip = _stripPrimitiveVertexStarts.begin();
	auto ps_fan = _fanPrimitiveVertexStarts.begin();
	for(GLint index = 0, baseIndex = 0; index < _chunkSize; index++) {
		for(int p = 0; p < stripsPerElement; p++, baseIndex += verticesPerStrip)
			*ps_strip++ = baseIndex;
		for(int p = 0; p < fansPerElement; p++, baseIndex += verticesPerFan)
			*ps_fan++ = baseIndex;
	}
#else
	// Prepare list of vertex indices needed for glDrawElements() call.
	_indicesPerElement = 3 * (stripsPerElement * std::max(verticesPerStrip - 2, 0) + fansPerElement * std::max(verticesPerFan - 2, 0));
	_trianglePrimitiveVertexIndices.resize(_indicesPerElement * _chunkSize);
	auto pvi = _trianglePrimitiveVertexIndices.begin();
	for(GLuint index = 0, baseIndex = 0; index < _chunkSize; index++) {
		for(int p = 0; p < stripsPerElement; p++, baseIndex += verticesPerStrip) {
			for(int u = 2; u < verticesPerStrip; u++) {
				if((u & 1) == 0) {
					*pvi++ = baseIndex + u - 2;
					*pvi++ = baseIndex + u - 1;
					*pvi++ = baseIndex + u - 0;
				}
				else {
					*pvi++ = baseIndex + u - 0;
					*pvi++ = baseIndex + u - 1;
					*pvi++ = baseIndex + u - 2;
				}
			}
		}
		for(int p = 0; p < fansPerElement; p++, baseIndex += verticesPerFan) {
			for(int u = 2; u < verticesPerFan; u++) {
				*pvi++ = baseIndex;
				*pvi++ = baseIndex + u - 1;
				*pvi++ = baseIndex + u - 0;
			}
		}
	}
	OVITO_ASSERT(pvi == _trianglePrimitiveVertexIndices.end());
#endif	

	// Precompute cos() and sin() functions.
	if(shadingMode() == NormalShading) {
		_cosTable.resize(_cylinderSegments+1);
		_sinTable.resize(_cylinderSegments+1);
		for(int i = 0; i <= _cylinderSegments; i++) {
			float angle = (FLOATTYPE_PI * 2 / _cylinderSegments) * i;
			_cosTable[i] = std::cos(angle);
			_sinTable[i] = std::sin(angle);
		}
	}
}

/******************************************************************************
* Sets the properties of a single element.
******************************************************************************/
void OpenGLArrowPrimitive::setElement(int index, const Point3& pos, const Vector3& dir, const ColorA& color, FloatType width)
{
	OVITO_ASSERT(index >= 0 && index < _elementCount);

	int chunkIndex = index / _chunkSize;
	if(chunkIndex != _mappedChunkIndex) {
		if(!_verticesWithNormals.empty()) {
			if(_mappedChunkIndex != -1)
				_verticesWithNormals[_mappedChunkIndex].unmap();
			_mappedVerticesWithNormals = _verticesWithNormals[chunkIndex].map();
		}
		else if(!_verticesWithElementInfo.empty()) {
			if(_mappedChunkIndex != -1)
				_verticesWithElementInfo[_mappedChunkIndex].unmap();
			_mappedVerticesWithElementInfo = _verticesWithElementInfo[chunkIndex].map();
		}
		_mappedChunkIndex = chunkIndex;
	}

	int relativeIndex = index - _mappedChunkIndex * _chunkSize;
#ifdef FLOATTYPE_FLOAT
	if(shape() == ArrowShape)
		createArrowElement(relativeIndex, pos, dir, color, width);
	else
		createCylinderElement(relativeIndex, pos, dir, color, width);
#else
	if(shape() == ArrowShape)
		createArrowElement(relativeIndex, (Point_3<float>)pos, (Vector_3<float>)dir, (ColorAT<float>)color, (float)width);
	else
		createCylinderElement(relativeIndex, (Point_3<float>)pos, (Vector_3<float>)dir, (ColorAT<float>)color, (float)width);
#endif
}

/******************************************************************************
* Creates the geometry for a single cylinder element.
******************************************************************************/
void OpenGLArrowPrimitive::createCylinderElement(int index, const Point_3<float>& pos, const Vector_3<float>& dir, const ColorAT<float>& color, float width)
{
	if(_usingGeometryShader && (shadingMode() == FlatShading || renderingQuality() == HighQuality)) {
		OVITO_ASSERT(_mappedVerticesWithElementInfo);
		OVITO_ASSERT(_verticesPerElement == 1);
		VertexWithElementInfo* vertex = _mappedVerticesWithElementInfo + index;
		vertex->pos = vertex->base = (Point_3<float>)pos;
		vertex->dir = (Vector_3<float>)dir;
		vertex->color = (ColorAT<float>)color;
		vertex->radius = width;
		return;
	}

	if(shadingMode() == NormalShading) {

		// Build local coordinate system.
		Vector_3<float> t, u, v;
		float length = dir.length();
		if(length != 0) {
			t = dir / length;
			if(dir.y() != 0 || dir.x() != 0)
				u = Vector_3<float>(dir.y(), -dir.x(), 0);
			else
				u = Vector_3<float>(-dir.z(), 0, dir.x());
			u.normalize();
			v = u.cross(t);
		}
		else {
			t.setZero();
			u.setZero();
			v.setZero();
		}

		ColorAT<float> c = color;
		Point_3<float> v1 = pos;
		Point_3<float> v2 = v1 + dir;

		if(renderingQuality() != HighQuality) {
			OVITO_ASSERT(_mappedVerticesWithNormals);
			VertexWithNormal* vertex = _mappedVerticesWithNormals + (index * _verticesPerElement);

			// Generate vertices for cylinder mantle.
			for(int i = 0; i <= _cylinderSegments; i++) {
				Vector_3<float> n = _cosTable[i] * u + _sinTable[i] * v;
				Vector_3<float> d = n * width;
				vertex->pos = v1 + d;
				vertex->normal = n;
				vertex->color = c;
				vertex++;
				vertex->pos = v2 + d;
				vertex->normal = n;
				vertex->color = c;
				vertex++;
			}

			// Generate vertices for first cylinder cap.
			for(int i = 0; i < _cylinderSegments; i++) {
				Vector_3<float> n = _cosTable[i] * u + _sinTable[i] * v;
				Vector_3<float> d = n * width;
				vertex->pos = v1 + d;
				vertex->normal = Vector_3<float>(0,0,-1);
				vertex->color = c;
				vertex++;
			}

			// Generate vertices for second cylinder cap.
			for(int i = _cylinderSegments - 1; i >= 0; i--) {
				Vector_3<float> n = _cosTable[i] * u + _sinTable[i] * v;
				Vector_3<float> d = n * width;
				vertex->pos = v2 + d;
				vertex->normal = Vector_3<float>(0,0,1);
				vertex->color = c;
				vertex++;
			}
		}
		else {
			// Create bounding box geometry around cylinder for raytracing.
			OVITO_ASSERT(_mappedVerticesWithElementInfo);
			VertexWithElementInfo* vertex = _mappedVerticesWithElementInfo + (index * _verticesPerElement);
			OVITO_ASSERT(_verticesPerElement == 14);
			u *= width;
			v *= width;
			Point_3<float> corners[8] = {
					v1 - u - v,
					v1 - u + v,
					v1 + u - v,
					v1 + u + v,
					v2 - u - v,
					v2 - u + v,
					v2 + u + v,
					v2 + u - v
			};
			const static size_t stripIndices[14] = { 3,2,6,7,4,2,0,3,1,6,5,4,1,0 };
			for(int i = 0; i < 14; i++, vertex++) {
				vertex->pos = corners[stripIndices[i]];
				vertex->base = v1;
				vertex->dir = dir;
				vertex->color = c;
				vertex->radius = width;
			}
		}
	}
	else if(shadingMode() == FlatShading) {

		Vector_3<float> t;
		float length = dir.length();
		if(length != 0)
			t = dir / length;
		else
			t.setZero();

		ColorAT<float> c = color;
		Point_3<float> base = pos;

		OVITO_ASSERT(_mappedVerticesWithElementInfo);
		VertexWithElementInfo* vertices = _mappedVerticesWithElementInfo + (index * _verticesPerElement);
		vertices[0].pos = Point_3<float>(0, width, 0);
		vertices[1].pos = Point_3<float>(0, -width, 0);
		vertices[2].pos = Point_3<float>(length, -width, 0);
		vertices[3].pos = Point_3<float>(length, width, 0);
		for(int i = 0; i < _verticesPerElement; i++, ++vertices) {
			vertices->base = base;
			vertices->dir = t;
			vertices->color = c;
		}
	}
}

/******************************************************************************
* Creates the geometry for a single arrow element.
******************************************************************************/
void OpenGLArrowPrimitive::createArrowElement(int index, const Point_3<float>& pos, const Vector_3<float>& dir, const ColorAT<float>& color, float width)
{
	const float arrowHeadRadius = width * 2.5f;
	const float arrowHeadLength = arrowHeadRadius * 1.8f;

	if(shadingMode() == NormalShading) {

		// Build local coordinate system.
		Vector_3<float> t, u, v;
		float length = dir.length();
		if(length != 0) {
			t = dir / length;
			if(dir.y() != 0 || dir.x() != 0)
				u = Vector_3<float>(dir.y(), -dir.x(), 0);
			else
				u = Vector_3<float>(-dir.z(), 0, dir.x());
			u.normalize();
			v = u.cross(t);
		}
		else {
			t.setZero();
			u.setZero();
			v.setZero();
		}

		ColorAT<float> c = color;
		Point_3<float> v1 = pos;
		Point_3<float> v2;
		Point_3<float> v3 = v1 + dir;
		float r;
		if(length > arrowHeadLength) {
			v2 = v1 + t * (length - arrowHeadLength);
			r = arrowHeadRadius;
		}
		else {
			v2 = v1;
			r = arrowHeadRadius * length / arrowHeadLength;
		}

		OVITO_ASSERT(_mappedVerticesWithNormals);
		VertexWithNormal* vertex = _mappedVerticesWithNormals + (index * _verticesPerElement);

		// Generate vertices for cylinder.
		for(int i = 0; i <= _cylinderSegments; i++) {
			Vector_3<float> n = _cosTable[i] * u + _sinTable[i] * v;
			Vector_3<float> d = n * width;
			vertex->pos = v1 + d;
			vertex->normal = n;
			vertex->color = c;
			vertex++;
			vertex->pos = v2 + d;
			vertex->normal = n;
			vertex->color = c;
			vertex++;
		}

		// Generate vertices for head cone.
		for(int i = 0; i <= _cylinderSegments; i++) {
			Vector_3<float> n = _cosTable[i] * u + _sinTable[i] * v;
			Vector_3<float> d = n * r;
			vertex->pos = v2 + d;
			vertex->normal = n;
			vertex->color = c;
			vertex++;
			vertex->pos = v3;
			vertex->normal = n;
			vertex->color = c;
			vertex++;
		}

		// Generate vertices for cylinder cap.
		for(int i = 0; i < _cylinderSegments; i++) {
			Vector_3<float> n = _cosTable[i] * u + _sinTable[i] * v;
			Vector_3<float> d = n * width;
			vertex->pos = v1 + d;
			vertex->normal = Vector_3<float>(0,0,-1);
			vertex->color = c;
			vertex++;
		}

		// Generate vertices for cone cap.
		for(int i = 0; i < _cylinderSegments; i++) {
			Vector_3<float> n = _cosTable[i] * u + _sinTable[i] * v;
			Vector_3<float> d = n * r;
			vertex->pos = v2 + d;
			vertex->normal = Vector_3<float>(0,0,-1);
			vertex->color = c;
			vertex++;
		}
	}
	else if(shadingMode() == FlatShading) {

		Vector_3<float> t;
		float length = dir.length();
		if(length != 0)
			t = dir / length;
		else
			t.setZero();

		ColorAT<float> c = color;
		Point_3<float> base = pos;

		OVITO_ASSERT(_mappedVerticesWithElementInfo);
		VertexWithElementInfo* vertices = _mappedVerticesWithElementInfo + (index * _verticesPerElement);
		OVITO_ASSERT(_verticesPerElement == 7);

		if(length > arrowHeadLength) {
			vertices[0].pos = Point_3<float>(length, 0, 0);
			vertices[1].pos = Point_3<float>(length - arrowHeadLength, arrowHeadRadius, 0);
			vertices[2].pos = Point_3<float>(length - arrowHeadLength, width, 0);
			vertices[3].pos = Point_3<float>(0, width, 0);
			vertices[4].pos = Point_3<float>(0, -width, 0);
			vertices[5].pos = Point_3<float>(length - arrowHeadLength, -width, 0);
			vertices[6].pos = Point_3<float>(length - arrowHeadLength, -arrowHeadRadius, 0);
		}
		else {
			float r = arrowHeadRadius * length / arrowHeadLength;
			vertices[0].pos = Point_3<float>(length, 0, 0);
			vertices[1].pos = Point_3<float>(0, r, 0);
			vertices[2].pos = Point_3<float>::Origin();
			vertices[3].pos = Point_3<float>::Origin();
			vertices[4].pos = Point_3<float>::Origin();
			vertices[5].pos = Point_3<float>::Origin();
			vertices[6].pos = Point_3<float>(0, -r, 0);
		}
		for(int i = 0; i < _verticesPerElement; i++, ++vertices) {
			vertices->base = base;
			vertices->dir = t;
			vertices->color = c;
		}
	}
}

/******************************************************************************
* Finalizes the geometry buffer after all elements have been set.
******************************************************************************/
void OpenGLArrowPrimitive::endSetElements()
{
	OVITO_ASSERT(QOpenGLContextGroup::currentContextGroup() == _contextGroup);
	OVITO_ASSERT(_elementCount >= 0);

	if(_mappedChunkIndex != -1) {
		if(!_verticesWithNormals.empty())
			_verticesWithNormals[_mappedChunkIndex].unmap();
		if(!_verticesWithElementInfo.empty())
			_verticesWithElementInfo[_mappedChunkIndex].unmap();
	}
	_mappedVerticesWithNormals = nullptr;
	_mappedVerticesWithElementInfo = nullptr;
	_mappedChunkIndex = -1;
}

/******************************************************************************
* Returns true if the geometry buffer is filled and can be rendered with the given renderer.
******************************************************************************/
bool OpenGLArrowPrimitive::isValid(SceneRenderer* renderer)
{
	OpenGLSceneRenderer* vpRenderer = dynamic_object_cast<OpenGLSceneRenderer>(renderer);
	if(!vpRenderer) return false;
	return _elementCount >= 0 && (_contextGroup == vpRenderer->glcontext()->shareGroup());
}

/******************************************************************************
* Renders the geometry.
******************************************************************************/
void OpenGLArrowPrimitive::render(SceneRenderer* renderer)
{
	OVITO_ASSERT(_contextGroup == QOpenGLContextGroup::currentContextGroup());
	OVITO_ASSERT(_elementCount >= 0);
	OVITO_ASSERT(_mappedChunkIndex == -1);

	OpenGLSceneRenderer* vpRenderer = dynamic_object_cast<OpenGLSceneRenderer>(renderer);

	if(_elementCount <= 0 || !vpRenderer)
		return;
	OVITO_REPORT_OPENGL_ERRORS(vpRenderer);

	// If object is translucent, don't render it during the first rendering pass.
	// Queue primitive so that it gets rendered during the second pass.
	if(!renderer->isPicking() && translucentElements() && vpRenderer->translucentPass() == false) {
		vpRenderer->registerTranslucentPrimitive(shared_from_this());
		return;
	}

	vpRenderer->rebindVAO();

	// Activate blend mode when rendering translucent elements.
	if(!vpRenderer->isPicking() && translucentElements()) {
		vpRenderer->glEnable(GL_BLEND);
		vpRenderer->glBlendEquation(GL_FUNC_ADD);
		vpRenderer->glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE_MINUS_DST_COLOR, GL_ONE);
	}

	if(shadingMode() == NormalShading) {
		if(renderingQuality() == HighQuality && shape() == CylinderShape)
			renderWithElementInfo(vpRenderer);
		else
			renderWithNormals(vpRenderer);
	}
	else if(shadingMode() == FlatShading) {
		renderWithElementInfo(vpRenderer);
	}
	OVITO_REPORT_OPENGL_ERRORS(vpRenderer);

	// Deactivate blend mode after rendering translucent elements.
	if(!vpRenderer->isPicking() && translucentElements())
		vpRenderer->glDisable(GL_BLEND);
}

/******************************************************************************
* Renders the geometry as triangle mesh with normals.
******************************************************************************/
void OpenGLArrowPrimitive::renderWithNormals(OpenGLSceneRenderer* renderer)
{
	QOpenGLShaderProgram* shader = renderer->isPicking() ? _pickingShader : _shader;
	if(!shader->bind())
		renderer->throwException(QStringLiteral("Failed to bind OpenGL shader."));

	renderer->glEnable(GL_CULL_FACE);
	renderer->glCullFace(GL_BACK);

	shader->setUniformValue("modelview_projection_matrix", (QMatrix4x4)(renderer->projParams().projectionMatrix * renderer->modelViewTM()));
	if(!renderer->isPicking())
		shader->setUniformValue("normal_matrix", (QMatrix3x3)(renderer->modelViewTM().linear().inverse().transposed()));

	GLint pickingBaseID = 0;
	if(renderer->isPicking()) {
		pickingBaseID = renderer->registerSubObjectIDs(elementCount());
		renderer->activateVertexIDs(shader, _chunkSize * _verticesPerElement, true);
	}
	OVITO_REPORT_OPENGL_ERRORS(renderer);

	for(int chunkIndex = 0; chunkIndex < _verticesWithNormals.size(); chunkIndex++) {
		int chunkStart = chunkIndex * _chunkSize;
		int chunkSize = std::min(_elementCount - chunkStart, _chunkSize);

		if(renderer->isPicking()) {
			shader->setUniformValue("pickingBaseID", pickingBaseID);
			OVITO_REPORT_OPENGL_ERRORS(renderer);
			pickingBaseID += _chunkSize;
		}

		_verticesWithNormals[chunkIndex].bindPositions(renderer, shader, offsetof(VertexWithNormal, pos));
		if(!renderer->isPicking()) {
			_verticesWithNormals[chunkIndex].bindNormals(renderer, shader, offsetof(VertexWithNormal, normal));
			_verticesWithNormals[chunkIndex].bindColors(renderer, shader, 4, offsetof(VertexWithNormal, color));
		}

#ifndef Q_OS_WASM
		int stripPrimitivesPerElement = _stripPrimitiveVertexCounts.size() / _chunkSize;
		int stripVerticesPerElement = std::accumulate(_stripPrimitiveVertexCounts.begin(), _stripPrimitiveVertexCounts.begin() + stripPrimitivesPerElement, 0);
		OVITO_CHECK_OPENGL(renderer, shader->setUniformValue("verticesPerElement", (GLint)_verticesPerElement));
		OVITO_CHECK_OPENGL(renderer, renderer->glMultiDrawArrays(GL_TRIANGLE_STRIP, _stripPrimitiveVertexStarts.data(), _stripPrimitiveVertexCounts.data(), stripPrimitivesPerElement * chunkSize));

		int fanPrimitivesPerElement = _fanPrimitiveVertexCounts.size() / _chunkSize;
		int fanVerticesPerElement = std::accumulate(_fanPrimitiveVertexCounts.begin(), _fanPrimitiveVertexCounts.begin() + fanPrimitivesPerElement, 0);
		OVITO_CHECK_OPENGL(renderer, shader->setUniformValue("verticesPerElement", (GLint)_verticesPerElement));
		OVITO_CHECK_OPENGL(renderer, renderer->glMultiDrawArrays(GL_TRIANGLE_FAN, _fanPrimitiveVertexStarts.data(), _fanPrimitiveVertexCounts.data(), fanPrimitivesPerElement * chunkSize));
#else
		OVITO_CHECK_OPENGL(renderer, shader->setUniformValue("verticesPerElement", (GLint)_verticesPerElement));
		OVITO_CHECK_OPENGL(renderer, renderer->glDrawElements(GL_TRIANGLES, _indicesPerElement * chunkSize, GL_UNSIGNED_INT, _trianglePrimitiveVertexIndices.data()));
#endif

		_verticesWithNormals[chunkIndex].detachPositions(renderer, shader);
		if(!renderer->isPicking()) {
			_verticesWithNormals[chunkIndex].detachNormals(renderer, shader);
			_verticesWithNormals[chunkIndex].detachColors(renderer, shader);
		}
	}
	if(renderer->isPicking())
		renderer->deactivateVertexIDs(shader, true);

	shader->release();

	renderer->glDisable(GL_CULL_FACE);
}

/******************************************************************************
* Renders the geometry as with extra information passed to the vertex shader.
******************************************************************************/
void OpenGLArrowPrimitive::renderWithElementInfo(OpenGLSceneRenderer* renderer)
{
	QOpenGLShaderProgram* shader = renderer->isPicking() ? _pickingShader : _shader;
	if(!shader)
		return;
	if(!shader->bind())
		renderer->throwException(QStringLiteral("Failed to bind OpenGL shader."));

	renderer->glEnable(GL_CULL_FACE);
	renderer->glCullFace(GL_BACK);

	shader->setUniformValue("modelview_matrix",
			(QMatrix4x4)renderer->modelViewTM());
	shader->setUniformValue("modelview_uniform_scale", (float)pow(std::abs(renderer->modelViewTM().determinant()), (FloatType(1.0/3.0))));
	shader->setUniformValue("modelview_projection_matrix",
			(QMatrix4x4)(renderer->projParams().projectionMatrix * renderer->modelViewTM()));
	shader->setUniformValue("projection_matrix", (QMatrix4x4)renderer->projParams().projectionMatrix);
	shader->setUniformValue("inverse_projection_matrix", (QMatrix4x4)renderer->projParams().inverseProjectionMatrix);
	shader->setUniformValue("is_perspective", renderer->projParams().isPerspective);

	AffineTransformation viewModelTM = renderer->modelViewTM().inverse();
	Vector3 eye_pos = viewModelTM.translation();
	shader->setUniformValue("eye_pos", eye_pos.x(), eye_pos.y(), eye_pos.z());
	Vector3 viewDir = viewModelTM * Vector3(0,0,1);
	shader->setUniformValue("parallel_view_dir", viewDir.x(), viewDir.y(), viewDir.z());

	GLint viewportCoords[4];
	renderer->glGetIntegerv(GL_VIEWPORT, viewportCoords);
	shader->setUniformValue("viewport_origin", (float)viewportCoords[0], (float)viewportCoords[1]);
	shader->setUniformValue("inverse_viewport_size", 2.0f / (float)viewportCoords[2], 2.0f / (float)viewportCoords[3]);

	GLint pickingBaseID = 0;
	if(renderer->isPicking()) {
		pickingBaseID = renderer->registerSubObjectIDs(elementCount());
		renderer->activateVertexIDs(shader, _chunkSize * _verticesPerElement, true);
		OVITO_CHECK_OPENGL(renderer, shader->setUniformValue("verticesPerElement", (GLint)_verticesPerElement));
	}

	for(int chunkIndex = 0; chunkIndex < _verticesWithElementInfo.size(); chunkIndex++) {
		int chunkStart = chunkIndex * _chunkSize;
		int chunkSize = std::min(_elementCount - chunkStart, _chunkSize);

		if(renderer->isPicking()) {
			shader->setUniformValue("pickingBaseID", pickingBaseID);
			pickingBaseID += _chunkSize;
		}

		_verticesWithElementInfo[chunkIndex].bindPositions(renderer, shader, offsetof(VertexWithElementInfo, pos));
		_verticesWithElementInfo[chunkIndex].bind(renderer, shader, "cylinder_base", GL_FLOAT, offsetof(VertexWithElementInfo, base), 3, sizeof(VertexWithElementInfo));
		_verticesWithElementInfo[chunkIndex].bind(renderer, shader, "cylinder_axis", GL_FLOAT, offsetof(VertexWithElementInfo, dir), 3, sizeof(VertexWithElementInfo));
		_verticesWithElementInfo[chunkIndex].bind(renderer, shader, "cylinder_radius", GL_FLOAT, offsetof(VertexWithElementInfo, radius), 1, sizeof(VertexWithElementInfo));
		if(!renderer->isPicking())
			_verticesWithElementInfo[chunkIndex].bindColors(renderer, shader, 4, offsetof(VertexWithElementInfo, color));

		if(_usingGeometryShader && (shadingMode() == FlatShading || renderingQuality() == HighQuality) && shape() == CylinderShape) {
			OVITO_CHECK_OPENGL(renderer, renderer->glDrawArrays(GL_POINTS, 0, chunkSize));
		}
		else {
#ifndef Q_OS_WASM
			int stripPrimitivesPerElement = _stripPrimitiveVertexCounts.size() / _chunkSize;
			OVITO_CHECK_OPENGL(renderer, renderer->glMultiDrawArrays(GL_TRIANGLE_STRIP, _stripPrimitiveVertexStarts.data(), _stripPrimitiveVertexCounts.data(), stripPrimitivesPerElement * chunkSize));

			int fanPrimitivesPerElement = _fanPrimitiveVertexCounts.size() / _chunkSize;
			OVITO_CHECK_OPENGL(renderer, renderer->glMultiDrawArrays(GL_TRIANGLE_FAN, _fanPrimitiveVertexStarts.data(), _fanPrimitiveVertexCounts.data(), fanPrimitivesPerElement * chunkSize));
#else
			OVITO_CHECK_OPENGL(renderer, shader->setUniformValue("verticesPerElement", (GLint)_verticesPerElement));
			OVITO_CHECK_OPENGL(renderer, renderer->glDrawElements(GL_TRIANGLES, _indicesPerElement * chunkSize, GL_UNSIGNED_INT, _trianglePrimitiveVertexIndices.data()));
#endif			
		}

		_verticesWithElementInfo[chunkIndex].detachPositions(renderer, shader);
		_verticesWithElementInfo[chunkIndex].detach(renderer, shader, "cylinder_base");
		_verticesWithElementInfo[chunkIndex].detach(renderer, shader, "cylinder_axis");
		_verticesWithElementInfo[chunkIndex].detach(renderer, shader, "cylinder_radius");
		if(!renderer->isPicking())
			_verticesWithElementInfo[chunkIndex].detachColors(renderer, shader);
	}

	if(renderer->isPicking())
		renderer->deactivateVertexIDs(shader, true);

	shader->release();

	renderer->glDisable(GL_CULL_FACE);
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
