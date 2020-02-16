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
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/viewport/Viewport.h>
#include <ovito/core/viewport/ViewportConfiguration.h>
#include <ovito/core/rendering/RenderSettings.h>
#include <ovito/core/app/Application.h>
#include <ovito/core/utilities/units/UnitsManager.h>
#include "StandardSceneRenderer.h"

namespace Ovito {

IMPLEMENT_OVITO_CLASS(StandardSceneRenderer);
DEFINE_PROPERTY_FIELD(StandardSceneRenderer, antialiasingLevel);
SET_PROPERTY_FIELD_LABEL(StandardSceneRenderer, antialiasingLevel, "Antialiasing level");
SET_PROPERTY_FIELD_UNITS_AND_RANGE(StandardSceneRenderer, antialiasingLevel, IntegerParameterUnit, 1, 6);

/******************************************************************************
* Prepares the renderer for rendering and sets the data set that is being rendered.
******************************************************************************/
bool StandardSceneRenderer::startRender(DataSet* dataset, RenderSettings* settings)
{
	if(Application::instance()->headlessMode())
		throwException(tr("Cannot use OpenGL renderer when running in headless mode. "
				"Please use a different rendering engine or run program on a machine where access to "
				"graphics hardware is possible."));

	if(!OpenGLSceneRenderer::startRender(dataset, settings))
		return false;

	int sampling = std::max(1, antialiasingLevel());

	if(Application::instance()->guiMode()) {
		// Create a temporary OpenGL context for rendering to an offscreen buffer.
		_offscreenContext.reset(new QOpenGLContext());
		_offscreenContext->setFormat(OpenGLSceneRenderer::getDefaultSurfaceFormat());
		// It should share its resources with the viewport renderer.
		const QVector<Viewport*>& viewports = renderDataset()->viewportConfig()->viewports();
		if(!viewports.empty() && viewports.front()->window()) {
			viewports.front()->window()->makeOpenGLContextCurrent();
			_offscreenContext->setShareContext(QOpenGLContext::currentContext());
		}
		if(!_offscreenContext->create())
			throwException(tr("Failed to create OpenGL context for rendering."));
	}
	else {
		// Create new OpenGL context for rendering in console mode.
		OVITO_ASSERT(QOpenGLContext::currentContext() == nullptr);
		_offscreenContext.reset(new QOpenGLContext());
		_offscreenContext->setFormat(OpenGLSceneRenderer::getDefaultSurfaceFormat());
		if(!_offscreenContext->create())
			throwException(tr("Failed to create OpenGL context for rendering."));
	}

	// Create offscreen buffer.
	if(_offscreenSurface.isNull())
		_offscreenSurface.reset(new QOffscreenSurface());
	_offscreenSurface->setFormat(_offscreenContext->format());
	_offscreenSurface->create();
	if(!_offscreenSurface->isValid())
		throwException(tr("Failed to create offscreen rendering surface."));

	// Make the context current.
	if(!_offscreenContext->makeCurrent(_offscreenSurface.data()))
		throwException(tr("Failed to make OpenGL context current."));

	// Create OpenGL framebuffer.
	_framebufferSize = QSize(settings->outputImageWidth() * sampling, settings->outputImageHeight() * sampling);
	QOpenGLFramebufferObjectFormat framebufferFormat;
	framebufferFormat.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
	_framebufferObject.reset(new QOpenGLFramebufferObject(_framebufferSize, framebufferFormat));
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
void StandardSceneRenderer::beginFrame(TimePoint time, const ViewProjectionParameters& params, Viewport* vp)
{
	// Make GL context current.
	if(!_offscreenContext || !_offscreenContext->makeCurrent(_offscreenSurface.data()))
		throwException(tr("Failed to make OpenGL context current."));

	OpenGLSceneRenderer::beginFrame(time, params, vp);
}

/******************************************************************************
* Puts the GL context into its default initial state before rendering
* a frame begins.
******************************************************************************/
void StandardSceneRenderer::initializeGLState()
{
	OpenGLSceneRenderer::initializeGLState();

	// Setup GL viewport.
	setRenderingViewport(0, 0, _framebufferSize.width(), _framebufferSize.height());
	setClearColor(ColorA(renderSettings()->backgroundColor(), 0));
}

/******************************************************************************
* Renders the current animation frame.
******************************************************************************/
bool StandardSceneRenderer::renderFrame(FrameBuffer* frameBuffer, StereoRenderingTask stereoTask, SynchronousOperation operation)
{
	// Let the base class do the main rendering work.
	if(!OpenGLSceneRenderer::renderFrame(frameBuffer, stereoTask, std::move(operation)))
		return false;

	// Flush the contents to the FBO before extracting image.
	glcontext()->swapBuffers(_offscreenSurface.data());

	// Fetch rendered image from OpenGL framebuffer.
	QImage bufferImage = _framebufferObject->toImage();
	// We need it in ARGB32 format for best results.
	QImage bufferImageArgb32(bufferImage.constBits(), bufferImage.width(), bufferImage.height(), QImage::Format_ARGB32);
	// Rescale supersampled image to final size.
	QImage scaledImage = bufferImageArgb32.scaled(frameBuffer->image().width(), frameBuffer->image().height(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

	// Transfer OpenGL image to the output frame buffer.
	{
		QPainter painter(&frameBuffer->image());
		painter.drawImage(painter.window(), scaledImage);
	}
	frameBuffer->update();

	return true;
}

/******************************************************************************
* Is called after rendering has finished.
******************************************************************************/
void StandardSceneRenderer::endRender()
{
	QOpenGLFramebufferObject::bindDefault();
	QOpenGLContext* ctxt = QOpenGLContext::currentContext();
	if(ctxt) ctxt->doneCurrent();
	_framebufferObject.reset();
	_offscreenContext.reset();
	_offscreenSurface.reset();
	OpenGLSceneRenderer::endRender();
}

}	// End of namespace
