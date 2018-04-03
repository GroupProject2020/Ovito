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

#pragma once


#include <core/Core.h>
#include <core/utilities/MemoryPool.h>
#include "TriMesh.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(Mesh)

// An empty structure that is used as default base class for edges, faces, and vertices in the HaldEdgeMesh class template.
template<typename>
struct EmptyHalfEdgeMeshStruct {};

/**
 * Stores a closed polygonal mesh as a half-edge data structure.
 *
 * Each half-edge is adjacent to one face.
 * Each half-edge has a pointer to the next half-edge adjacent to the same face.
 * Each half-edge has a pointer to its opposite half-edge.
 * Each half-edge has a pointer to the vertex it points to.
 * Each half-edge has a pointer to the next edge in the linked list of edges originating from the same vertex.
 * Each vertex has a pointer to the first edge originating from it.
 * Each face has a pointer to one of the edges adjacent to it.
 */
template<template<typename> class EdgeBase, template<typename> class FaceBase, template<typename> class VertexBase>
class OVITO_CORE_EXPORT HalfEdgeMesh
{
public:

	class Vertex;
	class Face;

	/// A single half-edge.
	class Edge : public EdgeBase<Edge>
	{
	public:

		/// Returns the vertex this half-edge is coming from.
		Vertex* vertex1() const { return prevFaceEdge()->_vertex2; }

		/// Returns the vertex this half-edge is pointing to.
		Vertex* vertex2() const { return _vertex2; }

		/// Changes the vertex this half-edge is pointing to. Use this method with care!
		void setVertex2(Vertex* v) { _vertex2 = v; }
		
		/// Returns a pointer to the face that is adjacent to this half-edge.
		Face* face() const { return _face; }

		/// Sets the edge's pointer to the face it belongs to. 
		/// This is an internal implementation method, which must be used with great care.
		void setFace(Face* face) { _face = face; }

		/// Returns the next half-edge in the linked-list of half-edges that
		/// leave the same vertex as this edge.
		Edge* nextVertexEdge() const { return _nextVertexEdge; }

		/// Returns the next half-edge in the linked-list of half-edges adjacent to the
		/// same face as this edge.
		Edge* nextFaceEdge() const { return _nextFaceEdge; }

		/// Sets the pointer to the edge following this edge along the face boundary.
		/// This is an internal implementation method, which must be used with great care.
		void setNextFaceEdge(Edge* edge) { _nextFaceEdge = edge; }

		/// Returns the previous half-edge in the linked-list of half-edges adjacent to the
		/// same face as this edge.
		Edge* prevFaceEdge() const { return _prevFaceEdge; }

		/// Sets the pointer to the edge preceding this edge along the face boundary.
		/// This is an internal implementation method, which must be used with great care.
		void setPrevFaceEdge(Edge* edge) { _prevFaceEdge = edge; }

		/// Returns a pointer to this edge's opposite half-edge.
		Edge* oppositeEdge() const { return _oppositeEdge; }

		/// Sets the pointer to this edge's opposite half-edge. Use with care!
		void setOppositeEdge(Edge* oe) { _oppositeEdge = oe; }
		
		/// Links two opposite half-edges.
		void linkToOppositeEdge(Edge* oe) {
			OVITO_ASSERT(oppositeEdge() == nullptr);
			OVITO_ASSERT(oe->oppositeEdge() == nullptr);
			OVITO_ASSERT(vertex1() == oe->vertex2());
			OVITO_ASSERT(vertex2() == oe->vertex1());
			setOppositeEdge(oe);
			oe->setOppositeEdge(this);
		}

		/// Unlinks this edge from its opposite edge.
		Edge* unlinkFromOppositeEdge() {
			OVITO_ASSERT(oppositeEdge() != nullptr);
			OVITO_ASSERT(oppositeEdge()->oppositeEdge() == this);
			Edge* oe = oppositeEdge();
			setOppositeEdge(nullptr);
			oe->setOppositeEdge(nullptr);
			return oe;
		}

	protected:

		/// Constructor.
		Edge(Vertex* vertex2, Face* face) : _vertex2(vertex2), _face(face) { 
			OVITO_ASSERT(vertex2 != nullptr);
			OVITO_ASSERT(face != nullptr);
		}

		/// The opposite half-edge.
		Edge* _oppositeEdge = nullptr;

		/// The vertex this half-edge is pointing to.
		Vertex* _vertex2;

		/// The face adjacent to this half-edge.
		Face* _face;

		/// The next half-edge in the linked-list of half-edges of the source vertex.
		Edge* _nextVertexEdge;

