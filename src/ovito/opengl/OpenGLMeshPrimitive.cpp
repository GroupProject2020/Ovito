////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2017 Alexander Stukowski
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
#include "OpenGLMeshPrimitive.h"
#include "OpenGLSceneRenderer.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Rendering) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/******************************************************************************
* Constructor.
******************************************************************************/
OpenGLMeshPrimitive::OpenGLMeshPrimitive(OpenGLSceneRenderer* renderer) :
	_contextGroup(QOpenGLContextGroup::currentContextGroup())
{
	OVITO_ASSERT(renderer->glcontext()->shareGroup() == _contextGroup);

	// Initialize OpenGL shader.
	_shader = renderer->loadShaderProgram("mesh", ":/openglrenderer/glsl/mesh/mesh.vs", ":/openglrenderer/glsl/mesh/mesh.fs");
	_pickingShader = renderer->loadShaderProgram("mesh.picking", ":/openglrenderer/glsl/mesh/picking/mesh.vs", ":/openglrenderer/glsl/mesh/picking/mesh.fs");
	_lineShader = renderer->loadShaderProgram("wireframe_line", ":/openglrenderer/glsl/lines/line.vs", ":/openglrenderer/glsl/lines/line.fs");
}

/******************************************************************************
* Sets the mesh to be stored in this buffer object.
******************************************************************************/
void OpenGLMeshPrimitive::setMesh(const TriMesh& mesh, const ColorA& meshColor, bool emphasizeEdges)
{
	OVITO_ASSERT(QOpenGLContextGroup::currentContextGroup() == _contextGroup);

	// Allocate render vertex buffer.
	_vertexBuffer.create(QOpenGLBuffer::StaticDraw, mesh.faceCount(), 3);
	if((mesh.hasVertexColors() || mesh.hasFaceColors()) && meshColor.a() == 1.0)
		_alpha = 1.0;
	else {
		if(materialColors().empty()) {
			_alpha = meshColor.a();
		}
		else {
			_alpha = 1.0;
			for(const ColorA& c : materialColors()) {
				if(c.a() != 1.0) {
					_alpha = c.a();
					break;
				}
			}
		}
	}

	// Discard any previous polygon edges.
	_edgeLinesBuffer.destroy();

	if(mesh.faceCount() == 0)
		return;

	ColoredVertexWithNormal* renderVertices = _vertexBuffer.map(QOpenGLBuffer::ReadWrite);

	if(!mesh.hasNormals()) {
		quint32 allMask = 0;

		// Compute face normals.
		std::vector<Vector_3<float>> faceNormals(mesh.faceCount());
		auto faceNormal = faceNormals.begin();
		for(auto face = mesh.faces().constBegin(); face != mesh.faces().constEnd(); ++face, ++faceNormal) {
			const Point3& p0 = mesh.vertex(face->vertex(0));
			Vector3 d1 = mesh.vertex(face->vertex(1)) - p0;
			Vector3 d2 = mesh.vertex(face->vertex(2)) - p0;
			*faceNormal = static_cast<Vector_3<float>>(d1.cross(d2));
			if(*faceNormal != Vector_3<float>::Zero()) {
				//faceNormal->normalize();
				allMask |= face->smoothingGroups();
			}
		}

		// Initialize render vertices.
		ColoredVertexWithNormal* rv = renderVertices;
		faceNormal = faceNormals.begin();
		ColorAT<float> defaultVertexColor = static_cast<ColorAT<float>>(meshColor);
		for(auto face = mesh.faces().constBegin(); face != mesh.faces().constEnd(); ++face, ++faceNormal) {

			// Initialize render vertices for this face.
			for(size_t v = 0; v < 3; v++, rv++) {
				if(face->smoothingGroups())
					rv->normal = Vector_3<float>::Zero();
				else
					rv->normal = *faceNormal;
				rv->pos = static_cast<Point_3<float>>(mesh.vertex(face->vertex(v)));
				if(mesh.hasVertexColors()) {
					rv->color = static_cast<ColorAT<float>>(mesh.vertexColor(face->vertex(v)));
					if(rv->color.a() != 1) _alpha = rv->color.a();
					else if(meshColor.a() != 1) rv->color.a() = meshColor.a();
				}
				else if(mesh.hasFaceColors()) {
					rv->color = static_cast<ColorAT<float>>(mesh.faceColor(face - mesh.faces().constBegin()));
					if(rv->color.a() != 1) _alpha = rv->color.a();
					else if(meshColor.a() != 1) rv->color.a() = meshColor.a();
				}
				else if(face->materialIndex() < materialColors().size() && face->materialIndex() >= 0) {
					rv->color = static_cast<ColorAT<float>>(materialColors()[face->materialIndex()]);
				}
				else {
					rv->color = defaultVertexColor;
				}
			}
		}

		if(allMask) {
			std::vector<Vector_3<float>> groupVertexNormals(mesh.vertexCount());
			for(int group = 0; group < OVITO_MAX_NUM_SMOOTHING_GROUPS; group++) {
				quint32 groupMask = quint32(1) << group;
				if((allMask & groupMask) == 0)
					continue;	// Group is not used.

				// Reset work arrays.
				std::fill(groupVertexNormals.begin(), groupVertexNormals.end(), Vector_3<float>::Zero());

				// Compute vertex normals at original vertices for current smoothing group.
				faceNormal = faceNormals.begin();
				for(auto face = mesh.faces().constBegin(); face != mesh.faces().constEnd(); ++face, ++faceNormal) {
					// Skip faces that do not belong to the current smoothing group.
					if((face->smoothingGroups() & groupMask) == 0) continue;

					// Add face's normal to vertex normals.
					for(size_t fv = 0; fv < 3; fv++)
						groupVertexNormals[face->vertex(fv)] += *faceNormal;
				}

				// Transfer vertex normals from original vertices to render vertices.
				rv = renderVertices;
				for(const auto& face : mesh.faces()) {
					if(face.smoothingGroups() & groupMask) {
						for(size_t fv = 0; fv < 3; fv++, ++rv)
							rv->normal += groupVertexNormals[face.vertex(fv)];
					}
					else rv += 3;
				}
			}
		}
	}
	else {
		// Use normals stored in the mesh.
		ColoredVertexWithNormal* rv = renderVertices;
		const Vector3* faceNormal = mesh.normals().begin();
		ColorAT<float> defaultVertexColor = static_cast<ColorAT<float>>(meshColor);
		for(auto face = mesh.faces().constBegin(); face != mesh.faces().constEnd(); ++face) {
			// Initialize render vertices for this face.
			for(size_t v = 0; v < 3; v++, rv++) {
				rv->normal = static_cast<Vector_3<float>>(*faceNormal++);
				rv->pos = static_cast<Point_3<float>>(mesh.vertex(face->vertex(v)));
				if(mesh.hasVertexColors()) {
					rv->color = static_cast<ColorAT<float>>(mesh.vertexColor(face->vertex(v)));
					if(rv->color.a() != 1) _alpha = rv->color.a();
					else if(meshColor.a() != 1) rv->color.a() = meshColor.a();
				}
				else if(mesh.hasFaceColors()) {
					rv->color = static_cast<ColorAT<float>>(mesh.faceColor(face - mesh.faces().constBegin()));
					if(rv->color.a() != 1) _alpha = rv->color.a();
					else if(meshColor.a() != 1) rv->color.a() = meshColor.a();
				}
				else if(face->materialIndex() >= 0 && face->materialIndex() < materialColors().size()) {
					rv->color = static_cast<ColorAT<float>>(materialColors()[face->materialIndex()]);
				}
				else {
					rv->color = defaultVertexColor;
				}
			}
		}
	}

	_vertexBuffer.unmap();

	// Save a list of coordinates which will be used to sort faces back-to-front.
	if(_alpha != 1) {
		_triangleCoordinates.resize(mesh.faceCount());
		auto tc = _triangleCoordinates.begin();
		for(auto face = mesh.faces().constBegin(); face != mesh.faces().constEnd(); ++face, ++tc) {
			// Compute centroid of triangle.
			const auto& v1 = mesh.vertex(face->vertex(0));
			const auto& v2 = mesh.vertex(face->vertex(1));
			const auto& v3 = mesh.vertex(face->vertex(2));
			tc->x() = (v1.x() + v2.x() + v3.x()) / FloatType(3);
			tc->y() = (v1.y() + v2.y() + v3.y()) / FloatType(3);
			tc->z() = (v1.z() + v2.z() + v3.z()) / FloatType(3);
		}
	}
	else _triangleCoordinates.clear();

	// Create buffer for rendering polygon edges.
	if(emphasizeEdges) {
		// Count how many polygon edge are in the mesh.
		int numVisibleEdges = 0;
		for(const TriMeshFace& face : mesh.faces()) {
			for(size_t e = 0; e < 3; e++)
				if(face.edgeVisible(e)) numVisibleEdges++;
		}
		// Allocate storage buffer for line elements.
		_edgeLinesBuffer.create(QOpenGLBuffer::StaticDraw, numVisibleEdges, 2);
		Point_3<float>* lineVertices = _edgeLinesBuffer.map(QOpenGLBuffer::ReadWrite);

		// Generate line elements.
		for(const TriMeshFace& face : mesh.faces()) {
			for(size_t e = 0; e < 3; e++) {
				if(face.edgeVisible(e)) {
					*lineVertices++ = Point_3<float>(mesh.vertex(face.vertex(e)));
					*lineVertices++ = Point_3<float>(mesh.vertex(face.vertex((e+1)%3)));
				}
			}
		}

		_edgeLinesBuffer.unmap();
	}
}

