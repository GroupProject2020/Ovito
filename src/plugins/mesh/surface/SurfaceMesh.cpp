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
#include <core/utilities/concurrent/ParallelFor.h>
#include <plugins/stdobj/simcell/SimulationCellObject.h>
#include "SurfaceMesh.h"

namespace Ovito { namespace Mesh {

IMPLEMENT_OVITO_CLASS(SurfaceMesh);
DEFINE_PROPERTY_FIELD(SurfaceMesh, storage);
DEFINE_PROPERTY_FIELD(SurfaceMesh, isCompletelySolid);

/******************************************************************************
* Constructs an empty surface mesh object.
******************************************************************************/
SurfaceMesh::SurfaceMesh(DataSet* dataset) : PeriodicDomainDataObject(dataset),
		_isCompletelySolid(false)
{
}

/******************************************************************************
* Returns the data encapsulated by this object after making sure it is not 
* shared with other owners.
******************************************************************************/
const SurfaceMeshPtr& SurfaceMesh::modifiableStorage() 
{
	// Copy data storage on write if there is more than one reference to the storage.
	OVITO_ASSERT(storage());
	OVITO_ASSERT(storage().use_count() >= 1);
	if(storage().use_count() > 1)
		_storage.mutableValue() = std::make_shared<HalfEdgeMesh<>>(*storage());
	OVITO_ASSERT(storage().use_count() == 1);
	return storage();
}

/******************************************************************************
* Fairs a closed triangle mesh.
******************************************************************************/
bool SurfaceMesh::smoothMesh(HalfEdgeMesh<>& mesh, const SimulationCell& cell, int numIterations, PromiseState& promise, FloatType k_PB, FloatType lambda)
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
		smoothMeshIteration(mesh, lambda, cell);
		smoothMeshIteration(mesh, mu, cell);
	}

	return !promise.isCanceled();
}

/******************************************************************************
* Performs one iteration of the smoothing algorithm.
******************************************************************************/
void SurfaceMesh::smoothMeshIteration(HalfEdgeMesh<>& mesh, FloatType prefactor, const SimulationCell& cell)
{
	const AffineTransformation absoluteToReduced = cell.inverseMatrix();
	const AffineTransformation reducedToAbsolute = cell.matrix();

	// Compute displacement for each vertex.
	std::vector<Vector3> displacements(mesh.vertexCount());
	parallelFor(mesh.vertexCount(), [&mesh, &displacements, prefactor, cell, absoluteToReduced](int index) {
		HalfEdgeMesh<>::Vertex* vertex = mesh.vertex(index);
		Vector3 d = Vector3::Zero();

		// Go in positive direction around vertex, facet by facet.
		HalfEdgeMesh<>::Edge* currentEdge = vertex->edges();
		if(currentEdge != nullptr) {
			int numManifoldEdges = 0;
			do {
				OVITO_ASSERT(currentEdge != nullptr && currentEdge->face() != nullptr);
				d += cell.wrapVector(currentEdge->vertex2()->pos() - vertex->pos());
				numManifoldEdges++;
				currentEdge = currentEdge->prevFaceEdge()->oppositeEdge();
			}
			while(currentEdge != vertex->edges());
			d *= (prefactor / numManifoldEdges);
		}

		displacements[index] = d;
	});

	// Apply computed displacements.
	auto d = displacements.cbegin();
	for(HalfEdgeMesh<>::Vertex* vertex : mesh.vertices())
		vertex->pos() += *d++;
}


/******************************************************************************
* Determines if a spatial location is inside or oustide of the region enclosed 
* by the surface.
*
* Return value:
*     -1 : The point is inside the enclosed region
*      0 : The point is right on the surface (approximately, within the given epsilon)
*     +1 : The point is outside the enclosed region
******************************************************************************/
int SurfaceMesh::locatePoint(const Point3& location, FloatType epsilon) const
{
	return locatePointStatic(location, *storage(), 
				domain() ? domain()->data() : SimulationCell(), 
				isCompletelySolid(), epsilon);
}	