		/// The next half-edge in the linked-list of half-edges adjacent to the face.
		Edge* _nextFaceEdge;

		/// The previous half-edge in the linked-list of half-edges adjacent to the face.
		Edge* _prevFaceEdge;

		friend class HalfEdgeMesh;
	};

	/// A vertex of a mesh.
	class Vertex : public VertexBase<Vertex>
	{
	public:

		/// Returns the head of the vertex' linked-list of outgoing half-edges.
		Edge* edges() const { return _edges; }

		/// Returns the coordinates of the vertex.
		const Point3& pos() const { return _pos; }

		/// Returns the coordinates of the vertex.
		Point3& pos() { return _pos; }

		/// Sets the coordinates of the vertex.
		void setPos(const Point3& p) { _pos = p; }

		/// Returns the index of the vertex in the list of vertices of the mesh.
		int index() const { return _index; }

		/// Returns the number of faces (as well as half-edges) adjacent to this vertex.
		int numEdges() const { return _numEdges; }

		/// Returns the number of manifolds this vertex is part of.
		int numManifolds() const {
			int n = 0;
			std::vector<Edge*> visitedEdges;
			for(Edge* startEdge = edges(); startEdge != nullptr; startEdge = startEdge->nextVertexEdge()) {
				if(std::find(visitedEdges.cbegin(), visitedEdges.cend(), startEdge) != visitedEdges.cend()) continue;
				n++;
				Edge* currentEdge = startEdge;
				do {
					OVITO_ASSERT(currentEdge->vertex1() == this);
					OVITO_ASSERT(std::find(visitedEdges.cbegin(), visitedEdges.cend(), currentEdge) == visitedEdges.cend());
					visitedEdges.push_back(currentEdge);
					currentEdge = currentEdge->prevFaceEdge()->oppositeEdge();
				}
				while(currentEdge != startEdge);
			}
			return n;
		}

		/// Disconnects an edge from this vertex and adds it to the list of edges of another vertex.
		/// Also transfers the opposite edge to the new vertex by default.
		void transferEdgeToVertex(Edge* edge, Vertex* newVertex, bool updateOppositeEdge = true) {
			if(updateOppositeEdge) {
				OVITO_ASSERT(edge->oppositeEdge() != nullptr);
				OVITO_ASSERT(edge->oppositeEdge()->vertex2() == this);
				edge->oppositeEdge()->_vertex2 = newVertex;
			}
			OVITO_ASSERT(newVertex != this);
			OVITO_ASSERT(hasEdge(edge));
			this->removeEdge(edge);
			newVertex->addEdge(edge);
		}

		/// Determines whether the given edge originates from this vertex.
		bool hasEdge(const Edge* edge) const {
			for(Edge* e = edges(); e != nullptr; e = e->nextVertexEdge())
				if(e == edge) return true;
			return false;
		}

	protected:

		/// Constructor.
		Vertex(const Point3& pos, int index = -1) : _pos(pos), _index(index) {}

		/// Adds an adjacent half-edge to this vertex.
		void addEdge(Edge* edge) {
			edge->_nextVertexEdge = _edges;
			_edges = edge;
			_numEdges++;
		}

		// Removes a half-edge from this vertex' list of edges.
		void removeEdge(Edge* edge) {
			_numEdges--;
			if(edge == _edges) {
				_edges = edge->nextVertexEdge();
				edge->_nextVertexEdge = nullptr;
			}
			else {
				for(Edge* precedingEdge = edges(); precedingEdge != nullptr; precedingEdge = precedingEdge->nextVertexEdge()) {
					if(precedingEdge->nextVertexEdge() == edge) {
						precedingEdge->_nextVertexEdge = edge->_nextVertexEdge;
						edge->_nextVertexEdge = nullptr;
						return;
					}
				}
				OVITO_ASSERT(false);
			}
		}

		/// The coordinates of the vertex.
		Point3 _pos;

		/// The number of faces (as well as half-edges) adjacent to this vertex.
		int _numEdges = 0;

		/// The head of the linked-list of outgoing half-edges.
		Edge* _edges = nullptr;

		/// The index of the vertex in the list of vertices of the mesh.
		int _index;

		friend class HalfEdgeMesh;
	};

	/// A polygonal face of the mesh.
	class Face : public FaceBase<Face>
	{
	public:

		/// Returns a pointer to the head of the linked-list of half-edges that bound this face.
		Edge* edges() const { return _edges; }

