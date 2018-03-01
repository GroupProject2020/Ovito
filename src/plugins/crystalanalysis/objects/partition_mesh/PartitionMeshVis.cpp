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

#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include <plugins/crystalanalysis/objects/clusters/ClusterGraphObject.h>
#include <plugins/mesh/surface/RenderableSurfaceMesh.h>
#include <core/rendering/SceneRenderer.h>
#include <core/utilities/mesh/TriMesh.h>
#include <core/utilities/concurrent/PromiseState.h>
#include <core/utilities/units/UnitsManager.h>
#include <plugins/stdobj/simcell/SimulationCellObject.h>
#include <core/dataset/animation/controller/Controller.h>
#include <core/dataset/DataSetContainer.h>
#include "PartitionMeshVis.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

IMPLEMENT_OVITO_CLASS(PartitionMeshVis);
DEFINE_PROPERTY_FIELD(PartitionMeshVis, surfaceColor);
DEFINE_PROPERTY_FIELD(PartitionMeshVis, smoothShading);
DEFINE_PROPERTY_FIELD(PartitionMeshVis, flipOrientation);
DEFINE_REFERENCE_FIELD(PartitionMeshVis, surfaceTransparencyController);
SET_PROPERTY_FIELD_LABEL(PartitionMeshVis, surfaceColor, "Free surface color");
SET_PROPERTY_FIELD_LABEL(PartitionMeshVis, smoothShading, "Smooth shading");
SET_PROPERTY_FIELD_LABEL(PartitionMeshVis, surfaceTransparencyController, "Surface transparency");
SET_PROPERTY_FIELD_LABEL(PartitionMeshVis, flipOrientation, "Flip surface orientation");
SET_PROPERTY_FIELD_UNITS_AND_RANGE(PartitionMeshVis, surfaceTransparencyController, PercentParameterUnit, 0, 1);

/******************************************************************************
* Constructor.
******************************************************************************/
PartitionMeshVis::PartitionMeshVis(DataSet* dataset) : TransformingDataVis(dataset),
	_surfaceColor(1, 1, 1),
	_smoothShading(true),
	_flipOrientation(false)
{
	setSurfaceTransparencyController(ControllerManager::createFloatController(dataset));
}

/******************************************************************************
* Is called when the value of a property of this object has changed.
******************************************************************************/
void PartitionMeshVis::propertyChanged(const PropertyFieldDescriptor& field)
{
	if(field == PROPERTY_FIELD(smoothShading) || field == PROPERTY_FIELD(flipOrientation)) {
		// This kind of parameter change triggers a regeneration of the cached RenderableSurfaceMesh.
		invalidateTransformedObjects();
	}
	TransformingDataVis::propertyChanged(field);
}

/******************************************************************************
* Computes the bounding box of the displayed data.
******************************************************************************/
Box3 PartitionMeshVis::boundingBox(TimePoint time, DataObject* dataObject, PipelineSceneNode* contextNode, const PipelineFlowState& flowState, TimeInterval& validityInterval)
{
	// Compute mesh bounding box.
	Box3 bb;
	if(OORef<RenderableSurfaceMesh> meshObj = dataObject->convertTo<RenderableSurfaceMesh>(time)) {
		bb.addBox(meshObj->surfaceMesh().boundingBox());
	}
	return bb;
}

