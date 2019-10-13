////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2018 Alexander Stukowski
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
#include "OpenGLSceneRenderer.h"

#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Rendering)

/**
 * \brief This is the default scene renderer used for high-quality image output.
 */
class OVITO_OPENGLRENDERER_EXPORT StandardSceneRenderer : public OpenGLSceneRenderer
{
	Q_OBJECT
	OVITO_CLASS(StandardSceneRenderer)
	Q_CLASSINFO("DisplayName", "OpenGL");

public:

	/// Default constructor.
	Q_INVOKABLE StandardSceneRenderer(DataSet* dataset) : OpenGLSceneRenderer(dataset), _antialiasingLevel(3) {}

	/// Prepares the renderer for rendering and sets the data set that is being rendered.
	virtual bool startRender(DataSet* dataset, RenderSettings* settings) override;

	/// This method is called just before renderFrame() is called.
	virtual void beginFrame(TimePoint time, const ViewProjectionParameters& params, Viewport* vp) override;

	/// Renders the current animation frame.
	virtual bool renderFrame(FrameBuffer* frameBuffer, StereoRenderingTask stereoTask, AsyncOperation& operation) override;

	/// Is called after rendering has finished.
	virtual void endRender() override;

	/// Returns whether this renderer is rendering an interactive viewport.
	/// \return true if rendering a real-time viewport; false if rendering an output image.
	virtual bool isInteractive() const override { return false; }

protected:

	/// Returns the supersampling level to use.
	virtual int antialiasingLevelInternal() override { return antialiasingLevel(); }

	/// Puts the GL context into its default initial state before rendering a frame begins.
	virtual void initializeGLState() override;

private:

	/// Controls the number of sub-pixels to render.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, antialiasingLevel, setAntialiasingLevel);

	/// The offscreen surface used to render into an image buffer using OpenGL.
	QScopedPointer<QOffscreenSurface> _offscreenSurface;

	/// The temporary OpenGL rendering context.
	QScopedPointer<QOpenGLContext> _offscreenContext;

	/// The OpenGL framebuffer.
	QScopedPointer<QOpenGLFramebufferObject> _framebufferObject;

	/// The resolution of the offscreen framebuffer.
	QSize _framebufferSize;
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