		/// Sets the pointer to the head of the linked-list of half-edges that bound this face.
		/// Note: This is an internal implementation method, which must be used with great care.
		void setEdges(Edge* edge) { _edges = edge; }

		/// Returns the index of the face in the list of faces of the mesh.
		int index() const { return _index; }

		/// Returns the bit flags assigned to this face.
		unsigned int flags() const { return _flags; }

		/// Replaces all bit flags for this face with new values.
		void setFlags(unsigned int flags) { _flags = flags; }

		/// Tests if a flag is set for this face.
		bool testFlag(unsigned int flag) const { return (_flags & flag); }

		/// Sets a bit flag for this face.
		void setFlag(unsigned int flag) { _flags |= flag; }

		/// Clears a bit flag of this face.
		void clearFlag(unsigned int flag) { _flags &= ~flag; }

		/// Computes the number of edges (as well as vertices) of this face.
		int edgeCount() const {
			int c = 0;
			OVITO_CHECK_POINTER(edges());
			Edge* e = edges();
			do {
				c++;
				e = e->nextFaceEdge();
			}
			while(e != edges());
			return c;
		}

		/// Returns the edge of this face that connects the given vertices.
		Edge* findEdge(Vertex* v1, Vertex* v2) const {
			Edge* e = edges();
			do {
				if(e->vertex2() == v2 && e->vertex1() == v1) return e;
				e = e->nextFaceEdge();
			}
			while(e != edges());
			return nullptr;
		}

		/// Marks the face for deletion by resetting its edges pointer.
		void markForDeletion() { 
			OVITO_ASSERT(edges() != nullptr);
			setEdges(nullptr); 
		}

	protected:

		/// Constructor.
		Face(int index = -1) : _index(index) {}

		/// Head of the linked-list of half-edges that bound this face.
		Edge* _edges = nullptr;

		/// The index of the face in the list of faces of the mesh.
		int _index;

		/// Bit flags of this face.
		unsigned int _flags = 0;

		friend class HalfEdgeMesh;
	};


public:

	/// Default constructor.
	HalfEdgeMesh() = default;

	/// Copy constructor.
	HalfEdgeMesh(const HalfEdgeMesh& other) {
		copyFrom(other);
	}

	/// Removes all faces, edges, and vertices from this mesh.
	void clear() {
		_vertices.clear();
		_faces.clear();
		_vertexPool.clear();
		_edgePool.clear();
		_facePool.clear();
		_reclaimedFaces.clear();
		_reclaimedEdges.clear();
		_reclaimedVertices.clear();
	}

	/// Returns the list of vertices in the mesh.
	const std::vector<Vertex*>& vertices() const { return _vertices; }

	/// Returns the list of faces in the mesh.
	const std::vector<Face*>& faces() const { return _faces; }

	/// Returns the number of vertices in this mesh.
	int vertexCount() const { return (int)_vertices.size(); }

	/// Returns the number of faces in this mesh.
	int faceCount() const { return (int)_faces.size(); }

	/// Returns a pointer to the vertex with the given index.
	Vertex* vertex(int index) const {
		OVITO_ASSERT(index >= 0 && index < vertexCount());
		return _vertices[index];
	}

	/// Returns a pointer to the face with the given index.
	Face* face(int index) const {
		OVITO_ASSERT(index >= 0 && index < faceCount());
		return _faces[index];
	}

	/// Reserves memory for the given total number of vertices.
	void reserveVertices(int vertexCount) { _vertices.reserve(vertexCount); }

	/// Reserves memory for the given total number of faces.
	void reserveFaces(int faceCount) { _faces.reserve(faceCount); }

	/// Adds a new vertex to the mesh.
	Vertex* createVertex(const Point3& pos) {
		Vertex* vert;
		if(_reclaimedVertices.empty()) {
			vert = _vertexPool.construct(pos, vertexCount());
		}
		else {
			vert = _reclaimedVertices.back();
			_reclaimedVertices.pop_back();
			vert->setPos(pos);
		}
		_vertices.push_back(vert);
		return vert;
	}

	/// Creates a new face defined by the given vertices.
	/// Half-edges connecting the vertices are created by this method too.
	Face* createFace(std::initializer_list<Vertex*> vertices) {
		return createFace(vertices.begin(), vertices.end());
	}

