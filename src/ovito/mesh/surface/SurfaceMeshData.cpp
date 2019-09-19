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

#include <ovito/mesh/Mesh.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/core/utilities/mesh/TriMesh.h>
#include <ovito/core/utilities/concurrent/ParallelFor.h>
#include "SurfaceMeshData.h"
#include "SurfaceMesh.h"

namespace Ovito { namespace Mesh {

/******************************************************************************
* Constructor creating an empty surface mesh.
******************************************************************************/
SurfaceMeshData::SurfaceMeshData(const SimulationCell& cell) :
    _topology(std::make_shared<HalfEdgeMesh>()),
	_cell(cell)
{
    createVertexProperty(SurfaceMeshVertices::PositionProperty);
    OVITO_ASSERT(_vertexCoords != nullptr);
}

/******************************************************************************
* Constructor that adopts the data from the given pipeline data object into
* this structure.
******************************************************************************/
SurfaceMeshData::SurfaceMeshData(const SurfaceMesh* sm) :
    _topology(sm->topology()),
    _cell(sm->domain()->data()),
    _spaceFillingRegion(sm->spaceFillingRegion())
{
	OVITO_ASSERT(_topology);
	for(const PropertyObject* property : sm->vertices()->properties()) {
	    addVertexProperty(const_pointer_cast<PropertyStorage>(property->storage()));
	}
	for(const PropertyObject* property : sm->faces()->properties()) {
	    addFaceProperty(const_pointer_cast<PropertyStorage>(property->storage()));
	}
	_regionCount = sm->regions()->elementCount();
	for(const PropertyObject* property : sm->regions()->properties()) {
	    addRegionProperty(const_pointer_cast<PropertyStorage>(property->storage()));
	}
    OVITO_ASSERT(_vertexCoords != nullptr);
}

/******************************************************************************
* Copies the data stored in this structure to the given pipeline data object.
******************************************************************************/
void SurfaceMeshData::transferTo(SurfaceMesh* sm) const
{
	sm->setTopology(topology());
    sm->setSpaceFillingRegion(spaceFillingRegion());

	sm->makeVerticesMutable()->setContent(vertexCount(), _vertexProperties);
	sm->makeFacesMutable()->setContent(faceCount(), _faceProperties);
	sm->makeRegionsMutable()->setContent(regionCount(), _regionProperties);
}

/******************************************************************************
* Swaps the contents of two surface meshes.
******************************************************************************/
void SurfaceMeshData::swap(SurfaceMeshData& other)
{
	_topology.swap(other._topology);
	std::swap(_cell, other._cell);
	_vertexProperties.swap(other._vertexProperties);
	_faceProperties.swap(other._faceProperties);
	_regionProperties.swap(other._regionProperties);
	std::swap(_regionCount, other._regionCount);
	std::swap(_spaceFillingRegion, other._spaceFillingRegion);
	std::swap(_vertexCoords, other._vertexCoords);
	std::swap(_faceRegions, other._faceRegions);
	std::swap(_burgersVectors, other._burgersVectors);
	std::swap(_crystallographicNormals, other._crystallographicNormals);
	std::swap(_faceTypes, other._faceTypes);
	std::swap(_regionPhases, other._regionPhases);
	std::swap(_regionVolumes, other._regionVolumes);
	std::swap(_regionSurfaceAreas, other._regionSurfaceAreas);
}

/******************************************************************************
* Fairs a closed triangle mesh.
******************************************************************************/
bool SurfaceMeshData::smoothMesh(int numIterations, Task& task, FloatType k_PB, FloatType lambda)
{
    OVITO_ASSERT(isVertexPropertyMutable(SurfaceMeshVertices::PositionProperty));

	// This is the implementation of the mesh smoothing algorithm:
	//
	// Gabriel Taubin
	// A Signal Processing Approach To Fair Surface Design
	// In SIGGRAPH 95 Conference Proceedings, pages 351-358 (1995)

    // Performs one iteration of the smoothing algorithm.
    auto smoothMeshIteration = [this](FloatType prefactor) {
        const AffineTransformation absoluteToReduced = cell().inverseMatrix();
        const AffineTransformation reducedToAbsolute = cell().matrix();

        // Compute displacement for each vertex.
        std::vector<Vector3> displacements(vertexCount());
        parallelFor(vertexCount(), [this, &displacements, prefactor, absoluteToReduced](vertex_index vertex) {
            Vector3 d = Vector3::Zero();

            // Go in positive direction around vertex, facet by facet.
            edge_index currentEdge = firstVertexEdge(vertex);
            if(currentEdge != HalfEdgeMesh::InvalidIndex) {
                int numManifoldEdges = 0;
                do {
                    OVITO_ASSERT(currentEdge != HalfEdgeMesh::InvalidIndex);
                    OVITO_ASSERT(adjacentFace(currentEdge) != HalfEdgeMesh::InvalidIndex);
                    d += edgeVector(currentEdge);
                    numManifoldEdges++;
                    currentEdge = oppositeEdge(prevFaceEdge(currentEdge));
                }
                while(currentEdge != firstVertexEdge(vertex));
                d *= (prefactor / numManifoldEdges);
            }

            displacements[vertex] = d;
        });

        // Apply computed displacements.
        auto d = displacements.cbegin();
        for(Point3& vertex : boost::make_iterator_range(vertexCoords(), vertexCoords() + vertexCount()))
            vertex += *d++;
    };

	FloatType mu = FloatType(1) / (k_PB - FloatType(1)/lambda);
	task.setProgressMaximum(numIterations);

	for(int iteration = 0; iteration < numIterations; iteration++) {
		if(!task.setProgressValue(iteration))
			return false;
		smoothMeshIteration(lambda);
		smoothMeshIteration(mu);
	}

	return !task.isCanceled();
}

/******************************************************************************
* Determines which spatial region contains the given point in space.
*
* Algorithm:
*
* J. Andreas Baerentzen and Henrik Aanaes:
* Signed Distance Computation Using the Angle Weighted Pseudonormal
* IEEE Transactions on Visualization and Computer Graphics 11 (2005), Page 243
******************************************************************************/
int SurfaceMeshData::locatePoint(const Point3& location, FloatType epsilon, const boost::dynamic_bitset<>& faceSubset)
{
	OVITO_ASSERT(spaceFillingRegion() >= 0);

	// Determine which vertex is closest to the test point.
	FloatType closestDistanceSq = FLOATTYPE_MAX;
	vertex_index closestVertex = HalfEdgeMesh::InvalidIndex;
	edge_index closestVertexFirstEdge = HalfEdgeMesh::InvalidIndex;
	Vector3 closestNormal, closestVector;
	int closestRegion = spaceFillingRegion();
    size_type vcount = vertexCount();
	for(vertex_index vindex = 0; vindex < vcount; vindex++) {
		edge_index firstEdge = firstVertexEdge(vindex);
		if(!faceSubset.empty()) {
			while(firstEdge != HalfEdgeMesh::InvalidIndex && !faceSubset[adjacentFace(firstEdge)])
				firstEdge = nextVertexEdge(firstEdge);
		}
		if(firstEdge == HalfEdgeMesh::InvalidIndex) continue;
		Vector3 r = cell().wrapVector(vertexPosition(vindex) - location);
		FloatType distSq = r.squaredLength();
		if(distSq < closestDistanceSq) {
			closestDistanceSq = distSq;
			closestVertex = vindex;
			closestVector = r;
			closestVertexFirstEdge = firstEdge;
		}
	}

	// If the surface is degenerate, any point is inside the space-filling region.
	if(closestVertex == HalfEdgeMesh::InvalidIndex)
		return spaceFillingRegion();

	// Check if any edge is closer to the test point than the closest vertex.
	size_type edgeCount = this->edgeCount();
	for(edge_index edge = 0; edge < edgeCount; edge++) {
		if(!faceSubset.empty() && !faceSubset[adjacentFace(edge)]) continue;
		OVITO_ASSERT_MSG(hasOppositeEdge(edge), "SurfaceMeshData::locatePoint", "Surface mesh is not fully closed. This should not happen.");
		const Point3& p1 = vertexPosition(vertex1(edge));
		const Point3& p2 = vertexPosition(vertex2(edge));
		Vector3 edgeDir = cell().wrapVector(p2 - p1);
		Vector3 r = cell().wrapVector(p1 - location);
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
			const Point3& p1a = vertexPosition(vertex2(nextFaceEdge(edge)));
			const Point3& p1b = vertexPosition(vertex2(nextFaceEdge(oppositeEdge(edge))));
			Vector3 e1 = cell().wrapVector(p1a - p1);
			Vector3 e2 = cell().wrapVector(p1b - p1);
			closestNormal = edgeDir.cross(e1).safelyNormalized() + e2.cross(edgeDir).safelyNormalized();
			if(_faceRegions) {
				closestRegion = _faceRegions[adjacentFace(edge)];
			}
			else closestRegion = 1;
		}
	}

