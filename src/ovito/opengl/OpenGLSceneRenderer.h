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
#include <ovito/core/rendering/SceneRenderer.h>
#include "OpenGLHelpers.h"

#include <QOpenGLFunctions>
#include <QOpenGLFunctions_2_0>
#include <QOpenGLFunctions_3_0>
#include <QOpenGLFunctions_3_2_Core>
#include <QOpenGLShader>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>

namespace Ovito {

/**
 * \brief An OpenGL-based scene renderer. This serves as base class for both the interactive renderer used
 *        by the viewports and the standard output renderer.
 */
class OVITO_OPENGLRENDERER_EXPORT OpenGLSceneRenderer : public SceneRenderer, protected QOpenGLFunctions
{
	Q_OBJECT
	OVITO_CLASS(OpenGLSceneRenderer)

public:

	/// Default constructor.
	explicit OpenGLSceneRenderer(DataSet* dataset) : SceneRenderer(dataset) {}

	/// Renders the current animation frame.
	virtual bool renderFrame(FrameBuffer* frameBuffer, StereoRenderingTask stereoTask, SynchronousOperation operation) override;

	/// This method is called just before renderFrame() is called.
	virtual void beginFrame(TimePoint time, const ViewProjectionParameters& params, Viewport* vp) override;

	/// This method is called after renderFrame() has been called.
	virtual void endFrame(bool renderSuccessful) override;

	/// Changes the current local to world transformation matrix.
	virtual void setWorldTransform(const AffineTransformation& tm) override;

	/// Returns the current local-to-world transformation matrix.
	virtual const AffineTransformation& worldTransform() const override {
		return _modelWorldTM;
	}

	/// Returns the current model-to-view transformation matrix.
	const AffineTransformation& modelViewTM() const { return _modelViewTM; }

	/// Requests a new line geometry buffer from the renderer.
	virtual std::shared_ptr<LinePrimitive> createLinePrimitive() override;

	/// Requests a new particle geometry buffer from the renderer.
	virtual std::shared_ptr<ParticlePrimitive> createParticlePrimitive(ParticlePrimitive::ShadingMode shadingMode,
			ParticlePrimitive::RenderingQuality renderingQuality, ParticlePrimitive::ParticleShape shape,
			bool translucentParticles) override;

	/// Requests a new marker geometry buffer from the renderer.
	virtual std::shared_ptr<MarkerPrimitive> createMarkerPrimitive(MarkerPrimitive::MarkerShape shape) override;

	/// Requests a new text geometry buffer from the renderer.
	virtual std::shared_ptr<TextPrimitive> createTextPrimitive() override;

	/// Requests a new image geometry buffer from the renderer.
	virtual std::shared_ptr<ImagePrimitive> createImagePrimitive() override;

	/// Requests a new arrow geometry buffer from the renderer.
	virtual std::shared_ptr<ArrowPrimitive> createArrowPrimitive(ArrowPrimitive::Shape shape,
			ArrowPrimitive::ShadingMode shadingMode,
			ArrowPrimitive::RenderingQuality renderingQuality,
			bool translucentElements) override;

	/// Requests a new triangle mesh buffer from the renderer.
	virtual std::shared_ptr<MeshPrimitive> createMeshPrimitive() override;

	/// Determines if this renderer can share geometry data and other resources with the given other renderer.
	virtual bool sharesResourcesWith(SceneRenderer* otherRenderer) const override;

	/// Renders a 2d polyline in the viewport.
	void render2DPolyline(const Point2* points, int count, const ColorA& color, bool closed);

	/// Returns the OpenGL context this renderer uses.
	QOpenGLContext* glcontext() const { return _glcontext; }

	/// Returns the surface format of the current OpenGL context.
	const QSurfaceFormat& glformat() const { return _glformat; }

	/// Indicates whether the current OpenGL implementation is according to the core profile.
	bool isCoreProfile() const { return _isCoreProfile; }

	/// Indicates whether it is okay to use OpenGL point sprites. Otherwise emulate them using explicit triangle geometry.
	bool usePointSprites() const { return _usePointSprites; }

	/// Indicates whether it is okay to use GLSL geometry shaders.
	bool useGeometryShaders() const { return _useGeometryShaders; }

	/// Translates an OpenGL error code to a human-readable message string.
	static const char* openglErrorString(GLenum errorCode);

	/// Loads and compiles an OpenGL shader program.
	QOpenGLShaderProgram* loadShaderProgram(const QString& id, const QString& vertexShaderFile, const QString& fragmentShaderFile, const QString& geometryShaderFile = QString());

	/// Make sure vertex IDs are available to use by the OpenGL shader.
	void activateVertexIDs(QOpenGLShaderProgram* shader, GLint vertexCount, bool alwaysUseVBO = false);

