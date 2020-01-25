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
#include <ovito/core/dataset/scene/SceneNode.h>
#include <ovito/core/dataset/scene/RootSceneNode.h>
#include <ovito/core/dataset/scene/PipelineSceneNode.h>
#include <ovito/core/dataset/pipeline/PipelineObject.h>
#include <ovito/core/dataset/pipeline/Modifier.h>
#include <ovito/core/dataset/data/DataVis.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/app/Application.h>
#include <ovito/core/rendering/RenderSettings.h>
#include <ovito/core/utilities/concurrent/AsyncOperation.h>
#include "OpenGLSceneRenderer.h"
#include "OpenGLLinePrimitive.h"
#include "OpenGLParticlePrimitive.h"
#include "OpenGLTextPrimitive.h"
#include "OpenGLImagePrimitive.h"
#include "OpenGLArrowPrimitive.h"
#include "OpenGLMeshPrimitive.h"
#include "OpenGLMarkerPrimitive.h"
#include "OpenGLHelpers.h"

#include <QOffscreenSurface>
#include <QSurface>
#include <QWindow>
#include <QScreen>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Rendering)

IMPLEMENT_OVITO_CLASS(OpenGLSceneRenderer);

/// The vendor of the OpenGL implementation in use.
QByteArray OpenGLSceneRenderer::_openGLVendor;

/// The renderer name of the OpenGL implementation in use.
QByteArray OpenGLSceneRenderer::_openGLRenderer;

/// The version string of the OpenGL implementation in use.
QByteArray OpenGLSceneRenderer::_openGLVersion;

/// The version of the OpenGL shading language supported by the system.
QByteArray OpenGLSceneRenderer::_openGLSLVersion;

/// The current surface format used by the OpenGL implementation.
QSurfaceFormat OpenGLSceneRenderer::_openglSurfaceFormat;

/// Indicates whether the OpenGL implementation supports geometry shader programs.
bool OpenGLSceneRenderer::_openglSupportsGeomShaders = false;

/******************************************************************************
* Determines the capabilities of the current OpenGL implementation.
******************************************************************************/
void OpenGLSceneRenderer::determineOpenGLInfo()
{
	if(!_openGLVendor.isEmpty())
		return;		// Already done.

	// Create a temporary GL context and an offscreen surface if necessary.
	QOpenGLContext tempContext;
	QOffscreenSurface offscreenSurface;
	std::unique_ptr<QWindow> window;
	if(QOpenGLContext::currentContext() == nullptr) {
		tempContext.setFormat(getDefaultSurfaceFormat());
		if(!tempContext.create())
			throw Exception(tr("Failed to create an OpenGL context. Please check your graphics driver installation to make sure your system supports OpenGL applications. "
								"Sometimes this may only be a temporary error due to an automatic system update taken place in the background. In this case, simply restarting the computer can help."));
		if(Application::instance()->headlessMode() == false) {
			// Create a hidden, temporary window to make the GL context current.
			window.reset(new QWindow());
			window->setSurfaceType(QSurface::OpenGLSurface);
			window->setFormat(tempContext.format());
			window->create();
			if(!tempContext.makeCurrent(window.get()))
				throw Exception(tr("Failed to make OpenGL context current. Cannot query OpenGL information."));
		}
		else {
			// Create temporary offscreen buffer to make GL context current.
			offscreenSurface.setFormat(tempContext.format());
			offscreenSurface.create();
			if(!offscreenSurface.isValid())
				throw Exception(tr("Failed to create temporary offscreen surface. Cannot query OpenGL information."));
			if(!tempContext.makeCurrent(&offscreenSurface))
				throw Exception(tr("Failed to make OpenGL context current on offscreen surface. Cannot query OpenGL information."));
		}
		OVITO_ASSERT(QOpenGLContext::currentContext() == &tempContext);
	}

	_openGLVendor = reinterpret_cast<const char*>(tempContext.functions()->glGetString(GL_VENDOR));
	_openGLRenderer = reinterpret_cast<const char*>(tempContext.functions()->glGetString(GL_RENDERER));
	_openGLVersion = reinterpret_cast<const char*>(tempContext.functions()->glGetString(GL_VERSION));
	_openGLSLVersion = reinterpret_cast<const char*>(tempContext.functions()->glGetString(GL_SHADING_LANGUAGE_VERSION));
	_openglSupportsGeomShaders = QOpenGLShader::hasOpenGLShaders(QOpenGLShader::Geometry);
	_openglSurfaceFormat = QOpenGLContext::currentContext()->format();
}