/******************************************************************************
* Lets the display object transform a data object in preparation for rendering.
******************************************************************************/
Future<PipelineFlowState> PartitionMeshVis::transformDataImpl(TimePoint time, DataObject* dataObject, PipelineFlowState&& flowState, const PipelineFlowState& cachedState, PipelineSceneNode* contextNode)
{
	// Get the partition mesh.
	PartitionMesh* partitionMeshObj = dynamic_object_cast<PartitionMesh>(dataObject);
	if(!partitionMeshObj)
		return std::move(flowState);

	// Get the simulation cell.
	SimulationCellObject* cellObject = partitionMeshObj->domain();
	if(!cellObject)
		return std::move(flowState);

	// Get the cluster graph.
	ClusterGraphObject* clusterGraphObject = flowState.findObject<ClusterGraphObject>();
	
	// Create compute engine.
	auto engine = std::make_shared<PrepareMeshEngine>(partitionMeshObj->storage(), 
		clusterGraphObject ? clusterGraphObject->storage() : nullptr, cellObject->data(), 
		partitionMeshObj->spaceFillingRegion(), partitionMeshObj->cuttingPlanes(), flipOrientation(), smoothShading());

	// Submit engine for execution and post-process results.
	return dataset()->container()->taskManager().runTaskAsync(engine)
		.then(executor(), [this, flowState = std::move(flowState), dataObject](TriMesh&& surfaceMesh, std::vector<ColorA>&& materialColors) mutable {
			UndoSuspender noUndo(this);

			// Output the computed mesh as a RenderableSurfaceMesh.
			OORef<RenderableSurfaceMesh> renderableMesh = new RenderableSurfaceMesh(this, dataObject, std::move(surfaceMesh), {});
			renderableMesh->setMaterialColors(std::move(materialColors));
			flowState.addObject(renderableMesh);
			return std::move(flowState);
		});
}

/******************************************************************************
* Computes the results and stores them in this object for later retrieval.
******************************************************************************/
void PartitionMeshVis::PrepareMeshEngine::perform()
{
	setProgressText(tr("Preparing microstructure mesh for display"));
	
	TriMesh surfaceMesh;

	if(!buildMesh(*_inputMesh, _simCell, _cuttingPlanes, surfaceMesh, this))
		throw Exception(tr("Failed to generate non-periodic version of microstructure mesh for display. Simulation cell might be too small."));

	if(isCanceled())
		return;

	if(!_flipOrientation)
		surfaceMesh.flipFaces();

	if(isCanceled())
		return;

	if(_smoothShading) {
		// Assign smoothing group to faces to interpolate normals.
		for(auto& face : surfaceMesh.faces())
			face.setSmoothingGroups(1);	
	}

	// Define surface colors for the regions by taking them from the cluster graph.
	int maxClusterId = 0;
	if(_clusterGraph) {
		for(Cluster* cluster : _clusterGraph->clusters())
			if(cluster->id > maxClusterId)
				maxClusterId = cluster->id;
	}
	std::vector<ColorA> materialColors(maxClusterId + 1, ColorA(1,1,1,1));
	if(_clusterGraph) {
		for(Cluster* cluster : _clusterGraph->clusters()) {
			if(cluster->id != 0)
				materialColors[cluster->id] = ColorA(cluster->color);
		}
	}
	

	setResult(std::move(surfaceMesh), std::move(materialColors));
}

/******************************************************************************
* Lets the visualization element render the data object.
******************************************************************************/
void PartitionMeshVis::render(TimePoint time, DataObject* dataObject, const PipelineFlowState& flowState, SceneRenderer* renderer, PipelineSceneNode* contextNode)
{
	// Ignore render calls for the original PartitionMesh.
	// We are only interested in the RenderableSurfaceMesh.
	if(dynamic_object_cast<PartitionMesh>(dataObject) != nullptr)
		return;

	if(renderer->isBoundingBoxPass()) {
		TimeInterval validityInterval;
		renderer->addToLocalBoundingBox(boundingBox(time, dataObject, contextNode, flowState, validityInterval));
		return;
	}

	// Get the cluster graph.
	ClusterGraphObject* clusterGraph = flowState.findObject<ClusterGraphObject>();

	// Get the rendering colors for the surface and cap meshes.
	FloatType surface_alpha = 1;
	TimeInterval iv;
	if(surfaceTransparencyController()) surface_alpha = FloatType(1) - surfaceTransparencyController()->getFloatValue(time, iv);
	ColorA color_surface(surfaceColor(), surface_alpha);

	// Do we have to re-create the render primitives from scratch?
	bool recreateSurfaceBuffer = !_surfaceBuffer || !_surfaceBuffer->isValid(renderer);

	// Do we have to update the render primitives?
	bool updateContents = _geometryCacheHelper.updateState(dataObject, color_surface, clusterGraph)
					|| recreateSurfaceBuffer;

	// Re-create the render primitives if necessary.
	if(recreateSurfaceBuffer)
		_surfaceBuffer = renderer->createMeshPrimitive();

	// Update render primitives.
	if(updateContents) {

		OORef<RenderableSurfaceMesh> meshObj = dataObject->convertTo<RenderableSurfaceMesh>(time);

		auto materialColors = meshObj->materialColors();
		materialColors[0] = color_surface;
		for(ColorA& c : materialColors)
			c.a() = surface_alpha;
		_surfaceBuffer->setMaterialColors(materialColors);
		
		_surfaceBuffer->setMesh(meshObj->surfaceMesh(), color_surface);
		_surfaceBuffer->setCullFaces(true);
	}

	// Handle picking of triangles.
	renderer->beginPickObject(contextNode);
	_surfaceBuffer->render(renderer);
	renderer->endPickObject();
}