	/// This needs to be called to deactivate vertex IDs, which were activated by a call to activateVertexIDs().
	void deactivateVertexIDs(QOpenGLShaderProgram* shader, bool alwaysUseVBO = false);

	/// Registers a range of sub-IDs belonging to the current object being rendered.
	/// This is an internal method used by the PickingSceneRenderer class to implement the picking mechanism.
	virtual quint32 registerSubObjectIDs(quint32 subObjectCount) { return 0; }

	/// Returns the line rendering width to use in object picking mode.
	virtual FloatType defaultLinePickingWidth() override;

	/// Returns the default OpenGL surface format requested by OVITO when creating OpenGL contexts.
	static QSurfaceFormat getDefaultSurfaceFormat();

	/// Returns whether we are currently rendering translucent objects.
	bool translucentPass() const { return _translucentPass; }

	/// Adds a primitive to the list of translucent primitives which will be rendered during the second
	/// rendering pass.
	void registerTranslucentPrimitive(const std::shared_ptr<PrimitiveBase>& primitive) {
		OVITO_ASSERT(!translucentPass());
		_translucentPrimitives.emplace_back(worldTransform(), primitive);
	}

	/// Binds the default vertex array object again in case another VAO was bound in between.
	/// This method should be called before calling an OpenGL rendering function.
	void rebindVAO() {
		makeContextCurrent();
		if(_vertexArrayObject) _vertexArrayObject->bind();
	}

	/// Sets the frame buffer background color.
	void setClearColor(const ColorA& color);

	/// Sets the rendering region in the frame buffer.
	void setRenderingViewport(int x, int y, int width, int height);

	/// Clears the frame buffer contents.
	void clearFrameBuffer(bool clearDepthBuffer = true, bool clearStencilBuffer = true);

	/// Temporarily enables/disables the depth test while rendering.
	virtual void setDepthTestEnabled(bool enabled) override;

	/// Activates the special highlight rendering mode.
	virtual void setHighlightMode(int pass) override;

	/// Returns the device pixel ratio of the output device we are rendering to.
	virtual qreal devicePixelRatio() const override;

	/// Reports OpenGL error status codes.
	void checkOpenGLErrorStatus(const char* command, const char* sourceFile, int sourceLine);

	/// Determines whether all viewport windows should share one GL context or not.
	static bool contextSharingEnabled(bool forceDefaultSetting = false);

	/// Determines whether OpenGL point sprites should be used or not.
	static bool pointSpritesEnabled(bool forceDefaultSetting = false);

	/// Determines whether OpenGL geometry shader programs should be used or not.
	static bool geometryShadersEnabled(bool forceDefaultSetting = false);

	/// Determines whether OpenGL geometry shader programs are supported by the hardware.
	static bool geometryShadersSupported() { return _openglSupportsGeomShaders; }

	/// Returns the vendor name of the OpenGL implementation in use.
	static const QByteArray& openGLVendor() { return _openGLVendor; }

	/// Returns the renderer name of the OpenGL implementation in use.
	static const QByteArray& openGLRenderer() { return _openGLRenderer; }

	/// Returns the version string of the OpenGL implementation in use.
	static const QByteArray& openGLVersion() { return _openGLVersion; }

	/// Returns the version of the OpenGL shading language supported by the system.
	static const QByteArray& openGLSLVersion() { return _openGLSLVersion; }

	/// Returns the current surface format used by the OpenGL implementation.
	static const QSurfaceFormat& openglSurfaceFormat() { return _openglSurfaceFormat; }

	/// Determines the capabilities of the current OpenGL implementation.
	static void determineOpenGLInfo();

protected:

	/// \brief Loads and compiles a GLSL shader and adds it to the given program object.
	void loadShader(QOpenGLShaderProgram* program, QOpenGLShader::ShaderType shaderType, const QString& filename);

	/// Makes the renderer's GL context current.
	void makeContextCurrent();

	/// Puts the GL context into its default initial state before rendering a frame begins.
	virtual void initializeGLState();

	/// This is called during rendering whenever the rendering process has been temporarily
	/// interrupted by an event loop and before rendering is resumed. It gives the renderer
	/// the opportunity to restore the active OpenGL context.
	virtual void resumeRendering() override {
		if(!isBoundingBoxPass())
			rebindVAO();
	}

	/// Returns the supersampling level to use.
	virtual int antialiasingLevelInternal() { return 1; }

#ifndef Q_OS_WASM

	/// The OpenGL glPointParameterf() function.
	void glPointSize(GLfloat size) {
		if(_glFunctions32) _glFunctions32->glPointSize(size);
		else if(_glFunctions30) _glFunctions30->glPointSize(size);
		else if(_glFunctions20) _glFunctions20->glPointSize(size);
	}

