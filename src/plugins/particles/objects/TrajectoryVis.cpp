///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2015) Alexander Stukowski
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

#include <plugins/particles/Particles.h>
#include <core/utilities/units/UnitsManager.h>
#include <core/dataset/DataSet.h>
#include <core/dataset/data/VersionedDataObjectRef.h>
#include <core/rendering/SceneRenderer.h>
#include "TrajectoryVis.h"

namespace Ovito { namespace Particles {

IMPLEMENT_OVITO_CLASS(TrajectoryVis);	
DEFINE_PROPERTY_FIELD(TrajectoryVis, lineWidth);
DEFINE_PROPERTY_FIELD(TrajectoryVis, lineColor);
DEFINE_PROPERTY_FIELD(TrajectoryVis, shadingMode);
DEFINE_PROPERTY_FIELD(TrajectoryVis, showUpToCurrentTime);
SET_PROPERTY_FIELD_LABEL(TrajectoryVis, lineWidth, "Line width");
SET_PROPERTY_FIELD_LABEL(TrajectoryVis, lineColor, "Line color");
SET_PROPERTY_FIELD_LABEL(TrajectoryVis, shadingMode, "Shading mode");
SET_PROPERTY_FIELD_LABEL(TrajectoryVis, showUpToCurrentTime, "Show up to current time only");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(TrajectoryVis, lineWidth, WorldParameterUnit, 0);

/******************************************************************************
* Constructor.
******************************************************************************/
TrajectoryVis::TrajectoryVis(DataSet* dataset) : DataVis(dataset),
	_lineWidth(0.2), 
	_lineColor(0.6, 0.6, 0.6),
	_shadingMode(ArrowPrimitive::FlatShading), 
	_showUpToCurrentTime(false)
{
}

/******************************************************************************
* Computes the bounding box of the object.
******************************************************************************/
Box3 TrajectoryVis::boundingBox(TimePoint time, const std::vector<const DataObject*>& objectStack, const PipelineSceneNode* contextNode, const PipelineFlowState& flowState, TimeInterval& validityInterval)
{
	const TrajectoryObject* trajObj = dynamic_object_cast<TrajectoryObject>(objectStack.back());

	// The key type used for caching the computed bounding box:
	using CacheKey = std::tuple<
		VersionedDataObjectRef,		// The data object + revision number
		FloatType					// Line width
	>;

	// Look up the bounding box in the vis cache.
	auto& bbox = dataset()->visCache().get<Box3>(CacheKey(trajObj, lineWidth()));

	// Check if the cached bounding box information is still up to date.
	if(bbox.isEmpty()) {
		// If not, recompute bounding box from trajectory data.
		if(trajObj)
			bbox.addPoints(trajObj->points().constData(), trajObj->points().size());
	}
	return bbox;
}

/******************************************************************************
* Lets the visualization element render the data object.
******************************************************************************/
void TrajectoryVis::render(TimePoint time, const std::vector<const DataObject*>& objectStack, const PipelineFlowState& flowState, SceneRenderer* renderer, const PipelineSceneNode* contextNode)
{
	if(renderer->isBoundingBoxPass()) {
		TimeInterval validityInterval;
		renderer->addToLocalBoundingBox(boundingBox(time, objectStack, contextNode, flowState, validityInterval));
		return;
	}
	
	const TrajectoryObject* trajObj = dynamic_object_cast<TrajectoryObject>(objectStack.back());

	// The key type used for caching the rendering primitive:
	using CacheKey = std::tuple<
		CompatibleRendererGroup,	// The scene renderer
		VersionedDataObjectRef,		// The trajectory data object + revision number
		FloatType,					// Line width
		Color,						// Line color,
		TimePoint					// End time
	>;

	// The values stored in the vis cache.
	struct CacheValue {
		std::shared_ptr<ArrowPrimitive> segments;
		std::shared_ptr<ParticlePrimitive> corners;
	};

	// The shading mode.
	ParticlePrimitive::ShadingMode cornerShadingMode = (shadingMode() == ArrowPrimitive::NormalShading)
			? ParticlePrimitive::NormalShading : ParticlePrimitive::FlatShading;
	TimePoint endTime = showUpToCurrentTime() ? time : TimePositiveInfinity();

	// Lookup the rendering primitives in the vis cache.
	auto& renderingPrimitives = dataset()->visCache().get<CacheValue>(CacheKey(
			renderer,
			trajObj, 
			lineWidth(), 
			lineColor(), 
			endTime));

	// Check if we already have a valid rendering primitives that are up to date.
	if(!renderingPrimitives.segments || !renderingPrimitives.corners
			|| !renderingPrimitives.segments->isValid(renderer)
			|| !renderingPrimitives.corners->isValid(renderer)
			|| !renderingPrimitives.segments->setShadingMode(shadingMode())
			|| !renderingPrimitives.corners->setShadingMode(cornerShadingMode)) {

		// Re-create the geometry buffers.
		FloatType lineRadius = lineWidth() / 2;
		if(trajObj && lineRadius > 0) {
			renderingPrimitives.segments = renderer->createArrowPrimitive(ArrowPrimitive::CylinderShape, shadingMode(), ArrowPrimitive::HighQuality);
			renderingPrimitives.corners = renderer->createParticlePrimitive(cornerShadingMode, ParticlePrimitive::HighQuality);

			int timeSamples = std::upper_bound(trajObj->sampleTimes().cbegin(), trajObj->sampleTimes().cend(), endTime) - trajObj->sampleTimes().cbegin();

			int lineSegmentCount = std::max(0, timeSamples - 1) * trajObj->trajectoryCount();

			renderingPrimitives.segments->startSetElements(lineSegmentCount);
			int lineSegmentIndex = 0;
			for(int pindex = 0; pindex < trajObj->trajectoryCount(); pindex++) {
				for(int tindex = 0; tindex < timeSamples - 1; tindex++) {
					const Point3& p1 = trajObj->points()[tindex * trajObj->trajectoryCount() + pindex];
					const Point3& p2 = trajObj->points()[(tindex+1) * trajObj->trajectoryCount() + pindex];
					renderingPrimitives.segments->setElement(lineSegmentIndex++, p1, p2 - p1, ColorA(lineColor()), lineRadius);
				}
			}
			renderingPrimitives.segments->endSetElements();

			int pointCount = std::max(0, timeSamples - 2) * trajObj->trajectoryCount();
			renderingPrimitives.corners->setSize(pointCount);
			if(pointCount)
				renderingPrimitives.corners->setParticlePositions(trajObj->points().constData() + trajObj->trajectoryCount());
			renderingPrimitives.corners->setParticleColor(ColorA(lineColor()));
			renderingPrimitives.corners->setParticleRadius(lineRadius);
		}
		else {
			renderingPrimitives.segments.reset();
			renderingPrimitives.corners.reset();
		}
	}

	if(!renderingPrimitives.segments)
		return;

	renderer->beginPickObject(contextNode);
	renderingPrimitives.segments->render(renderer);
	renderingPrimitives.corners->render(renderer);
	renderer->endPickObject();
}

}	// End of namespace
}	// End of namespace