	/// Creates a new face defined by the given range of vertices.
	/// Half-edges connecting the vertices are created by this method too.
	template<typename VertexPointerIterator>
	Face* createFace(VertexPointerIterator begin, VertexPointerIterator end) {
		OVITO_ASSERT(std::distance(begin, end) >= 2);
		Face* face = createFace();

		VertexPointerIterator v1, v2;
		for(v2 = begin, v1 = v2++; v2 != end; v1 = v2++)
			createEdge(*v1, *v2, face);
		createEdge(*v1, *begin, face);

		// First edge of face should start at first supplied vertex.
		OVITO_ASSERT(face->edges()->vertex1() == *begin);

		return face;
	}

	/// Creates a new face without edges. This is for internal use only.
	Face* createFace() {
		Face* face;
		if(_reclaimedFaces.empty()) {
			face = _facePool.construct(faceCount());
		}
		else {
			face = _reclaimedFaces.back();
			face->setEdges(nullptr);
			face->_flags = 0;
			_reclaimedFaces.pop_back();
		}
		_faces.push_back(face);
		return face;
	}

	/// Deletes a halfedge from the mesh.Merge 
	/// This method assumes that the edge is not connected to any part of the mesh.
	void removeEdge(Edge* edge) {
		OVITO_ASSERT(edge != nullptr);
		OVITO_ASSERT(edge->oppositeEdge() == nullptr);
		_reclaimedEdges.push_back(edge);
	}

	/// Deletes a face from the mesh.
	/// A hole in the mesh will be left behind.
	/// The halfedges of the face are also disconnected from their respective opposite halfedges 
	/// and reclaimed by this method.
	void removeFace(int faceIndex) {
		Face* face = this->face(faceIndex);
		if(Edge* e = face->edges()) {
			do {
				OVITO_ASSERT(e->vertex1());
				e->vertex1()->removeEdge(e);
				if(e->oppositeEdge() != nullptr)
					e->unlinkFromOppositeEdge();
				removeEdge(e);
				e = e->nextFaceEdge();
			}
			while(e != face->edges());
		}
		_faces[faceIndex] = _faces.back();
		_faces.erase(_faces.end() - 1);
		_reclaimedFaces.push_back(face);
	}

	/// Deletes a face from the mesh.
	void removeFace(Face* face) {
		OVITO_ASSERT(face != nullptr);
		OVITO_ASSERT(faces()[face->index()] == face);
		removeFace(face->index());
	}

	/// Deletes a vertex from the mesh. 
	/// This method assumes that the vertex is not connected to any edges or faces of the mesh.
	void removeVertex(int vertexIndex) {
		Vertex* vertex = this->vertex(vertexIndex);
		OVITO_ASSERT(vertex->edges() == nullptr);
		OVITO_ASSERT(vertex->numEdges() == 0);
		_vertices[vertexIndex] = _vertices.back();
		_vertices.erase(_vertices.end() - 1);
		_reclaimedVertices.push_back(vertex);
	}

	void joinFaces(Edge* edge) {
		Edge* oppositeEdge = edge->oppositeEdge();
		OVITO_ASSERT(oppositeEdge);

		for(Edge* currentEdge = edge->nextFaceEdge(); currentEdge != edge; currentEdge = currentEdge->nextFaceEdge()) {
			currentEdge->_face = oppositeEdge->face();
		}
		oppositeEdge->face()->setEdges(oppositeEdge->nextFaceEdge());
		edge->face()->markForDeletion();
		edge->vertex1()->removeEdge(edge);
		edge->vertex2()->removeEdge(oppositeEdge);
		edge->unlinkFromOppositeEdge();
		edge->prevFaceEdge()->setNextFaceEdge(oppositeEdge->nextFaceEdge());
		oppositeEdge->nextFaceEdge()->setPrevFaceEdge(edge->prevFaceEdge());
		oppositeEdge->prevFaceEdge()->setNextFaceEdge(edge->nextFaceEdge());
		edge->nextFaceEdge()->setPrevFaceEdge(oppositeEdge->prevFaceEdge());
		removeEdge(edge);
		removeEdge(oppositeEdge);
	}