/******************************************************************************
* Returns whether all viewport windows should share one GL context or not.
******************************************************************************/
bool OpenGLSceneRenderer::contextSharingEnabled(bool forceDefaultSetting)
{
	if(!forceDefaultSetting) {
		// The user can override the use of multiple GL contexts.
		QVariant userSetting = QSettings().value("display/share_opengl_context");
		if(userSetting.isValid())
			return userSetting.toBool();
	}

	determineOpenGLInfo();

#if defined(Q_OS_OSX)
	// On Mac OS X 10.9 with Intel graphics, using a single context for multiple viewports doesn't work very well.
	return false;
#elif defined(Q_OS_LINUX)
	// On Intel graphics under Linux, sharing a single context doesn't work very well either.
	if(_openGLVendor.contains("Intel"))
		return false;
#endif

	// By default, all viewports of a main window use the same GL context.
	return true;
}

/******************************************************************************
* Determines whether OpenGL point sprites should be used or not.
******************************************************************************/
bool OpenGLSceneRenderer::pointSpritesEnabled(bool forceDefaultSetting)
{
#ifdef Q_OS_WASM
	// Don't use point sprites on WebAssembly platform for now. 
	return false;
#endif

	if(!forceDefaultSetting) {
		// The user can override the use of point sprites.
		QVariant userSetting = QSettings().value("display/use_point_sprites");
		if(userSetting.isValid())
			return userSetting.toBool();
	}

	determineOpenGLInfo();

#if defined(Q_OS_WIN)
	// Point sprites don't seem to work well on Intel graphics under Windows.
	if(_openGLVendor.contains("Intel"))
		return false;
#elif defined(Q_OS_OSX)
	// Point sprites don't seem to work well on ATI graphics under Mac OS X.
	if(_openGLVendor.contains("ATI"))
		return false;
#endif

	// Use point sprites by default.
	return true;
}

/******************************************************************************
* Determines whether OpenGL geometry shader programs should be used or not.
******************************************************************************/
bool OpenGLSceneRenderer::geometryShadersEnabled(bool forceDefaultSetting)
{
#ifdef Q_OS_WASM
	// Completely disable support for geometry shaders on WebAssembly platform for now. 
	return false;
#endif

	if(!forceDefaultSetting) {
		// The user can override the use of geometry shaders.
		QVariant userSetting = QSettings().value("display/use_geometry_shaders");
		if(userSetting.isValid())
			return userSetting.toBool() && geometryShadersSupported();
	}

#ifdef Q_OS_WIN
	// Geometry shaders don't seem to work well on AMD/ATI hardware under Windows.
	if(_openGLVendor.contains("Radeon") || _openGLRenderer.contains("Radeon"))
		return false;
#endif

	if(Application::instance()->guiMode())
		return geometryShadersSupported();
	else if(QOpenGLContext::currentContext())
		return QOpenGLShader::hasOpenGLShaders(QOpenGLShader::Geometry);
	else
		return false;
}

/******************************************************************************
* Returns the default OpenGL surface format requested by OVITO when creating
* OpenGL contexts.
******************************************************************************/
QSurfaceFormat OpenGLSceneRenderer::getDefaultSurfaceFormat()
{
	QSurfaceFormat format;
	format.setDepthBufferSize(24);
	format.setSwapInterval(0);
	format.setMajorVersion(OVITO_OPENGL_REQUESTED_VERSION_MAJOR);
	format.setMinorVersion(OVITO_OPENGL_REQUESTED_VERSION_MINOR);
	format.setProfile(QSurfaceFormat::CoreProfile);
#ifdef Q_OS_WIN
	// Always request deprecated functions to be included in the context profile on Windows
	// to work around a compatibility issue between Qt 5.4.1 and the Intel OpenGL driver.
	// Otherwise the driver will complain about missing #version directives in the shader programs provided by Qt.
	format.setOption(QSurfaceFormat::DeprecatedFunctions);
#endif
	format.setStencilBufferSize(1);
	return format;
}

/******************************************************************************
* Determines if this renderer can share geometry data and other resources with
* the given other renderer.
******************************************************************************/
bool OpenGLSceneRenderer::sharesResourcesWith(SceneRenderer* otherRenderer) const
{
	// Two OpenGL renderers are compatible and share resources if they use the same QOpenGLContextGroup.
	OpenGLSceneRenderer* otherGLRenderer = dynamic_object_cast<OpenGLSceneRenderer>(otherRenderer);
	if(!otherGLRenderer) return false;
	if(_glcontextGroup.isNull()) return false;
	return _glcontextGroup == otherGLRenderer->_glcontextGroup;
}