/******************************************************************************
* Generates the final triangle mesh, which will be rendered.
******************************************************************************/
bool PartitionMeshVis::buildMesh(const PartitionMeshData& input, const SimulationCell& cell, const QVector<Plane3>& cuttingPlanes, TriMesh& output, PromiseState* promise)
{
	// Convert half-edge mesh to triangle mesh.
	input.convertToTriMesh(output);

	// Transfer region IDs to triangle faces.
	auto fout = output.faces().begin();
	for(PartitionMeshData::Face* face : input.faces()) {
		for(PartitionMeshData::Edge* edge = face->edges()->nextFaceEdge()->nextFaceEdge(); edge != face->edges(); edge = edge->nextFaceEdge()) {
			fout->setMaterialIndex(face->region);
			++fout;
		}
	}
	OVITO_ASSERT(fout == output.faces().end());

	if(promise && promise->isCanceled())
		return false;

	// Convert vertex positions to reduced coordinates.
	for(Point3& p : output.vertices()) {
		p = cell.absoluteToReduced(p);
		OVITO_ASSERT(std::isfinite(p.x()) && std::isfinite(p.y()) && std::isfinite(p.z()));
	}

	// Wrap mesh at periodic boundaries.
	for(size_t dim = 0; dim < 3; dim++) {
		if(cell.pbcFlags()[dim] == false) continue;

		if(promise && promise->isCanceled())
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
			if(!splitFace(output, findex, oldVertexCount, newVertices, newVertexLookupMap, cell, dim))
				return false;
		}

		// Insert newly created vertices into mesh.
		output.setVertexCount(oldVertexCount + (int)newVertices.size());
		std::copy(newVertices.cbegin(), newVertices.cend(), output.vertices().begin() + oldVertexCount);
	}

	// Check for early abortion.
	if(promise && promise->isCanceled())
		return false;

	// Convert vertex positions back from reduced coordinates to absolute coordinates.
	AffineTransformation cellMatrix = cell.matrix();
	for(Point3& p : output.vertices())
		p = cellMatrix * p;

	// Clip mesh at cutting planes.
	for(const Plane3& plane : cuttingPlanes) {
		if(promise && promise->isCanceled())
			return false;

		output.clipAtPlane(plane);
	}

	output.invalidateVertices();
	output.invalidateFaces();

	return !promise || !promise->isCanceled();
}

/******************************************************************************
* Splits a triangle face at a periodic boundary.
******************************************************************************/
bool PartitionMeshVis::splitFace(TriMesh& output, int faceIndex, int oldVertexCount, std::vector<Point3>& newVertices,
		std::map<std::pair<int,int>,std::pair<int,int>>& newVertexLookupMap, const SimulationCell& cell, size_t dim)
{
	TriMeshFace& face = output.face(faceIndex);
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

	int materialIndex = face.materialIndex();
	output.setFaceCount(output.faceCount() + 2);
	TriMeshFace& newFace1 = output.face(output.faceCount() - 2);
	TriMeshFace& newFace2 = output.face(output.faceCount() - 1);
	newFace1.setVertices(originalVertices[(properEdge+1)%3], newVertexIndices[(properEdge+1)%3][0], newVertexIndices[(properEdge+2)%3][1]);
	newFace2.setVertices(newVertexIndices[(properEdge+1)%3][1], originalVertices[(properEdge+2)%3], newVertexIndices[(properEdge+2)%3][0]);
	newFace1.setMaterialIndex(materialIndex);
	newFace2.setMaterialIndex(materialIndex);

	return true;
}

}	// End of namespace
}	// End of namespace
}	// End of namespace
