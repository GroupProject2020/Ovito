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


#include <ovito/particles/Particles.h>
#include <ovito/opengl/OpenGLSceneRenderer.h>

#include <QOffscreenSurface>
#include <QOpenGLFramebufferObject>

namespace Ovito { namespace Particles {

/**
 * \brief A renderer used to compute ambient occlusion lighting.
 */
class AmbientOcclusionRenderer : public OpenGLSceneRenderer
{
	Q_OBJECT
	OVITO_CLASS(AmbientOcclusionRenderer)

public:

	/// Constructor.
	AmbientOcclusionRenderer(DataSet* dataset, QSize resolution) : OpenGLSceneRenderer(dataset), _resolution(resolution) {
		setPicking(true);
		_offscreenSurface.setFormat(OpenGLSceneRenderer::getDefaultSurfaceFormat());
		_offscreenSurface.create();
	}

	/// Prepares the renderer for rendering and sets the data set that is being rendered.
	virtual bool startRender(DataSet* dataset, RenderSettings* settings) override;

	/// This method is called just before renderFrame() is called.
	virtual void beginFrame(TimePoint time, const ViewProjectionParameters& params, Viewport* vp) override;

	/// This method is called after renderFrame() has been called.
	virtual void endFrame(bool renderSuccessful) override;

	/// Is called after rendering has finished.
	virtual void endRender() override;

	/// Returns the rendered image.
	const QImage& image() const { return _image; }

	/// Returns the final size of the rendered image in pixels.
	virtual QSize outputSize() const override { return _image.size(); }

	/// Registers a range of sub-IDs belonging to the current object being rendered.
	virtual quint32 registerSubObjectIDs(quint32 subObjectCount) override { return 1; }

	/// Returns whether this renderer is rendering an interactive viewport.
	virtual bool isInteractive() const override { return false; }

protected:

	/// Puts the GL context into its default initial state before rendering a frame begins.
	virtual void initializeGLState() override;

private:

	/// The OpenGL framebuffer.
	QScopedPointer<QOpenGLFramebufferObject> _framebufferObject;

	/// The OpenGL rendering context.
	QScopedPointer<QOpenGLContext> _offscreenContext;

	/// The offscreen surface used to render into an image buffer using OpenGL.
	QOffscreenSurface _offscreenSurface;

	/// The rendered image.
	QImage _image;

	/// The rendering resolution.
	QSize _resolution;
};

}	// End of namespace
}	// End of namespace