/******************************************************************************
* This method is called just before renderFrame() is called.
******************************************************************************/
void OpenGLSceneRenderer::beginFrame(TimePoint time, const ViewProjectionParameters& params, Viewport* vp)
{
	SceneRenderer::beginFrame(time, params, vp);

	if(Application::instance()->headlessMode())
		throwException(tr("Cannot use OpenGL renderer in headless mode."));

	// Get the GL context being used for the current rendering pass.
	_glcontext = QOpenGLContext::currentContext();
	if(!_glcontext)
		throwException(tr("Cannot render scene: There is no active OpenGL context"));
	_glcontextGroup = _glcontext->shareGroup();
	_glsurface = _glcontext->surface();
	OVITO_ASSERT(_glsurface != nullptr);

	// Prepare a functions table allowing us to call OpenGL functions in a platform-independent way.
    initializeOpenGLFunctions();
    OVITO_REPORT_OPENGL_ERRORS(this);

#if 0
	static bool printedGLInfo = false;
	if(!printedGLInfo) {
		printedGLInfo = true;
		_openGLVendor = reinterpret_cast<const char*>(_glcontext->functions()->glGetString(GL_VENDOR));
		_openGLRenderer = reinterpret_cast<const char*>(_glcontext->functions()->glGetString(GL_RENDERER));
		_openGLVersion = reinterpret_cast<const char*>(_glcontext->functions()->glGetString(GL_VERSION));
		_openGLSLVersion = reinterpret_cast<const char*>(_glcontext->functions()->glGetString(GL_SHADING_LANGUAGE_VERSION));
		_openglSupportsGeomShaders = QOpenGLShader::hasOpenGLShaders(QOpenGLShader::Geometry);
		_openglSurfaceFormat = QOpenGLContext::currentContext()->format();
		qDebug() << "OpenGL version: " << _glcontext->format().majorVersion() << QStringLiteral(".") << _glcontext->format().minorVersion();
		qDebug() << "OpenGL profile: " << (_glcontext->format().profile() == QSurfaceFormat::CoreProfile ? "core" : (_glcontext->format().profile() == QSurfaceFormat::CompatibilityProfile ? "compatibility" : "none"));
		qDebug() << "OpenGL vendor: " << QString(OpenGLSceneRenderer::openGLVendor());
		qDebug() << "OpenGL renderer: " << QString(OpenGLSceneRenderer::openGLRenderer());
		qDebug() << "OpenGL version string: " << QString(OpenGLSceneRenderer::openGLVersion());
		qDebug() << "OpenGL shading language: " << QString(OpenGLSceneRenderer::openGLSLVersion());
		qDebug() << "OpenGL shader programs: " << QOpenGLShaderProgram::hasOpenGLShaderPrograms();
		qDebug() << "OpenGL geometry shaders: " << QOpenGLShader::hasOpenGLShaders(QOpenGLShader::Geometry, _glcontext);
		qDebug() << "isOpenGLES:" << _glcontext->isOpenGLES();
		qDebug() << "Extensions:";
		for(const QByteArray& e : _glcontext->extensions())
			qDebug() << e;
	}
#endif

	// Obtain surface format.
    OVITO_REPORT_OPENGL_ERRORS(this);
	_glformat = _glcontext->format();

	// OpenGL of a Windows guest machine running inside a VirtualBox reports "2.1 Chromium 1.9" as version string,
	// which is not correctly parsed by Qt. We have to work around this by explicitly setting the major/minor version numbers.
	if(qstrncmp((const char*)this->glGetString(GL_VERSION), "2.1 ", 4) == 0) {
		_glformat.setMajorVersion(2);
		_glformat.setMinorVersion(1);
	}

#ifndef Q_OS_WASM	
    // Obtain a functions object that allows to call OpenGL 2.0 functions in a platform-independent way.
	_glFunctions20 = _glcontext->versionFunctions<QOpenGLFunctions_2_0>();
	if(!_glFunctions20 || !_glFunctions20->initializeOpenGLFunctions())
		_glFunctions20 = nullptr;

	// Obtain a functions object that allows to call OpenGL 3.0 functions in a platform-independent way.
	_glFunctions30 = _glcontext->versionFunctions<QOpenGLFunctions_3_0>();
	if(!_glFunctions30 || !_glFunctions30->initializeOpenGLFunctions())
		_glFunctions30 = nullptr;

	// Obtain a functions object that allows to call OpenGL 3.2 core functions in a platform-independent way.
	_glFunctions32 = _glcontext->versionFunctions<QOpenGLFunctions_3_2_Core>();
	if(!_glFunctions32 || !_glFunctions32->initializeOpenGLFunctions())
		_glFunctions32 = nullptr;

	if(!_glFunctions20 && !_glFunctions30 && !_glFunctions32)
		throwException(tr("Could not resolve OpenGL functions. Invalid OpenGL context."));
#endif

	// Check if this context implements the core profile.
	_isCoreProfile = (_glformat.profile() == QSurfaceFormat::CoreProfile)
			|| (glformat().majorVersion() > 3)
			|| (glformat().majorVersion() == 3 && glformat().minorVersion() >= 2);

	// Qt reports the core profile only for OpenGL >= 3.2. Assume core profile also for 3.1 contexts.
	if(glformat().majorVersion() == 3 && glformat().minorVersion() == 1 && _glformat.profile() != QSurfaceFormat::CompatibilityProfile)
		_isCoreProfile = true;

	// Determine whether it's okay to use point sprites.
	_usePointSprites = pointSpritesEnabled();

	// Determine whether its okay to use geometry shaders.
	_useGeometryShaders = geometryShadersEnabled() && QOpenGLShader::hasOpenGLShaders(QOpenGLShader::Geometry);

	// Set up a vertex array object (VAO). An active VAO is required during rendering according to the OpenGL core profile.
	if(glformat().majorVersion() >= 3) {
		_vertexArrayObject.reset(new QOpenGLVertexArrayObject());
		OVITO_CHECK_OPENGL(this, _vertexArrayObject->create());
		OVITO_CHECK_OPENGL(this, _vertexArrayObject->bind());
	}
    OVITO_REPORT_OPENGL_ERRORS(this);

	// Reset OpenGL state.
	initializeGLState();

	// Clear background.
	clearFrameBuffer();
	OVITO_REPORT_OPENGL_ERRORS(this);
}