	void collapseEdge(Edge* edge) {

		Edge* oppositeEdge = edge->oppositeEdge();
		OVITO_ASSERT(oppositeEdge);
		Edge* ep1 = edge->prevFaceEdge();
		Edge* epo1 = ep1->oppositeEdge();
		Edge* en1 = edge->nextFaceEdge();
		Edge* eno1 = en1->oppositeEdge();
		Edge* ep2 = oppositeEdge->prevFaceEdge();
		Edge* epo2 = ep2->oppositeEdge();
		Edge* en2 = oppositeEdge->nextFaceEdge();
		Edge* eno2 = en2->oppositeEdge();
		Vertex* deletedVertex = edge->vertex1();
		Vertex* remainingVertex = edge->vertex2();

		// Go around the deleted vertex and transfer edges.
		Edge* currentEdge = edge;
		do {
			if(currentEdge != edge)
				deletedVertex->transferEdgeToVertex(currentEdge, remainingVertex);
			currentEdge = currentEdge->prevFaceEdge()->oppositeEdge();
			OVITO_ASSERT(currentEdge);
		}
		while(currentEdge != edge);

		edge->prevFaceEdge()->setNextFaceEdge(edge->nextFaceEdge());
		edge->nextFaceEdge()->setPrevFaceEdge(edge->prevFaceEdge());
		oppositeEdge->prevFaceEdge()->setNextFaceEdge(oppositeEdge->nextFaceEdge());
		oppositeEdge->nextFaceEdge()->setPrevFaceEdge(oppositeEdge->prevFaceEdge());
		edge->face()->setEdges(edge->nextFaceEdge());
		oppositeEdge->face()->setEdges(oppositeEdge->nextFaceEdge());

		deletedVertex->removeEdge(edge);
		remainingVertex->removeEdge(oppositeEdge);

		edge->unlinkFromOppositeEdge();
		removeEdge(edge);
		removeEdge(oppositeEdge);
	}

	/// Create a new half-edge. This is for internal use only.
	Edge* createEdge(Vertex* vertex1, Vertex* vertex2, Face* face) {
		Edge* edge;
		if(_reclaimedEdges.empty()) {
			edge = _edgePool.construct(vertex2, face);
		}
		else {
			edge = _reclaimedEdges.back();
			_reclaimedEdges.pop_back();
			edge->_vertex2 = vertex2;
			edge->setFace(face);
			OVITO_ASSERT(!edge->oppositeEdge());
		}
		vertex1->addEdge(edge);
		if(face->edges()) {
			edge->setNextFaceEdge(face->edges());
			edge->setPrevFaceEdge(face->edges()->prevFaceEdge());
			face->edges()->prevFaceEdge()->setNextFaceEdge(edge);
			face->edges()->setPrevFaceEdge(edge);
		}
		else {
			edge->setNextFaceEdge(edge);
			edge->setPrevFaceEdge(edge);
			face->setEdges(edge);
		}
		return edge;
	}

	/// Tries to wire each half-edge of the mesh with its opposite (reverse) half-edge.
	bool connectOppositeHalfedges() {
		bool isClosed = true;
		for(Vertex* v1 : vertices()) {
			for(Edge* edge = v1->edges(); edge != nullptr; edge = edge->nextVertexEdge()) {
				if(edge->oppositeEdge() != nullptr) {
					OVITO_ASSERT(edge->oppositeEdge()->oppositeEdge() == edge);
					continue;		// Edge is already linked to its opposite edge.
				}

				// Search in the edge list of the second vertex for an half-edge that goes back to the first vertex.
				for(Edge* oppositeEdge = edge->vertex2()->edges(); oppositeEdge != nullptr; oppositeEdge = oppositeEdge->nextVertexEdge()) {
					if(oppositeEdge->oppositeEdge() != nullptr) continue;
					if(oppositeEdge->vertex2() == v1) {

						// Link the two half-edges.
						edge->linkToOppositeEdge(oppositeEdge);
						break;
					}
				}

				if(edge->oppositeEdge() == nullptr)
					isClosed = false;
			}
		}
		return isClosed;
	}

