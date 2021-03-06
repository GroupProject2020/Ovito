////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2019 Alexander Stukowski
//
//  This file is part of OVITO (Open Visualization Tool).
//
//  OVITO is free software; you can redistribute it and/or modify it either under the
//  terms of the GNU General Public License version 3 as published by the Free Software
//  Foundation (the "GPL") or, at your option, under the terms of the MIT License.
//  If you do not alter this notice, a recipient may use your version of this
//  file under either the GPL or the MIT License.
//
//  You should have received a copy of the GPL along with this program in a
//  file LICENSE.GPL.txt.  You should have received a copy of the MIT License along
//  with this program in a file LICENSE.MIT.txt
//
//  This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND,
//  either express or implied. See the GPL or the MIT License for the specific language
//  governing rights and limitations.
//
////////////////////////////////////////////////////////////////////////////////////////

#pragma once


#include <ovito/crystalanalysis/CrystalAnalysis.h>
#include <ovito/mesh/surface/SurfaceMesh.h>
#include <ovito/mesh/surface/SurfaceMeshData.h>

namespace Ovito { namespace CrystalAnalysis {

/**
 * Helper data structure that encapsulates a microstructure consisting
 * of a surface mesh topology and a set of per-vertex, per-face and per-region properties.
 * The class is used in the implementation of algorithms to build up or operate on microstructure
 * data.
 */
class OVITO_CRYSTALANALYSIS_EXPORT MicrostructureData : public SurfaceMeshData
{
public:

	/// Possible values for the 'Face type' property in a microstructure mesh.
	enum MicrostructureFaceType {
		INTERFACE = 0,
		DISLOCATION = 1,
		SLIP_FACET = 2
	};

	/// Constructor creating an empty microstructure.
	MicrostructureData(const SimulationCell& cell = {});

	/// Constructor that adopts the data from the given pipeline data object into this structure.
	explicit MicrostructureData(const SurfaceMesh* mo);

	/// Returns the Burgers vector of a dislocation mesh face or the slip vector of a slip facet.
	const Vector3& burgersVector(face_index face) const {
		OVITO_ASSERT(face >= 0 && face < faceCount());
		return burgersVectors()[face];
	}

	/// Assigns a Burgers vector to a dislocation mesh face or the slip vector to a slip facet.
	void setBurgersVector(face_index face, const Vector3& b) {
		OVITO_ASSERT(face >= 0 && face < faceCount());
		burgersVectors()[face] = b;
	}

	/// Returns the crystallographic normal vector of a mesh face.
	const Vector3& crystallographicNormal(face_index face) const {
		OVITO_ASSERT(face >= 0 && face < faceCount());
		return crystallographicNormals()[face];
	}

	/// Assigns a crystallographic normal vector to a mesh face.
	void setCrystallographicNormal(face_index face, const Vector3& b) {
		OVITO_ASSERT(face >= 0 && face < faceCount());
		crystallographicNormals()[face] = b;
	}

	/// Returns whether the given mesh face represents a dislocation line.
	bool isDislocationFace(face_index face) const {
		OVITO_ASSERT(face >= 0 && face < faceCount());
		return faceTypes()[face] == DISLOCATION;
	}

	/// Returns whether the given mesh edge is a physical dislocation segment.
	bool isPhysicalDislocationEdge(edge_index edge) const {
		return isDislocationFace(adjacentFace(edge)) && hasOppositeEdge(edge);
	}

	/// Returns whether the given mesh face represents a slip facet.
	bool isSlipSurfaceFace(face_index face) const {
		OVITO_ASSERT(face >= 0 && face < faceCount());
		return faceTypes()[face] == SLIP_FACET;
	}

	/// Sets the type of the given mesh face.
	void setFaceType(face_index face, MicrostructureFaceType type) {
		OVITO_ASSERT(face >= 0 && face < faceCount());
		faceTypes()[face] = type;
	}

	/// Determines the number of dislocation arms connected to the given mesh vertex.
	int countDislocationArms(vertex_index vertex) const {
		int armCount = 0;
		for(edge_index e = firstVertexEdge(vertex); e != HalfEdgeMesh::InvalidIndex; e = nextVertexEdge(e)) {
			if(isPhysicalDislocationEdge(e)) armCount++;
		}
		return armCount;
	}

	/// Create a dislocation line segment between two nodal points.
	edge_index createDislocationSegment(vertex_index vertex1, vertex_index vertex2, const Vector3& burgersVector, region_index region);

	/// Merges virtual dislocation faces to build continuous lines from individual dislocation segments.
	void makeContinuousDislocationLines();

	/// Creates a new face without any edges.
	/// Returns the index of the new face.
	face_index createFace(std::initializer_list<vertex_index> vertices, region_index faceRegion, MicrostructureFaceType faceType, const Vector3& burgersVector, const Vector3& slipFacetNormal) {
		face_index fidx = SurfaceMeshData::createFace(std::move(vertices), faceRegion);
		faceTypes()[fidx] = faceType;
		burgersVectors()[fidx] = burgersVector;
		crystallographicNormals()[fidx] = slipFacetNormal;
		return fidx;
	}

	/// Returns the phase ID of the given region.
	int regionPhase(region_index region) const {
		OVITO_ASSERT(regionPhases());
		OVITO_ASSERT(region >= 0 && region < regionCount());
		return regionPhases()[region];
	}
};

/**
 * Stores a microstructure description including dislocation lines,
 * grain boundaries, slip surfaces and stacking faults.
 */
class OVITO_CRYSTALANALYSIS_EXPORT Microstructure : public SurfaceMesh
{
	Q_OBJECT
	OVITO_CLASS(Microstructure)

public:

	/// Constructor.
	Q_INVOKABLE Microstructure(DataSet* dataset) : SurfaceMesh(dataset) {}

	/// Returns the title of this object.
	virtual QString objectTitle() const override { return tr("Microstructure"); }
};

}	// End of namespace
}	// End of namespace