/******************************************************************************
* Puts the GL context into its default initial state before rendering
* a frame begins.
******************************************************************************/
void OpenGLSceneRenderer::initializeGLState()
{
	// Set up OpenGL state.
    OVITO_CHECK_OPENGL(this, this->glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE));
	OVITO_CHECK_OPENGL(this, this->glDisable(GL_STENCIL_TEST));
	OVITO_CHECK_OPENGL(this, this->glEnable(GL_DEPTH_TEST));
	OVITO_CHECK_OPENGL(this, this->glDepthFunc(GL_LESS));
	OVITO_CHECK_OPENGL(this, this->glDepthRangef(0, 1));
	OVITO_CHECK_OPENGL(this, this->glClearDepthf(1));
	OVITO_CHECK_OPENGL(this, this->glDepthMask(GL_TRUE));
	OVITO_CHECK_OPENGL(this, this->glDisable(GL_SCISSOR_TEST));
	_translucentPass = false;
	setClearColor(ColorA(0, 0, 0, 0));

	// Set up default viewport rectangle.
    if(viewport() && viewport()->window()) {
    	QSize vpSize = viewport()->window()->viewportWindowDeviceSize();
    	setRenderingViewport(0, 0, vpSize.width(), vpSize.height());
    }
    OVITO_REPORT_OPENGL_ERRORS(this);
}

/******************************************************************************
* This method is called after renderFrame() has been called.
******************************************************************************/
void OpenGLSceneRenderer::endFrame(bool renderSuccessful)
{
    OVITO_REPORT_OPENGL_ERRORS(this);
	OVITO_CHECK_OPENGL(this, _vertexArrayObject.reset());
	_glcontext = nullptr;

	SceneRenderer::endFrame(renderSuccessful);
}

/******************************************************************************
* Renders the current animation frame.
******************************************************************************/
bool OpenGLSceneRenderer::renderFrame(FrameBuffer* frameBuffer, StereoRenderingTask stereoTask, AsyncOperation& operation)
{
	OVITO_ASSERT(_glcontext == QOpenGLContext::currentContext());

	// Set up poor man's stereosopic rendering using red/green filtering.
	if(stereoTask == StereoscopicLeft)
		this->glColorMask(GL_TRUE, GL_FALSE, GL_FALSE, GL_FALSE);
	else if(stereoTask == StereoscopicRight)
		this->glColorMask(GL_FALSE, GL_TRUE, GL_TRUE, GL_TRUE);

	// Render the 3D scene objects.
	if(renderScene(operation)) {
		OVITO_REPORT_OPENGL_ERRORS(this);

		// Call subclass to render additional content that is only visible in the interactive viewports.
		renderInteractiveContent();
		OVITO_REPORT_OPENGL_ERRORS(this);

		// Render translucent objects in a second pass.
		_translucentPass = true;
		for(auto& record : _translucentPrimitives) {
			setWorldTransform(std::get<0>(record));
			std::get<1>(record)->render(this);
		}
		_translucentPrimitives.clear();
	}

	// Restore default OpenGL state.
	this->glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	OVITO_REPORT_OPENGL_ERRORS(this);

	return !operation.isCanceled();
}

/******************************************************************************
* Makes the renderer's GL context current.
******************************************************************************/
void OpenGLSceneRenderer::makeContextCurrent()
{
#ifndef Q_OS_WASM
	OVITO_ASSERT(glcontext());
	if(!glcontext()->makeCurrent(_glsurface))
		throwException(tr("Failed to make OpenGL context current."));
#endif		
}

/******************************************************************************
* Changes the current local to world transformation matrix.
******************************************************************************/
void OpenGLSceneRenderer::setWorldTransform(const AffineTransformation& tm)
{
	_modelWorldTM = tm;
	_modelViewTM = projParams().viewMatrix * tm;
}