	/// Mesh copy function.
	template<template<typename> class EdgeBase2, template<typename> class FaceBase2, template<typename> class VertexBase2>
	void copyFrom(const HalfEdgeMesh<EdgeBase2, FaceBase2, VertexBase2>& other) {
		clear();

		typedef typename HalfEdgeMesh<EdgeBase2, FaceBase2, VertexBase2>::Edge OtherEdge;
		typedef typename HalfEdgeMesh<EdgeBase2, FaceBase2, VertexBase2>::Face OtherFace;
		typedef typename HalfEdgeMesh<EdgeBase2, FaceBase2, VertexBase2>::Vertex OtherVertex;

		// Copy vertices.
		reserveVertices(other.vertexCount());
		for(OtherVertex* v : other.vertices()) {
			int index = createVertex(v->pos())->index();
			OVITO_ASSERT(index == v->index());
		}

		// Copy faces and half-edges.
		reserveFaces(other.faceCount());
		for(OtherFace* face_o : other.faces()) {
			Face* face_c = createFace();
			OVITO_ASSERT(face_c->index() == face_o->index());
			face_c->setFlags(face_o->flags());

			if(!face_o->edges()) continue;
			OtherEdge* edge_o = face_o->edges();
			do {
				Vertex* v1 = vertex(edge_o->vertex1()->index());
				Vertex* v2 = vertex(edge_o->vertex2()->index());
				Edge* edge_c = createEdge(v1, v2, face_c);
				edge_o = edge_o->nextFaceEdge();
			}
			while(edge_o != face_o->edges());
		}

		// Link opposite half-edges.
		auto face_o = other.faces().cbegin();
		auto face_c = faces().cbegin();
		for(; face_c != faces().cend(); ++face_c, ++face_o) {
			OtherEdge* edge_o = (*face_o)->edges();
			Edge* edge_c = (*face_c)->edges();
			if(!edge_o) continue;
			do {
				if(edge_o->oppositeEdge() != nullptr && edge_c->oppositeEdge() == nullptr) {
					Face* oppositeFace = face(edge_o->oppositeEdge()->face()->index());
					Edge* oppositeEdge = oppositeFace->edges();
					do {
						OVITO_CHECK_POINTER(oppositeEdge);
						if(oppositeEdge->vertex1() == edge_c->vertex2() && oppositeEdge->vertex2() == edge_c->vertex1()) {
							edge_c->linkToOppositeEdge(oppositeEdge);
							break;
						}
						oppositeEdge = oppositeEdge->nextFaceEdge();
					}
					while(oppositeEdge != oppositeFace->edges());
					OVITO_ASSERT(edge_c->oppositeEdge());
				}
				edge_o = edge_o->nextFaceEdge();
				edge_c = edge_c->nextFaceEdge();
			}
			while(edge_o != (*face_o)->edges());
		}
	}

	/// Copy assignment operator.
	HalfEdgeMesh& operator=(const HalfEdgeMesh& other) {
		copyFrom(other);
		return *this;
	}

	/// Swaps the contents of this mesh with another mesh.
	void swap(HalfEdgeMesh& other) {
		_vertices.swap(other._vertices);
		_faces.swap(other._faces);
		_vertexPool.swap(other._vertexPool);
		_edgePool.swap(other._edgePool);
		_facePool.swap(other._facePool);
		_reclaimedFaces.swap(other._reclaimedFaces);
		_reclaimedEdges.swap(other._reclaimedEdges);
		_reclaimedVertices.swap(other._reclaimedVertices);
	}

	/// Converts this half-edge mesh to a triangle mesh.
	void convertToTriMesh(TriMesh& output) const {
		output.clear();

		// Transfer vertices.
		output.setVertexCount(vertexCount());
		auto vout = output.vertices().begin();
		for(Vertex* v : vertices()) {
			OVITO_ASSERT(v->index() == (vout - output.vertices().begin()));
			*vout++ = v->pos();
		}

		// Count number of output triangles.
		int triangleCount = 0;
		for(Face* face : faces()) {
			triangleCount += std::max(face->edgeCount() - 2, 0);
		}

		// Transfer faces.
		output.setFaceCount(triangleCount);
		auto fout = output.faces().begin();
		for(Face* face : faces()) {
			int baseVertex = face->edges()->vertex2()->index();
			Edge* edge = face->edges()->nextFaceEdge()->nextFaceEdge();
			while(edge != face->edges()) {

				// Validate face's edge topology.
				OVITO_ASSERT(edge->vertex1() != edge->vertex2());
				OVITO_ASSERT(edge->nextFaceEdge()->vertex1() == edge->vertex2());
				OVITO_ASSERT(edge->prevFaceEdge()->vertex2() == edge->vertex1());
				OVITO_ASSERT(edge->nextFaceEdge()->prevFaceEdge() == edge);
				OVITO_ASSERT(edge->nextFaceEdge() != edge->oppositeEdge());
				OVITO_ASSERT(edge->prevFaceEdge() != edge->oppositeEdge());
				
				fout->setVertices(baseVertex, edge->vertex1()->index(), edge->vertex2()->index());
				++fout;
				edge = edge->nextFaceEdge();
			}
		}
		OVITO_ASSERT(fout == output.faces().end());

		output.invalidateVertices();
		output.invalidateFaces();
	}