/******************************************************************************
* Activates rendering of multiple instances of the mesh.
******************************************************************************/
void OpenGLMeshPrimitive::setInstancedRendering(std::vector<AffineTransformation> perInstanceTMs, std::vector<ColorA> perInstanceColors)
{
	OVITO_ASSERT(perInstanceTMs.size() == perInstanceColors.size() || perInstanceColors.empty());
	_alpha = std::any_of(perInstanceColors.begin(), perInstanceColors.end(), [](const ColorA& c) { return c.a() != FloatType(1); }) ? 0.5 : 1.0;
	_perInstanceTMs = std::move(perInstanceTMs);
	_perInstanceColors = std::move(perInstanceColors);
	_useInstancedRendering = true;
}

/******************************************************************************
* Returns true if the geometry buffer is filled and can be rendered with the given renderer.
******************************************************************************/
bool OpenGLMeshPrimitive::isValid(SceneRenderer* renderer)
{
	OpenGLSceneRenderer* vpRenderer = qobject_cast<OpenGLSceneRenderer*>(renderer);
	if(!vpRenderer) return false;
	return _vertexBuffer.isCreated() && (_contextGroup == vpRenderer->glcontext()->shareGroup());
}

/******************************************************************************
* Renders the geometry.
******************************************************************************/
void OpenGLMeshPrimitive::render(SceneRenderer* renderer)
{
	OVITO_ASSERT(_contextGroup == QOpenGLContextGroup::currentContextGroup());
	OpenGLSceneRenderer* vpRenderer = dynamic_object_cast<OpenGLSceneRenderer>(renderer);

	if(faceCount() <= 0 || !vpRenderer || (_useInstancedRendering && _perInstanceTMs.empty()))
		return;

	// If object is translucent, don't render it during the first rendering pass.
	// Queue primitive so that it gets rendered during the second pass.
	if(!renderer->isPicking() && _alpha != 1 && vpRenderer->translucentPass() == false) {
		vpRenderer->registerTranslucentPrimitive(shared_from_this());
		return;
	}

	vpRenderer->rebindVAO();

	// Render wireframe edges.
	if(!renderer->isPicking() && _edgeLinesBuffer.isCreated()) {
		if(!_lineShader->bind())
			vpRenderer->throwException(QStringLiteral("Failed to bind OpenGL shader."));
		ColorA wireframeColor(0.1f, 0.1f, 0.1f, _alpha);
		if(vpRenderer->glformat().majorVersion() >= 3) {
			OVITO_CHECK_OPENGL(vpRenderer, _lineShader->setAttributeValue("color", wireframeColor.r(), wireframeColor.g(), wireframeColor.b(), wireframeColor.a()));
		}
#ifndef Q_OS_WASM	
		else if(vpRenderer->oldGLFunctions()) {
			// Older OpenGL implementations cannot take vertex colors through a custom shader attribute.
			OVITO_CHECK_OPENGL(vpRenderer, vpRenderer->oldGLFunctions()->glColor4f(wireframeColor.r(), wireframeColor.g(), wireframeColor.b(), wireframeColor.a()));
		}
#endif		
		if(_alpha != 1.0) {
			vpRenderer->glEnable(GL_BLEND);
			vpRenderer->glBlendEquation(GL_FUNC_ADD);
			vpRenderer->glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE_MINUS_DST_COLOR, GL_ONE);
		}
		_edgeLinesBuffer.bindPositions(vpRenderer, _lineShader);
		Matrix4 mvp_matrix = vpRenderer->projParams().projectionMatrix * vpRenderer->modelViewTM();
		if(!_useInstancedRendering) {
			_lineShader->setUniformValue("modelview_projection_matrix", (QMatrix4x4)mvp_matrix);
			OVITO_CHECK_OPENGL(vpRenderer, vpRenderer->glDrawArrays(GL_LINES, 0, _edgeLinesBuffer.elementCount() * _edgeLinesBuffer.verticesPerElement()));
		}
		else {
			if(_alpha == 1.0) {
				for(const AffineTransformation& instanceTM : _perInstanceTMs) {
					_lineShader->setUniformValue("modelview_projection_matrix", (QMatrix4x4)(mvp_matrix * instanceTM));
					OVITO_CHECK_OPENGL(vpRenderer, vpRenderer->glDrawArrays(GL_LINES, 0, _edgeLinesBuffer.elementCount() * _edgeLinesBuffer.verticesPerElement()));
				}
			}
			else {
				OVITO_ASSERT(_perInstanceColors.size() == _perInstanceTMs.size());
				auto instanceColor = _perInstanceColors.cbegin();
				for(const AffineTransformation& instanceTM : _perInstanceTMs) {
					_lineShader->setUniformValue("modelview_projection_matrix", (QMatrix4x4)(mvp_matrix * instanceTM));
					wireframeColor.a() = instanceColor->a();
					++instanceColor;
					if(vpRenderer->glformat().majorVersion() >= 3) {
						OVITO_CHECK_OPENGL(vpRenderer, _lineShader->setAttributeValue("color", wireframeColor.r(), wireframeColor.g(), wireframeColor.b(), wireframeColor.a()));
					}
#ifndef Q_OS_WASM	
					else if(vpRenderer->oldGLFunctions()) {
						// Older OpenGL implementations cannot take vertex colors through a custom shader attribute.
						OVITO_CHECK_OPENGL(vpRenderer, vpRenderer->oldGLFunctions()->glColor4f(wireframeColor.r(), wireframeColor.g(), wireframeColor.b(), wireframeColor.a()));
					}
#endif					
					OVITO_CHECK_OPENGL(vpRenderer, vpRenderer->glDrawArrays(GL_LINES, 0, _edgeLinesBuffer.elementCount() * _edgeLinesBuffer.verticesPerElement()));
				}
			}
		}
		_edgeLinesBuffer.detachPositions(vpRenderer, _lineShader);
		_lineShader->release();
		vpRenderer->glEnable(GL_POLYGON_OFFSET_FILL);
		vpRenderer->glPolygonOffset(1.0f, 1.0f);
		if(_alpha != 1.0)
			vpRenderer->glDisable(GL_BLEND);
	}

	OVITO_REPORT_OPENGL_ERRORS(vpRenderer);

	if(cullFaces()) {
		vpRenderer->glEnable(GL_CULL_FACE);
		vpRenderer->glCullFace(GL_BACK);
	}
	else {
		vpRenderer->glDisable(GL_CULL_FACE);
	}

	QOpenGLShaderProgram* shader;
	if(!renderer->isPicking())
		shader = _shader;
	else
		shader = _pickingShader;

	if(!shader->bind())
		renderer->throwException(QStringLiteral("Failed to bind OpenGL shader."));

	_vertexBuffer.bindPositions(vpRenderer, shader, offsetof(ColoredVertexWithNormal, pos));
	if(!renderer->isPicking()) {
		if(_alpha != 1.0) {
			vpRenderer->glEnable(GL_BLEND);
			vpRenderer->glBlendEquation(GL_FUNC_ADD);
			vpRenderer->glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE_MINUS_DST_COLOR, GL_ONE);
		}
		_vertexBuffer.bindNormals(vpRenderer, shader, offsetof(ColoredVertexWithNormal, normal));
	}
	else {
		vpRenderer->activateVertexIDs(_pickingShader, _vertexBuffer.elementCount() * _vertexBuffer.verticesPerElement());
	}

	size_t numInstances = !_useInstancedRendering ? 1 : _perInstanceTMs.size();
	for(size_t instance = 0; instance < numInstances; instance++) {

		AffineTransformation mv_matrix;
		if(_useInstancedRendering)
			mv_matrix = vpRenderer->modelViewTM() * _perInstanceTMs[instance];
		else
			mv_matrix = vpRenderer->modelViewTM();
		shader->setUniformValue("modelview_projection_matrix", (QMatrix4x4)(vpRenderer->projParams().projectionMatrix * mv_matrix));
		if(!renderer->isPicking()) {
			shader->setUniformValue("normal_matrix", (QMatrix3x3)(mv_matrix.linear().inverse().transposed()));
			if(!_useInstancedRendering || _perInstanceColors.empty()) {
				_vertexBuffer.bindColors(vpRenderer, shader, 4, offsetof(ColoredVertexWithNormal, color));
			}
			else {
				const ColorA& color = _perInstanceColors[instance];
				if(vpRenderer->glformat().majorVersion() >= 3) {
					OVITO_CHECK_OPENGL(vpRenderer, shader->setAttributeValue("color", color.r(), color.g(), color.b(), color.a()));
				}
#ifndef Q_OS_WASM	
				else if(vpRenderer->oldGLFunctions()) {
					// Older OpenGL implementations cannot take colors through a custom shader attribute.
					OVITO_CHECK_OPENGL(vpRenderer, vpRenderer->oldGLFunctions()->glColor4f(color.r(), color.g(), color.b(), color.a()));
				}
#endif				
			}
		}
		else {
			if(!_useInstancedRendering) {
				_pickingShader->setUniformValue("pickingBaseID", (GLint)vpRenderer->registerSubObjectIDs(faceCount()));
				_pickingShader->setUniformValue("vertexIdDivisor", (GLint)3);
			}
			else {
				_pickingShader->setUniformValue("pickingBaseID", (GLint)vpRenderer->registerSubObjectIDs(1));
				_pickingShader->setUniformValue("vertexIdDivisor", (GLint)faceCount()*3);
			}
		}

		if(!renderer->isPicking() && _alpha != 1.0 && !_triangleCoordinates.empty()) {
			OVITO_ASSERT(_triangleCoordinates.size() == faceCount());
			OVITO_ASSERT(_vertexBuffer.verticesPerElement() == 3);
			// Render faces in back-to-front order to avoid artifacts at overlapping translucent faces.
			std::vector<GLuint> indices(faceCount());
			std::iota(indices.begin(), indices.end(), 0);
			// First compute distance of each face from the camera along viewing direction (=camera z-axis).
			std::vector<FloatType> distances(faceCount());
			Vector3 direction = mv_matrix.inverse().column(2);
			std::transform(_triangleCoordinates.begin(), _triangleCoordinates.end(), distances.begin(), [direction](const Point3& p) {
				return direction.dot(p - Point3::Origin());
			});
			// Now sort face indices with respect to distance (back-to-front order).
			std::sort(indices.begin(), indices.end(), [&distances](GLuint a, GLuint b) {
				return distances[a] < distances[b];
			});
			// Create OpenGL index buffer which can be used with glDrawElements.
			OpenGLBuffer<GLuint> primitiveIndices(QOpenGLBuffer::IndexBuffer);
			primitiveIndices.create(QOpenGLBuffer::StaticDraw, 3 * faceCount());
			GLuint* p = primitiveIndices.map(QOpenGLBuffer::WriteOnly);
			for(size_t i = 0; i < indices.size(); i++, p += 3)
				std::iota(p, p + 3, indices[i]*3);
			primitiveIndices.unmap();
			primitiveIndices.oglBuffer().bind();
			OVITO_CHECK_OPENGL(vpRenderer, vpRenderer->glDrawElements(GL_TRIANGLES, _vertexBuffer.elementCount() * _vertexBuffer.verticesPerElement(), GL_UNSIGNED_INT, nullptr));
			primitiveIndices.oglBuffer().release();
		}
		else {
			// Render faces in arbitrary order.
			OVITO_CHECK_OPENGL(vpRenderer, vpRenderer->glDrawArrays(GL_TRIANGLES, 0, _vertexBuffer.elementCount() * _vertexBuffer.verticesPerElement()));
		}
	}

	if(!renderer->isPicking() && _edgeLinesBuffer.isCreated()) {
		vpRenderer->glDisable(GL_POLYGON_OFFSET_FILL);
	}

	_vertexBuffer.detachPositions(vpRenderer, shader);
	if(!renderer->isPicking()) {
		if(!_useInstancedRendering)
			_vertexBuffer.detachColors(vpRenderer, shader);
		_vertexBuffer.detachNormals(vpRenderer, shader);
		if(_alpha != 1.0) vpRenderer->glDisable(GL_BLEND);
	}
	else {
		vpRenderer->deactivateVertexIDs(_pickingShader);
	}
	shader->release();

	OVITO_REPORT_OPENGL_ERRORS(vpRenderer);

	// Restore old state.
	if(cullFaces()) {
		vpRenderer->glDisable(GL_CULL_FACE);
		vpRenderer->glCullFace(GL_BACK);
	}

}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
