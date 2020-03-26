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

#include <ovito/stdobj/StdObj.h>
#include <ovito/core/utilities/units/UnitsManager.h>
#include <ovito/core/rendering/SceneRenderer.h>
#include <ovito/core/rendering/LinePrimitive.h>
#include <ovito/core/rendering/ArrowPrimitive.h>
#include <ovito/core/rendering/ParticlePrimitive.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/dataset/data/VersionedDataObjectRef.h>
#include "SimulationCellVis.h"
#include "SimulationCellObject.h"

namespace Ovito { namespace StdObj {

IMPLEMENT_OVITO_CLASS(SimulationCellVis);
DEFINE_PROPERTY_FIELD(SimulationCellVis, cellLineWidth);
DEFINE_PROPERTY_FIELD(SimulationCellVis, renderCellEnabled);
DEFINE_PROPERTY_FIELD(SimulationCellVis, cellColor);
SET_PROPERTY_FIELD_LABEL(SimulationCellVis, cellLineWidth, "Line width");
SET_PROPERTY_FIELD_LABEL(SimulationCellVis, renderCellEnabled, "Render cell");
SET_PROPERTY_FIELD_LABEL(SimulationCellVis, cellColor, "Line color");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(SimulationCellVis, cellLineWidth, WorldParameterUnit, 0);

/******************************************************************************
* Constructor.
******************************************************************************/
SimulationCellVis::SimulationCellVis(DataSet* dataset) : DataVis(dataset),
	_renderCellEnabled(true),
	_cellLineWidth(0.5),
	_cellColor(0, 0, 0)
{
}

/******************************************************************************
* Computes the bounding box of the object.
******************************************************************************/
Box3 SimulationCellVis::boundingBox(TimePoint time, const std::vector<const DataObject*>& objectStack, const PipelineSceneNode* contextNode, const PipelineFlowState& flowState, TimeInterval& validityInterval)
{
	const SimulationCellObject* cellObject = dynamic_object_cast<SimulationCellObject>(objectStack.back());
	OVITO_CHECK_OBJECT_POINTER(cellObject);

	AffineTransformation matrix = cellObject->cellMatrix();
	if(cellObject->is2D()) {
		matrix.column(2).setZero();
		matrix.translation().z() = 0;
	}

	return Box3(Point3(0), Point3(1)).transformed(matrix);
}

/******************************************************************************
* Lets the visualization element render the data object.
******************************************************************************/
void SimulationCellVis::render(TimePoint time, const std::vector<const DataObject*>& objectStack, const PipelineFlowState& flowState, SceneRenderer* renderer, const PipelineSceneNode* contextNode)
{
	const SimulationCellObject* cell = dynamic_object_cast<SimulationCellObject>(objectStack.back());
	OVITO_CHECK_OBJECT_POINTER(cell);
	if(!cell) return;

	if(renderer->isInteractive() && !renderer->viewport()->renderPreviewMode()) {
		if(!renderer->isBoundingBoxPass()) {
			renderWireframe(time, cell, flowState, renderer, contextNode);
		}
		else {
			TimeInterval validityInterval;
			renderer->addToLocalBoundingBox(boundingBox(time, objectStack, contextNode, flowState, validityInterval));
		}
	}
	else {
		if(!renderCellEnabled())
			return;		// Do nothing if rendering has been disabled by the user.

		if(!renderer->isBoundingBoxPass()) {
			renderSolid(time, cell, flowState, renderer, contextNode);
		}
		else {
			TimeInterval validityInterval;
			Box3 bb = boundingBox(time, objectStack, contextNode, flowState, validityInterval);
			renderer->addToLocalBoundingBox(bb.padBox(cellLineWidth()));
		}
	}
}

/******************************************************************************
* Renders the given simulation cell using lines.
******************************************************************************/
void SimulationCellVis::renderWireframe(TimePoint time, const SimulationCellObject* cell, const PipelineFlowState& flowState, SceneRenderer* renderer, const PipelineSceneNode* contextNode)
{
	OVITO_ASSERT(!renderer->isBoundingBoxPass());

	// The key type used for caching the geometry primitive:
	using CacheKey = std::tuple<
		CompatibleRendererGroup,	// The scene renderer
		VersionedDataObjectRef,		// The SimulationCellObject
		ColorA						// The wireframe color.
	>;

	// The values stored in the vis cache.
	struct CacheValue {
		std::shared_ptr<LinePrimitive> lines;
		std::shared_ptr<LinePrimitive> pickLines;
	};

	ColorA color = ViewportSettings::getSettings().viewportColor(contextNode->isSelected() ? ViewportSettings::COLOR_SELECTION : ViewportSettings::COLOR_UNSELECTED);

	// Lookup the rendering primitive in the vis cache.
	auto& wireframePrimitives = dataset()->visCache().get<CacheValue>(CacheKey(renderer, cell, color));

	// Check if we already have a valid rendering primitive that is up to date.
	if(!wireframePrimitives.lines || !wireframePrimitives.pickLines
			|| !wireframePrimitives.lines->isValid(renderer)
			|| !wireframePrimitives.pickLines->isValid(renderer)) {

		wireframePrimitives.lines = renderer->createLinePrimitive();
		wireframePrimitives.pickLines = renderer->createLinePrimitive();
		wireframePrimitives.lines->setVertexCount(cell->is2D() ? 8 : 24);
		wireframePrimitives.pickLines->setVertexCount(wireframePrimitives.lines->vertexCount(), renderer->defaultLinePickingWidth());
		Point3 corners[8];
		corners[0] = cell->cellOrigin();
		if(cell->is2D()) corners[0].z() = 0;
		corners[1] = corners[0] + cell->cellVector1();
		corners[2] = corners[1] + cell->cellVector2();
		corners[3] = corners[0] + cell->cellVector2();
		corners[4] = corners[0] + cell->cellVector3();
		corners[5] = corners[1] + cell->cellVector3();
		corners[6] = corners[2] + cell->cellVector3();
		corners[7] = corners[3] + cell->cellVector3();
		Point3 vertices[24] = {
			corners[0], corners[1],
			corners[1], corners[2],
			corners[2], corners[3],
			corners[3], corners[0],
			corners[4], corners[5],
			corners[5], corners[6],
			corners[6], corners[7],
			corners[7], corners[4],
			corners[0], corners[4],
			corners[1], corners[5],
			corners[2], corners[6],
			corners[3], corners[7]};
		wireframePrimitives.lines->setVertexPositions(vertices);
		wireframePrimitives.lines->setLineColor(color);
		wireframePrimitives.pickLines->setVertexPositions(vertices);
		wireframePrimitives.pickLines->setLineColor(color);
	}

	renderer->beginPickObject(contextNode);
	if(!renderer->isPicking())
		wireframePrimitives.lines->render(renderer);
	else
		wireframePrimitives.pickLines->render(renderer);
	renderer->endPickObject();
}

/******************************************************************************
* Renders the given simulation cell using solid shading mode.
******************************************************************************/
void SimulationCellVis::renderSolid(TimePoint time, const SimulationCellObject* cell, const PipelineFlowState& flowState, SceneRenderer* renderer, const PipelineSceneNode* contextNode)
{
	OVITO_ASSERT(!renderer->isBoundingBoxPass());

	// The key type used for caching the geometry primitive:
	using CacheKey = std::tuple<
		CompatibleRendererGroup,	// The scene renderer
		VersionedDataObjectRef,		// The simulation cell + revision number
		FloatType, Color			// Line width + color
	>;

	// The values stored in the vis cache.
	struct CacheValue {
		std::shared_ptr<ArrowPrimitive> lines;
		std::shared_ptr<ParticlePrimitive> corners;
	};

	// Lookup the rendering primitive in the vis cache.
	auto& solidPrimitives = dataset()->visCache().get<CacheValue>(CacheKey(renderer, cell, cellLineWidth(), cellColor()));

	// Check if we already have a valid rendering primitive that is up to date.
	if(!solidPrimitives.lines || !solidPrimitives.corners
			|| !solidPrimitives.lines->isValid(renderer)
			|| !solidPrimitives.corners->isValid(renderer)) {

		solidPrimitives.lines = renderer->createArrowPrimitive(ArrowPrimitive::CylinderShape, ArrowPrimitive::NormalShading, ArrowPrimitive::HighQuality);
		solidPrimitives.corners  = renderer->createParticlePrimitive(ParticlePrimitive::NormalShading, ParticlePrimitive::HighQuality);
		solidPrimitives.lines->startSetElements(cell->is2D() ? 4 : 12);
		ColorA color = (ColorA)cellColor();
		Point3 corners[8];
		corners[0] = cell->cellOrigin();
		if(cell->is2D()) corners[0].z() = 0;
		corners[1] = corners[0] + cell->cellVector1();
		corners[2] = corners[1] + cell->cellVector2();
		corners[3] = corners[0] + cell->cellVector2();
		corners[4] = corners[0] + cell->cellVector3();
		corners[5] = corners[1] + cell->cellVector3();
		corners[6] = corners[2] + cell->cellVector3();
		corners[7] = corners[3] + cell->cellVector3();
		solidPrimitives.lines->setElement(0, corners[0], corners[1] - corners[0], color, cellLineWidth());
		solidPrimitives.lines->setElement(1, corners[1], corners[2] - corners[1], color, cellLineWidth());
		solidPrimitives.lines->setElement(2, corners[2], corners[3] - corners[2], color, cellLineWidth());
		solidPrimitives.lines->setElement(3, corners[3], corners[0] - corners[3], color, cellLineWidth());
		if(cell->is2D() == false) {
			solidPrimitives.lines->setElement(4, corners[4], corners[5] - corners[4], color, cellLineWidth());
			solidPrimitives.lines->setElement(5, corners[5], corners[6] - corners[5], color, cellLineWidth());
			solidPrimitives.lines->setElement(6, corners[6], corners[7] - corners[6], color, cellLineWidth());
			solidPrimitives.lines->setElement(7, corners[7], corners[4] - corners[7], color, cellLineWidth());
			solidPrimitives.lines->setElement(8, corners[0], corners[4] - corners[0], color, cellLineWidth());
			solidPrimitives.lines->setElement(9, corners[1], corners[5] - corners[1], color, cellLineWidth());
			solidPrimitives.lines->setElement(10, corners[2], corners[6] - corners[2], color, cellLineWidth());
			solidPrimitives.lines->setElement(11, corners[3], corners[7] - corners[3], color, cellLineWidth());
		}
		solidPrimitives.lines->endSetElements();
		solidPrimitives.corners->setSize(cell->is2D() ? 4 : 8);
		solidPrimitives.corners->setParticlePositions(corners);
		solidPrimitives.corners->setParticleRadius(cellLineWidth());
		solidPrimitives.corners->setParticleColor(cellColor());
	}
	renderer->beginPickObject(contextNode);
	solidPrimitives.lines->render(renderer);
	solidPrimitives.corners->render(renderer);
	renderer->endPickObject();
}

}	// End of namespace
}	// End of namespace
