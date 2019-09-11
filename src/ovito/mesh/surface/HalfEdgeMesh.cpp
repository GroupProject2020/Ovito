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
#include <ovito/core/utilities/mesh/TriMesh.h>
#include "HalfEdgeMesh.h"

namespace Ovito { namespace Mesh {

constexpr HalfEdgeMesh::size_type HalfEdgeMesh::InvalidIndex;

/******************************************************************************
* Removes all faces, edges and vertices from this mesh.
******************************************************************************/
void HalfEdgeMesh::clear()
{
	_vertexEdges.clear();
	_faceEdges.clear();
	_oppositeFaces.clear();
	_edgeFaces.clear();
	_edgeVertices.clear();
	_nextVertexEdges.clear();
	_nextFaceEdges.clear();
	_prevFaceEdges.clear();
	_oppositeEdges.clear();
	_nextManifoldEdges.clear();
}

/******************************************************************************
* Adds a new vertex to the mesh.
* Returns the index of the newly created vertex.
******************************************************************************/
HalfEdgeMesh::vertex_index HalfEdgeMesh::createVertex()
{
	vertex_index newIndex = vertexCount();
	_vertexEdges.push_back(InvalidIndex);
	return newIndex;
}

/******************************************************************************
* Adds several new vertices to the mesh.
******************************************************************************/
void HalfEdgeMesh::createVertices(size_type n)
{
	OVITO_ASSERT(n >= 0);
	_vertexEdges.resize(_vertexEdges.size() + n, InvalidIndex);
}

/******************************************************************************
* Internal method that creates a new face without edges.
* Returns the index of the new face.
******************************************************************************/
HalfEdgeMesh::face_index HalfEdgeMesh::createFace()
{
	face_index newIndex = faceCount();
	_faceEdges.push_back(InvalidIndex);
	_oppositeFaces.push_back(InvalidIndex);
	return newIndex;
}

/******************************************************************************
* Creates a new half-edge between two vertices and adjacent to the given face.
* Returns the index of the new half-edge.
******************************************************************************/
HalfEdgeMesh::edge_index HalfEdgeMesh::createEdge(vertex_index vertex1, vertex_index vertex2, face_index face)
{
	OVITO_ASSERT(vertex1 >= 0 && vertex1 < vertexCount());
	OVITO_ASSERT(vertex2 >= 0 && vertex2 < vertexCount());
	OVITO_ASSERT(face >= 0 && face < faceCount());
	edge_index newIndex = edgeCount();

	// Connect the half-edge to the face.
	_edgeFaces.push_back(face);

	// Connect the half-edge to the second vertex.
	_edgeVertices.push_back(vertex2);

	// Insert the half-edge into the linked-list of edges of the first vertex.
	_nextVertexEdges.push_back(_vertexEdges[vertex1]);
	_vertexEdges[vertex1] = newIndex;

	// Insert the half-edge into the linked-list of edges of the face.
	edge_index& faceEdge = _faceEdges[face];
	if(faceEdge != InvalidIndex) {
		_nextFaceEdges.push_back(faceEdge);
		_prevFaceEdges.push_back(prevFaceEdge(faceEdge));
		setNextFaceEdge(prevFaceEdge(faceEdge), newIndex);
		setPrevFaceEdge(faceEdge, newIndex);
	}
	else {
		_nextFaceEdges.push_back(newIndex);
		_prevFaceEdges.push_back(newIndex);
		faceEdge = newIndex;
	}

	// Initialize opposite edge field.
	_oppositeEdges.push_back(InvalidIndex);

	// Initialize next-manifold field.
	_nextManifoldEdges.push_back(InvalidIndex);

	return newIndex;
}

/******************************************************************************
* Tries to wire each half-edge with its opposite (reverse) half-edge.
* Returns true if every half-edge has an opposite half-edge, i.e. if the mesh
* is closed after this method returns.
******************************************************************************/
bool HalfEdgeMesh::connectOppositeHalfedges()
{
	bool isClosed = true;
	auto v2 = _edgeVertices.cbegin();
	auto prevFaceEdge = _prevFaceEdges.cbegin();
	edge_index edgeIndex = 0;
	for(edge_index& oppositeEdge : _oppositeEdges) {
		if(oppositeEdge == InvalidIndex) {
			// Search in the edge list of the second vertex for a half-edge that leads back to the first vertex.
			vertex_index vertex1 = vertex2(*prevFaceEdge);
			for(edge_index currentEdge = firstVertexEdge(*v2); currentEdge != InvalidIndex; currentEdge = nextVertexEdge(currentEdge)) {
				if(vertex2(currentEdge) == vertex1 && !hasOppositeEdge(currentEdge)) {
					// Link the two half-edges together.
					oppositeEdge = currentEdge;
					_oppositeEdges[currentEdge] = edgeIndex;
					break;
				}
			}
			if(oppositeEdge == InvalidIndex)
				isClosed = false;
		}
		else {
			OVITO_ASSERT(_oppositeEdges[oppositeEdge] == edgeIndex);
		}
		++v2;
		++prevFaceEdge;
		++edgeIndex;
	}
	return isClosed;
}

/******************************************************************************
* Links each half-edge leaving from the given vertex to an opposite (reverse)
* half-edge leading back to the vertex.
******************************************************************************/
void HalfEdgeMesh::connectOppositeHalfedges(vertex_index vert)
{
	for(edge_index edge = firstVertexEdge(vert); edge != InvalidIndex; edge = _nextVertexEdges[edge]) {
		if(hasOppositeEdge(edge)) continue;
		for(edge_index oppositeEdge = firstVertexEdge(vertex2(edge)); oppositeEdge != InvalidIndex; oppositeEdge = _nextVertexEdges[oppositeEdge]) {
			if(vertex2(oppositeEdge) == vert) {
				if(hasOppositeEdge(oppositeEdge)) continue;
				linkOppositeEdges(edge, oppositeEdge);
				break;
			}
		}
		OVITO_ASSERT(hasOppositeEdge(edge));
	}
}

/******************************************************************************
* Duplicates those vertices which are shared by more than one manifold.
* The method may only be called on a closed mesh.
* Returns the number of vertices that were duplicated by the method.
******************************************************************************/
HalfEdgeMesh::size_type HalfEdgeMesh::makeManifold(const std::function<void(vertex_index)>& vertexDuplicationFunc)
{
	size_type numSharedVertices = 0;
	size_type oldVertexCount = vertexCount();
	std::vector<edge_index> visitedEdges;
	for(vertex_index vertex = 0; vertex < oldVertexCount; vertex++) {
		// Count the number of half-edge connected to the current vertex.
		size_type numEdges = vertexEdgeCount(vertex);
		OVITO_ASSERT(numEdges >= 2);

		// Go in positive direction around vertex, facet by facet.
		edge_index firstEdge = firstVertexEdge(vertex);
		edge_index currentEdge = firstEdge;
		size_type numManifoldEdges = 0;
		do {
			OVITO_ASSERT(currentEdge != InvalidIndex); // Mesh must be closed.
			OVITO_ASSERT(_edgeFaces[currentEdge] != InvalidIndex); // Every edge must be connected to a face.
			OVITO_ASSERT(prevFaceEdge(currentEdge) != InvalidIndex); // Every edge must be preceding edge along the face.
			currentEdge = oppositeEdge(prevFaceEdge(currentEdge));
			numManifoldEdges++;
		}
		while(currentEdge != firstEdge);

		if(numManifoldEdges == numEdges)
			continue;		// Vertex is not part of multiple manifolds.

		visitedEdges.clear();
		currentEdge = firstEdge;
		do {
			visitedEdges.push_back(currentEdge);
			currentEdge = oppositeEdge(prevFaceEdge(currentEdge));
		}
		while(currentEdge != firstEdge);
		OVITO_ASSERT(visitedEdges.size() == numManifoldEdges);

		do {
			// Create a second vertex that takes the edges not visited yet.
			vertex_index newVertex = createVertex();

			for(firstEdge = firstVertexEdge(vertex); firstEdge != InvalidIndex; firstEdge = nextVertexEdge(firstEdge)) {
				if(std::find(visitedEdges.cbegin(), visitedEdges.cend(), firstEdge) == visitedEdges.end())
					break;
			}
			OVITO_ASSERT(firstEdge != InvalidIndex);

			currentEdge = firstEdge;
			do {
				OVITO_ASSERT(currentEdge != InvalidIndex); // Mesh must be closed.
				OVITO_ASSERT(_edgeFaces[currentEdge] != InvalidIndex); // Every edge must be connected to a face.
				OVITO_ASSERT(prevFaceEdge(currentEdge) != InvalidIndex); // Every edge must be preceding edge along the face.
				OVITO_ASSERT(std::find(visitedEdges.cbegin(), visitedEdges.cend(), currentEdge) == visitedEdges.end());
				visitedEdges.push_back(currentEdge);
				OVITO_ASSERT(firstVertexEdge(vertex) != currentEdge);
				transferEdgeToVertex(currentEdge, vertex, newVertex);
				currentEdge = _oppositeEdges[prevFaceEdge(currentEdge)];
			}
			while(currentEdge != firstEdge);

			// Copy the properties of the vertex to its duplicate.
			vertexDuplicationFunc(vertex);
		}
		while(visitedEdges.size() != numEdges);

		numSharedVertices++;
	}

	return numSharedVertices;
}

/******************************************************************************
* Disconnects a half-edge from a vertex and adds it to the list of half-edges
* of another vertex. Moves the opposite half-edge to the new vertex as well
* by default.
******************************************************************************/
void HalfEdgeMesh::transferEdgeToVertex(edge_index edge, vertex_index oldVertex, vertex_index newVertex, bool updateOppositeEdge)
{
	OVITO_ASSERT(edge >= 0 && edge < edgeCount());
	OVITO_ASSERT(oldVertex >= 0 && oldVertex < vertexCount());
	OVITO_ASSERT(newVertex >= 0 && newVertex < vertexCount());
	OVITO_ASSERT(newVertex != oldVertex);
	if(updateOppositeEdge) {
		OVITO_ASSERT(hasOppositeEdge(edge));
		OVITO_ASSERT(_edgeVertices[oppositeEdge(edge)] == oldVertex);
		_edgeVertices[oppositeEdge(edge)] = newVertex;
	}
	removeEdgeFromVertex(oldVertex, edge);
	addEdgeToVertex(newVertex, edge);
}

/******************************************************************************
* Removes a half-edge from a vertex' list of half-edges.
******************************************************************************/
void HalfEdgeMesh::removeEdgeFromVertex(vertex_index vertex, edge_index edge)
{
	OVITO_ASSERT(edge >= 0 && edge < edgeCount());
	OVITO_ASSERT(vertex >= 0 && vertex < vertexCount());
	edge_index& vertexEdge = _vertexEdges[vertex];
	if(vertexEdge == edge) {
		vertexEdge = _nextVertexEdges[edge];
		_nextVertexEdges[edge] = InvalidIndex;
	}
	else {
		for(edge_index precedingEdge = vertexEdge; precedingEdge != InvalidIndex; precedingEdge = _nextVertexEdges[precedingEdge]) {
			OVITO_ASSERT(precedingEdge != edge);
			if(_nextVertexEdges[precedingEdge] == edge) {
				_nextVertexEdges[precedingEdge] = _nextVertexEdges[edge];
				_nextVertexEdges[edge] = InvalidIndex;
				return;
			}
		}
		OVITO_ASSERT(false); // Half-edge to be removed was not found in the vertex' list of half-edges.
	}
}

/******************************************************************************
* Determines whether the mesh represents a closed two-dimensional manifold,
* i.e., every half-edge is linked to an opposite half-edge.
******************************************************************************/
bool HalfEdgeMesh::isClosed() const
{
	return std::find(_oppositeEdges.cbegin(), _oppositeEdges.cend(), InvalidIndex) == _oppositeEdges.cend();
}

/******************************************************************************
* Flips the orientation of all faces in the mesh.
******************************************************************************/
void HalfEdgeMesh::flipFaces()
{
	for(edge_index firstFaceEdge : _faceEdges) {
		if(firstFaceEdge == InvalidIndex) continue;
		edge_index e = firstFaceEdge;
		do {
			transferEdgeToVertex(e, vertex1(e), vertex2(e), false);
			e = nextFaceEdge(e);
		}
		while(e != firstFaceEdge);
		vertex_index v1 = vertex1(e);
		do {
			std::swap(_edgeVertices[e], v1);
			std::swap(_nextFaceEdges[e], _prevFaceEdges[e]);
			e = prevFaceEdge(e);
		}
		while(e != firstFaceEdge);
	}
}

/******************************************************************************
* Converts the half-edge mesh to a triangle mesh.
* Note that the HalfEdgeMesh structure holds only the mesh topology and no
* vertex coordinates. Thus, it is the respondisbility of the caller to assign
* coordinates to the vertices of the generated TriMesh.
******************************************************************************/
void HalfEdgeMesh::convertToTriMesh(TriMesh& output) const
{
	// Create output vertices.
	output.setVertexCount(vertexCount());

	// Count number of output triangles to be generated.
	size_type triangleCount = 0;
	for(edge_index faceEdge : _faceEdges) {
		triangleCount += std::max(faceEdgeCount(faceEdge) - 2, 0);
	}

	// Transfer faces.
	output.setFaceCount(triangleCount);
	auto fout = output.faces().begin();
	for(edge_index faceEdge : _faceEdges) {
		vertex_index baseVertex = _edgeVertices[faceEdge];
		edge_index edge1 = _nextFaceEdges[faceEdge];
		edge_index edge2 = _nextFaceEdges[edge1];
		while(edge2 != faceEdge) {
			fout->setVertices(baseVertex, _edgeVertices[edge1], _edgeVertices[edge2]);
			++fout;
			edge1 = edge2;
			edge2 = _nextFaceEdges[edge2];
		}
	}
	OVITO_ASSERT(fout == output.faces().end());

	output.invalidateVertices();
	output.invalidateFaces();
}

/******************************************************************************
* Deletes a face from the mesh. A hole in the mesh will be left behind.
* The half-edges of the face are also disconnected from their respective
* opposite half-edges and deleted by this method.
******************************************************************************/
void HalfEdgeMesh::deleteFace(face_index face)
{
	OVITO_ASSERT(!hasOppositeFace(face));

	edge_index ffe = firstFaceEdge(face);
	if(ffe != InvalidIndex) {
		// Disconnect face edges from their source vertices.
		// and from their opposite edges.
		edge_index e = ffe;
		do {
			OVITO_ASSERT(prevFaceEdge(nextFaceEdge(e)) == e);
			removeEdgeFromVertex(vertex1(e), e);
			if(hasOppositeEdge(e) && oppositeEdge(e) != e) {
				_oppositeEdges[oppositeEdge(e)] = InvalidIndex;
				_oppositeEdges[e] = InvalidIndex;
			}
			e = nextFaceEdge(e);
		}
		while(e != ffe);
		// Break circular edge list.
		_nextFaceEdges[_prevFaceEdges[e]] = InvalidIndex;
		// Now delete the half-edges of the face.
		do {
			e = deleteEdge(e);
		}
		while(e != InvalidIndex);
	}

	// There shouldn't be any edges left in the mesh referring to the face being deleted.
	// Note: The following line has been commented out for performance reasons.
	// OVITO_ASSERT(std::find(_edgeFaces.begin(), _edgeFaces.end(), face) == _edgeFaces.end());

	if(face < faceCount() - 1) {
		// Move the last face to the index of the face being deleted.
		_faceEdges[face] = _faceEdges.back();
		// Update all references to the last face to point to its new list index.
		edge_index estart = _faceEdges.back();
		edge_index e = estart;
		do {
			OVITO_ASSERT(_edgeFaces[e] == faceCount() - 1);
			_edgeFaces[e] = face;
			e = nextFaceEdge(e);
		}
		while(e != estart);
		// Update back-reference from opposite face.
		_oppositeFaces[face] = _oppositeFaces.back();
		face_index of = _oppositeFaces.back();
		if(of != InvalidIndex) {
			OVITO_ASSERT(_oppositeFaces[of] == faceCount() - 1);
			_oppositeFaces[of] = face;
		}
	}
	_faceEdges.pop_back();
	_oppositeFaces.pop_back();
}

/******************************************************************************
* Deletes a half-edge from the mesh.
* This method assumes that the half-edge is not connected to any part of the mesh.
* Returns the successor edge along the face's boundary.
******************************************************************************/
HalfEdgeMesh::edge_index HalfEdgeMesh::deleteEdge(edge_index edge)
{
	// Make sure the edge is no longer connected to other parts of the mesh.
	OVITO_ASSERT(!hasOppositeEdge(edge));
	OVITO_ASSERT(_nextVertexEdges[edge] == InvalidIndex);
	OVITO_ASSERT(_nextManifoldEdges[edge] == InvalidIndex);

	edge_index successorEdge = nextFaceEdge(edge);
	if(successorEdge == edge) successorEdge = InvalidIndex;
	if(edge < edgeCount() - 1) {
		// Move the last half-edge to the index of the half-edge being deleted.
		_edgeFaces[edge] = _edgeFaces.back();
		_edgeVertices[edge] = _edgeVertices.back();
		_nextVertexEdges[edge] = _nextVertexEdges.back();
		_nextFaceEdges[edge] = _nextFaceEdges.back();
		_prevFaceEdges[edge] = _prevFaceEdges.back();
		_oppositeEdges[edge] = _oppositeEdges.back();
		_nextManifoldEdges[edge] = _nextManifoldEdges.back();
		// Update all references to the last half-edge to point to its new list index.
		edge_index movedEdge = edgeCount() - 1;
		// Update the opposite edge.
		edge_index oe = oppositeEdge(movedEdge);
		if(oe != InvalidIndex) {
			_oppositeEdges[oe] = edge;

			// Update the manifold link to the edge.
			edge_index nme = nextManifoldEdge(oe);
			if(nme != InvalidIndex) {
				OVITO_ASSERT(vertex1(movedEdge) == vertex2(nme));
				OVITO_ASSERT(vertex2(movedEdge) == vertex1(nme));
				OVITO_ASSERT(hasOppositeEdge(nme));
				edge_index nme_oe = oppositeEdge(nme);
				OVITO_ASSERT(vertex1(movedEdge) == vertex1(nme_oe));
				OVITO_ASSERT(vertex2(movedEdge) == vertex2(nme_oe));
				OVITO_ASSERT(nextManifoldEdge(nme_oe) == movedEdge);
				_nextManifoldEdges[nme_oe] = edge;
			}
		}
		// Update the vertex edge list.
		vertex_index v = vertex1(movedEdge);
		if(firstVertexEdge(v) == movedEdge) {
			_vertexEdges[v] = edge;
		}
		else {
			for(edge_index e = firstVertexEdge(v); e != InvalidIndex; e = nextVertexEdge(e)) {
				if(nextVertexEdge(e) == movedEdge) {
					_nextVertexEdges[e] = edge;
					break;
				}
			}
		}
		// Update the face.
		face_index face = adjacentFace(movedEdge);
		if(face != InvalidIndex && _faceEdges[face] == movedEdge)
			_faceEdges[face] = edge;
		// Update next and prev pointers.
		edge_index nextEdge = nextFaceEdge(movedEdge);
		OVITO_ASSERT(nextEdge != movedEdge);
		if(nextEdge != InvalidIndex) {
			OVITO_ASSERT(_prevFaceEdges[nextEdge] == movedEdge);
			_prevFaceEdges[nextEdge] = edge;
		}
		edge_index prevEdge = prevFaceEdge(movedEdge);
		OVITO_ASSERT(prevEdge != movedEdge);
		if(prevEdge != InvalidIndex) {
			OVITO_ASSERT(_nextFaceEdges[prevEdge] == movedEdge);
			_nextFaceEdges[prevEdge] = edge;
		}

		if(successorEdge == movedEdge)
			successorEdge = edge;
	}
	_edgeFaces.pop_back();
	_edgeVertices.pop_back();
	_nextVertexEdges.pop_back();
	_nextFaceEdges.pop_back();
	_prevFaceEdges.pop_back();
	_oppositeEdges.pop_back();
	_nextManifoldEdges.pop_back();
	return successorEdge;
}

/******************************************************************************
* Deletes a vertex from the mesh.
* This method assumes that the vertex is not connected to any part of the mesh.
******************************************************************************/
void HalfEdgeMesh::deleteVertex(vertex_index vertex)
{
	OVITO_ASSERT(firstVertexEdge(vertex) == InvalidIndex);
	if(vertex < vertexCount() - 1) {
		// Move the last vertex to the index of the vertex being deleted.
		_vertexEdges[vertex] = _vertexEdges.back();
		// Update all references to the last vertex to point to its new list index.
		vertex_index movedVertex = vertexCount() - 1;
		// Update the vertex pointers of adjacent edges.
		for(edge_index e = firstVertexEdge(movedVertex); e != InvalidIndex; e = nextVertexEdge(e)) {
			OVITO_ASSERT(vertex2(e) != movedVertex);
			edge_index pe = prevFaceEdge(e);
			OVITO_ASSERT(vertex2(pe) == movedVertex);
			_edgeVertices[pe] = vertex;
		}
	}
	_vertexEdges.pop_back();
}

}	// End of namespace
}	// End of namespace