	// Check if any facet is closer to the test point than the closest vertex and the closest edge.
	size_type faceCount = this->faceCount();
	for(face_index face = 0; face < faceCount; face++) {
		if(!faceSubset.empty() && !faceSubset[face]) continue;
		edge_index edge1 = firstFaceEdge(face);
		edge_index edge2 = nextFaceEdge(edge1);
		const Point3& p1 = vertexPosition(vertex1(edge1));
		const Point3& p2 = vertexPosition(vertex2(edge1));
		const Point3& p3 = vertexPosition(vertex2(edge2));
		Vector3 edgeVectors[3];
		edgeVectors[0] = cell().wrapVector(p2 - p1);
		edgeVectors[1] = cell().wrapVector(p3 - p2);
		Vector3 r = cell().wrapVector(p1 - location);
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
				if(_faceRegions) {
					closestRegion = _faceRegions[face];
				}
				else closestRegion = 1;
			}
		}
	}

	// If a vertex is closest, we still have to compute the local pseudo-normal at the vertex.
	if(closestVertex != HalfEdgeMesh::InvalidIndex) {
		edge_index edge = closestVertexFirstEdge;
		closestNormal.setZero();
		const Point3& closestVertexPos = vertexPosition(closestVertex);
		Vector3 edge1v = cell().wrapVector(vertexPosition(vertex2(edge)) - closestVertexPos);
		edge1v.normalizeSafely();
		do {
			edge_index nextEdge = nextFaceEdge(oppositeEdge(edge));
			OVITO_ASSERT(vertex1(nextEdge) == closestVertex);
			Vector3 edge2v = cell().wrapVector(vertexPosition(vertex2(nextEdge)) - closestVertexPos);
			edge2v.normalizeSafely();
			FloatType angle = std::acos(edge1v.dot(edge2v));
			Vector3 normal = edge2v.cross(edge1v);
			if(normal != Vector3::Zero())
				closestNormal += normal.normalized() * angle;
			edge = nextEdge;
			edge1v = edge2v;
		}
		while(edge != closestVertexFirstEdge);
		if(_faceRegions) {
			closestRegion = _faceRegions[adjacentFace(edge)];
		}
		else closestRegion = 1;
	}
	OVITO_ASSERT(closestRegion >= 0);

	FloatType dot = closestNormal.dot(closestVector);
	if(dot >= epsilon) return closestRegion;
	if(dot <= -epsilon) return spaceFillingRegion();
	return -1;
}