/******************************************************************************
* Translates an OpenGL error code to a human-readable message string.
******************************************************************************/
const char* OpenGLSceneRenderer::openglErrorString(GLenum errorCode)
{
	switch(errorCode) {
	case GL_NO_ERROR: return "GL_NO_ERROR - No error has been recorded.";
	case GL_INVALID_ENUM: return "GL_INVALID_ENUM - An unacceptable value is specified for an enumerated argument.";
	case GL_INVALID_VALUE: return "GL_INVALID_VALUE - A numeric argument is out of range.";
	case GL_INVALID_OPERATION: return "GL_INVALID_OPERATION - The specified operation is not allowed in the current state.";
	case 0x0503 /*GL_STACK_OVERFLOW*/: return "GL_STACK_OVERFLOW - This command would cause a stack overflow.";
	case 0x0504 /*GL_STACK_UNDERFLOW*/: return "GL_STACK_UNDERFLOW - This command would cause a stack underflow.";
	case GL_OUT_OF_MEMORY: return "GL_OUT_OF_MEMORY - There is not enough memory left to execute the command.";
	case 0x8031 /*GL_TABLE_TOO_LARGE*/: return "GL_TABLE_TOO_LARGE - The specified table exceeds the implementation's maximum supported table size.";
	default: return "Unknown OpenGL error code.";
	}
}

/******************************************************************************
* Requests a new line geometry buffer from the renderer.
******************************************************************************/
std::shared_ptr<LinePrimitive> OpenGLSceneRenderer::createLinePrimitive()
{
	OVITO_ASSERT(!isBoundingBoxPass());
	makeContextCurrent();
	return std::make_shared<OpenGLLinePrimitive>(this);
}

/******************************************************************************
* Requests a new particle geometry buffer from the renderer.
******************************************************************************/
std::shared_ptr<ParticlePrimitive> OpenGLSceneRenderer::createParticlePrimitive(ParticlePrimitive::ShadingMode shadingMode,
		ParticlePrimitive::RenderingQuality renderingQuality, ParticlePrimitive::ParticleShape shape,
		bool translucentParticles)
{
	OVITO_ASSERT(!isBoundingBoxPass());
	makeContextCurrent();
	return std::make_shared<OpenGLParticlePrimitive>(this, shadingMode, renderingQuality, shape, translucentParticles);
}

/******************************************************************************
* Requests a new text geometry buffer from the renderer.
******************************************************************************/
std::shared_ptr<TextPrimitive> OpenGLSceneRenderer::createTextPrimitive()
{
	OVITO_ASSERT(!isBoundingBoxPass());
	makeContextCurrent();
	return std::make_shared<OpenGLTextPrimitive>(this);
}

/******************************************************************************
* Requests a new image geometry buffer from the renderer.
******************************************************************************/
std::shared_ptr<ImagePrimitive> OpenGLSceneRenderer::createImagePrimitive()
{
	OVITO_ASSERT(!isBoundingBoxPass());
	makeContextCurrent();
	return std::make_shared<OpenGLImagePrimitive>(this);
}

/******************************************************************************
* Requests a new arrow geometry buffer from the renderer.
******************************************************************************/
std::shared_ptr<ArrowPrimitive> OpenGLSceneRenderer::createArrowPrimitive(ArrowPrimitive::Shape shape,
		ArrowPrimitive::ShadingMode shadingMode,
		ArrowPrimitive::RenderingQuality renderingQuality,
		bool translucentElements)
{
	OVITO_ASSERT(!isBoundingBoxPass());
	makeContextCurrent();
	return std::make_shared<OpenGLArrowPrimitive>(this, shape, shadingMode, renderingQuality, translucentElements);
}

/******************************************************************************
* Requests a new marker geometry buffer from the renderer.
******************************************************************************/
std::shared_ptr<MarkerPrimitive> OpenGLSceneRenderer::createMarkerPrimitive(MarkerPrimitive::MarkerShape shape)
{
	OVITO_ASSERT(!isBoundingBoxPass());
	makeContextCurrent();
	return std::make_shared<OpenGLMarkerPrimitive>(this, shape);
}

/******************************************************************************
* Requests a new triangle mesh buffer from the renderer.
******************************************************************************/
std::shared_ptr<MeshPrimitive> OpenGLSceneRenderer::createMeshPrimitive()
{
	OVITO_ASSERT(!isBoundingBoxPass());
	makeContextCurrent();
	return std::make_shared<OpenGLMeshPrimitive>(this);
}