	/// Converts selected faces of this half-edge mesh to a triangle mesh.
	template<class UnaryPredicate>
	void convertToTriMesh(TriMesh& output, UnaryPredicate predicate) const {
		output.clear();

		// Count number of output vertices.
		int outputVertexCount = 0;
		for(Vertex* vertex : vertices()) {
			// Vertex is copied to the output mesh if there is at least one incident face that
			// is selected by the predicate.
			for(Edge* edge = vertex->edges(); edge != nullptr; edge = edge->nextVertexEdge()) {
				if(predicate(edge->face())) {
					outputVertexCount++;
					break;
				}
			}
		}

		// Transfer vertices and build a mapping from input vertices to output vertices.
		output.setVertexCount(outputVertexCount);
		std::vector<int> indexMap(vertices().size());
		size_t vindex = 0;
		auto vout = output.vertices().begin();
		for(Vertex* vertex : vertices()) {
			OVITO_ASSERT(vertex->index() == vindex);
			for(Edge* edge = vertex->edges(); edge != nullptr; edge = edge->nextVertexEdge()) {
				if(predicate(edge->face())) {
					indexMap[vindex] = vout - output.vertices().begin();
					*vout++ = vertex->pos();
					break;
				}
			}
			vindex++;
		}
		OVITO_ASSERT(vout == output.vertices().end());

		// Count number of output triangles.
		int triangleCount = 0;
		for(Face* face : faces()) {
			if(predicate(face))
				triangleCount += std::max(face->edgeCount() - 2, 0);
		}

		// Transfer faces.
		output.setFaceCount(triangleCount);
		auto fout = output.faces().begin();
		for(Face* face : faces()) {
			if(!predicate(face)) continue;

			int baseVertex = indexMap[face->edges()->vertex2()->index()];
			Edge* edge = face->edges()->nextFaceEdge()->nextFaceEdge();
			while(edge != face->edges()) {

				// Validate face's edge topology.
				OVITO_ASSERT(edge->vertex1() != edge->vertex2());
				OVITO_ASSERT(edge->nextFaceEdge()->vertex1() == edge->vertex2());
				OVITO_ASSERT(edge->prevFaceEdge()->vertex2() == edge->vertex1());
				OVITO_ASSERT(edge->nextFaceEdge()->prevFaceEdge() == edge);
				OVITO_ASSERT(edge->nextFaceEdge() != edge->oppositeEdge());
				OVITO_ASSERT(edge->prevFaceEdge() != edge->oppositeEdge());
				
				fout->setVertices(baseVertex, indexMap[edge->vertex1()->index()], indexMap[edge->vertex2()->index()]);
				++fout;
				edge = edge->nextFaceEdge();
			}
		}
		OVITO_ASSERT(fout == output.faces().end());

		output.invalidateVertices();
		output.invalidateFaces();
	}	

	/// Duplicates vertices which are part of more than one manifold.
	size_t duplicateSharedVertices() {
		size_t numSharedVertices = 0;
		size_t oldVertexCount = vertices().size();
		std::vector<Edge*> visitedEdges;
		for(size_t vertexIndex = 0; vertexIndex < oldVertexCount; vertexIndex++) {
			Vertex* vertex = vertices()[vertexIndex];
			OVITO_ASSERT(vertex->numEdges() >= 2);

			// Go in positive direction around vertex, facet by facet.
			Edge* currentEdge = vertex->edges();
			int numManifoldEdges = 0;
			do {
				OVITO_ASSERT(currentEdge != nullptr && currentEdge->face() != nullptr);
				currentEdge = currentEdge->prevFaceEdge()->oppositeEdge();
				numManifoldEdges++;
			}
			while(currentEdge != vertex->edges());

			if(numManifoldEdges == vertex->numEdges())
				continue;		// Vertex is not part of multiple manifolds.

			visitedEdges.clear();
			currentEdge = vertex->edges();
			do {
				visitedEdges.push_back(currentEdge);
				currentEdge = currentEdge->prevFaceEdge()->oppositeEdge();
			}
			while(currentEdge != vertex->edges());

			int oldEdgeCount = vertex->numEdges();
			int newEdgeCount = visitedEdges.size();

			while(visitedEdges.size() != oldEdgeCount) {

				// Create a second vertex that takes the edges not visited yet.
				Vertex* secondVertex = createVertex(vertex->pos());

				Edge* startEdge;
				for(startEdge = vertex->edges(); startEdge != nullptr; startEdge = startEdge->nextVertexEdge()) {
					if(std::find(visitedEdges.cbegin(), visitedEdges.cend(), startEdge) == visitedEdges.end())
						break;
				}
				OVITO_ASSERT(startEdge != nullptr);

				currentEdge = startEdge;
				do {
					OVITO_ASSERT(std::find(visitedEdges.cbegin(), visitedEdges.cend(), currentEdge) == visitedEdges.end());
					visitedEdges.push_back(currentEdge);
					OVITO_ASSERT(vertex->edges() != currentEdge);
					vertex->transferEdgeToVertex(currentEdge, secondVertex);
					currentEdge = currentEdge->prevFaceEdge()->oppositeEdge();
				}
				while(currentEdge != startEdge);
			}
			OVITO_ASSERT(vertex->numEdges() == newEdgeCount);

			numSharedVertices++;
		}

		return numSharedVertices;
	}