/******************************************************************************
* Constructs the convex hull from a set of points and adds the resulting
* polyhedron to the mesh.
******************************************************************************/
void SurfaceMeshData::constructConvexHull(std::vector<Point3> vecs)
{
	// Create a new spatial region for the polyhedron in the output mesh.
	SurfaceMeshData::region_index region = createRegion();

	if(vecs.size() < 4) return;	// Convex hull requires at least 4 input points.

	// Keep track of how many faces and vertices we started with.
	// We won't touch the existing mesh faces and vertices.
	auto originalFaceCount = faceCount();
	auto originalVertexCount = vertexCount();

	// Determine which points should form the initial tetrahedron.
	// Make sure they are not co-planar.
	size_t tetrahedraCorners[4];
	tetrahedraCorners[0] = 0;
	size_t n = 1;
	Matrix3 m;
	for(size_t i = 1; i < vecs.size(); i++) {
		if(n == 1) {
			m.column(0) = vecs[i] - vecs[0];
			tetrahedraCorners[1] = i;
			if(!m.column(0).isZero()) n = 2;
		}
		else if(n == 2) {
			m.column(1) = vecs[i] - vecs[0];
			tetrahedraCorners[2] = i;
			if(!m.column(0).cross(m.column(1)).isZero()) n = 3;
		}
		else if(n == 3) {
			m.column(2) = vecs[i] - vecs[0];
			FloatType det = m.determinant();
			if(std::abs(det) > FLOATTYPE_EPSILON) {
				tetrahedraCorners[3] = i;
				if(det < 0) std::swap(tetrahedraCorners[0], tetrahedraCorners[1]);
				n = 4;
				break;
			}
		}
	}
	if(n != 4) return;

	// Create the initial tetrahedron.
	HalfEdgeMesh::vertex_index tetverts[4];
	for(size_t i = 0; i < 4; i++) {
        tetverts[i] = createVertex(vecs[tetrahedraCorners[i]]);
	}
	createFace({tetverts[0], tetverts[1], tetverts[3]}, region);
	createFace({tetverts[2], tetverts[0], tetverts[3]}, region);
	createFace({tetverts[0], tetverts[2], tetverts[1]}, region);
	createFace({tetverts[1], tetverts[2], tetverts[3]}, region);
	// Connect opposite half-edges to link the four faces together.
	for(size_t i = 0; i < 4; i++)
		topology()->connectOppositeHalfedges(tetverts[i]);

	// Remove 4 points of initial tetrahedron from input list.
	for(size_t i = 1; i <= 4; i++)
		vecs[tetrahedraCorners[4-i]] = vecs[vecs.size()-i];
	vecs.erase(vecs.end() - 4, vecs.end());

	// Simplified Quick-hull algorithm.
	for(;;) {
		// Find the point on the positive side of a face and furthest away from it.
		// Also remove points from list which are on the negative side of all faces.
		auto furthestPoint = vecs.rend();
		FloatType furthestPointDistance = 0;
		size_t remainingPointCount = vecs.size();
		for(auto p = vecs.rbegin(); p != vecs.rend(); ++p) {
			bool insideHull = true;
			for(auto faceIndex = originalFaceCount; faceIndex < faceCount(); faceIndex++) {
				auto v0 = firstFaceVertex(faceIndex);
				auto v1 = secondFaceVertex(faceIndex);
				auto v2 = thirdFaceVertex(faceIndex);
				Plane3 plane(vertexPosition(v0), vertexPosition(v1), vertexPosition(v2), true);
				FloatType signedDistance = plane.pointDistance(*p);
				if(signedDistance > FLOATTYPE_EPSILON) {
					insideHull = false;
					if(signedDistance > furthestPointDistance) {
						furthestPointDistance = signedDistance;
						furthestPoint = p;
					}
				}
			}
			// When point is inside the hull, remove it from the input list.
			if(insideHull) {
				remainingPointCount--;
				*p = vecs[remainingPointCount];
			}
		}
		if(!remainingPointCount) break;
		OVITO_ASSERT(furthestPointDistance > 0 && furthestPoint != vecs.rend());

		// Kill all faces of the polyhedron that can be seen from the selected point.
		for(auto face = originalFaceCount; face < faceCount(); face++) {
			auto v0 = firstFaceVertex(face);
			auto v1 = secondFaceVertex(face);
			auto v2 = thirdFaceVertex(face);
			Plane3 plane(vertexPosition(v0), vertexPosition(v1), vertexPosition(v2), true);
			if(plane.pointDistance(*furthestPoint) > FLOATTYPE_EPSILON) {
				deleteFace(face);
				face--;
			}
		}

		// Find an edge that borders the newly created hole in the mesh.
		HalfEdgeMesh::edge_index firstBorderEdge = HalfEdgeMesh::InvalidIndex;
		for(auto face = originalFaceCount; face < faceCount() && firstBorderEdge == HalfEdgeMesh::InvalidIndex; face++) {
			HalfEdgeMesh::edge_index e = firstFaceEdge(face);
			OVITO_ASSERT(e != HalfEdgeMesh::InvalidIndex);
			do {
				if(!hasOppositeEdge(e)) {
					firstBorderEdge = e;
					break;
				}
				e = nextFaceEdge(e);
			}
			while(e != firstFaceEdge(face));
		}
		OVITO_ASSERT(firstBorderEdge != HalfEdgeMesh::InvalidIndex); // If this assert fails, then there was no hole in the mesh.

		// Create new faces that connects the edges at the horizon (i.e. the border of the hole) with
		// the selected vertex.
		HalfEdgeMesh::vertex_index vertex = createVertex(*furthestPoint);
		HalfEdgeMesh::edge_index borderEdge = firstBorderEdge;
		HalfEdgeMesh::face_index previousFace = HalfEdgeMesh::InvalidIndex;
		HalfEdgeMesh::face_index firstFace = HalfEdgeMesh::InvalidIndex;
		HalfEdgeMesh::face_index newFace;
		do {
			newFace = createFace({ vertex2(borderEdge), vertex1(borderEdge), vertex }, region);
			linkOppositeEdges(firstFaceEdge(newFace), borderEdge);
			if(borderEdge == firstBorderEdge)
				firstFace = newFace;
			else
				linkOppositeEdges(secondFaceEdge(newFace), prevFaceEdge(firstFaceEdge(previousFace)));
			previousFace = newFace;
			// Proceed to next edge along the hole's border.
			for(;;) {
				borderEdge = nextFaceEdge(borderEdge);
				if(!hasOppositeEdge(borderEdge) || borderEdge == firstBorderEdge)
					break;
				borderEdge = oppositeEdge(borderEdge);
			}
		}
		while(borderEdge != firstBorderEdge);
		OVITO_ASSERT(firstFace != newFace);
		linkOppositeEdges(secondFaceEdge(firstFace), prevFaceEdge(firstFaceEdge(newFace)));

		// Remove selected point from the input list as well.
		remainingPointCount--;
		*furthestPoint = vecs[remainingPointCount];
		vecs.resize(remainingPointCount);
	}

	// Delete interior vertices from the mesh that are no longer attached to any of the faces.
	for(auto vertex = originalVertexCount; vertex < vertexCount(); vertex++) {
		if(vertexEdgeCount(vertex) == 0) {
			// Delete the vertex from the mesh topology.
			deleteVertex(vertex);
			// Adjust index to point to next vertex in the mesh after loop incrementation.
			vertex--;
		}
	}
}