/******************************************************************************
* Loads an OpenGL shader program.
******************************************************************************/
QOpenGLShaderProgram* OpenGLSceneRenderer::loadShaderProgram(const QString& id, const QString& vertexShaderFile, const QString& fragmentShaderFile, const QString& geometryShaderFile)
{
	QOpenGLContextGroup* contextGroup = glcontext()->shareGroup();
	OVITO_ASSERT(contextGroup == QOpenGLContextGroup::currentContextGroup());

	OVITO_ASSERT(QOpenGLShaderProgram::hasOpenGLShaderPrograms());
	OVITO_ASSERT(QOpenGLShader::hasOpenGLShaders(QOpenGLShader::Vertex));
	OVITO_ASSERT(QOpenGLShader::hasOpenGLShaders(QOpenGLShader::Fragment));

	// The OpenGL shaders are only created once per OpenGL context group.
	QScopedPointer<QOpenGLShaderProgram> program(contextGroup->findChild<QOpenGLShaderProgram*>(id));
	if(program)
		return program.take();

	program.reset(new QOpenGLShaderProgram(contextGroup));
	program->setObjectName(id);

	// Load and compile vertex shader source.
	loadShader(program.data(), QOpenGLShader::Vertex, vertexShaderFile);

	// Load and compile fragment shader source.
	loadShader(program.data(), QOpenGLShader::Fragment, fragmentShaderFile);

	// Load and compile geometry shader source.
	if(!geometryShaderFile.isEmpty()) {
		OVITO_ASSERT(useGeometryShaders());
		loadShader(program.data(), QOpenGLShader::Geometry, geometryShaderFile);
	}

	// Compile the shader program.
	if(!program->link()) {
		Exception ex(QString("The OpenGL shader program %1 failed to link.").arg(id));
		ex.appendDetailMessage(program->log());
		throw ex;
	}

	OVITO_ASSERT(contextGroup->findChild<QOpenGLShaderProgram*>(id) == program.data());
	OVITO_REPORT_OPENGL_ERRORS(this);

	return program.take();
}

/******************************************************************************
* Loads and compiles a GLSL shader and adds it to the given program object.
******************************************************************************/
void OpenGLSceneRenderer::loadShader(QOpenGLShaderProgram* program, QOpenGLShader::ShaderType shaderType, const QString& filename)
{
	// Load shader source.
	QFile shaderSourceFile(filename);
	if(!shaderSourceFile.open(QFile::ReadOnly))
		throw Exception(QString("Unable to open shader source file %1.").arg(filename));
	QByteArray shaderSource;

	// Insert GLSL version string at the top.
	// Pick GLSL language version based on current OpenGL version.
#ifndef Q_OS_WASM
	if((glformat().majorVersion() >= 3 && glformat().minorVersion() >= 2) || glformat().majorVersion() > 3)
		shaderSource.append("#version 150\n");
	else if(glformat().majorVersion() >= 3)
		shaderSource.append("#version 130\n");
	else
		shaderSource.append("#version 120\n");
#else
//	shaderSource.append("#version 300 es\n");
#endif

	// Preprocess shader source while reading it from the file.
	//
	// This is a workaround for some older OpenGL drivers, which do not perform the
	// preprocessing of shader source files correctly (probably the __VERSION__ macro is not working).
	//
	// Here, in our own simple preprocessor implementation, we only handle
	//    #if __VERSION__ >= 130
	//       ...
	//    #else
	//       ...
	//    #endif
	// statements, which are used by most shaders to discriminate core and compatibility profiles.
	bool isFiltered = false;
	int ifstack = 0;
	int filterstackpos = 0;
	while(!shaderSourceFile.atEnd()) {
		QByteArray line = shaderSourceFile.readLine();
		if(line.contains("__VERSION__") && line.contains("130")) {
			OVITO_ASSERT(line.contains("#if"));
			OVITO_ASSERT(!isFiltered);
			if(line.contains(">=") && glformat().majorVersion() < 3) isFiltered = true;
			if(line.contains("<") && glformat().majorVersion() >= 3) isFiltered = true;
			filterstackpos = ifstack;
			continue;
		}
		else if(line.contains("#if")) {
			ifstack++;
		}
		else if(line.contains("#else")) {
			if(ifstack == filterstackpos) {
				isFiltered = !isFiltered;
				continue;
			}
		}
		else if(line.contains("#endif")) {
			if(ifstack == filterstackpos) {
				filterstackpos = -1;
				isFiltered = false;
				continue;
			}
			ifstack--;
		}

		if(!isFiltered) {
			shaderSource.append(line);
		}
	}

	// Load and compile vertex shader source.
	if(!program->addShaderFromSourceCode(shaderType, shaderSource)) {
		Exception ex(QString("The shader source file %1 failed to compile.").arg(filename));
		ex.appendDetailMessage(program->log());
		ex.appendDetailMessage(QStringLiteral("Problematic shader source:"));
		ex.appendDetailMessage(shaderSource);
		throw ex;
	}

	OVITO_REPORT_OPENGL_ERRORS(this);
}

