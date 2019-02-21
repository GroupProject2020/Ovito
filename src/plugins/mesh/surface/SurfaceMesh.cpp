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

#include <plugins/mesh/Mesh.h>
#include <core/utilities/concurrent/ParallelFor.h>
#include <plugins/stdobj/simcell/SimulationCellObject.h>
#include "SurfaceMesh.h"
#include "SurfaceMeshVis.h"

namespace Ovito { namespace Mesh {

IMPLEMENT_OVITO_CLASS(SurfaceMesh);
DEFINE_PROPERTY_FIELD(SurfaceMesh, topology);
DEFINE_PROPERTY_FIELD(SurfaceMesh, spaceFillingRegion);
DEFINE_PROPERTY_FIELD(SurfaceMesh, title);
DEFINE_REFERENCE_FIELD(SurfaceMesh, vertices);
DEFINE_REFERENCE_FIELD(SurfaceMesh, faces);
SET_PROPERTY_FIELD_LABEL(SurfaceMesh, title, "Title");
SET_PROPERTY_FIELD_LABEL(SurfaceMesh, vertices, "Vertices");
SET_PROPERTY_FIELD_LABEL(SurfaceMesh, faces, "Faces");

/******************************************************************************
* Constructs an empty surface mesh object.
******************************************************************************/
SurfaceMesh::SurfaceMesh(DataSet* dataset, const QString& title) : PeriodicDomainDataObject(dataset),
	_title(title),
	_spaceFillingRegion(0)
{
	// Attach a visualization element for rendering the surface mesh.
	addVisElement(new SurfaceMeshVis(dataset));

	// Create the sub-object for storing the vertex properties.
	setVertices(new SurfaceMeshVertices(dataset));

	// Create the sub-object for storing the face properties.
	setFaces(new SurfaceMeshFaces(dataset));
}

/******************************************************************************
* Checks if the surface mesh is valid and all vertex and face properties 
* are consistent with the topology of the mesh. If this is not the case, 
* the method throws an exception. 
******************************************************************************/
void SurfaceMesh::verifyMeshIntegrity() const
{
	if(!topology())
		throwException(tr("Surface mesh has no topology object attached."));

	if(!vertices()) 
		throwException(tr("Surface mesh has no vertex properties container attached."));
	if(!vertices()->getProperty(SurfaceMeshVertices::PositionProperty))
		throwException(tr("Invalid data structure. Surface mesh is missing the position vertex property."));
	if(topology()->vertexCount() != vertices()->elementCount())
		throwException(tr("Length of vertex property arrays of surface mesh do not match number of vertices in the mesh topology."));

	if(!faces()) 
		throwException(tr("Surface mesh has no face properties container attached."));
	if(!faces()->properties().empty() && topology()->faceCount() != faces()->elementCount())
		throwException(tr("Length of face property arrays of surface mesh do not match number of faces in the mesh topology."));
}

/******************************************************************************
* Returns the topology data after making sure it is not shared with other owners.
******************************************************************************/
const HalfEdgeMeshPtr& SurfaceMesh::modifiableTopology() 
{
	// Copy data if there is more than one reference to the storage.
	OVITO_ASSERT(topology());
	OVITO_ASSERT(topology().use_count() >= 1);
	if(topology().use_count() > 1)
		_topology.mutableValue() = std::make_shared<HalfEdgeMesh>(*topology());
	OVITO_ASSERT(topology().use_count() == 1);
	return topology();
}

/******************************************************************************
* Duplicates the SurfaceMeshVertices sub-object if it is shared with other surface meshes.
* After this method returns, the sub-object is exclusively owned by the container and 
* can be safely modified without unwanted side effects.
******************************************************************************/
SurfaceMeshVertices* SurfaceMesh::makeVerticesMutable()
{
    OVITO_ASSERT(vertices());
    return makeMutable(vertices());
}

/******************************************************************************
* Duplicates the SurfaceMeshFaces sub-object if it is shared with other surface meshes.
* After this method returns, the sub-object is exclusively owned by the container and 
* can be safely modified without unwanted side effects.
******************************************************************************/
SurfaceMeshFaces* SurfaceMesh::makeFacesMutable()
{
    OVITO_ASSERT(faces());
    return makeMutable(faces());
}

/******************************************************************************
* Fairs a closed triangle mesh.
******************************************************************************/
bool SurfaceMesh::smoothMesh(const HalfEdgeMesh& mesh, PropertyStorage& vertexCoords, const SimulationCell& cell, int numIterations, PromiseState& promise, FloatType k_PB, FloatType lambda)
{
	// This is the implementation of the mesh smoothing algorithm:
	//
	// Gabriel Taubin
	// A Signal Processing Approach To Fair Surface Design
	// In SIGGRAPH 95 Conference Proceedings, pages 351-358 (1995)

	FloatType mu = FloatType(1) / (k_PB - FloatType(1)/lambda);
	promise.setProgressMaximum(numIterations);

	for(int iteration = 0; iteration < numIterations; iteration++) {
		if(!promise.setProgressValue(iteration))
			return false;
		smoothMeshIteration(mesh, vertexCoords, lambda, cell);
		smoothMeshIteration(mesh, vertexCoords, mu, cell);
	}

	return !promise.isCanceled();
}

/******************************************************************************
* Performs one iteration of the smoothing algorithm.
******************************************************************************/
void SurfaceMesh::smoothMeshIteration(const HalfEdgeMesh& mesh, PropertyStorage& vertexCoords, FloatType prefactor, const SimulationCell& cell)
{
	const AffineTransformation absoluteToReduced = cell.inverseMatrix();
	const AffineTransformation reducedToAbsolute = cell.matrix();

	// Compute displacement for each vertex.
	std::vector<Vector3> displacements(vertexCoords.size());
	parallelFor(vertexCoords.size(), [&mesh, &vertexCoords, &displacements, prefactor, cell, absoluteToReduced](size_t vertex) {
		Vector3 d = Vector3::Zero();

		// Go in positive direction around vertex, facet by facet.
		HalfEdgeMesh::edge_index currentEdge = mesh.firstVertexEdge(vertex);
		if(currentEdge != HalfEdgeMesh::InvalidIndex) {
			int numManifoldEdges = 0;
			do {
				OVITO_ASSERT(currentEdge != HalfEdgeMesh::InvalidIndex);
				OVITO_ASSERT(mesh.adjacentFace(currentEdge) != HalfEdgeMesh::InvalidIndex);
				d += cell.wrapVector(vertexCoords.getPoint3(mesh.vertex2(currentEdge)) - vertexCoords.getPoint3(vertex));
				numManifoldEdges++;
				currentEdge = mesh.oppositeEdge(mesh.prevFaceEdge(currentEdge));
			}
			while(currentEdge != mesh.firstVertexEdge(vertex));
			d *= (prefactor / numManifoldEdges);
		}

		displacements[vertex] = d;
	});

	// Apply computed displacements.
	auto d = displacements.cbegin();
	for(Point3& vertex : vertexCoords.point3Range())
		vertex += *d++;
}

/******************************************************************************
* Determines which spatial region contains the given point in space.
* Returns -1 if the point is exactly on a region boundary.
******************************************************************************/
int SurfaceMesh::locatePoint(const Point3& location, FloatType epsilon) const
{
	// Get the vertex coordinates of the mesh.
	const PropertyObject* vertexCoords = vertices()->getProperty(SurfaceMeshVertices::PositionProperty);
	if(!vertexCoords) return spaceFillingRegion();

	// Get the 'region' property of the mesh faces.
	const PropertyObject* faceRegions = faces()->getProperty(SurfaceMeshFaces::RegionProperty);

	return locatePointStatic(
				location, 
				*topology(), 
				*vertexCoords->storage(),
				domain() ? domain()->data() : SimulationCell(), 
				spaceFillingRegion(), 
				faceRegions ? faceRegions->storage() : nullptr,
				epsilon);
}	

/******************************************************************************
* Implementation of the locatePoint() method.
*
* Algorithm:
*
* J. Andreas Baerentzen and Henrik Aanaes:
* Signed Distance Computation Using the Angle Weighted Pseudonormal
* IEEE Transactions on Visualization and Computer Graphics 11 (2005), Page 243
******************************************************************************/
int SurfaceMesh::locatePointStatic(const Point3& location, const HalfEdgeMesh& mesh, const PropertyStorage& vertexCoords, const SimulationCell cell, int spaceFillingRegion, const ConstPropertyPtr& faceRegions, FloatType epsilon)
{
	OVITO_ASSERT(!faceRegions || faceRegions->size() == mesh.faceCount());
	OVITO_ASSERT(!faceRegions || (faceRegions->type() == SurfaceMeshFaces::RegionProperty && faceRegions->dataType() == PropertyStorage::Int));
	OVITO_ASSERT(spaceFillingRegion >= 0);

	// Determine which vertex is closest to the test point.
	FloatType closestDistanceSq = FLOATTYPE_MAX;
	HalfEdgeMesh::vertex_index closestVertex = HalfEdgeMesh::InvalidIndex;
	Vector3 closestNormal, closestVector;
	int closestRegion = spaceFillingRegion;
	for(const Point3& vpos : vertexCoords.constPoint3Range()) {
		Vector3 r = cell.wrapVector(vpos - location);
		FloatType distSq = r.squaredLength();
		if(distSq < closestDistanceSq) {
			HalfEdgeMesh::vertex_index vindex = &vpos - vertexCoords.constDataPoint3();
			if(mesh.firstVertexEdge(vindex) != HalfEdgeMesh::InvalidIndex) {
				closestDistanceSq = distSq;
				closestVertex = vindex;
				closestVector = r;
			}
		}
	}

	// If the surface is degenerate, any point is inside the space-filling region.
	if(closestVertex == HalfEdgeMesh::InvalidIndex)
		return spaceFillingRegion;

	// Check if any edge is closer to the test point than the closest vertex.
	const HalfEdgeMesh::size_type edgeCount = mesh.edgeCount();
	for(HalfEdgeMesh::edge_index edge = 0; edge < edgeCount; edge++) {
		OVITO_ASSERT_MSG(mesh.hasOppositeEdge(edge), "SurfaceMesh::locatePoint", "Surface mesh is not fully closed. This should not happen.");
		const Point3& p1 = vertexCoords.getPoint3(mesh.vertex1(edge));
		const Point3& p2 = vertexCoords.getPoint3(mesh.vertex2(edge));
		Vector3 edgeDir = cell.wrapVector(p2 - p1);
		Vector3 r = cell.wrapVector(p1 - location);
		FloatType edgeLength = edgeDir.length();
		if(edgeLength <= FLOATTYPE_EPSILON) continue;
		edgeDir /= edgeLength;
		FloatType d = -edgeDir.dot(r);
		if(d <= 0 || d >= edgeLength) continue;
		Vector3 c = r + edgeDir * d;
		FloatType distSq = c.squaredLength();
		if(distSq < closestDistanceSq) {
			closestDistanceSq = distSq;
			closestVertex = HalfEdgeMesh::InvalidIndex;
			closestVector = c;
			const Point3& p1a = vertexCoords.getPoint3(mesh.vertex2(mesh.nextFaceEdge(edge)));
			const Point3& p1b = vertexCoords.getPoint3(mesh.vertex2(mesh.nextFaceEdge(mesh.oppositeEdge(edge))));
			Vector3 e1 = cell.wrapVector(p1a - p1);
			Vector3 e2 = cell.wrapVector(p1b - p1);
			closestNormal = edgeDir.cross(e1).safelyNormalized() + e2.cross(edgeDir).safelyNormalized();
			if(faceRegions) {
				closestRegion = faceRegions->getInt(mesh.adjacentFace(edge));
			}
		}
	}

	// Check if any facet is closer to the test point than the closest vertex and the closest edge.
	const HalfEdgeMesh::size_type faceCount = mesh.faceCount();
	for(HalfEdgeMesh::face_index face = 0; face < faceCount; face++) {
		HalfEdgeMesh::edge_index edge1 = mesh.firstFaceEdge(face);
		HalfEdgeMesh::edge_index edge2 = mesh.nextFaceEdge(edge1);
		const Point3& p1 = vertexCoords.getPoint3(mesh.vertex1(edge1));
		const Point3& p2 = vertexCoords.getPoint3(mesh.vertex2(edge1));
		const Point3& p3 = vertexCoords.getPoint3(mesh.vertex2(edge2));
		Vector3 edgeVectors[3];
		edgeVectors[0] = cell.wrapVector(p2 - p1);
		edgeVectors[1] = cell.wrapVector(p3 - p2);
		Vector3 r = cell.wrapVector(p1 - location);
		edgeVectors[2] = -edgeVectors[1] - edgeVectors[0];

		Vector3 normal = edgeVectors[0].cross(edgeVectors[1]);
		bool isInsideTriangle = true;
		Vector3 vertexVector = r;
		for(size_t v = 0; v < 3; v++) {
			if(vertexVector.dot(normal.cross(edgeVectors[v])) >= 0.0) {
				isInsideTriangle = false;
				break;
			}
			vertexVector += edgeVectors[v];
		}
		if(isInsideTriangle) {
			FloatType normalLengthSq = normal.squaredLength();
			if(std::abs(normalLengthSq) <= FLOATTYPE_EPSILON) continue;
			normal /= sqrt(normalLengthSq);
			FloatType planeDist = normal.dot(r);
			if(planeDist * planeDist < closestDistanceSq) {
				closestDistanceSq = planeDist * planeDist;
				closestVector = normal * planeDist;
				closestVertex = HalfEdgeMesh::InvalidIndex;
				closestNormal = normal;
				if(faceRegions) {
					closestRegion = faceRegions->getInt(face);
				}
			}
		}
	}

	// If a vertex is closest, we still have to compute the local pseudo-normal at the vertex.
	if(closestVertex != HalfEdgeMesh::InvalidIndex) {
		HalfEdgeMesh::edge_index edge = mesh.firstVertexEdge(closestVertex);
		closestNormal.setZero();
		const Point3& closestVertexPos = vertexCoords.getPoint3(closestVertex);
		Vector3 edge1v = cell.wrapVector(vertexCoords.getPoint3(mesh.vertex2(edge)) - closestVertexPos);
		edge1v.normalizeSafely();
		do {
			HalfEdgeMesh::edge_index nextEdge = mesh.nextFaceEdge(mesh.oppositeEdge(edge));
			OVITO_ASSERT(mesh.vertex1(nextEdge) == closestVertex);
			Vector3 edge2v = cell.wrapVector(vertexCoords.getPoint3(mesh.vertex2(nextEdge)) - closestVertexPos);
			edge2v.normalizeSafely();
			FloatType angle = std::acos(edge1v.dot(edge2v));
			Vector3 normal = edge2v.cross(edge1v);
			if(normal != Vector3::Zero())
				closestNormal += normal.normalized() * angle;
			edge = nextEdge;
			edge1v = edge2v;
		}
		while(edge != mesh.firstVertexEdge(closestVertex));
		if(faceRegions) {
			closestRegion = faceRegions->getInt(mesh.adjacentFace(edge));
		}
	}

	FloatType dot = closestNormal.dot(closestVector);
	if(dot >= epsilon) return closestRegion;
	if(dot <= -epsilon) return 0;
	return -1;
}

}	// End of namespace
}	// End of namespace