/******************************************************************************
* Triangulates the polygonal faces of this mesh and outputs the results as a TriMesh object.
******************************************************************************/
void SurfaceMeshData::convertToTriMesh(TriMesh& outputMesh, bool smoothShading, const boost::dynamic_bitset<>& faceSubset, std::vector<size_t>* originalFaceMap) const
{
	const HalfEdgeMesh& topology = *this->topology();
	HalfEdgeMesh::size_type faceCount = topology.faceCount();
	OVITO_ASSERT(faceSubset.empty() || faceSubset.size() == faceCount);

	// Create output vertices.
	outputMesh.setVertexCount(topology.vertexCount());
	SurfaceMeshData::vertex_index vidx = 0;
	for(Point3& p : outputMesh.vertices())
		p = vertexPosition(vidx++);

	// Transfer faces from surface mesh to output triangle mesh.
	for(HalfEdgeMesh::face_index face = 0; face < faceCount; face++) {
		if(!faceSubset.empty() && !faceSubset[face]) continue;

		// Go around the edges of the face to triangulate the general polygon.
		HalfEdgeMesh::edge_index faceEdge = topology.firstFaceEdge(face);
		HalfEdgeMesh::vertex_index baseVertex = topology.vertex2(faceEdge);
		HalfEdgeMesh::edge_index edge1 = topology.nextFaceEdge(faceEdge);
		HalfEdgeMesh::edge_index edge2 = topology.nextFaceEdge(edge1);
		while(edge2 != faceEdge) {
			TriMeshFace& outputFace = outputMesh.addFace();
			outputFace.setVertices(baseVertex, topology.vertex2(edge1), topology.vertex2(edge2));
			outputFace.setEdgeVisibility(edge1 == topology.nextFaceEdge(faceEdge), true, false);
			if(originalFaceMap)
				originalFaceMap->push_back(face);
			edge1 = edge2;
			edge2 = topology.nextFaceEdge(edge2);
			if(edge2 == faceEdge)
				outputFace.setEdgeVisible(2);
		}
	}

	if(smoothShading) {
		// Compute mesh face normals.
		std::vector<Vector3> faceNormals(faceCount);
		auto faceNormal = faceNormals.begin();
		for(HalfEdgeMesh::face_index face = 0; face < faceCount; face++, ++faceNormal) {
			if(!faceSubset.empty() && !faceSubset[face])
				faceNormal->setZero();
			else
				*faceNormal = computeFaceNormal(face);
		}

		// Smooth normals.
		std::vector<Vector3> newFaceNormals(faceCount);
		auto oldFaceNormal = faceNormals.begin();
		auto newFaceNormal = newFaceNormals.begin();
		for(HalfEdgeMesh::face_index face = 0; face < faceCount; face++, ++oldFaceNormal, ++newFaceNormal) {
			*newFaceNormal = *oldFaceNormal;
			if(!faceSubset.empty() && !faceSubset[face]) continue;

			HalfEdgeMesh::edge_index faceEdge = topology.firstFaceEdge(face);
			HalfEdgeMesh::edge_index edge = faceEdge;
			do {
				HalfEdgeMesh::edge_index oe = topology.oppositeEdge(edge);
				if(oe != HalfEdgeMesh::InvalidIndex) {
					*newFaceNormal += faceNormals[topology.adjacentFace(oe)];
				}
				edge = topology.nextFaceEdge(edge);
			}
			while(edge != faceEdge);

			newFaceNormal->normalizeSafely();
		}
		faceNormals = std::move(newFaceNormals);
		newFaceNormals.clear();
		newFaceNormals.shrink_to_fit();

		// Helper method that calculates the mean normal at a surface mesh vertex.
		// The method takes an half-edge incident on the vertex as input (instead of the vertex itself),
		// because the method will only take into account incident faces belonging to one manifold.
		auto calculateNormalAtVertex = [&](HalfEdgeMesh::edge_index startEdge) {
			Vector3 normal = Vector3::Zero();
			HalfEdgeMesh::edge_index edge = startEdge;
			do {
				normal += faceNormals[topology.adjacentFace(edge)];
				edge = topology.oppositeEdge(topology.nextFaceEdge(edge));
				if(edge == HalfEdgeMesh::InvalidIndex) break;
			}
			while(edge != startEdge);
			if(edge == HalfEdgeMesh::InvalidIndex) {
				edge = topology.oppositeEdge(startEdge);
				while(edge != HalfEdgeMesh::InvalidIndex) {
					normal += faceNormals[topology.adjacentFace(edge)];
					edge = topology.oppositeEdge(topology.prevFaceEdge(edge));
				}
			}
			return normal;
		};

		// Compute normal at each face vertex.
		outputMesh.setHasNormals(true);
		auto outputNormal = outputMesh.normals().begin();
		for(HalfEdgeMesh::face_index face = 0; face < faceCount; face++) {
			if(!faceSubset.empty() && !faceSubset[face]) continue;

			// Go around the edges of the face.
			HalfEdgeMesh::edge_index faceEdge = topology.firstFaceEdge(face);
			HalfEdgeMesh::edge_index edge1 = topology.nextFaceEdge(faceEdge);
			HalfEdgeMesh::edge_index edge2 = topology.nextFaceEdge(edge1);
			Vector3 baseNormal = calculateNormalAtVertex(faceEdge);
			Vector3 normal1 = calculateNormalAtVertex(edge1);
			while(edge2 != faceEdge) {
				Vector3 normal2 = calculateNormalAtVertex(edge2);
				*outputNormal++ = baseNormal;
				*outputNormal++ = normal1;
				*outputNormal++ = normal2;
				normal1 = normal2;
				edge2 = topology.nextFaceEdge(edge2);
			}
		}
		OVITO_ASSERT(outputNormal == outputMesh.normals().end());
	}
}