/******************************************************************************
* Renders a 2d polyline in the viewport.
******************************************************************************/
void OpenGLSceneRenderer::render2DPolyline(const Point2* points, int count, const ColorA& color, bool closed)
{
	if(isBoundingBoxPass())
		return;

	makeContextCurrent();

	// Load OpenGL shader.
	QOpenGLShaderProgram* shader = loadShaderProgram("line", ":/openglrenderer/glsl/lines/line.vs", ":/openglrenderer/glsl/lines/line.fs");
	if(!shader->bind())
		throwException(tr("Failed to bind OpenGL shader."));

	bool wasDepthTestEnabled = this->glIsEnabled(GL_DEPTH_TEST);
	this->glDisable(GL_DEPTH_TEST);

	GLint vc[4];
	this->glGetIntegerv(GL_VIEWPORT, vc);
	QMatrix4x4 tm;
	tm.ortho(vc[0], vc[0] + vc[2], vc[1] + vc[3], vc[1], -1, 1);
	OVITO_CHECK_OPENGL(this, shader->setUniformValue("modelview_projection_matrix", tm));

	OpenGLBuffer<Point_2<float>> vertexBuffer;
	OpenGLBuffer<ColorAT<float>> colorBuffer;
	if(glformat().majorVersion() >= 3) {
		vertexBuffer.create(QOpenGLBuffer::StaticDraw, count);
		vertexBuffer.fill(points);
		vertexBuffer.bind(this, shader, "position", GL_FLOAT, 0, 2);
		colorBuffer.create(QOpenGLBuffer::StaticDraw, count);
		colorBuffer.fillConstant(color);
		OVITO_CHECK_OPENGL(this, colorBuffer.bindColors(this, shader, 4));
	}
#ifndef Q_OS_WASM	
	else if(oldGLFunctions()) {
		OVITO_CHECK_OPENGL(this, oldGLFunctions()->glEnableClientState(GL_VERTEX_ARRAY));
#ifdef FLOATTYPE_FLOAT
		OVITO_CHECK_OPENGL(this, oldGLFunctions()->glVertexPointer(2, GL_FLOAT, 0, points));
		OVITO_CHECK_OPENGL(this, oldGLFunctions()->glColor4fv(color.data()));
#else
		OVITO_CHECK_OPENGL(this, oldGLFunctions()->glVertexPointer(2, GL_DOUBLE, 0, points));
		OVITO_CHECK_OPENGL(this, oldGLFunctions()->glColor4dv(color.data()));
#endif
	}
#endif

	OVITO_CHECK_OPENGL(this, glDrawArrays(closed ? GL_LINE_LOOP : GL_LINE_STRIP, 0, count));

	if(glformat().majorVersion() >= 3) {
		vertexBuffer.detach(this, shader, "position");
		colorBuffer.detachColors(this, shader);
	}
#ifndef Q_OS_WASM	
	else if(oldGLFunctions()) {
		OVITO_CHECK_OPENGL(this, oldGLFunctions()->glDisableClientState(GL_VERTEX_ARRAY));
	}
#endif	
	shader->release();
	if(wasDepthTestEnabled) this->glEnable(GL_DEPTH_TEST);
}

/******************************************************************************
* Makes vertex IDs available to the shader.
******************************************************************************/
void OpenGLSceneRenderer::activateVertexIDs(QOpenGLShaderProgram* shader, GLint vertexCount, bool alwaysUseVBO)
{
	// Older OpenGL implementations do not provide the built-in gl_VertexID shader
	// variable. Therefore we have to provide the IDs in a vertex buffer.
	if(glformat().majorVersion() < 3 || alwaysUseVBO) {
		if(!_glVertexIDBuffer.isCreated() || _glVertexIDBufferSize < vertexCount) {
			OVITO_REPORT_OPENGL_ERRORS(this);
			if(!_glVertexIDBuffer.isCreated()) {
				// Create the ID buffer only once and keep it until the number of element grows.
				if(!_glVertexIDBuffer.create())
					throwException(QStringLiteral("Failed to create OpenGL vertex ID buffer."));
				_glVertexIDBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
				OVITO_REPORT_OPENGL_ERRORS(this);
			}
			if(!_glVertexIDBuffer.bind())
				throwException(QStringLiteral("Failed to bind OpenGL vertex ID buffer."));
			OVITO_CHECK_OPENGL(this, _glVertexIDBuffer.allocate(vertexCount * sizeof(GLfloat)));
			_glVertexIDBufferSize = vertexCount;
			if(vertexCount > 0) {
				if(!glcontext()->isOpenGLES()) {
					GLfloat* bufferData = static_cast<GLfloat*>(_glVertexIDBuffer.map(QOpenGLBuffer::WriteOnly));
					if(!bufferData)
						throwException(QStringLiteral("Failed to map OpenGL vertex ID buffer to memory."));
					GLfloat* bufferDataEnd = bufferData + vertexCount;
					for(GLint index = 0; bufferData != bufferDataEnd; ++index, ++bufferData)
						*bufferData = index;
					_glVertexIDBuffer.unmap();
				}
				else {
					// OpenGL ES does not support memory mapping of VBOs.
					std::vector<GLfloat> bufferData(vertexCount);
					std::iota(bufferData.begin(), bufferData.end(), 0);
					OVITO_CHECK_OPENGL(this, _glVertexIDBuffer.write(0, bufferData.data(), bufferData.size() * sizeof(GLfloat)));
				}
			}
		}
		else {
			if(!_glVertexIDBuffer.bind())
				throwException(QStringLiteral("Failed to bind OpenGL vertex ID buffer."));
		}

		// Make this vertex attribute available to vertex shaders.
		OVITO_CHECK_OPENGL(this, shader->enableAttributeArray("vertexID"));
		OVITO_CHECK_OPENGL(this, shader->setAttributeBuffer("vertexID", GL_FLOAT, 0, 1));
		OVITO_CHECK_OPENGL(this, _glVertexIDBuffer.release());
	}
}