	/// The OpenGL glPointParameterf() function.
	void glPointParameterf(GLenum pname, GLfloat param) {
		if(_glFunctions32) _glFunctions32->glPointParameterf(pname, param);
		else if(_glFunctions30) _glFunctions30->glPointParameterf(pname, param);
		else if(_glFunctions20) _glFunctions20->glPointParameterf(pname, param);
	}

	/// The OpenGL glPointParameterfv() function.
	void glPointParameterfv(GLenum pname, const GLfloat* params) {
		if(_glFunctions32) _glFunctions32->glPointParameterfv(pname, params);
		else if(_glFunctions30) _glFunctions30->glPointParameterfv(pname, params);
		else if(_glFunctions20) _glFunctions20->glPointParameterfv(pname, params);
	}

	/// The OpenGL glMultiDrawArrays() function.
	void glMultiDrawArrays(GLenum mode, const GLint* first, const GLsizei* count, GLsizei drawcount) {
		if(_glFunctions32) _glFunctions32->glMultiDrawArrays(mode, first, count, drawcount);
		else if(_glFunctions30) _glFunctions30->glMultiDrawArrays(mode, first, count, drawcount);
		else if(_glFunctions20) _glFunctions20->glMultiDrawArrays(mode, first, count, drawcount);
	}

	void glTexEnvf(GLenum target, GLenum pname, GLfloat param) {
		if(_glFunctions30) _glFunctions30->glTexEnvf(target, pname, param);
		else if(_glFunctions20) _glFunctions20->glTexEnvf(target, pname, param);
	}

	/// The OpenGL 2.0 functions object.
	QOpenGLFunctions_2_0* oldGLFunctions() const { return _glFunctions20; }

#endif

private:

	/// The OpenGL context this renderer uses.
	QOpenGLContext* _glcontext = nullptr;

	/// The GL context group this renderer uses.
	QPointer<QOpenGLContextGroup> _glcontextGroup;

	/// The surface used by the GL context.
	QSurface* _glsurface = nullptr;

#ifndef Q_OS_WASM

	/// The OpenGL 2.0 functions object.
	QOpenGLFunctions_2_0* _glFunctions20;

	/// The OpenGL 3.0 functions object.
	QOpenGLFunctions_3_0* _glFunctions30;

	/// The OpenGL 3.2 core profile functions object.
	QOpenGLFunctions_3_2_Core* _glFunctions32;

#endif	

	/// The OpenGL vertex array object that is required by OpenGL 3.2 core profile.
	QScopedPointer<QOpenGLVertexArrayObject> _vertexArrayObject;

	/// The OpenGL surface format.
	QSurfaceFormat _glformat;

	/// Indicates whether the current OpenGL implementation is based on the core or the compatibility profile.
	bool _isCoreProfile = false;

	/// Indicates whether it is okay to use OpenGL point sprites. Otherwise emulate them using explicit triangle geometry.
	bool _usePointSprites = false;

	/// Indicates whether it is okay to use GLSL geometry shaders.
	bool _useGeometryShaders = false;

	/// The current model-to-world transformation matrix.
	AffineTransformation _modelWorldTM = AffineTransformation::Identity();

	/// The current model-to-view transformation matrix.
	AffineTransformation _modelViewTM = AffineTransformation::Identity();

	/// The internal OpenGL vertex buffer that stores vertex IDs.
	QOpenGLBuffer _glVertexIDBuffer;

	/// The number of IDs stored in the OpenGL buffer.
	GLint _glVertexIDBufferSize = 0;

	/// Indicates that we are currently rendering the translucent objects during a second rendering pass.
	bool _translucentPass = false;

	/// List of translucent graphics primitives collected during the first rendering pass, which
	/// need to be rendered during the second pass.
	std::vector<std::tuple<AffineTransformation, std::shared_ptr<PrimitiveBase>>> _translucentPrimitives;

	/// The vendor of the OpenGL implementation in use.
	static QByteArray _openGLVendor;

	/// The renderer name of the OpenGL implementation in use.
	static QByteArray _openGLRenderer;

	/// The version string of the OpenGL implementation in use.
	static QByteArray _openGLVersion;

	/// The version of the OpenGL shading language supported by the system.
	static QByteArray _openGLSLVersion;

	/// The current surface format used by the OpenGL implementation.
	static QSurfaceFormat _openglSurfaceFormat;

	/// Indicates whether the current OpenGL implementation supports geometry shader programs.
	static bool _openglSupportsGeomShaders;

	friend class OpenGLMeshPrimitive;
	friend class OpenGLArrowPrimitive;
	friend class OpenGLImagePrimitive;
	friend class OpenGLLinePrimitive;
	friend class OpenGLTextPrimitive;
	friend class OpenGLParticlePrimitive;
	friend class OpenGLMarkerPrimitive;
	template<typename T> friend class OpenGLBuffer;
};

}	// End of namespace
