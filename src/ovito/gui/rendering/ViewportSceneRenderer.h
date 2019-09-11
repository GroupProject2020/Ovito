///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2013) Alexander Stukowski
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

#pragma once


#include <ovito/gui/GUI.h>
#include <ovito/opengl/OpenGLSceneRenderer.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Rendering)

/**
 * \brief This is the default scene renderer used to render the contents
 *        of the interactive viewports.
 */
class OVITO_GUI_EXPORT ViewportSceneRenderer : public OpenGLSceneRenderer
{
	Q_OBJECT
	OVITO_CLASS(ViewportSceneRenderer)

public:

	/// Standard constructor.
	ViewportSceneRenderer(DataSet* dataset) : OpenGLSceneRenderer(dataset) {}

	/// This method is called just before renderFrame() is called.
	virtual void beginFrame(TimePoint time, const ViewProjectionParameters& params, Viewport* vp) override;

	/// Returns whether this renderer is rendering an interactive viewport.
	/// \return true if rendering a real-time viewport; false if rendering an output image.
	virtual bool isInteractive() const override { return true; }

	/// Returns the final size of the rendered image in pixels.
	virtual QSize outputSize() const override;

	/// Returns the device pixel ratio of the output device we are rendering to.
	qreal devicePixelRatio() const;

protected:

	/// Puts the GL context into its default initial state before rendering a frame begins.
	virtual void initializeGLState() override;

	/// \brief This virtual method is responsible for rendering additional content that is only
	///       visible in the interactive viewports.
	virtual void renderInteractiveContent() override;

	/// Determines the range of the construction grid to display.
	std::tuple<FloatType, Box2I> determineGridRange(Viewport* vp);

	/// Renders the construction grid in a viewport.
	void renderGrid();

private:

	/// The geometry buffer used to render the construction grid of a viewport.
	std::shared_ptr<LinePrimitive> _constructionGridGeometry;
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