/******************************************************************************
* Disables vertex IDs.
******************************************************************************/
void OpenGLSceneRenderer::deactivateVertexIDs(QOpenGLShaderProgram* shader, bool alwaysUseVBO)
{
	if(glformat().majorVersion() < 3 || alwaysUseVBO)
		shader->disableAttributeArray("vertexID");
}

/******************************************************************************
* Returns the line rendering width to use in object picking mode.
******************************************************************************/
FloatType OpenGLSceneRenderer::defaultLinePickingWidth()
{
	return FloatType(6) * devicePixelRatio();
}

/******************************************************************************
* Returns the device pixel ratio of the output device we are rendering to.
******************************************************************************/
qreal OpenGLSceneRenderer::devicePixelRatio() const
{
	if(glcontext() && glcontext()->screen())
		return glcontext()->screen()->devicePixelRatio();
	else
		return SceneRenderer::devicePixelRatio();
}

/******************************************************************************
* Sets the frame buffer background color.
******************************************************************************/
void OpenGLSceneRenderer::setClearColor(const ColorA& color)
{
	OVITO_CHECK_OPENGL(this, this->glClearColor(color.r(), color.g(), color.b(), color.a()));
}

/******************************************************************************
* Sets the rendering region in the frame buffer.
******************************************************************************/
void OpenGLSceneRenderer::setRenderingViewport(int x, int y, int width, int height)
{
	OVITO_CHECK_OPENGL(this, this->glViewport(x, y, width, height));
}

/******************************************************************************
* Clears the frame buffer contents.
******************************************************************************/
void OpenGLSceneRenderer::clearFrameBuffer(bool clearDepthBuffer, bool clearStencilBuffer)
{
	OVITO_CHECK_OPENGL(this, this->glClear(GL_COLOR_BUFFER_BIT |
			(clearDepthBuffer ? GL_DEPTH_BUFFER_BIT : 0) |
			(clearStencilBuffer ? GL_STENCIL_BUFFER_BIT : 0)));
}

/******************************************************************************
* Temporarily enables/disables the depth test while rendering.
******************************************************************************/
void OpenGLSceneRenderer::setDepthTestEnabled(bool enabled)
{
	if(enabled) this->glEnable(GL_DEPTH_TEST);
	else this->glDisable(GL_DEPTH_TEST);
}

/******************************************************************************
* Activates the special highlight rendering mode.
******************************************************************************/
void OpenGLSceneRenderer::setHighlightMode(int pass)
{
	if(pass == 1) {
		this->glEnable(GL_DEPTH_TEST);
		this->glClearStencil(0);
		this->glClear(GL_STENCIL_BUFFER_BIT);
		this->glEnable(GL_STENCIL_TEST);
		this->glStencilFunc(GL_ALWAYS, 0x1, 0x1);
		this->glStencilMask(0x1);
		this->glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
		this->glDepthFunc(GL_LEQUAL);
	}
	else if(pass == 2) {
		this->glDisable(GL_DEPTH_TEST);
		this->glStencilFunc(GL_NOTEQUAL, 0x1, 0x1);
		this->glStencilMask(0x1);
		this->glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	}
	else {
		this->glDepthFunc(GL_LESS);
		this->glEnable(GL_DEPTH_TEST);
		this->glDisable(GL_STENCIL_TEST);
	}
}

/******************************************************************************
* Reports OpenGL error status codes.
******************************************************************************/
void OpenGLSceneRenderer::checkOpenGLErrorStatus(const char* command, const char* sourceFile, int sourceLine)
{
	GLenum error;
	while((error = this->glGetError()) != GL_NO_ERROR) {
		qDebug() << "WARNING: OpenGL call" << command << "failed "
				"in line" << sourceLine << "of file" << sourceFile
				<< "with error" << OpenGLSceneRenderer::openglErrorString(error);
	}
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
