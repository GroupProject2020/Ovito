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

#pragma once


#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include <plugins/mesh/surface/SurfaceMesh.h>

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

/**
 * Helper data structure that encapsulates a microstructure consisting
 * of a surface mesh topology and a set of per-vertex, per-face and per-region properties.
 * The class is used in the implementation of algorithms to build up or operate on microstructure
 * data.
 */
class OVITO_CRYSTALANALYSIS_EXPORT Microstructure
{
public:

	using size_type = HalfEdgeMesh::size_type;
	using vertex_index = HalfEdgeMesh::vertex_index;
	using edge_index = HalfEdgeMesh::edge_index;
	using face_index = HalfEdgeMesh::face_index;

	/// Possible values for the 'Face type' property in a microstructure mesh.
	enum MicrostructureFaceType {
		INTERFACE = 0,
		DISLOCATION = 1,
		SLIP_FACET = 2
	};

	/// Constructor that adopts the data fields from the given microstructure data object.
	explicit Microstructure(const MicrostructureObject* mo);

	/// Returns the Burgers vector of a dislocation mesh face or the slip vector of a slip facet.
	const Vector3& burgersVector(face_index face) const { return _burgersVectors->getVector3(face); }

	/// Assigns a Burgers vector to a dislocation mesh face or the slip vector to a slip facet.
	void setBurgersVector(face_index face, const Vector3& b) { _burgersVectors->setVector3(face, b); }

	/// Returns the volumetric region which the given face belongs to.
	int faceRegion(face_index face) const { return _faceRegions->getInt(face); }

	/// Sets the cluster a dislocation/slip face is embedded in.
	void setFaceRegion(face_index face, int region) { _faceRegions->setInt(face, region); }

	/// Returns the volumetric region which the given mesh edge belongs to.
	int edgeRegion(edge_index edge) const { return faceRegion(adjacentFace(edge)); }

	/// Returns whether the given mesh face represents a dislocation line.
	bool isDislocationFace(face_index face) const {
		return _faceTypes->getInt(face) == DISLOCATION;
	}

	/// Returns whether the given mesh edge is a dislocation segment.
	bool isDislocationEdge(edge_index edge) const {
		return isDislocationFace(adjacentFace(edge)) && hasOppositeEdge(edge);
	}

	/// Returns whether the given mesh face represents a slip facet.
	bool isSlipSurfaceFace(face_index face) const {
		return _faceTypes->getInt(face) == SLIP_FACET;
	}

	/// Sets the type of the given mesh face.
	void setFaceType(face_index face, MicrostructureFaceType type) {
		_faceTypes->setInt(face, type);
	}

	/// Determines the number of dislocation arms connected to the given mesh vertex.
	int countDislocationArms(vertex_index vertex) const {
		int armCount = 0;
		for(edge_index e = firstVertexEdge(vertex); e != HalfEdgeMesh::InvalidIndex; e = nextVertexEdge(e)) {
			if(isDislocationEdge(e)) armCount++;
		}
		return armCount;
	}

	/// Create a dislocation line segment between two nodal points.
	edge_index createDislocationSegment(vertex_index vertex1, vertex_index vertex2, const Vector3& burgersVector, int region);

	/// Merges virtual dislocation faces to build continuous lines from individual dislocation segments.
	void makeContinuousDislocationLines();

	/// Aligns the orientation of slip faces and builds contiguous two-dimensional manifolds
	/// of maximum extent, i.e. slip surfaces with constant slip vector.
	void makeSlipSurfaces();

	/// Returns the mesh topology of the microstructure.
	const HalfEdgeMeshPtr& topology() const { return _topology; }

	/// Returns the first edge from a vertex' list of outgoing half-edges.
	edge_index firstVertexEdge(vertex_index vertex) const { return topology()->firstVertexEdge(vertex); }

	/// Returns the half-edge following the given half-edge in the linked list of half-edges of a vertex.
	edge_index nextVertexEdge(edge_index edge) const { return topology()->nextVertexEdge(edge); }

	/// Returns the first half-edge from the linked-list of half-edges of a face.
	edge_index firstFaceEdge(face_index face) const { return topology()->firstFaceEdge(face); }

	/// Returns the opposite face of a face.
	face_index oppositeFace(face_index face) const { return topology()->oppositeFace(face); };

	/// Determines whether the given face is linked to an opposite face.
	bool hasOppositeFace(face_index face) const { return topology()->hasOppositeFace(face); };

	/// Returns the next half-edge following the given half-edge in the linked-list of half-edges of a face.
	edge_index nextFaceEdge(edge_index edge) const { return topology()->nextFaceEdge(edge); }

