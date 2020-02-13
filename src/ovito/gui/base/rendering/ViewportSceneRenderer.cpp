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

#include <ovito/gui/base/GUIBase.h>
#include <ovito/core/dataset/scene/SceneNode.h>
#include <ovito/core/dataset/scene/RootSceneNode.h>
#include <ovito/core/dataset/scene/PipelineSceneNode.h>
#include <ovito/core/dataset/pipeline/PipelineObject.h>
#include <ovito/core/dataset/pipeline/Modifier.h>
#include <ovito/core/dataset/data/DataVis.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/app/Application.h>
#include <ovito/core/rendering/RenderSettings.h>
#include <ovito/core/viewport/ViewportWindowInterface.h>
#include <ovito/core/viewport/ViewportGizmo.h>
#include "ViewportSceneRenderer.h"

namespace Ovito {

IMPLEMENT_OVITO_CLASS(ViewportSceneRenderer);

/******************************************************************************
* This method is called just before renderFrame() is called.
******************************************************************************/
void ViewportSceneRenderer::beginFrame(TimePoint time, const ViewProjectionParameters& params, Viewport* vp)
{
	OpenGLSceneRenderer::beginFrame(time, params, vp);
}

/******************************************************************************
* Puts the GL context into its default initial state before rendering
* a frame begins.
******************************************************************************/
void ViewportSceneRenderer::initializeGLState()
{
	OpenGLSceneRenderer::initializeGLState();

	// Set viewport background color.
	Color backgroundColor;
	if(!viewport()->renderPreviewMode())
		backgroundColor = Viewport::viewportColor(ViewportSettings::COLOR_VIEWPORT_BKG);
	else
		backgroundColor = renderSettings()->backgroundColor();
	setClearColor(ColorA(backgroundColor));
}

/******************************************************************************
* This virtual method is responsible for rendering additional content that is only
* visible in the interactive viewports.
******************************************************************************/
void ViewportSceneRenderer::renderInteractiveContent()
{
	OVITO_ASSERT(viewport());

	// Render construction grid.
	if(viewport()->isGridVisible())
		renderGrid();

	// Render visual 3D representation of the modifiers.
	renderModifiers(false);

	// Render visual 2D representation of the modifiers.
	renderModifiers(true);

	// Render viewport gizmos.
	if(ViewportWindowInterface* viewportWindow = viewport()->window()) {
		// First, render 3D content.
		for(ViewportGizmo* gizmo : viewportWindow->viewportGizmos()) {
			gizmo->renderOverlay3D(viewport(), this);
		}
		// Then, render 2D content on top.
		for(ViewportGizmo* gizmo : viewportWindow->viewportGizmos()) {
			gizmo->renderOverlay2D(viewport(), this);
		}
	}
}

/******************************************************************************
* Returns the final size of the rendered image in pixels.
******************************************************************************/
QSize ViewportSceneRenderer::outputSize() const
{
	return viewport()->windowSize();
}

/******************************************************************************
* Determines the range of the construction grid to display.
******************************************************************************/
std::tuple<FloatType, Box2I> ViewportSceneRenderer::determineGridRange(Viewport* vp)
{
	// Determine the area of the construction grid that is visible in the viewport.
	static const Point2 testPoints[] = {
		{-1,-1}, {1,-1}, {1, 1}, {-1, 1}, {0,1}, {0,-1}, {1,0}, {-1,0},
		{0,1}, {0,-1}, {1,0}, {-1,0}, {-1, 0.5}, {-1,-0.5}, {1,-0.5}, {1,0.5}, {0,0}
	};

	// Compute intersection points of test rays with grid plane.
	Box2 visibleGridRect;
	size_t numberOfIntersections = 0;
	for(size_t i = 0; i < sizeof(testPoints)/sizeof(testPoints[0]); i++) {
		Point3 p;
		if(vp->computeConstructionPlaneIntersection(testPoints[i], p, 0.1f)) {
			numberOfIntersections++;
			visibleGridRect.addPoint(p.x(), p.y());
		}
	}

	if(numberOfIntersections < 2) {
		// Cannot determine visible parts of the grid.
        return std::tuple<FloatType, Box2I>(0.0f, Box2I());
	}

	// Determine grid spacing adaptively.
	Point3 gridCenter(visibleGridRect.center().x(), visibleGridRect.center().y(), 0);
	FloatType gridSpacing = vp->nonScalingSize(vp->gridMatrix() * gridCenter) * 2.0f;
	// Round to nearest power of 10.
	gridSpacing = pow((FloatType)10, floor(log10(gridSpacing)));

	// Determine how many grid lines need to be rendered.
	int xstart = (int)floor(visibleGridRect.minc.x() / (gridSpacing * 10)) * 10;
	int xend = (int)ceil(visibleGridRect.maxc.x() / (gridSpacing * 10)) * 10;
	int ystart = (int)floor(visibleGridRect.minc.y() / (gridSpacing * 10)) * 10;
	int yend = (int)ceil(visibleGridRect.maxc.y() / (gridSpacing * 10)) * 10;

	return std::tuple<FloatType, Box2I>(gridSpacing, Box2I(Point2I(xstart, ystart), Point2I(xend, yend)));
}

/******************************************************************************
* Renders the construction grid.
******************************************************************************/
void ViewportSceneRenderer::renderGrid()
{
	if(isPicking())
		return;

	FloatType gridSpacing;
	Box2I gridRange;
	std::tie(gridSpacing, gridRange) = determineGridRange(viewport());
	if(gridSpacing <= 0) return;

	// Determine how many grid lines need to be rendered.
	int xstart = gridRange.minc.x();
	int ystart = gridRange.minc.y();
	int numLinesX = gridRange.size(0) + 1;
	int numLinesY = gridRange.size(1) + 1;

	FloatType xstartF = (FloatType)xstart * gridSpacing;
	FloatType ystartF = (FloatType)ystart * gridSpacing;
	FloatType xendF = (FloatType)(xstart + numLinesX - 1) * gridSpacing;
	FloatType yendF = (FloatType)(ystart + numLinesY - 1) * gridSpacing;

	setWorldTransform(viewport()->gridMatrix());

	if(!isBoundingBoxPass()) {

		// Allocate vertex buffer.
		int numVertices = 2 * (numLinesX + numLinesY);
		std::unique_ptr<Point3[]> vertexPositions(new Point3[numVertices]);
		std::unique_ptr<ColorA[]> vertexColors(new ColorA[numVertices]);

		// Build lines array.
		ColorA color = Viewport::viewportColor(ViewportSettings::COLOR_GRID);
		ColorA majorColor = Viewport::viewportColor(ViewportSettings::COLOR_GRID_INTENS);
		ColorA majorMajorColor = Viewport::viewportColor(ViewportSettings::COLOR_GRID_AXIS);

		Point3* v = vertexPositions.get();
		ColorA* c = vertexColors.get();
		FloatType x = xstartF;
		for(int i = xstart; i < xstart + numLinesX; i++, x += gridSpacing, c += 2) {
			*v++ = Point3(x, ystartF, 0);
			*v++ = Point3(x, yendF, 0);
			if((i % 10) != 0)
				c[0] = c[1] = color;
			else if(i != 0)
				c[0] = c[1] = majorColor;
			else
				c[0] = c[1] = majorMajorColor;
		}
		FloatType y = ystartF;
		for(int i = ystart; i < ystart + numLinesY; i++, y += gridSpacing, c += 2) {
			*v++ = Point3(xstartF, y, 0);
			*v++ = Point3(xendF, y, 0);
			if((i % 10) != 0)
				c[0] = c[1] = color;
			else if(i != 0)
				c[0] = c[1] = majorColor;
			else
				c[0] = c[1] = majorMajorColor;
		}
		OVITO_ASSERT(c == vertexColors.get() + numVertices);

		// Render grid lines.
		if(!_constructionGridGeometry || !_constructionGridGeometry->isValid(this))
			_constructionGridGeometry = createLinePrimitive();
		_constructionGridGeometry->setVertexCount(numVertices);
		_constructionGridGeometry->setVertexPositions(vertexPositions.get());
		_constructionGridGeometry->setVertexColors(vertexColors.get());
		_constructionGridGeometry->render(this);
	}
	else {
		addToLocalBoundingBox(Box3(Point3(xstartF, ystartF, 0), Point3(xendF, yendF, 0)));
	}
}

/******************************************************************************
* Returns the device pixel ratio of the output device we are rendering to.
******************************************************************************/
qreal ViewportSceneRenderer::devicePixelRatio() const
{
	if(viewport()) {
		// Query the device pixel ratio from the UI window associated with the viewport we are rendering into.
		if(ViewportWindowInterface* window = viewport()->window())
			return window->devicePixelRatio();
	}

	return OpenGLSceneRenderer::devicePixelRatio();
}

}	// End of namespace