	/// Clears the given flag for all faces.
	void clearFaceFlag(unsigned int flag) const {
		for(Face* face : faces())
			face->clearFlag(flag);
	}

	/// Determines if this mesh is a closed manifold, i.e., every half edge is linked to an opposite half edge.
	bool isClosed() const {
		for(Vertex* vertex : vertices()) {
			for(Edge* edge = vertex->edges(); edge != nullptr; edge = edge->nextVertexEdge()) {
				OVITO_ASSERT(edge->face() != nullptr);
				if(edge->oppositeEdge() == nullptr)
					return false;
				OVITO_ASSERT(edge->oppositeEdge()->oppositeEdge() == edge);
				OVITO_ASSERT(edge->oppositeEdge()->face() != edge->face());
				OVITO_ASSERT(edge->nextFaceEdge()->face() == edge->face());
				OVITO_ASSERT(edge->prevFaceEdge()->face() == edge->face());
				OVITO_ASSERT(edge->vertex2()->hasEdge(edge->oppositeEdge()));
				OVITO_ASSERT(edge->vertex2()->hasEdge(edge->nextFaceEdge()));
			}
		}
		return true;
	}

	/// \brief Flip the orientation of all faces.
	void flipFaces() {
		for(Face* face : faces()) {
			if(Edge* e = face->edges()) {
				do {
					e->vertex1()->transferEdgeToVertex(e, e->_vertex2, false);
					e = e->nextFaceEdge();
				}
				while(e != face->edges());
				Vertex* v1 = e->vertex1();
				do {
					std::swap(e->_vertex2, v1);
					std::swap(e->_nextFaceEdge, e->_prevFaceEdge);
					e = e->prevFaceEdge();
				}
				while(e != face->edges());
			}
		}
	}

	/// Re-assigned indices to faces and vertices of the mesh so that the indices
	/// form a consecutive sequence starting at zero.
	void reindexVerticesAndFaces() {
		int vindex = 0;
		for(Vertex* vertex : vertices()) vertex->_index = vindex++;
		int findex = 0;
		for(Face* face : faces()) face->_index = findex++;
	}

	/// Feletes the faces from the face list that have previously been marked for
	/// deletion using the Face::markForDeletion() method.
	/// Remaining faces will be re-indexed.
	void removeMarkedFaces() {
		for(int f = faceCount() - 1; f >= 0; f--) {
			if(faces()[f]->edges() == nullptr)
				removeFace(f);
		}
		int findex = 0;
		for(Face* face : faces()) face->_index = findex++;
	}

private:

	/// This derived class is needed to make the protected Vertex constructor accessible.
	class InternalVertex : public Vertex {
	public:
		InternalVertex(const Point3& pos, int index = -1) : Vertex(pos, index) {}
	};

	/// This derived class is needed to make the protected Edge constructor accessible.
	class InternalEdge : public Edge {
	public:
		InternalEdge(Vertex* vertex2, Face* face) : Edge(vertex2, face) {}
	};

	/// This derived class is needed to make the protected Face constructor accessible.
	class InternalFace : public Face {
	public:
		InternalFace(int index = -1) : Face(index) {}
	};

private:

	/// The vertices of the mesh.
	std::vector<Vertex*> _vertices;
	MemoryPool<InternalVertex> _vertexPool;

	/// The edges of the mesh.
	MemoryPool<InternalEdge> _edgePool;

	/// The faces of the mesh.
	std::vector<Face*> _faces;
	MemoryPool<InternalFace> _facePool;

	/// A list of faces that have been deleted from the mesh.
	/// They can be reused when a new face is to be created.
	std::vector<Face*> _reclaimedFaces;

	/// A list of haldedges that have been deleted from the mesh.
	/// They can be reused when a new edge is to be created.
	std::vector<Edge*> _reclaimedEdges;

	/// A list of vertices that have been deleted from the mesh.
	/// They can be reused when a new vertex is to be created.
	std::vector<Vertex*> _reclaimedVertices;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