/******************************************************************************
* Computes the unit normal vector of a mesh face.
******************************************************************************/
Vector3 SurfaceMeshData::computeFaceNormal(face_index face) const
{
	Vector3 faceNormal = Vector3::Zero();

	// Go around the edges of the face to triangulate the general polygon.
	HalfEdgeMesh::edge_index faceEdge = firstFaceEdge(face);
	HalfEdgeMesh::edge_index edge1 = nextFaceEdge(faceEdge);
	HalfEdgeMesh::edge_index edge2 = nextFaceEdge(edge1);
	Point3 base = vertexPosition(vertex2(faceEdge));
	Vector3 e1 = cell().wrapVector(vertexPosition(vertex2(edge1)) - base);
	while(edge2 != faceEdge) {
		Vector3 e2 = cell().wrapVector(vertexPosition(vertex2(edge2)) - base);
		faceNormal += e1.cross(e2);
		e1 = e2;
		edge1 = edge2;
		edge2 = nextFaceEdge(edge2);
	}

	return faceNormal.safelyNormalized();
}

/******************************************************************************
* Joins adjacent faces that are coplanar.
******************************************************************************/
void SurfaceMeshData::joinCoplanarFaces(FloatType thresholdAngle)
{
	FloatType dotThreshold = std::cos(thresholdAngle);

	// Compute mesh face normals.
	std::vector<Vector3> faceNormals(faceCount());
	for(face_index face = 0; face < faceCount(); face++) {
		faceNormals[face] = computeFaceNormal(face);
	}

	// Visit each face and its adjacent faces.
	for(face_index face = 0; face < faceCount(); ) {
		face_index nextFace = face + 1;
		const Vector3& normal1 = faceNormals[face];
		edge_index faceEdge = firstFaceEdge(face);
		edge_index edge = faceEdge;
		do {
			edge_index opp_edge = oppositeEdge(edge);
			if(opp_edge != HalfEdgeMesh::InvalidIndex) {
				face_index adj_face = adjacentFace(opp_edge);
				OVITO_ASSERT(adj_face >= 0 && adj_face < faceNormals.size());
				if(adj_face > face) {

					// Check if current face and its current neighbor are coplanar.
					const Vector3& normal2 = faceNormals[adj_face];
					if(normal1.dot(normal2) > dotThreshold) {
						// Eliminate this half-edge pair and join the two faces.
						for(edge_index currentEdge = nextFaceEdge(edge); currentEdge != edge; currentEdge = nextFaceEdge(currentEdge)) {
							OVITO_ASSERT(adjacentFace(currentEdge) == face);
							topology()->setAdjacentFace(currentEdge, adj_face);
						}
						topology()->setFirstFaceEdge(adj_face, nextFaceEdge(opp_edge));
						topology()->setFirstFaceEdge(face, edge);
						topology()->setNextFaceEdge(prevFaceEdge(edge), nextFaceEdge(opp_edge));
						topology()->setPrevFaceEdge(nextFaceEdge(opp_edge), prevFaceEdge(edge));
						topology()->setNextFaceEdge(prevFaceEdge(opp_edge), nextFaceEdge(edge));
						topology()->setPrevFaceEdge(nextFaceEdge(edge), prevFaceEdge(opp_edge));
						topology()->setNextFaceEdge(edge, opp_edge);
						topology()->setNextFaceEdge(opp_edge, edge);
						topology()->setPrevFaceEdge(edge, opp_edge);
						topology()->setPrevFaceEdge(opp_edge, edge);
						topology()->setAdjacentFace(opp_edge, face);
						OVITO_ASSERT(adjacentFace(edge) == face);
						OVITO_ASSERT(topology()->countFaceEdges(face) == 2);
						faceNormals[face] = faceNormals[faceCount() - 1];
						deleteFace(face);
						nextFace = face;
						break;
					}
				}
			}
			edge = nextFaceEdge(edge);
		}
		while(edge != faceEdge);
		face = nextFace;
	}
}

