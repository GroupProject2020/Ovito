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

#include <ovito/particles/Particles.h>
#include <ovito/core/viewport/Viewport.h>
#include "AmbientOcclusionRenderer.h"

namespace Ovito { namespace Particles {

IMPLEMENT_OVITO_CLASS(AmbientOcclusionRenderer);

/******************************************************************************
* Prepares the renderer for rendering and sets the data set that is being rendered.
******************************************************************************/
bool AmbientOcclusionRenderer::startRender(DataSet* dataset, RenderSettings* settings)
{
	if(!OpenGLSceneRenderer::startRender(dataset, settings))
		return false;

	// Create new OpenGL context for rendering in this background thread.
	OVITO_ASSERT(QOpenGLContext::currentContext() == nullptr);
	_offscreenContext.reset(new QOpenGLContext());
	_offscreenContext->setFormat(OpenGLSceneRenderer::getDefaultSurfaceFormat());
	if(!_offscreenContext->create())
		throwException(tr("Failed to create OpenGL context."));

	// Check offscreen buffer.
	if(!_offscreenSurface.isValid())
		throwException(tr("Failed to create offscreen rendering surface."));

	// Make the context current.
	if(!_offscreenContext->makeCurrent(&_offscreenSurface))
		throwException(tr("Failed to make OpenGL context current."));

	// Check OpenGL version.
	if(_offscreenContext->format().majorVersion() < OVITO_OPENGL_MINIMUM_VERSION_MAJOR || (_offscreenContext->format().majorVersion() == OVITO_OPENGL_MINIMUM_VERSION_MAJOR && _offscreenContext->format().minorVersion() < OVITO_OPENGL_MINIMUM_VERSION_MINOR)) {
		throwException(tr(
				"The OpenGL implementation available on this system does not support OpenGL version %4.%5 or newer.\n\n"
				"Ovito requires modern graphics hardware to accelerate 3d rendering. You current system configuration is not compatible with Ovito.\n\n"
				"To avoid this error message, please install the newest graphics driver, or upgrade your graphics card.\n\n"
				"The currently installed OpenGL graphics driver reports the following information:\n\n"
				"OpenGL Vendor: %1\n"
				"OpenGL Renderer: %2\n"
				"OpenGL Version: %3\n\n"
				"Ovito requires OpenGL version %4.%5 or higher.")
				.arg(QString(OpenGLSceneRenderer::openGLVendor()))
				.arg(QString(OpenGLSceneRenderer::openGLRenderer()))
				.arg(QString(OpenGLSceneRenderer::openGLVersion()))
				.arg(OVITO_OPENGL_MINIMUM_VERSION_MAJOR)
				.arg(OVITO_OPENGL_MINIMUM_VERSION_MINOR)
				);
	}

	// Create OpenGL framebuffer.
	QOpenGLFramebufferObjectFormat framebufferFormat;
	framebufferFormat.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
	_framebufferObject.reset(new QOpenGLFramebufferObject(_resolution, framebufferFormat));
	if(!_framebufferObject->isValid())
		throwException(tr("Failed to create OpenGL framebuffer object for offscreen rendering."));

	// Bind OpenGL buffer.
	if(!_framebufferObject->bind())
		throwException(tr("Failed to bind OpenGL framebuffer object for offscreen rendering."));

	return true;
}

/******************************************************************************
* This method is called just before renderFrame() is called.
******************************************************************************/
void AmbientOcclusionRenderer::beginFrame(TimePoint time, const ViewProjectionParameters& params, Viewport* vp)
{
	// Make GL context current.
	if(!_offscreenContext->makeCurrent(&_offscreenSurface))
		throwException(tr("Failed to make OpenGL context current."));

	OpenGLSceneRenderer::beginFrame(time, params, vp);
}

/******************************************************************************
* Puts the GL context into its default initial state before rendering
* a frame begins.
******************************************************************************/
void AmbientOcclusionRenderer::initializeGLState()
{
	OpenGLSceneRenderer::initializeGLState();

	// Setup GL viewport and background color.
	setRenderingViewport(0, 0, _resolution.width(), _resolution.height());
	setDepthTestEnabled(true);
}

/******************************************************************************
* This method is called after renderFrame() has been called.
******************************************************************************/
void AmbientOcclusionRenderer::endFrame(bool renderSuccessful)
{
	if(renderSuccessful) {
		// Flush the contents to the FBO before extracting image.
		_offscreenContext->swapBuffers(&_offscreenSurface);

		// Fetch rendered image from OpenGL framebuffer.
		QSize size = _framebufferObject->size();
		if(_image.isNull() || _image.size() != size)
			_image = QImage(size, QImage::Format_ARGB32);
		while(glGetError() != GL_NO_ERROR);
		glReadPixels(0, 0, size.width(), size.height(), 0x80E1 /*GL_BGRA*/, GL_UNSIGNED_BYTE, _image.bits());
		if(glGetError() != GL_NO_ERROR) {
			glReadPixels(0, 0, size.width(), size.height(), GL_RGBA, GL_UNSIGNED_BYTE, _image.bits());
			_image = _image.rgbSwapped();
		}
	}

	OpenGLSceneRenderer::endFrame(renderSuccessful);
}

/******************************************************************************
* Is called after rendering has finished.
******************************************************************************/
void AmbientOcclusionRenderer::endRender()
{
	_framebufferObject.reset();
	_offscreenContext.reset();
	OpenGLSceneRenderer::endRender();
}

}	// End of namespace
}	// End of namespace
