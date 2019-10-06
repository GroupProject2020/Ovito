///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2019) Alexander Stukowski
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

#include <ovito/crystalanalysis/CrystalAnalysis.h>
#include <ovito/crystalanalysis/objects/Microstructure.h>
#include <ovito/crystalanalysis/objects/ClusterGraphObject.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/core/dataset/data/VersionedDataObjectRef.h>
#include <ovito/core/rendering/ParticlePrimitive.h>
#include <ovito/core/rendering/ArrowPrimitive.h>
#include <ovito/core/rendering/SceneRenderer.h>
#include "DislocationVis.h"
#include "RenderableDislocationLines.h"

namespace Ovito { namespace CrystalAnalysis {

IMPLEMENT_OVITO_CLASS(DislocationVis);
DEFINE_PROPERTY_FIELD(DislocationVis, lineWidth);
DEFINE_PROPERTY_FIELD(DislocationVis, shadingMode);
DEFINE_PROPERTY_FIELD(DislocationVis, burgersVectorWidth);
DEFINE_PROPERTY_FIELD(DislocationVis, burgersVectorScaling);
DEFINE_PROPERTY_FIELD(DislocationVis, burgersVectorColor);
DEFINE_PROPERTY_FIELD(DislocationVis, showBurgersVectors);
DEFINE_PROPERTY_FIELD(DislocationVis, showLineDirections);
DEFINE_PROPERTY_FIELD(DislocationVis, lineColoringMode);
SET_PROPERTY_FIELD_LABEL(DislocationVis, lineWidth, "Line width");
SET_PROPERTY_FIELD_LABEL(DislocationVis, shadingMode, "Shading mode");
SET_PROPERTY_FIELD_LABEL(DislocationVis, burgersVectorWidth, "Burgers vector width");
SET_PROPERTY_FIELD_LABEL(DislocationVis, burgersVectorScaling, "Burgers vector scaling");
SET_PROPERTY_FIELD_LABEL(DislocationVis, burgersVectorColor, "Burgers vector color");
SET_PROPERTY_FIELD_LABEL(DislocationVis, showBurgersVectors, "Show Burgers vectors");
SET_PROPERTY_FIELD_LABEL(DislocationVis, showLineDirections, "Indicate line directions");
SET_PROPERTY_FIELD_LABEL(DislocationVis, lineColoringMode, "Line coloring");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(DislocationVis, lineWidth, WorldParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(DislocationVis, burgersVectorWidth, WorldParameterUnit, 0);

IMPLEMENT_OVITO_CLASS(DislocationPickInfo);

/******************************************************************************
* Constructor.
******************************************************************************/
DislocationVis::DislocationVis(DataSet* dataset) : TransformingDataVis(dataset),
	_lineWidth(1.0),
	_shadingMode(ArrowPrimitive::NormalShading),
	_burgersVectorWidth(0.6),
	_burgersVectorScaling(3.0),
	_burgersVectorColor(0.7, 0.7, 0.7),
	_showBurgersVectors(false),
	_showLineDirections(false),
	_lineColoringMode(ColorByDislocationType)
{
}

/******************************************************************************
* Lets the vis element transform a data object in preparation for rendering.
******************************************************************************/
Future<PipelineFlowState> DislocationVis::transformDataImpl(TimePoint time, const DataObject* dataObject, PipelineFlowState&& flowState, const PipelineFlowState& cachedState, const PipelineSceneNode* contextNode)
{
	// Get the input object.
	const PeriodicDomainDataObject* periodicDomainObj = dynamic_object_cast<PeriodicDomainDataObject>(dataObject);
	if(!periodicDomainObj)
		return std::move(flowState);

	// Get the simulation cell (must be 3D).
	const SimulationCellObject* cellObject = periodicDomainObj->domain();
	if(!cellObject || cellObject->is2D())
		return std::move(flowState);

	// Generate the list of clipped line segments.
	const SimulationCell cellData = cellObject->data();
	std::vector<RenderableDislocationLines::Segment> outputSegments;

	if(const DislocationNetworkObject* dislocationsObj = dynamic_object_cast<DislocationNetworkObject>(periodicDomainObj)) {
		// Convert the dislocations object.
		int segmentIndex = 0;
		for(const DislocationSegment* segment : dislocationsObj->segments()) {
			const ClusterVector& b = segment->burgersVector;
			// Determine the Burgers vector family the dislocation segment belongs to.
			if(const MicrostructurePhase* phase = dislocationsObj->structureById(b.cluster()->structure)) {
				const BurgersVectorFamily* family = phase->defaultBurgersVectorFamily();
				for(const BurgersVectorFamily* f : phase->burgersVectorFamilies()) {
					if(f->isMember(b.localVec(), phase)) {
						family = f;
						break;
					}
				}
				// Don't render dislocation segment if the Burgers vector family has been disabled.
				if(family && !family->enabled()) {
					segmentIndex++;
					continue;
				}
			}
			clipDislocationLine(segment->line, cellData, periodicDomainObj->cuttingPlanes(), [segmentIndex, &outputSegments, &b](const Point3& p1, const Point3& p2, bool isInitialSegment) {
				outputSegments.push_back({ { p1, p2 }, b.localVec(), b.cluster()->id, segmentIndex });
			});
			segmentIndex++;
		}
	}
	else if(const Microstructure* microstructureObj = dynamic_object_cast<Microstructure>(periodicDomainObj)) {
		// Extract the dislocation segments from the microstructure object.
		std::deque<Point3> line(2);
		microstructureObj->verifyMeshIntegrity();
		const PropertyObject* phaseProperty = microstructureObj->regions()->getProperty(SurfaceMeshRegions::PhaseProperty);
		MicrostructureData mdata(microstructureObj);
		// Since every dislocation line is represented by a pair two directed lines in the data structure,
		// make sure we render only every other dislocation line (the "even" ones).
		for(MicrostructureData::face_index face = 0; face < mdata.faceCount(); face += 2) {
			if(mdata.isDislocationFace(face)) {
				const Vector3& b = mdata.burgersVector(face);
				MicrostructureData::region_index region = mdata.faceRegion(face);

				// Determine if the display of dislocations of this type is enabled.
				int phaseId = mdata.regionPhase(region);
				if(const MicrostructurePhase* phase = dynamic_object_cast<MicrostructurePhase>(phaseProperty->elementType(phaseId))) {
					const BurgersVectorFamily* family = phase->defaultBurgersVectorFamily();
					for(const BurgersVectorFamily* f : phase->burgersVectorFamilies()) {
						if(f->isMember(b, phase)) {
							family = f;
							break;
						}
					}
					if(family && !family->enabled()) {
						continue;
					}
				}

				// Walk along the sequence of segments that make up the continous dislocation line.
				MicrostructureData::edge_index edge = mdata.firstFaceEdge(face);
				Point3 p = mdata.vertexPosition(mdata.vertex1(edge));
				do {
					line[0] = p;
					p += mdata.edgeVector(edge);
					line[1] = p;
					clipDislocationLine(line, cellData, periodicDomainObj->cuttingPlanes(), [face, &outputSegments, &b, region](const Point3& p1, const Point3& p2, bool isInitialSegment) {
						outputSegments.push_back({ { p1, p2 }, b, region, face });
					});
					MicrostructureData::vertex_index v1 = mdata.vertex1(edge);
					edge = mdata.nextFaceEdge(edge);
					if(mdata.vertex2(edge) == v1) break;	// Reached end of continuous dislocation line.
				}
				while(edge != mdata.firstFaceEdge(face));
			}
		}
	}

	// Create output RenderableDislocationLines object.
	OORef<RenderableDislocationLines> renderableLines = new RenderableDislocationLines(this, dataObject);
	renderableLines->setLineSegments(std::move(outputSegments));
	flowState.addObject(renderableLines);

	return std::move(flowState);
}

/******************************************************************************
* Computes the bounding box of the object.
******************************************************************************/
Box3 DislocationVis::boundingBox(TimePoint time, const std::vector<const DataObject*>& objectStack, const PipelineSceneNode* contextNode, const PipelineFlowState& flowState, TimeInterval& validityInterval)
{
	const RenderableDislocationLines* renderableObj = dynamic_object_cast<RenderableDislocationLines>(objectStack.back());
	if(!renderableObj) return {};
	const PeriodicDomainDataObject* domainObj = dynamic_object_cast<PeriodicDomainDataObject>(renderableObj->sourceDataObject().get());
	if(!domainObj) return {};
	const SimulationCellObject* cellObject = domainObj->domain();
	if(!cellObject) return {};
	SimulationCell cell = cellObject->data();

	// The key type used for caching the computed bounding box:
	using CacheKey = std::tuple<
		VersionedDataObjectRef,	// Source object + revision number
		SimulationCell,			// Simulation cell geometry
		FloatType,				// Line width
		bool,					// Burgers vector display
		FloatType,				// Burgers vectors scaling
		FloatType				// Burgers vector width
	>;

	// Look up the bounding box in the vis cache.
	auto& bbox = dataset()->visCache().get<Box3>(CacheKey(
			renderableObj,
			cell,
			lineWidth(),
			showBurgersVectors(),
			burgersVectorScaling(),
			burgersVectorWidth()));

	// Check if the cached bounding box information is still up to date.
	if(bbox.isEmpty()) {

		// If not, recompute bounding box from dislocation data.
		Box3 bb = Box3(Point3(0,0,0), Point3(1,1,1)).transformed(cellObject->cellMatrix());
		FloatType padding = std::max(lineWidth(), FloatType(0));

		if(showBurgersVectors()) {
			padding = std::max(padding, burgersVectorWidth() * FloatType(2));
			if(const DislocationNetworkObject* dislocationObj = dynamic_object_cast<DislocationNetworkObject>(domainObj)) {
				for(const DislocationSegment* segment : dislocationObj->segments()) {
					Point3 center = cell.wrapPoint(segment->getPointOnLine(FloatType(0.5)));
					Vector3 dir = burgersVectorScaling() * segment->burgersVector.toSpatialVector();
					bb.addPoint(center + dir);
				}
			}
		}
		bbox = bb.padBox(padding * FloatType(0.5));
	}
	return bbox;
}

/******************************************************************************
* Lets the vis element render a data object.
******************************************************************************/
void DislocationVis::render(TimePoint time, const std::vector<const DataObject*>& objectStack, const PipelineFlowState& flowState, SceneRenderer* renderer, const PipelineSceneNode* contextNode)
{
	// Ignore render calls for the original DislocationNetworkObject or MicrostrucureObject.
	// We are only interested in the RenderableDIslocationLines.
	if(dynamic_object_cast<DislocationNetworkObject>(objectStack.back())) return;
	if(dynamic_object_cast<Microstructure>(objectStack.back())) return;

	// Just compute the bounding box of the rendered objects if requested.
	if(renderer->isBoundingBoxPass()) {
		TimeInterval validityInterval;
		renderer->addToLocalBoundingBox(boundingBox(time, objectStack, contextNode, flowState, validityInterval));
		return;
	}

	// The key type used for caching the rendering primitives:
	using CacheKey = std::tuple<
		CompatibleRendererGroup,// The scene renderer
		VersionedDataObjectRef,	// Source object + revision number
		VersionedDataObjectRef,	// Renderable object + revision number
		SimulationCell,			// Simulation cell geometry
		FloatType,				// Line width
		bool,					// Burgers vector display
		FloatType,				// Burgers vectors scaling
		FloatType,				// Burgers vector width
		Color,					// Burgers vector color
		LineColoringMode		// Way to color lines
	>;

	// The values stored in the vis cache.
	struct CacheValue {
		std::shared_ptr<ArrowPrimitive> segments;
		std::shared_ptr<ParticlePrimitive> corners;
		std::shared_ptr<ArrowPrimitive> burgersArrows;
		OORef<DislocationPickInfo> pickInfo;
	};

	ArrowPrimitive::Shape segmentShape = (showLineDirections() ? ArrowPrimitive::ArrowShape : ArrowPrimitive::CylinderShape);
	ParticlePrimitive::ShadingMode cornerShadingMode = (shadingMode() == ArrowPrimitive::NormalShading)
			? ParticlePrimitive::NormalShading : ParticlePrimitive::FlatShading;

	// Get the renderable dislocation lines.
	const RenderableDislocationLines* renderableLines = dynamic_object_cast<RenderableDislocationLines>(objectStack.back());
	if(!renderableLines) return;

	// Make sure we don't exceed our internal limits.
	if(renderableLines->lineSegments().size() > (size_t)std::numeric_limits<int>::max()) {
		qWarning() << "WARNING: Cannot render more than" << std::numeric_limits<int>::max() << "dislocation segments.";
		return;
	}

	// Get the original dislocation lines.
	const PeriodicDomainDataObject* domainObj = dynamic_object_cast<PeriodicDomainDataObject>(renderableLines->sourceDataObject().get());
	const DislocationNetworkObject* dislocationsObj = dynamic_object_cast<DislocationNetworkObject>(domainObj);
	const Microstructure* microstructureObj = dynamic_object_cast<Microstructure>(domainObj);
	const PropertyObject* phaseProperty = microstructureObj ? microstructureObj->regions()->getProperty(SurfaceMeshRegions::PhaseProperty) : nullptr;
	const PropertyObject* correspondenceProperty = microstructureObj ? microstructureObj->regions()->getProperty(SurfaceMeshRegions::LatticeCorrespondenceProperty) : nullptr;
	const ClusterGraphObject* clusterGraphObj = flowState.getObject<ClusterGraphObject>();
	if(!dislocationsObj && !microstructureObj) return;

	// Get the simulation cell.
	const SimulationCellObject* cellObject = domainObj->domain();
	if(!cellObject) return;

	// Lookup the rendering primitives in the vis cache.
	auto& primitives = dataset()->visCache().get<CacheValue>(CacheKey(
		renderer,
		domainObj,
		renderableLines,
		cellObject->data(),
		lineWidth(),
		showBurgersVectors(),
		burgersVectorScaling(),
		burgersVectorWidth(),
		burgersVectorColor(),
		lineColoringMode()));

	// Check if we already have valid rendering primitives that are up to date.
	if(!primitives.segments || !primitives.corners || !primitives.burgersArrows
			|| !primitives.segments->isValid(renderer)
			|| !primitives.corners->isValid(renderer)
			|| !primitives.burgersArrows->isValid(renderer)
			|| !primitives.segments->setShadingMode(shadingMode())
			|| !primitives.corners->setShadingMode(cornerShadingMode)
			|| !primitives.burgersArrows->setShadingMode(shadingMode())
			|| primitives.segments->shape() != segmentShape) {

		primitives.segments = renderer->createArrowPrimitive(segmentShape, shadingMode(), ArrowPrimitive::HighQuality);
		primitives.corners = renderer->createParticlePrimitive(cornerShadingMode, ParticlePrimitive::HighQuality);
		primitives.burgersArrows.reset();

		SimulationCell cellData = cellObject->data();
		// First determine number of corner vertices/segments that are going to be rendered.
		int lineSegmentCount = renderableLines->lineSegments().size();
		int cornerCount = 0;
		for(size_t i = 1; i < renderableLines->lineSegments().size(); i++) {
			const auto& s1 = renderableLines->lineSegments()[i-1];
			const auto& s2 = renderableLines->lineSegments()[i];
			if(s1.verts[1].equals(s2.verts[0])) cornerCount++;
		}
		// Allocate render buffer.
		primitives.segments->startSetElements(lineSegmentCount);
		std::vector<int> subobjToSegmentMap(lineSegmentCount + cornerCount);
		FloatType lineRadius = std::max(lineWidth() / 2, FloatType(0));
		std::vector<Point3> cornerPoints;
		std::vector<Color> cornerColors;
		cornerPoints.reserve(cornerCount);
		cornerColors.reserve(cornerCount);
		Color lineColor;
		Vector3 normalizedBurgersVector;
		Vector3 lastBurgersVector = Vector3::Zero();
		int lastRegion = -1;
		for(size_t lineSegmentIndex = 0; lineSegmentIndex < renderableLines->lineSegments().size(); lineSegmentIndex++) {
			const auto& lineSegment = renderableLines->lineSegments()[lineSegmentIndex];
			if(lineSegment.burgersVector != lastBurgersVector || lineSegment.region != lastRegion) {
				lastBurgersVector = lineSegment.burgersVector;
				lastRegion = lineSegment.region;
				lineColor = Color(0.8f,0.8f,0.8f);
				const MicrostructurePhase* phase = nullptr;
				if(dislocationsObj && clusterGraphObj) {
					Cluster* cluster = clusterGraphObj->storage()->findCluster(lineSegment.region);
					OVITO_ASSERT(cluster != nullptr);
					phase = dislocationsObj->structureById(cluster->structure);
					normalizedBurgersVector = ClusterVector(lineSegment.burgersVector, cluster).toSpatialVector();
					normalizedBurgersVector.normalizeSafely();
				}
				else if(phaseProperty && lineSegment.region >= 0 && lineSegment.region < phaseProperty->size()) {
					int phaseId = phaseProperty->getInt(lineSegment.region);
					phase = dynamic_object_cast<MicrostructurePhase>(phaseProperty->elementType(phaseId));
					if(correspondenceProperty) {
						normalizedBurgersVector = correspondenceProperty->getMatrix3(lineSegment.region) * lineSegment.burgersVector;
						normalizedBurgersVector.normalizeSafely();
					}
					else {
						normalizedBurgersVector = lineSegment.burgersVector.safelyNormalized();
					}
				}
				if(phase) {
					if(lineColoringMode() == ColorByDislocationType) {
						const BurgersVectorFamily* family = phase->defaultBurgersVectorFamily();
						for(const BurgersVectorFamily* f : phase->burgersVectorFamilies()) {
							if(f->isMember(lineSegment.burgersVector, phase)) {
								family = f;
								break;
							}
						}
						if(family)
							lineColor = family->color();
					}
					else if(lineColoringMode() == ColorByBurgersVector) {
						lineColor = MicrostructurePhase::getBurgersVectorColor(phase->name(), lineSegment.burgersVector);
					}
				}
			}
			subobjToSegmentMap[lineSegmentIndex] = lineSegment.dislocationIndex;
			Vector3 delta = lineSegment.verts[1] - lineSegment.verts[0];
			if(lineColoringMode() == ColorByCharacter) {
				FloatType dot = std::abs(delta.dot(normalizedBurgersVector));
				if(dot != 0) dot /= delta.length();
				if(dot > 1) dot = 1;
				FloatType angle = std::acos(dot) / (FLOATTYPE_PI/2);
				if(angle <= FloatType(0.5))
					lineColor = Color(1, angle * 2, angle * 2);
				else
					lineColor = Color((FloatType(1)-angle) * 2, (FloatType(1)-angle) * 2, 1);
			}
			primitives.segments->setElement(lineSegmentIndex, lineSegment.verts[0], delta, ColorA(lineColor), lineRadius);
			if(lineSegmentIndex != 0 && lineSegment.verts[0].equals(renderableLines->lineSegments()[lineSegmentIndex-1].verts[1])) {
				subobjToSegmentMap[cornerPoints.size() + lineSegmentCount] = lineSegment.dislocationIndex;
				cornerPoints.push_back(lineSegment.verts[0]);
				cornerColors.push_back(lineColor);
			}
		}
		OVITO_ASSERT(cornerPoints.size() == cornerCount);
		primitives.segments->endSetElements();
		primitives.corners->setSize(cornerPoints.size());
		primitives.corners->setParticlePositions(cornerPoints.empty() ? nullptr : cornerPoints.data());
		primitives.corners->setParticleColors(cornerColors.empty() ? nullptr : cornerColors.data());
		primitives.corners->setParticleRadius(lineRadius);

		if(dislocationsObj) {
			if(showBurgersVectors()) {
				primitives.burgersArrows = renderer->createArrowPrimitive(ArrowPrimitive::ArrowShape, shadingMode(), ArrowPrimitive::HighQuality);
				primitives.burgersArrows->startSetElements(dislocationsObj->segments().size());
				subobjToSegmentMap.reserve(subobjToSegmentMap.size() + dislocationsObj->segments().size());
				int arrowIndex = 0;
				ColorA arrowColor = burgersVectorColor();
				FloatType arrowRadius = std::max(burgersVectorWidth() / 2, FloatType(0));
				for(const DislocationSegment* segment : dislocationsObj->segments()) {
					subobjToSegmentMap.push_back(arrowIndex);
					Point3 center = cellData.wrapPoint(segment->getPointOnLine(FloatType(0.5)));
					Vector3 dir = burgersVectorScaling() * segment->burgersVector.toSpatialVector();
					// Check if arrow is clipped away by cutting planes.
					for(const Plane3& plane : dislocationsObj->cuttingPlanes()) {
						if(plane.classifyPoint(center) > 0) {
							dir.setZero(); // Hide arrow by setting length to zero.
							break;
						}
					}
					primitives.burgersArrows->setElement(arrowIndex++, center, dir, arrowColor, arrowRadius);
				}
				primitives.burgersArrows->endSetElements();
			}
			primitives.pickInfo = new DislocationPickInfo(this, dislocationsObj, std::move(subobjToSegmentMap));
		}
		else if(microstructureObj) {
			primitives.pickInfo = new DislocationPickInfo(this, microstructureObj, std::move(subobjToSegmentMap));
		}
	}

	renderer->beginPickObject(contextNode, primitives.pickInfo);

	// Render dislocation segments.
	primitives.segments->render(renderer);

	// Render segment vertices.
	primitives.corners->render(renderer);

	// Render Burgers vectors.
	if(primitives.burgersArrows && showBurgersVectors()) {
		primitives.burgersArrows->render(renderer);
	}

	renderer->endPickObject();
}

/******************************************************************************
* Renders an overlay marker for a single dislocation segment.
******************************************************************************/
void DislocationVis::renderOverlayMarker(TimePoint time, const DataObject* dataObject, const PipelineFlowState& flowState, int segmentIndex, SceneRenderer* renderer, const PipelineSceneNode* contextNode)
{
	if(renderer->isPicking())
		return;

	// Get the dislocations.
	const DislocationNetworkObject* dislocationsObj = dynamic_object_cast<DislocationNetworkObject>(dataObject);
	if(!dislocationsObj)
		return;

	// Get the simulation cell.
	const SimulationCellObject* cellObject = dislocationsObj->domain();
	if(!cellObject)
		return;
	SimulationCell cellData = cellObject->data();

	if(segmentIndex < 0 || segmentIndex >= dislocationsObj->segments().size())
		return;

	const DislocationSegment* segment = dislocationsObj->segments()[segmentIndex];

	// Generate the polyline segments to render.
	std::vector<std::pair<Point3,Point3>> lineSegments;
	std::vector<Point3> cornerVertices;
	clipDislocationLine(segment->line, cellData, dislocationsObj->cuttingPlanes(), [&lineSegments, &cornerVertices](const Point3& v1, const Point3& v2, bool isInitialSegment) {
		lineSegments.push_back({v1,v2});
		if(!isInitialSegment)
			cornerVertices.push_back(v1);
	});

	// Set up transformation.
	TimeInterval iv;
	const AffineTransformation& nodeTM = contextNode->getWorldTransform(time, iv);
	renderer->setWorldTransform(nodeTM);
	FloatType lineRadius = std::max(lineWidth() / 4, FloatType(0));
	FloatType headRadius = lineRadius * 3;

	// Compute bounding box if requested.
	if(renderer->isBoundingBoxPass()) {
		Box3 bb;
		for(const auto& seg : lineSegments) {
			bb.addPoint(seg.first);
			bb.addPoint(seg.second);
		}
		renderer->addToLocalBoundingBox(bb.padBox(headRadius));
		return;
	}

	// Draw the marker on top of everything.
	renderer->setDepthTestEnabled(false);

	std::shared_ptr<ArrowPrimitive> segmentBuffer = renderer->createArrowPrimitive(ArrowPrimitive::CylinderShape, ArrowPrimitive::FlatShading, ArrowPrimitive::HighQuality);
	segmentBuffer->startSetElements(lineSegments.size());
	int index = 0;
	for(const auto& seg : lineSegments)
		segmentBuffer->setElement(index++, seg.first, seg.second - seg.first, ColorA(1,1,1), lineRadius);
	segmentBuffer->endSetElements();
	segmentBuffer->render(renderer);

	std::shared_ptr<ParticlePrimitive> cornerBuffer = renderer->createParticlePrimitive(ParticlePrimitive::FlatShading, ParticlePrimitive::HighQuality);
	cornerBuffer->setSize(cornerVertices.size());
	cornerBuffer->setParticlePositions(cornerVertices.data());
	cornerBuffer->setParticleColor(Color(1,1,1));
	cornerBuffer->setParticleRadius(lineRadius);
	cornerBuffer->render(renderer);

	if(!segment->line.empty()) {
		Point3 wrappedHeadPos = cellData.wrapPoint(segment->line.front());
		std::shared_ptr<ParticlePrimitive> headBuffer = renderer->createParticlePrimitive(ParticlePrimitive::FlatShading, ParticlePrimitive::HighQuality);
		headBuffer->setSize(1);
		headBuffer->setParticlePositions(&wrappedHeadPos);
		headBuffer->setParticleColor(Color(1,1,1));
		headBuffer->setParticleRadius(headRadius);
		headBuffer->render(renderer);
	}

	// Restore old state.
	renderer->setDepthTestEnabled(true);
}

/******************************************************************************
* Clips a dislocation line at the periodic box boundaries.
******************************************************************************/
void DislocationVis::clipDislocationLine(const std::deque<Point3>& line, const SimulationCell& simulationCell, const QVector<Plane3>& clippingPlanes, const std::function<void(const Point3&, const Point3&, bool)>& segmentCallback)
{
	bool isInitialSegment = true;
	auto clippingFunction = [&clippingPlanes, &segmentCallback, &isInitialSegment](Point3 p1, Point3 p2) {
		bool isClipped = false;
		for(const Plane3& plane : clippingPlanes) {
			FloatType c1 = plane.pointDistance(p1);
			FloatType c2 = plane.pointDistance(p2);
			if(c1 >= 0 && c2 >= 0.0) {
				isClipped = true;
				break;
			}
			else if(c1 > FLOATTYPE_EPSILON && c2 < -FLOATTYPE_EPSILON) {
				p1 += (p2 - p1) * (c1 / (c1 - c2));
			}
			else if(c1 < -FLOATTYPE_EPSILON && c2 > FLOATTYPE_EPSILON) {
				p2 += (p1 - p2) * (c2 / (c2 - c1));
			}
		}
		if(!isClipped) {
			segmentCallback(p1, p2, isInitialSegment);
			isInitialSegment = false;
		}
	};

	auto v1 = line.cbegin();
	Point3 rp1 = simulationCell.absoluteToReduced(*v1);
	Vector3 shiftVector = Vector3::Zero();
	for(size_t dim = 0; dim < 3; dim++) {
		if(simulationCell.pbcFlags()[dim]) {
			while(rp1[dim] > 0) { rp1[dim] -= 1; shiftVector[dim] -= 1; }
			while(rp1[dim] < 0) { rp1[dim] += 1; shiftVector[dim] += 1; }
		}
	}
	for(auto v2 = v1 + 1; v2 != line.cend(); v1 = v2, ++v2) {
		Point3 rp2 = simulationCell.absoluteToReduced(*v2) + shiftVector;
		FloatType smallestT;
		bool clippedDimensions[3] = { false, false, false };
		do {
			size_t crossDim;
			FloatType crossDir;
			smallestT = FLOATTYPE_MAX;
			for(size_t dim = 0; dim < 3; dim++) {
				if(simulationCell.pbcFlags()[dim] && !clippedDimensions[dim]) {
					int d = (int)floor(rp2[dim]) - (int)floor(rp1[dim]);
					if(d == 0) continue;
					FloatType t;
					if(d > 0)
						t = (ceil(rp1[dim]) - rp1[dim]) / (rp2[dim] - rp1[dim]);
					else
						t = (floor(rp1[dim]) - rp1[dim]) / (rp2[dim] - rp1[dim]);
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
				intersection[crossDim] = floor(intersection[crossDim] + FloatType(0.5));
				Point3 rp1abs = simulationCell.reducedToAbsolute(rp1);
				Point3 intabs = simulationCell.reducedToAbsolute(intersection);
				if(!intabs.equals(rp1abs)) {
					clippingFunction(rp1abs, intabs);
				}
				shiftVector[crossDim] -= crossDir;
				rp1 = intersection;
				rp1[crossDim] -= crossDir;
				rp2[crossDim] -= crossDir;
				isInitialSegment = true;
			}
		}
		while(smallestT != FLOATTYPE_MAX);

		clippingFunction(simulationCell.reducedToAbsolute(rp1), simulationCell.reducedToAbsolute(rp2));
		rp1 = rp2;
	}
}

/******************************************************************************
* Checks if the given floating point number is integer.
******************************************************************************/
static bool isInteger(FloatType v, int& intPart)
{
	static const FloatType epsilon = FloatType(1e-2);
	FloatType ip;
	FloatType frac = std::modf(v, &ip);
	if(frac >= -epsilon && frac <= epsilon) intPart = (int)ip;
	else if(frac >= FloatType(1)-epsilon) intPart = (int)ip + 1;
	else if(frac <= FloatType(-1)+epsilon) intPart = (int)ip - 1;
	else return false;
	return true;
}

/******************************************************************************
* Generates a pretty string representation of the Burgers vector.
******************************************************************************/
QString DislocationVis::formatBurgersVector(const Vector3& b, const MicrostructurePhase* structure)
{
	if(structure) {
		if(structure->crystalSymmetryClass() == MicrostructurePhase::CrystalSymmetryClass::CubicSymmetry) {
			if(b.isZero())
				return QStringLiteral("[0 0 0]");
			FloatType smallestCompnt = FLOATTYPE_MAX;
			for(int i = 0; i < 3; i++) {
				FloatType c = std::abs(b[i]);
				if(c < smallestCompnt && c > FloatType(1e-3))
					smallestCompnt = c;
			}
			if(smallestCompnt != FLOATTYPE_MAX) {
				FloatType m = FloatType(1) / smallestCompnt;
				for(int f = 1; f <= 11; f++) {
					int multiplier;
					if(!isInteger(m*f, multiplier)) continue;
					if(multiplier < 80) {
						Vector3 bm = b * (FloatType)multiplier;
						Vector3I bmi;
						if(isInteger(bm.x(),bmi.x()) && isInteger(bm.y(),bmi.y()) && isInteger(bm.z(),bmi.z())) {
							if(multiplier != 1)
								return QString("1/%1[%2 %3 %4]")
										.arg(multiplier)
										.arg(bmi.x()).arg(bmi.y()).arg(bmi.z());
							else
								return QString("[%1 %2 %3]")
										.arg(bmi.x()).arg(bmi.y()).arg(bmi.z());
						}
					}
				}
			}
		}
		else if(structure->crystalSymmetryClass() == MicrostructurePhase::CrystalSymmetryClass::HexagonalSymmetry) {
			if(b.isZero())
				return QStringLiteral("[0 0 0 0]");
			// Determine vector components U, V, and W, with b = U*a1 + V*a2 + W*c.
			FloatType U = sqrt(2.0)*b.x() - sqrt(2.0/3.0)*b.y();
			FloatType V = sqrt(2.0)*b.x() + sqrt(2.0/3.0)*b.y();
			FloatType W = sqrt(3.0/4.0)*b.z();
			Vector4 uvwt((2*U-V)/3, (2*V-U)/3, -(U+V)/3, W);
			FloatType smallestCompnt = FLOATTYPE_MAX;
			for(int i = 0; i < 4; i++) {
				FloatType c = std::abs(uvwt[i]);
				if(c < smallestCompnt && c > FloatType(1e-3))
					smallestCompnt = c;
			}
			if(smallestCompnt != FLOATTYPE_MAX) {
				FloatType m = FloatType(1) / smallestCompnt;
				for(int f = 1; f <= 11; f++) {
					int multiplier;
					if(!isInteger(m*f, multiplier)) continue;
					if(multiplier < 80) {
						Vector4 bm = uvwt * (FloatType)multiplier;
						int bmi[4];
						if(isInteger(bm.x(),bmi[0]) && isInteger(bm.y(),bmi[1]) && isInteger(bm.z(),bmi[2]) && isInteger(bm.w(),bmi[3])) {
							if(multiplier != 1)
								return QString("1/%1[%2 %3 %4 %5]")
										.arg(multiplier)
										.arg(bmi[0]).arg(bmi[1]).arg(bmi[2]).arg(bmi[3]);
							else
								return QString("[%1 %2 %3 %4]")
										.arg(bmi[0]).arg(bmi[1]).arg(bmi[2]).arg(bmi[3]);
						}
					}
				}
			}
			return QString("[%1 %2 %3 %4]")
					.arg(QLocale::c().toString(uvwt.x(), 'f'), 7)
					.arg(QLocale::c().toString(uvwt.y(), 'f'), 7)
					.arg(QLocale::c().toString(uvwt.z(), 'f'), 7)
					.arg(QLocale::c().toString(uvwt.w(), 'f'), 7);
		}
	}

	if(b.isZero())
		return QStringLiteral("0 0 0");

	return QString("%1 %2 %3")
			.arg(QLocale::c().toString(b.x(), 'f'), 7)
			.arg(QLocale::c().toString(b.y(), 'f'), 7)
			.arg(QLocale::c().toString(b.z(), 'f'), 7);
}

/******************************************************************************
* Returns a human-readable string describing the picked object,
* which will be displayed in the status bar by OVITO.
******************************************************************************/
QString DislocationPickInfo::infoString(PipelineSceneNode* objectNode, quint32 subobjectId)
{
	QString str;

	int segmentIndex = segmentIndexFromSubObjectID(subobjectId);
	if(dislocationObj()) {
		if(segmentIndex >= 0 && segmentIndex < dislocationObj()->segments().size()) {
			DislocationSegment* segment = dislocationObj()->segments()[segmentIndex];
			MicrostructurePhase* structure = dislocationObj()->structureById(segment->burgersVector.cluster()->structure);
			QString formattedBurgersVector = DislocationVis::formatBurgersVector(segment->burgersVector.localVec(), structure);
			str = tr("True Burgers vector: %1").arg(formattedBurgersVector);
			Vector3 transformedVector = segment->burgersVector.toSpatialVector();
			str += tr(" | Spatial Burgers vector: [%1 %2 %3]")
					.arg(QLocale::c().toString(transformedVector.x(), 'f', 4), 7)
					.arg(QLocale::c().toString(transformedVector.y(), 'f', 4), 7)
					.arg(QLocale::c().toString(transformedVector.z(), 'f', 4), 7);
			str += tr(" | Cluster Id: %1").arg(segment->burgersVector.cluster()->id);
			str += tr(" | Dislocation Id: %1").arg(segment->id);
			if(structure) {
				str += tr(" | Crystal structure: %1").arg(structure->name());
			}
		}
	}
	else if(microstructureObj()) {
		const PropertyObject* burgersVectorProperty = microstructureObj()->faces()->getProperty(SurfaceMeshFaces::BurgersVectorProperty);
		const PropertyObject* faceRegionProperty = microstructureObj()->faces()->getProperty(SurfaceMeshFaces::RegionProperty);
		const PropertyObject* phaseProperty = microstructureObj()->regions()->getProperty(SurfaceMeshRegions::PhaseProperty);
		if(burgersVectorProperty && faceRegionProperty && phaseProperty && segmentIndex >= 0 && segmentIndex < burgersVectorProperty->size()) {
			const MicrostructurePhase* phase = nullptr;
			int region = faceRegionProperty->getInt(segmentIndex);
			if(region >= 0 && region < phaseProperty->size()) {
				int phaseId = phaseProperty->getInt(region);
				if(const MicrostructurePhase* phase = dynamic_object_cast<MicrostructurePhase>(phaseProperty->elementType(phaseId))) {
					const Vector3& burgersVector = burgersVectorProperty->getVector3(segmentIndex);
					QString formattedBurgersVector = DislocationVis::formatBurgersVector(burgersVector, phase);
					str = tr("True Burgers vector: %1").arg(formattedBurgersVector);
					const PropertyObject* correspondenceProperty = microstructureObj()->regions()->getProperty(SurfaceMeshRegions::LatticeCorrespondenceProperty);
					if(correspondenceProperty) {
						Vector3 transformedVector = correspondenceProperty->getMatrix3(region) * burgersVector;
						str += tr(" | Spatial Burgers vector: [%1 %2 %3]")
								.arg(QLocale::c().toString(transformedVector.x(), 'f', 4), 7)
								.arg(QLocale::c().toString(transformedVector.y(), 'f', 4), 7)
								.arg(QLocale::c().toString(transformedVector.z(), 'f', 4), 7);
					}
					str += tr(" | Crystal region: %1").arg(region);
					str += tr(" | Dislocation segment: %1").arg(segmentIndex);
					str += tr(" | Crystal structure: %1").arg(phase->name());
				}
			}
		}
	}
	return str;
}

}	// End of namespace
}	// End of namespace
