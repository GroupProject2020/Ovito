///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2017) Alexander Stukowski
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

#include <plugins/mesh/Mesh.h>
#include <plugins/mesh/util/CapPolygonTessellator.h>
#include <core/rendering/SceneRenderer.h>
#include <core/rendering/MeshPrimitive.h>
#include <core/utilities/mesh/TriMesh.h>
#include <core/utilities/units/UnitsManager.h>
#include <core/dataset/animation/controller/Controller.h>
#include <core/dataset/data/VersionedDataObjectRef.h>
#include <core/dataset/DataSetContainer.h>
#include <plugins/stdobj/simcell/SimulationCellObject.h>
#include "SurfaceMeshVis.h"
#include "SurfaceMesh.h"
#include "RenderableSurfaceMesh.h"

namespace Ovito { namespace Mesh {

IMPLEMENT_OVITO_CLASS(SurfaceMeshVis);
DEFINE_PROPERTY_FIELD(SurfaceMeshVis, surfaceColor);
DEFINE_PROPERTY_FIELD(SurfaceMeshVis, capColor);
DEFINE_PROPERTY_FIELD(SurfaceMeshVis, showCap);
DEFINE_PROPERTY_FIELD(SurfaceMeshVis, smoothShading);
DEFINE_PROPERTY_FIELD(SurfaceMeshVis, reverseOrientation);
DEFINE_REFERENCE_FIELD(SurfaceMeshVis, surfaceTransparencyController);
DEFINE_REFERENCE_FIELD(SurfaceMeshVis, capTransparencyController);
SET_PROPERTY_FIELD_LABEL(SurfaceMeshVis, surfaceColor, "Surface color");
SET_PROPERTY_FIELD_LABEL(SurfaceMeshVis, capColor, "Cap color");
SET_PROPERTY_FIELD_LABEL(SurfaceMeshVis, showCap, "Show cap polygons");
SET_PROPERTY_FIELD_LABEL(SurfaceMeshVis, smoothShading, "Smooth shading");
SET_PROPERTY_FIELD_LABEL(SurfaceMeshVis, surfaceTransparencyController, "Surface transparency");
SET_PROPERTY_FIELD_LABEL(SurfaceMeshVis, capTransparencyController, "Cap transparency");
SET_PROPERTY_FIELD_LABEL(SurfaceMeshVis, reverseOrientation, "Inside out");
SET_PROPERTY_FIELD_UNITS_AND_RANGE(SurfaceMeshVis, surfaceTransparencyController, PercentParameterUnit, 0, 1);
SET_PROPERTY_FIELD_UNITS_AND_RANGE(SurfaceMeshVis, capTransparencyController, PercentParameterUnit, 0, 1);

/******************************************************************************
* Constructor.
******************************************************************************/
SurfaceMeshVis::SurfaceMeshVis(DataSet* dataset) : TransformingDataVis(dataset),
	_surfaceColor(1, 1, 1), 
	_capColor(0.8, 0.8, 1.0), 
	_showCap(true), 
	_smoothShading(true), 
	_reverseOrientation(false)
{
	setSurfaceTransparencyController(ControllerManager::createFloatController(dataset));
	setCapTransparencyController(ControllerManager::createFloatController(dataset));
}

/******************************************************************************
* Is called when the value of a property of this object has changed.
******************************************************************************/
void SurfaceMeshVis::propertyChanged(const PropertyFieldDescriptor& field)
{
	if(field == PROPERTY_FIELD(smoothShading) || field == PROPERTY_FIELD(reverseOrientation)) {
		// This kind of parameter change triggers a regeneration of the cached RenderableSurfaceMesh.
		invalidateTransformedObjects();
	}
	TransformingDataVis::propertyChanged(field);
}

/******************************************************************************
* Lets the vis element transform a data object in preparation for rendering.
******************************************************************************/
Future<PipelineFlowState> SurfaceMeshVis::transformDataImpl(TimePoint time, DataObject* dataObject, PipelineFlowState&& flowState, const PipelineFlowState& cachedState, PipelineSceneNode* contextNode)
{
	// Get the input surface mesh.
	SurfaceMesh* surfaceMeshObj = dynamic_object_cast<SurfaceMesh>(dataObject);
	if(!surfaceMeshObj)
		return std::move(flowState);

	// Get the simulation cell.
	SimulationCellObject* cellObject = surfaceMeshObj->domain();
	if(!cellObject)
		return std::move(flowState);

	// Create compute engine.
	auto engine = std::make_shared<PrepareSurfaceEngine>(surfaceMeshObj->storage(), cellObject->data(), 
		surfaceMeshObj->isCompletelySolid(), reverseOrientation(), 
		surfaceMeshObj->cuttingPlanes(), smoothShading());

	// Submit engine for execution and post-process results.
	return dataset()->container()->taskManager().runTaskAsync(engine)
		.then(executor(), [this, flowState = std::move(flowState), dataObject](TriMesh&& surfaceMesh, TriMesh&& capPolygonsMesh) mutable {
			UndoSuspender noUndo(this);

			// Output the computed mesh as a RenderableSurfaceMesh.
			OORef<RenderableSurfaceMesh> renderableMesh = new RenderableSurfaceMesh(this, dataObject, std::move(surfaceMesh), std::move(capPolygonsMesh));
			flowState.addObject(renderableMesh);
			return std::move(flowState);
		});
}

/******************************************************************************
* Computes the results and stores them in this object for later retrieval.
******************************************************************************/
void SurfaceMeshVis::PrepareSurfaceEngine::perform()
{
	setProgressText(tr("Preparing surface mesh for display"));

	TriMesh surfaceMesh;
	TriMesh capPolygonsMesh;

	if(!buildSurfaceMesh(*_inputMesh, _simCell, _reverseOrientation, _cuttingPlanes, surfaceMesh, this))
		throw Exception(tr("Failed to generate non-periodic mesh. Periodic domain might be too small."));

	if(isCanceled())
		return;

	buildCapMesh(*_inputMesh, _simCell, _isCompletelySolid, _reverseOrientation, _cuttingPlanes, capPolygonsMesh, this);

	if(_smoothShading) {
		// Assign smoothing group to faces to interpolate normals.
		for(auto& face : surfaceMesh.faces())
			face.setSmoothingGroups(1);	
	}

	setResult(std::move(surfaceMesh), std::move(capPolygonsMesh));
}

/******************************************************************************
* Computes the bounding box of the displayed data.
******************************************************************************/
Box3 SurfaceMeshVis::boundingBox(TimePoint time, const std::vector<DataObject*>& objectStack, PipelineSceneNode* contextNode, const PipelineFlowState& flowState, TimeInterval& validityInterval)
{
	Box3 bb;

	// Compute mesh bounding box.
	// Requires that we have already transformed the periodic SurfaceMesh into a non-periodic RenderableSurfaceMesh.
	if(OORef<RenderableSurfaceMesh> meshObj = objectStack.back()->convertTo<RenderableSurfaceMesh>(time)) {
		bb.addBox(meshObj->surfaceMesh().boundingBox());
		bb.addBox(meshObj->capPolygonsMesh().boundingBox());
	}
	return bb;
}

/******************************************************************************
* Lets the visualization element render the data object.
******************************************************************************/
void SurfaceMeshVis::render(TimePoint time, const std::vector<DataObject*>& objectStack, const PipelineFlowState& flowState, SceneRenderer* renderer, PipelineSceneNode* contextNode)
{
	// Ignore render calls for the original SurfaceMesh.
	// We are only interested in the RenderableSurfaceMesh.
	if(dynamic_object_cast<SurfaceMesh>(objectStack.back()) != nullptr)
		return;

	if(renderer->isBoundingBoxPass()) {
		TimeInterval validityInterval;
		renderer->addToLocalBoundingBox(boundingBox(time, objectStack, contextNode, flowState, validityInterval));
		return;
	}

	// Get the rendering colors for the surface and cap meshes.
	FloatType transp_surface = 0;
	FloatType transp_cap = 0;
	TimeInterval iv;
	if(surfaceTransparencyController()) transp_surface = surfaceTransparencyController()->getFloatValue(time, iv);
	if(capTransparencyController()) transp_cap = capTransparencyController()->getFloatValue(time, iv);
	ColorA color_surface(surfaceColor(), FloatType(1) - transp_surface);
	ColorA color_cap(capColor(), FloatType(1) - transp_cap);

	// The key type used for caching the rendering primitive:
	using SurfaceCacheKey = std::tuple<
		CompatibleRendererGroup,	// The scene renderer
		VersionedDataObjectRef,		// Mesh object
		ColorA						// Surface color
	>;

	// The key type used for caching the rendering primitive:
	using CapCacheKey = std::tuple<
		CompatibleRendererGroup,	// The scene renderer
		VersionedDataObjectRef,		// Mesh object
		ColorA						// Cap color
	>;

	// Lookup the rendering primitive in the vis cache.
	auto& surfacePrimitive = dataset()->visCache().get<std::shared_ptr<MeshPrimitive>>(SurfaceCacheKey(renderer, objectStack.back(), color_surface));

	// Check if we already have a valid rendering primitive that is up to date.
	if(!surfacePrimitive || !surfacePrimitive->isValid(renderer)) {
		if(OORef<RenderableSurfaceMesh> meshObj = objectStack.back()->convertTo<RenderableSurfaceMesh>(time)) {
			surfacePrimitive = renderer->createMeshPrimitive();
			surfacePrimitive->setMesh(meshObj->surfaceMesh(), color_surface);
		}
		else {
			surfacePrimitive.reset();
		}
	}

	// Lookup the rendering primitive in the vis cache.
	auto& capPrimitive = dataset()->visCache().get<std::shared_ptr<MeshPrimitive>>(CapCacheKey(renderer, objectStack.back(), color_cap));

	// Check if we already have a valid rendering primitive that is up to date.
	if(!capPrimitive || !capPrimitive->isValid(renderer)) {
		capPrimitive.reset();
		if(OORef<RenderableSurfaceMesh> meshObj = objectStack.back()->convertTo<RenderableSurfaceMesh>(time)) {
			if(showCap()) {
				capPrimitive = renderer->createMeshPrimitive();
				capPrimitive->setMesh(meshObj->capPolygonsMesh(), color_cap);
			}
		}
	}	

	// Handle picking of triangles.
	renderer->beginPickObject(contextNode);
	if(surfacePrimitive) surfacePrimitive->render(renderer);
	if(showCap())
		capPrimitive->render(renderer);
	else
		capPrimitive.reset();
	renderer->endPickObject();
}

/******************************************************************************
* Generates the final triangle mesh, which will be rendered.
******************************************************************************/
bool SurfaceMeshVis::buildSurfaceMesh(const HalfEdgeMesh<>& input, const SimulationCell& cell, bool reverseOrientation, const QVector<Plane3>& cuttingPlanes, TriMesh& output, PromiseState* progress)
{
	if(cell.is2D())
		throw Exception(tr("Cannot generate surface triangle mesh when domain is two-dimensional."));
	
	OVITO_ASSERT(input.isClosed());

	// Convert half-edge mesh to triangle mesh.
	input.convertToTriMesh(output);

	// Flip orientation of mesh faces if requested.
	if(reverseOrientation)
		output.flipFaces();

	// Check for early abortion.
	if(progress && progress->isCanceled())
		return false;

	// Convert vertex positions to reduced coordinates.
	for(Point3& p : output.vertices()) {
		p = cell.absoluteToReduced(p);
		OVITO_ASSERT(std::isfinite(p.x()) && std::isfinite(p.y()) && std::isfinite(p.z()));
	}

	// Wrap mesh at periodic boundaries.
	for(size_t dim = 0; dim < 3; dim++) {
		if(cell.pbcFlags()[dim] == false) continue;

		if(progress && progress->isCanceled())
			return false;

		// Make sure all vertices are located inside the periodic box.
		for(Point3& p : output.vertices()) {
			OVITO_ASSERT(std::isfinite(p[dim]));
			p[dim] -= floor(p[dim]);
			OVITO_ASSERT(p[dim] >= FloatType(0) && p[dim] <= FloatType(1));
		}

		// Split triangle faces at periodic boundaries.
		int oldFaceCount = output.faceCount();
		int oldVertexCount = output.vertexCount();
		std::vector<Point3> newVertices;
		std::map<std::pair<int,int>,std::pair<int,int>> newVertexLookupMap;
		for(int findex = 0; findex < oldFaceCount; findex++) {
			if(!splitFace(output, output.face(findex), oldVertexCount, newVertices, newVertexLookupMap, cell, dim))
				return false;
		}

		// Insert newly created vertices into mesh.
		output.setVertexCount(oldVertexCount + (int)newVertices.size());
		std::copy(newVertices.cbegin(), newVertices.cend(), output.vertices().begin() + oldVertexCount);
	}

	// Check for early abortion.
	if(progress && progress->isCanceled())
		return false;

	// Convert vertex positions back from reduced coordinates to absolute coordinates.
	AffineTransformation cellMatrix = cell.matrix();
	for(Point3& p : output.vertices())
		p = cellMatrix * p;

	// Clip mesh at cutting planes.
	for(const Plane3& plane : cuttingPlanes) {
		if(progress && progress->isCanceled())
			return false;

		output.clipAtPlane(plane);
	}

	output.invalidateVertices();
	output.invalidateFaces();

	return true;
}

/******************************************************************************
* Splits a triangle face at a periodic boundary.
******************************************************************************/
bool SurfaceMeshVis::splitFace(TriMesh& output, TriMeshFace& face, int oldVertexCount, std::vector<Point3>& newVertices,
		std::map<std::pair<int,int>,std::pair<int,int>>& newVertexLookupMap, const SimulationCell& cell, size_t dim)
{
	OVITO_ASSERT(face.vertex(0) != face.vertex(1));
	OVITO_ASSERT(face.vertex(1) != face.vertex(2));
	OVITO_ASSERT(face.vertex(2) != face.vertex(0));

	FloatType z[3];
	for(int v = 0; v < 3; v++)
		z[v] = output.vertex(face.vertex(v))[dim];
	FloatType zd[3] = { z[1] - z[0], z[2] - z[1], z[0] - z[2] };

	OVITO_ASSERT(z[1] - z[0] == -(z[0] - z[1]));
	OVITO_ASSERT(z[2] - z[1] == -(z[1] - z[2]));
	OVITO_ASSERT(z[0] - z[2] == -(z[2] - z[0]));

	if(std::abs(zd[0]) < FloatType(0.5) && std::abs(zd[1]) < FloatType(0.5) && std::abs(zd[2]) < FloatType(0.5))
		return true;	// Face is not crossing the periodic boundary.

	// Create four new vertices (or use existing ones created during splitting of adjacent faces).
	int properEdge = -1;
	int newVertexIndices[3][2];
	for(int i = 0; i < 3; i++) {
		if(std::abs(zd[i]) < FloatType(0.5)) {
			if(properEdge != -1)
				return false;		// The simulation box may be too small or invalid.
			properEdge = i;
			continue;
		}
		int vi1 = face.vertex(i);
		int vi2 = face.vertex((i+1)%3);
		int oi1, oi2;
		if(zd[i] <= FloatType(-0.5)) {
			std::swap(vi1, vi2);
			oi1 = 1; oi2 = 0;
		}
		else {
			oi1 = 0; oi2 = 1;
		}
		auto entry = newVertexLookupMap.find(std::make_pair(vi1, vi2));
		if(entry != newVertexLookupMap.end()) {
			newVertexIndices[i][oi1] = entry->second.first;
			newVertexIndices[i][oi2] = entry->second.second;
		}
		else {
			Vector3 delta = output.vertex(vi2) - output.vertex(vi1);
			delta[dim] -= FloatType(1);
			for(size_t d = dim + 1; d < 3; d++) {
				if(cell.pbcFlags()[d])
					delta[d] -= floor(delta[d] + FloatType(0.5));
			}
			FloatType t;
			if(delta[dim] != 0)
				t = output.vertex(vi1)[dim] / (-delta[dim]);
			else
				t = FloatType(0.5);
			OVITO_ASSERT(std::isfinite(t));
			Point3 p = delta * t + output.vertex(vi1);
			newVertexIndices[i][oi1] = oldVertexCount + (int)newVertices.size();
			newVertexIndices[i][oi2] = oldVertexCount + (int)newVertices.size() + 1;
			newVertexLookupMap.insert(std::make_pair(std::pair<int,int>(vi1, vi2), std::pair<int,int>(newVertexIndices[i][oi1], newVertexIndices[i][oi2])));
			newVertices.push_back(p);
			p[dim] += FloatType(1);
			newVertices.push_back(p);
		}
	}
	OVITO_ASSERT(properEdge != -1);

	// Build output triangles.
	int originalVertices[3] = { face.vertex(0), face.vertex(1), face.vertex(2) };
	face.setVertices(originalVertices[properEdge], originalVertices[(properEdge+1)%3], newVertexIndices[(properEdge+2)%3][1]);

	output.setFaceCount(output.faceCount() + 2);
	TriMeshFace& newFace1 = output.face(output.faceCount() - 2);
	TriMeshFace& newFace2 = output.face(output.faceCount() - 1);
	newFace1.setVertices(originalVertices[(properEdge+1)%3], newVertexIndices[(properEdge+1)%3][0], newVertexIndices[(properEdge+2)%3][1]);
	newFace2.setVertices(newVertexIndices[(properEdge+1)%3][1], originalVertices[(properEdge+2)%3], newVertexIndices[(properEdge+2)%3][0]);

	return true;
}

/******************************************************************************
* Generates the triangle mesh for the PBC caps.
******************************************************************************/
void SurfaceMeshVis::buildCapMesh(const HalfEdgeMesh<>& input, const SimulationCell& cell, bool isCompletelySolid, bool reverseOrientation, const QVector<Plane3>& cuttingPlanes, TriMesh& output, PromiseState* promise)
{
	bool flipCapNormal = (cell.matrix().determinant() < 0);

	// Convert vertex positions to reduced coordinates.
	std::vector<Point3> reducedPos(input.vertexCount());
	auto inputVertex = input.vertices().begin();
	AffineTransformation invCellMatrix = cell.inverseMatrix();
	if(flipCapNormal)
		invCellMatrix.column(0) = -invCellMatrix.column(0);
	for(Point3& p : reducedPos)
		p = invCellMatrix * (*inputVertex++)->pos();

	int isBoxCornerInside3DRegion = -1;

	// Create caps for each periodic boundary.
	for(size_t dim = 0; dim < 3; dim++) {
		if(cell.pbcFlags()[dim] == false) continue;

		if(promise && promise->isCanceled())
			return;

		// Make sure all vertices are located inside the periodic box.
		for(Point3& p : reducedPos) {
			FloatType& c = p[dim];
			OVITO_ASSERT(std::isfinite(c));
			if(FloatType s = floor(c)) c -= s;
			OVITO_ASSERT(std::isfinite(c));
		}

		// Reset 'visited' flag for all faces.
		input.clearFaceFlag(1);

		/// The list of clipped contours.
		std::vector<std::vector<Point2>> openContours;
		std::vector<std::vector<Point2>> closedContours;

		// Find a first edge that crosses the boundary.
		for(HalfEdgeMesh<>::Vertex* vert : input.vertices()) {
			if(promise && promise->isCanceled())
				return;
			for(HalfEdgeMesh<>::Edge* edge = vert->edges(); edge != nullptr; edge = edge->nextVertexEdge()) {
				// Skip faces that have already been visited.
				if(edge->face()->testFlag(1)) continue;

				const Point3& v1 = reducedPos[edge->vertex1()->index()];
				const Point3& v2 = reducedPos[edge->vertex2()->index()];
				if(v2[dim] - v1[dim] >= FloatType(0.5)) {
					std::vector<Point2> contour = traceContour(edge, reducedPos, cell, dim);
					if(contour.empty()) throw Exception(tr("Surface mesh is not a proper manifold."));
					clipContour(contour, std::array<bool,2>{{ cell.pbcFlags()[(dim+1)%3], cell.pbcFlags()[(dim+2)%3] }}, openContours, closedContours);
				}
			}
		}

		if(reverseOrientation) {
			for(auto& contour : openContours)
				std::reverse(std::begin(contour), std::end(contour));
		}

		// Feed contours into tessellator to create triangles.
		CapPolygonTessellator tessellator(output, dim);
		tessellator.beginPolygon();
		for(const auto& contour : closedContours) {
			if(promise && promise->isCanceled())
				return;
			tessellator.beginContour();
			for(const Point2& p : contour) {
				tessellator.vertex(p);
			}
			tessellator.endContour();
		}

		auto yxCoord2ArcLength = [](const Point2& p) {
			if(p.x() == 0) return p.y();
			else if(p.y() == 1) return p.x() + FloatType(1);
			else if(p.x() == 1) return FloatType(3) - p.y();
			else return std::fmod(FloatType(4) - p.x(), FloatType(4));
		};

		// Build the outer contour.
		if(!openContours.empty()) {
			boost::dynamic_bitset<> visitedContours(openContours.size());
			for(auto c1 = openContours.begin(); c1 != openContours.end(); ++c1) {
				if(promise && promise->isCanceled())
					return;
				if(!visitedContours.test(c1 - openContours.begin())) {
					tessellator.beginContour();
					auto currentContour = c1;
					do {
						for(const Point2& p : *currentContour) {
							tessellator.vertex(p);
						}
						visitedContours.set(currentContour - openContours.begin());

						FloatType t_exit = yxCoord2ArcLength(currentContour->back());

						// Find the next contour.
						FloatType t_entry;
						FloatType closestDist = FLOATTYPE_MAX;
						for(auto c = openContours.begin(); c != openContours.end(); ++c) {
							FloatType t = yxCoord2ArcLength(c->front());
							FloatType dist = t_exit - t;
							if(dist < 0) dist += FloatType(4);
							if(dist < closestDist) {
								closestDist = dist;
								currentContour = c;
								t_entry = t;
							}
						}
						int exitCorner = (int)std::floor(t_exit);
						int entryCorner = (int)std::floor(t_entry);
						OVITO_ASSERT(exitCorner >= 0 && exitCorner < 4);
						OVITO_ASSERT(entryCorner >= 0 && entryCorner < 4);
						if(exitCorner != entryCorner || t_exit < t_entry) {
							for(int corner = exitCorner;;) {
								switch(corner) {
								case 0: tessellator.vertex(Point2(0,0)); break;
								case 1: tessellator.vertex(Point2(0,1)); break;
								case 2: tessellator.vertex(Point2(1,1)); break;	
								case 3: tessellator.vertex(Point2(1,0)); break;
								}
								corner = (corner + 3) % 4;
								if(corner == entryCorner) break;
							}
						}
					}
					while(!visitedContours.test(currentContour - openContours.begin()));
					tessellator.endContour();
				}
			}
		}
		else {
			if(isBoxCornerInside3DRegion == -1) {
				if(closedContours.empty()) {
					isBoxCornerInside3DRegion = (SurfaceMesh::locatePointStatic(Point3::Origin() + cell.matrix().column(3), input, cell, isCompletelySolid, 0) < 0);
				}
				else {
					isBoxCornerInside3DRegion = isCornerInside2DRegion(closedContours);
				}
				if(reverseOrientation)
					isBoxCornerInside3DRegion = !isBoxCornerInside3DRegion;
			}
			if(isBoxCornerInside3DRegion) {
				tessellator.beginContour();
				tessellator.vertex(Point2(0,0));
				tessellator.vertex(Point2(1,0));
				tessellator.vertex(Point2(1,1));
				tessellator.vertex(Point2(0,1));
				tessellator.endContour();
			}
		}

		tessellator.endPolygon();
	}

	// Check for early abortion.
	if(promise && promise->isCanceled())
		return;

	// Convert vertex positions back from reduced coordinates to absolute coordinates.
	const AffineTransformation cellMatrix = invCellMatrix.inverse();
	for(Point3& p : output.vertices())
		p = cellMatrix * p;

	// Clip mesh at cutting planes.
	for(const Plane3& plane : cuttingPlanes) {
		if(promise && promise->isCanceled())
			return;
		output.clipAtPlane(plane);
	}
}

/******************************************************************************
* Traces the closed contour of the surface-boundary intersection.
******************************************************************************/
std::vector<Point2> SurfaceMeshVis::traceContour(HalfEdgeMesh<>::Edge* firstEdge, const std::vector<Point3>& reducedPos, const SimulationCell& cell, size_t dim)
{
	size_t dim1 = (dim + 1) % 3;
	size_t dim2 = (dim + 2) % 3;
	std::vector<Point2> contour;
	HalfEdgeMesh<>::Edge* edge = firstEdge;
	do {
		OVITO_ASSERT(edge->face() != nullptr);
		OVITO_ASSERT(!edge->face()->testFlag(1));

		// Mark face as visited.
		edge->face()->setFlag(1);

		// Compute intersection point.
		const Point3& v1 = reducedPos[edge->vertex1()->index()];
		const Point3& v2 = reducedPos[edge->vertex2()->index()];
		Vector3 delta = v2 - v1;
		OVITO_ASSERT(delta[dim] >= FloatType(0.5));

		delta[dim] -= FloatType(1);
		if(cell.pbcFlags()[dim1]) {
			FloatType& c = delta[dim1];
			if(FloatType s = floor(c + FloatType(0.5)))
				c -= s;
		}
		if(cell.pbcFlags()[dim2]) {
			FloatType& c = delta[dim2];
			if(FloatType s = floor(c + FloatType(0.5)))
				c -= s;
		}
		if(std::abs(delta[dim]) > FloatType(1e-9f)) {
			FloatType t = v1[dim] / delta[dim];
			FloatType x = v1[dim1] - delta[dim1] * t;
			FloatType y = v1[dim2] - delta[dim2] * t;
			OVITO_ASSERT(std::isfinite(x) && std::isfinite(y));
			if(contour.empty() || std::abs(x - contour.back().x()) > FLOATTYPE_EPSILON || std::abs(y - contour.back().y()) > FLOATTYPE_EPSILON) {
				contour.push_back({x,y});
			}
		}
		else {
			FloatType x1 = v1[dim1];
			FloatType y1 = v1[dim2];
			FloatType x2 = v1[dim1] + delta[dim1];
			FloatType y2 = v1[dim2] + delta[dim2];
			if(contour.empty() || std::abs(x1 - contour.back().x()) > FLOATTYPE_EPSILON || std::abs(y1 - contour.back().y()) > FLOATTYPE_EPSILON) {
				contour.push_back({x1,y1});
			}
			else if(contour.empty() || std::abs(x2 - contour.back().x()) > FLOATTYPE_EPSILON || std::abs(y2 - contour.back().y()) > FLOATTYPE_EPSILON) {
				contour.push_back({x2,y2});
			}
		}

		// Find the face edge that crosses the boundary in the reverse direction.
		for(;;) {
			edge = edge->nextFaceEdge();
			const Point3& v1 = reducedPos[edge->vertex1()->index()];
			const Point3& v2 = reducedPos[edge->vertex2()->index()];
			if(v2[dim] - v1[dim] <= FloatType(-0.5))
				break;
		}

		edge = edge->oppositeEdge();
		if(!edge) {
			// Mesh is not closed (not a proper manifold).
			contour.clear();
			break;
		}
	}
	while(edge != firstEdge);

	return contour;
}

/******************************************************************************
* Clips a 2d contour at a periodic boundary.
******************************************************************************/
void SurfaceMeshVis::clipContour(std::vector<Point2>& input, std::array<bool,2> pbcFlags, std::vector<std::vector<Point2>>& openContours, std::vector<std::vector<Point2>>& closedContours)
{
	if(!pbcFlags[0] && !pbcFlags[1]) {
		closedContours.push_back(std::move(input));
		return;
	}

	// Ensure all coordinates are within the primary image.
	if(pbcFlags[0]) {
		for(auto& v : input) {
			OVITO_ASSERT(std::isfinite(v.x()));
			if(FloatType s = floor(v.x())) v.x() -= s;
		}
	}
	if(pbcFlags[1]) {
		for(auto& v : input) {
			OVITO_ASSERT(std::isfinite(v.y()));
			if(FloatType s = floor(v.y())) v.y() -= s;
		}
	}

	std::vector<std::vector<Point2>> contours;
	contours.push_back({});

	auto v1 = input.cend() - 1;
	for(auto v2 = input.cbegin(); v2 != input.cend(); v1 = v2, ++v2) {
		contours.back().push_back(*v1);

		Vector2 delta = (*v2) - (*v1);
		if(std::abs(delta.x()) < FloatType(0.5) && std::abs(delta.y()) < FloatType(0.5))
			continue;

		FloatType t[2] = { 2, 2 };
		Vector2I crossDir(0, 0);
		for(size_t dim = 0; dim < 2; dim++) {
			if(pbcFlags[dim]) {
				if(delta[dim] >= FloatType(0.5)) {
					delta[dim] -= FloatType(1);
					if(std::abs(delta[dim]) > FLOATTYPE_EPSILON)
						t[dim] = std::min((*v1)[dim] / -delta[dim], FloatType(1));
					else
						t[dim] = FloatType(0.5);
					crossDir[dim] = -1;
					OVITO_ASSERT(t[dim] >= 0 && t[dim] <= 1);
				}
				else if(delta[dim] <= FloatType(-0.5)) {
					delta[dim] += FloatType(1);
					if(std::abs(delta[dim]) > FLOATTYPE_EPSILON)
						t[dim] = std::max((FloatType(1) - (*v1)[dim]) / delta[dim], FloatType(0));
					else
						t[dim] = FloatType(0.5);
					crossDir[dim] = +1;
					OVITO_ASSERT(t[dim] >= 0 && t[dim] <= 1);
				}
			}
		}

		Point2 base = *v1;
		if(t[0] < t[1]) {
			OVITO_ASSERT(t[0] <= 1);
			computeContourIntersection(0, t[0], base, delta, crossDir[0], contours);
			if(crossDir[1] != 0) {
				OVITO_ASSERT(t[1] <= 1);
				computeContourIntersection(1, t[1], base, delta, crossDir[1], contours);
			}
		}
		else if(t[1] < t[0]) {
			OVITO_ASSERT(t[1] <= 1);
			computeContourIntersection(1, t[1], base, delta, crossDir[1], contours);
			if(crossDir[0] != 0) {
				OVITO_ASSERT(t[0] <= 1);
				computeContourIntersection(0, t[0], base, delta, crossDir[0], contours);
			}
		}
	}

	if(contours.size() == 1) {
		closedContours.push_back(std::move(contours.back()));
	}
	else {
		auto& firstSegment = contours.front();
		auto& lastSegment = contours.back();
		firstSegment.insert(firstSegment.begin(), lastSegment.begin(), lastSegment.end());
		contours.pop_back();
		for(auto& contour : contours) {
			bool isDegenerate = std::all_of(contour.begin(), contour.end(), [&contour](const Point2& p) { 
				return p.equals(contour.front()); 
			});
			if(!isDegenerate)
				openContours.push_back(std::move(contour));
		}
	}
}

/******************************************************************************
* Computes the intersection point of a 2d contour segment crossing a
* periodic boundary.
******************************************************************************/
void SurfaceMeshVis::computeContourIntersection(size_t dim, FloatType t, Point2& base, Vector2& delta, int crossDir, std::vector<std::vector<Point2>>& contours)
{
	OVITO_ASSERT(std::isfinite(t));
	Point2 intersection = base + t * delta;
	intersection[dim] = (crossDir == -1) ? 0 : 1;
	contours.back().push_back(intersection);
	intersection[dim] = (crossDir == +1) ? 0 : 1;
	contours.push_back({intersection});
	base = intersection;
	delta *= (FloatType(1) - t);
}

/******************************************************************************
* Determines if the 2D box corner (0,0) is inside the closed region described
* by the 2d polygon.
*
* 2D version of the algorithm:
*
* J. Andreas Baerentzen and Henrik Aanaes
* Signed Distance Computation Using the Angle Weighted Pseudonormal
* IEEE Transactions on Visualization and Computer Graphics 11 (2005), Page 243
******************************************************************************/
bool SurfaceMeshVis::isCornerInside2DRegion(const std::vector<std::vector<Point2>>& contours)
{
	OVITO_ASSERT(!contours.empty());
	bool isInside = true;

	// Determine which vertex is closest to the test point.
	std::vector<Point2>::const_iterator closestVertex = contours.front().end();
	FloatType closestDistanceSq = FLOATTYPE_MAX;
	for(const auto& contour : contours) {
		auto v1 = contour.end() - 1;
		for(auto v2 = contour.begin(); v2 != contour.end(); v1 = v2++) {
			Vector2 r = (*v1) - Point2::Origin();
			FloatType distanceSq = r.squaredLength();
			if(distanceSq < closestDistanceSq) {
				closestDistanceSq = distanceSq;
				closestVertex = v1;

				// Compute pseuso-normal at vertex.
				auto v0 = (v1 == contour.begin()) ? (contour.end() - 1) : (v1 - 1);
				Vector2 edgeDir = (*v2) - (*v0);
				Vector2 normal(edgeDir.y(), -edgeDir.x());
				isInside = (normal.dot(r) > 0);
			}

			// Check if any edge is closer to the test point.
			Vector2 edgeDir = (*v2) - (*v1);
			FloatType edgeLength = edgeDir.length();
			if(edgeLength <= FLOATTYPE_EPSILON) continue;
			edgeDir /= edgeLength;
			FloatType d = -edgeDir.dot(r);
			if(d <= 0 || d >= edgeLength) continue;
			Vector2 c = r + edgeDir * d;
			distanceSq = c.squaredLength();
			if(distanceSq < closestDistanceSq) {
				closestDistanceSq = distanceSq;

				// Compute normal at edge.
				Vector2 normal(edgeDir.y(), -edgeDir.x());
				isInside = (normal.dot(c) > 0);
			}
		}
	}

	return isInside;
}

}	// End of namespace
}	// End of namespace