/******************************************************************************
* Implementation of the locatePoint() method.
*
* Return value:
*     -1 : The point is inside the enclosed region
*      0 : The point is right on the surface (within the precision given by epsilon)
*     +1 : The point is outside the enclosed region
*
* Algorithm:
*
* J. Andreas Baerentzen and Henrik Aanaes:
* Signed Distance Computation Using the Angle Weighted Pseudonormal
* IEEE Transactions on Visualization and Computer Graphics 11 (2005), Page 243
******************************************************************************/
int SurfaceMesh::locatePointStatic(const Point3& location, const HalfEdgeMesh<>& mesh, const SimulationCell cell, bool isCompletelySolid, FloatType epsilon)
{
	// Determine which vertex is closest to the test point.
	FloatType closestDistanceSq = FLOATTYPE_MAX;
	HalfEdgeMesh<>::Vertex* closestVertex = nullptr;
	Vector3 closestNormal, closestVector;
	for(HalfEdgeMesh<>::Vertex* v : mesh.vertices()) {
		if(v->edges() == nullptr) continue;
		Vector3 r = cell.wrapVector(v->pos() - location);
		FloatType distSq = r.squaredLength();
		if(distSq < closestDistanceSq) {
			closestDistanceSq = distSq;
			closestVertex = v;
			closestVector = r;
		}
	}

	// If the surface is degenerate, all points are either inside or outside.
	if(!closestVertex)
		return isCompletelySolid ? -1 : +1;

	// Check if any edge is closer to the test point than the closest vertex.
	for(HalfEdgeMesh<>::Vertex* v : mesh.vertices()) {
		for(HalfEdgeMesh<>::Edge* edge = v->edges(); edge != nullptr; edge = edge->nextVertexEdge()) {
			OVITO_ASSERT_MSG(edge->oppositeEdge() != nullptr, "SurfaceMesh::locatePoint", "Surface mesh is not fully closed. This should not happen.");
			const Point3& p1 = edge->vertex1()->pos();
			const Point3& p2 = edge->vertex2()->pos();
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
				closestVertex = nullptr;
				closestVector = c;
				Vector3 e1 = cell.wrapVector(edge->nextFaceEdge()->vertex2()->pos() - p1);
				Vector3 e2 = cell.wrapVector(edge->oppositeEdge()->nextFaceEdge()->vertex2()->pos() - p1);
				closestNormal = edgeDir.cross(e1).safelyNormalized() + e2.cross(edgeDir).safelyNormalized();
			}
		}
	}

	// Check if any facet is closer to the test point than the closest vertex and the closest edge.
	HalfEdgeMesh<>::Face* closestFace = nullptr;
	for(HalfEdgeMesh<>::Face* face : mesh.faces()) {
		HalfEdgeMesh<>::Edge* edge1 = face->edges();
		HalfEdgeMesh<>::Edge* edge2 = edge1->nextFaceEdge();
		const Point3& p1 = edge1->vertex1()->pos();
		const Point3& p2 = edge1->vertex2()->pos();
		const Point3& p3 = edge2->vertex2()->pos();
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
				closestVertex = nullptr;
				closestNormal = normal;
			}
		}
	}

	// If a vertex is closest, we still have to compute the local pseudo-normal at the vertex.
	if(closestVertex) {
		HalfEdgeMesh<>::Edge* edge = closestVertex->edges();
		OVITO_ASSERT(edge);
		closestNormal.setZero();
		Vector3 edge1v = cell.wrapVector(edge->vertex2()->pos() - closestVertex->pos());
		edge1v.normalizeSafely();
		do {
			HalfEdgeMesh<>::Edge* nextEdge = edge->oppositeEdge()->nextFaceEdge();
			OVITO_ASSERT(nextEdge->vertex1() == closestVertex);
			Vector3 edge2v = cell.wrapVector(nextEdge->vertex2()->pos() - closestVertex->pos());
			edge2v.normalizeSafely();
			FloatType angle = std::acos(edge1v.dot(edge2v));
			Vector3 normal = edge2v.cross(edge1v);
			if(normal != Vector3::Zero())
				closestNormal += normal.normalized() * angle;
			edge = nextEdge;
			edge1v = edge2v;
		}
		while(edge != closestVertex->edges());
	}

	FloatType dot = closestNormal.dot(closestVector);
	if(dot >= epsilon) return -1;
	if(dot <= -epsilon) return +1;
	return 0;
}

}	// End of namespace
}	// End of namespace
