///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2018) Alexander Stukowski
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

#include <ovito/particles/Particles.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/core/utilities/units/UnitsManager.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/dataset/data/VersionedDataObjectRef.h>
#include <ovito/core/rendering/SceneRenderer.h>
#include "TrajectoryVis.h"

namespace Ovito { namespace Particles {

IMPLEMENT_OVITO_CLASS(TrajectoryVis);
DEFINE_PROPERTY_FIELD(TrajectoryVis, lineWidth);
DEFINE_PROPERTY_FIELD(TrajectoryVis, lineColor);
DEFINE_PROPERTY_FIELD(TrajectoryVis, shadingMode);
DEFINE_PROPERTY_FIELD(TrajectoryVis, showUpToCurrentTime);
DEFINE_PROPERTY_FIELD(TrajectoryVis, wrappedLines);
SET_PROPERTY_FIELD_LABEL(TrajectoryVis, lineWidth, "Line width");
SET_PROPERTY_FIELD_LABEL(TrajectoryVis, lineColor, "Line color");
SET_PROPERTY_FIELD_LABEL(TrajectoryVis, shadingMode, "Shading mode");
SET_PROPERTY_FIELD_LABEL(TrajectoryVis, showUpToCurrentTime, "Show up to current time only");
SET_PROPERTY_FIELD_LABEL(TrajectoryVis, wrappedLines, "Wrapped trajectory lines");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(TrajectoryVis, lineWidth, WorldParameterUnit, 0);

/******************************************************************************
* Constructor.
******************************************************************************/
TrajectoryVis::TrajectoryVis(DataSet* dataset) : DataVis(dataset),
	_lineWidth(0.2),
	_lineColor(0.6, 0.6, 0.6),
	_shadingMode(ArrowPrimitive::FlatShading),
	_showUpToCurrentTime(false),
	_wrappedLines(false)
{
}

/******************************************************************************
* Computes the bounding box of the object.
******************************************************************************/
Box3 TrajectoryVis::boundingBox(TimePoint time, const std::vector<const DataObject*>& objectStack, const PipelineSceneNode* contextNode, const PipelineFlowState& flowState, TimeInterval& validityInterval)
{
	const TrajectoryObject* trajObj = dynamic_object_cast<TrajectoryObject>(objectStack.back());

	// Get the simulation cell.
	const SimulationCellObject* simulationCell = wrappedLines() ? flowState.getObject<SimulationCellObject>() : nullptr;

	// The key type used for caching the computed bounding box:
	using CacheKey = std::tuple<
		VersionedDataObjectRef,		// The data object + revision number
		FloatType,					// Line width
		VersionedDataObjectRef		// Simulation cell + revision number
	>;

	// Look up the bounding box in the vis cache.
	auto& bbox = dataset()->visCache().get<Box3>(CacheKey(trajObj, lineWidth(), simulationCell));

	// Check if the cached bounding box information is still up to date.
	if(bbox.isEmpty()) {
		// If not, recompute bounding box from trajectory data.
		if(trajObj) {
			if(const PropertyObject* posProperty = trajObj->getProperty(TrajectoryObject::PositionProperty)) {
				if(!simulationCell) {
					bbox.addPoints(posProperty->constDataPoint3(), posProperty->size());
				}
				else {
					bbox = Box3(Point3(0,0,0), Point3(1,1,1)).transformed(simulationCell->cellMatrix());
				}
				bbox = bbox.padBox(lineWidth() / 2);
			}
		}
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

	// Get the simulation cell.
	const SimulationCellObject* simulationCell = wrappedLines() ? flowState.getObject<SimulationCellObject>() : nullptr;
	const SimulationCell cell = simulationCell ? simulationCell->data() : SimulationCell();

	// The key type used for caching the rendering primitive:
	using CacheKey = std::tuple<
		CompatibleRendererGroup,	// The scene renderer
		VersionedDataObjectRef,		// The trajectory data object + revision number
		FloatType,					// Line width
		Color,						// Line color,
		FloatType,					// End frame
		SimulationCell				// Simulation cell
	>;

	// The data structure stored in the vis cache.
	struct CacheValue {
		std::shared_ptr<ArrowPrimitive> segments;
		std::shared_ptr<ParticlePrimitive> corners;
	};

	// The shading mode.
	ParticlePrimitive::ShadingMode cornerShadingMode = (shadingMode() == ArrowPrimitive::NormalShading)
			? ParticlePrimitive::NormalShading : ParticlePrimitive::FlatShading;
	FloatType endFrame = showUpToCurrentTime() ? dataset()->animationSettings()->timeToFrame(time) : std::numeric_limits<FloatType>::max();

	// Lookup the rendering primitives in the vis cache.
	auto& renderingPrimitives = dataset()->visCache().get<CacheValue>(CacheKey(
			renderer,
			trajObj,
			lineWidth(),
			lineColor(),
			endFrame,
			cell));

	// Check if we already have a valid rendering primitives that are up to date.
	if(!renderingPrimitives.segments || !renderingPrimitives.corners
			|| !renderingPrimitives.segments->isValid(renderer)
			|| !renderingPrimitives.corners->isValid(renderer)
			|| !renderingPrimitives.segments->setShadingMode(shadingMode())
			|| !renderingPrimitives.corners->setShadingMode(cornerShadingMode)) {

		// Update the rendering primitives.
		renderingPrimitives.segments.reset();
		renderingPrimitives.corners.reset();

		FloatType lineRadius = lineWidth() / 2;
		if(trajObj && lineRadius > 0) {

			// Retrieve the line data stored in the TrajectoryObject.
			const PropertyObject* posProperty = trajObj->getProperty(TrajectoryObject::PositionProperty);
			const PropertyObject* timeProperty = trajObj->getProperty(TrajectoryObject::SampleTimeProperty);
			const PropertyObject* idProperty = trajObj->getProperty(TrajectoryObject::ParticleIdentifierProperty);
			if(posProperty && timeProperty && idProperty && posProperty->size() == timeProperty->size() && timeProperty->size() == idProperty->size() && posProperty->size() >= 2) {

				// Determine the number of line segments and corner points to render.
				size_t lineSegmentCount = 0;
				std::vector<Point3> cornerPoints;
				const Point3* pos = posProperty->constDataPoint3();
				const int* sampleTime = timeProperty->constDataInt();
				const qlonglong* id = idProperty->constDataInt64();
				if(!simulationCell) {
					for(auto pos_end = pos + posProperty->size() - 1; pos != pos_end; ++pos, ++sampleTime, ++id) {
						if(id[0] == id[1] && sampleTime[1] <= endFrame) {
							lineSegmentCount++;
							if(pos + 1 != pos_end && id[1] == id[2] && sampleTime[2] <= endFrame) {
								cornerPoints.push_back(pos[1]);
							}
						}
					}
				}
				else {
					for(auto pos_end = pos + posProperty->size() - 1; pos != pos_end; ++pos, ++sampleTime, ++id) {
						if(id[0] == id[1] && sampleTime[1] <= endFrame) {
							clipTrajectoryLine(pos[0], pos[1], cell, [&lineSegmentCount](const Point3& p1, const Point3& p2) {
								lineSegmentCount++;
							});
							if(pos + 1 != pos_end && id[1] == id[2] && sampleTime[2] <= endFrame) {
								cornerPoints.push_back(cell.wrapPoint(pos[1]));
							}
						}
					}
				}

				renderingPrimitives.segments = renderer->createArrowPrimitive(ArrowPrimitive::CylinderShape, shadingMode(), ArrowPrimitive::HighQuality);
				renderingPrimitives.corners = renderer->createParticlePrimitive(cornerShadingMode, ParticlePrimitive::HighQuality);

				renderingPrimitives.segments->startSetElements(lineSegmentCount);
				int lineSegmentIndex = 0;

				// Create the line segment geometry.
				pos = posProperty->constDataPoint3();
				sampleTime = timeProperty->constDataInt();
				id = idProperty->constDataInt64();
				ColorA color = lineColor();
				if(!simulationCell) {
					for(auto pos_end = pos + posProperty->size() - 1; pos != pos_end; ++pos, ++sampleTime, ++id) {
						if(id[0] == id[1] && sampleTime[1] <= endFrame) {
							renderingPrimitives.segments->setElement(lineSegmentIndex++, pos[0], pos[1] - pos[0], color, lineRadius);
						}
					}
				}
				else {
					for(auto pos_end = pos + posProperty->size() - 1; pos != pos_end; ++pos, ++sampleTime, ++id) {
						if(id[0] == id[1] && sampleTime[1] <= endFrame) {
							clipTrajectoryLine(pos[0], pos[1], cell, [&](const Point3& p1, const Point3& p2) {
								renderingPrimitives.segments->setElement(lineSegmentIndex++, p1, p2 - p1, color, lineRadius);
							});
						}
					}
				}
				OVITO_ASSERT(lineSegmentIndex == lineSegmentCount);
				renderingPrimitives.segments->endSetElements();

				// Create corner points.
				renderingPrimitives.corners->setSize(cornerPoints.size());
				if(!cornerPoints.empty())
					renderingPrimitives.corners->setParticlePositions(cornerPoints.data());
				renderingPrimitives.corners->setParticleColor(color);
				renderingPrimitives.corners->setParticleRadius(lineRadius);
			}
		}
	}

	if(!renderingPrimitives.segments)
		return;

	renderer->beginPickObject(contextNode);
	renderingPrimitives.segments->render(renderer);
	renderingPrimitives.corners->render(renderer);
	renderer->endPickObject();
}

/******************************************************************************
* Clips a trajectory line at the periodic box boundaries.
******************************************************************************/
void TrajectoryVis::clipTrajectoryLine(const Point3& v1, const Point3& v2, const SimulationCell& simulationCell, const std::function<void(const Point3&, const Point3&)>& segmentCallback)
{
	Point3 rp1 = simulationCell.absoluteToReduced(v1);
	Vector3 shiftVector = Vector3::Zero();
	for(size_t dim = 0; dim < 3; dim++) {
		if(simulationCell.pbcFlags()[dim]) {
			while(rp1[dim] >= 1) { rp1[dim] -= 1; shiftVector[dim] -= 1; }
			while(rp1[dim] < 0) { rp1[dim] += 1; shiftVector[dim] += 1; }
		}
	}
	Point3 rp2 = simulationCell.absoluteToReduced(v2) + shiftVector;
	FloatType smallestT;
	bool clippedDimensions[3] = { false, false, false };
	do {
		size_t crossDim;
		FloatType crossDir;
		smallestT = FLOATTYPE_MAX;
		for(size_t dim = 0; dim < 3; dim++) {
			if(simulationCell.pbcFlags()[dim] && !clippedDimensions[dim]) {
				int d = (int)std::floor(rp2[dim]) - (int)std::floor(rp1[dim]);
				if(d == 0) continue;
				FloatType t;
				if(d > 0)
					t = (std::ceil(rp1[dim]) - rp1[dim]) / (rp2[dim] - rp1[dim]);
				else
					t = (std::floor(rp1[dim]) - rp1[dim]) / (rp2[dim] - rp1[dim]);
				if(t >= 0 && t < smallestT) {
					smallestT = t;
					crossDim = dim;
					crossDir = (d > 0) ? 1 : -1;
				}
			}
		}
		if(smallestT != FLOATTYPE_MAX) {
			clippedDimensions[crossDim] = true;
			Point3 intersection = rp1 + smallestT * (rp2 - rp1);
			intersection[crossDim] = std::floor(intersection[crossDim] + FloatType(0.5));
			Point3 rp1abs = simulationCell.reducedToAbsolute(rp1);
			Point3 intabs = simulationCell.reducedToAbsolute(intersection);
			if(!intabs.equals(rp1abs)) {
				segmentCallback(rp1abs, intabs);
			}
			shiftVector[crossDim] -= crossDir;
			rp1 = intersection;
			rp1[crossDim] -= crossDir;
			rp2[crossDim] -= crossDir;
		}
	}
	while(smallestT != FLOATTYPE_MAX);

	segmentCallback(simulationCell.reducedToAbsolute(rp1), simulationCell.reducedToAbsolute(rp2));
}

}	// End of namespace
}	// End of namespace
