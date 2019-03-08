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
#include <plugins/stdobj/simcell/SimulationCellObject.h>
#include <core/utilities/concurrent/ParallelFor.h>
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
    createFaceProperty(SurfaceMeshFaces::RegionProperty);
    OVITO_ASSERT(_vertexCoords != nullptr);
    OVITO_ASSERT(_faceRegions != nullptr);
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
	for(const PropertyObject* property : sm->vertices()->properties()) {
	    addVertexProperty(const_pointer_cast<PropertyStorage>(property->storage()));
	}
	for(const PropertyObject* property : sm->faces()->properties()) {
	    addFaceProperty(const_pointer_cast<PropertyStorage>(property->storage()));
	}
	for(const PropertyObject* property : sm->regions()->properties()) {
	    addRegionProperty(const_pointer_cast<PropertyStorage>(property->storage()));
	}
    OVITO_ASSERT(_vertexCoords != nullptr);
    OVITO_ASSERT(_faceRegions != nullptr);
}

/******************************************************************************
* Copies the data stored in this structure to the given pipeline data object.
******************************************************************************/
void SurfaceMeshData::transferTo(SurfaceMesh* sm) const
{
	sm->setTopology(topology());
    sm->setSpaceFillingRegion(spaceFillingRegion());

	// Helper function that transfers a list of storage objects into the given property container.
	// Existing properties which are no longer used will be removed from the container.
	auto transferProperties = [](PropertyContainer* container, const std::vector<PropertyPtr>& properties) {
		// Insertion phase:
		for(const auto& property : properties) {
			OORef<PropertyObject> propertyObj = (property->type() != 0) ? container->getProperty(property->type()) : container->getProperty(property->name());
			if(propertyObj)
				propertyObj->setStorage(property);
			else
				container->createProperty(property);
		}
		// Removal phase:
		for(int i = container->properties().size() - 1; i >= 0; i--) {
			PropertyObject* property = container->properties()[i];
			if(std::find(properties.cbegin(), properties.cend(), property->storage()) == properties.cend())
				container->removeProperty(property);
		}
	};

	transferProperties(sm->makeVerticesMutable(), _vertexProperties);
	transferProperties(sm->makeFacesMutable(), _faceProperties);
	transferProperties(sm->makeRegionsMutable(), _regionProperties);

    OVITO_ASSERT(sm->vertices()->properties().size() == _vertexProperties.size());
    OVITO_ASSERT(sm->faces()->properties().size() == _faceProperties.size());
    OVITO_ASSERT(sm->regions()->properties().size() == _regionProperties.size());
}

/******************************************************************************
* Fairs a closed triangle mesh.
******************************************************************************/
bool SurfaceMeshData::smoothMesh(int numIterations, PromiseState& promise, FloatType k_PB, FloatType lambda)
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
	promise.setProgressMaximum(numIterations);

	for(int iteration = 0; iteration < numIterations; iteration++) {
		if(!promise.setProgressValue(iteration))
			return false;
		smoothMeshIteration(lambda);
		smoothMeshIteration(mu);
	}

	return !promise.isCanceled();
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
	}

	FloatType dot = closestNormal.dot(closestVector);
	if(dot >= epsilon) return closestRegion;
	if(dot <= -epsilon) return 0;
	return -1;
}

}	// End of namespace
}	// End of namespace