/******************************************************************************
* Splits a face along the edge given by two vertices of the face.
******************************************************************************/
SurfaceMeshData::edge_index SurfaceMeshData::splitFace(edge_index edge1, edge_index edge2)
{
	OVITO_ASSERT(adjacentFace(edge1) == adjacentFace(edge2));
	OVITO_ASSERT(nextFaceEdge(edge1) != edge2);
	OVITO_ASSERT(prevFaceEdge(edge1) != edge2);
	OVITO_ASSERT(!hasOppositeFace(adjacentFace(edge1)));

	face_index old_f = adjacentFace(edge1);
	face_index new_f = createFace({}, hasFaceRegions() ? faceRegion(old_f) : 1);

	vertex_index v1 = vertex2(edge1);
	vertex_index v2 = vertex2(edge2);
	edge_index edge1_successor = nextFaceEdge(edge1);
	edge_index edge2_successor = nextFaceEdge(edge2);

	// Create the new pair of half-edges.
	edge_index new_e = topology()->createEdge(v1, v2, old_f, edge1);
	edge_index new_oe = createOppositeEdge(new_e, new_f);

	// Rewire edge sequence of the primary face.
	OVITO_ASSERT(prevFaceEdge(new_e) == edge1);
	OVITO_ASSERT(nextFaceEdge(edge1) == new_e);
	topology()->setNextFaceEdge(new_e, edge2_successor);
	topology()->setPrevFaceEdge(edge2_successor, new_e);

	// Rewire edge sequence of the secondary face.
	topology()->setNextFaceEdge(edge2, new_oe);
	topology()->setPrevFaceEdge(new_oe, edge2);
	topology()->setNextFaceEdge(new_oe, edge1_successor);
	topology()->setPrevFaceEdge(edge1_successor, new_oe);

	// Connect the edges with the newly created secondary face.
	edge_index e = edge1_successor;
	do {
		topology()->setAdjacentFace(e, new_f);
		e = nextFaceEdge(e);
	}
	while(e != new_oe);
	OVITO_ASSERT(adjacentFace(edge2) == new_f);
	OVITO_ASSERT(adjacentFace(new_oe) == new_f);

	// Make the newly created edge the leading edge of the original face.
	topology()->setFirstFaceEdge(old_f, new_e);

	return new_e;
}


}	// End of namespace
}	// End of namespace