	/// Returns the previous half-edge preceding the given edge in the linked-list of half-edges of a face.
	edge_index prevFaceEdge(edge_index edge) const { return topology()->prevFaceEdge(edge); }

	/// Returns the vertex the given half-edge is originating from.
	vertex_index vertex1(edge_index edge) const { return topology()->vertex1(edge); }

	/// Returns the vertex the given half-edge is leading to.
	vertex_index vertex2(edge_index edge) const { return topology()->vertex2(edge); }

	/// Returns the face which is adjacent to the given half-edge.
	face_index adjacentFace(edge_index edge) const { return topology()->adjacentFace(edge); }

	/// Returns the opposite half-edge of the given edge.
	edge_index oppositeEdge(edge_index edge) const { return topology()->oppositeEdge(edge); }

	/// Returns whether the given half-edge has an opposite half-edge.
	bool hasOppositeEdge(edge_index edge) const { return topology()->hasOppositeEdge(edge); }

	/// Returns the next incident manifold when going around the given half-edge.
	edge_index nextManifoldEdge(edge_index edge) const { return topology()->nextManifoldEdge(edge); };

	/// Sets what is the next incident manifold when going around the given half-edge.
	void setNextManifoldEdge(edge_index edge, edge_index nextEdge) {
		OVITO_ASSERT(_isMutable);
		topology()->setNextManifoldEdge(edge, nextEdge);
	}

	/// Determines the number of manifolds adjacent to a half-edge.
	int countManifolds(edge_index edge) const { return topology()->countManifolds(edge); }

	/// Returns the position of the i-th mesh vertex.
	const Point3& vertexPosition(vertex_index vertex) const { return _vertexCoords->getPoint3(vertex); }

	/// Creates a new vertex at the given coordinates.
	vertex_index createVertex(const Point3& pos) {
		OVITO_ASSERT(_isMutable);
		vertex_index vidx = _topology->createVertex();
		_vertexCoords->grow(1);
		_vertexCoords->setPoint3(vidx, pos);
		return vidx;
	}

	/// Creates a new face without any edges.
	/// Returns the index of the new face.
	face_index createFace(MicrostructureFaceType faceType, int faceRegion, const Vector3& burgersVector) {
		OVITO_ASSERT(_isMutable);
		face_index fidx = _topology->createFace();
		_faceTypes->grow(1);
		_faceTypes->setInt(fidx, faceType);
		_faceRegions->grow(1);
		_faceRegions->setInt(fidx, faceRegion);
		_burgersVectors->grow(1);
		_burgersVectors->setVector3(fidx, burgersVector);
		return fidx;
	}

	/// Creates a new half-edge between two vertices and adjacent to the given face.
	/// Returns the index of the new half-edge.
	edge_index createEdge(vertex_index vertex1, vertex_index vertex2, face_index face) {
		OVITO_ASSERT(_isMutable);
		return _topology->createEdge(vertex1, vertex2, face);
	}

	/// Links two opposite half-edges together.
	void linkOppositeEdges(edge_index edge1, edge_index edge2) {
		OVITO_ASSERT(_isMutable);
		topology()->linkOppositeEdges(edge1, edge2);
	}

	/// Returns the simulation cell the microstructure is embedded in.
	const SimulationCell& cell() const { return _cell; }

	/// Returns the (wrapped) vector corresponding to an half-edge of the microstructure mesh.
	Vector3 edgeVector(edge_index edge) const {
		return cell().wrapVector(vertexPosition(vertex2(edge)) - vertexPosition(vertex1(edge)));
	}

private:

	HalfEdgeMeshPtr _topology;		///< Holds the mesh topology of the microstructure.
	PropertyPtr _vertexCoords;		///< The property array holding the per-vertex mesh coordinates.
	PropertyPtr _burgersVectors;	///< The property array holding the per-face Burgers vector information.
	PropertyPtr _faceTypes;			///< The property array holding the per-face type information.
	PropertyPtr _faceRegions;		///< The property array holding the per-face region information.
	SimulationCell _cell;			///< The simulation cell the microstructure is embedded in.
	bool _isMutable;				///< Indicates whether the data in this struture may be modified.
};

/**
 * Stores a microstructure description including dislocation lines,
 * grain boundaries, slip surfaces and stacking faults.
 */
class OVITO_CRYSTALANALYSIS_EXPORT MicrostructureObject : public SurfaceMesh
{
	Q_OBJECT
	OVITO_CLASS(MicrostructureObject)

public:

	/// Constructor.
	Q_INVOKABLE MicrostructureObject(DataSet* dataset) : SurfaceMesh(dataset) {}

	/// Returns the title of this object.
	virtual QString objectTitle() const override { return tr("Microstructure"); }
};

}	// End of namespace
}	// End of namespace
}	// End of namespace
