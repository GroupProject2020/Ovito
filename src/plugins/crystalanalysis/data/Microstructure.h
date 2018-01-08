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


#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include <core/utilities/mesh/HalfEdgeMesh.h>
#include "ClusterVector.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

template<typename> class MicrostructureVertexInfo;	// defined below
template<typename> class MicrostructureEdgeInfo;	// defined below
template<typename> class MicrostructureFaceInfo;	// defined below
using MicrostructureBase = HalfEdgeMesh<MicrostructureEdgeInfo, MicrostructureFaceInfo, MicrostructureVertexInfo>;

/**
 * Extension data structure associated with each vertex of a Microstructure.
 */
template<typename Vertex>
class MicrostructureVertexInfo
{
public:

	/// Determines the number of dislocation arms connected to this vertex.
	int countDislocationArms() const {
		int armCount = 0;
		for(auto e = reinterpret_cast<const Vertex*>(this)->edges(); e != nullptr; e = e->nextVertexEdge()) {
			if(e->isDislocation()) armCount++;
		}
		return armCount;
	}
};

/**
 * Extension data structure associated with each half-edge of a Microstructure.
 */
template<typename Edge>
class MicrostructureEdgeInfo
{
public:

	/// Returns whether this edge is a dislocation segment.
	bool isDislocation() const {
		return reinterpret_cast<const Edge*>(this)->oppositeEdge() != nullptr;
	}

	/// Returns the Burgers vector if this edge is a dislocation segment.
	const Vector3& burgersVector() const { return reinterpret_cast<const Edge*>(this)->face()->burgersVector(); }

	/// Returns the crystal cluster if this edge is a dislocation segment.
	Cluster* cluster() const { return reinterpret_cast<const Edge*>(this)->face()->cluster(); }
};

/**
 * Extension data structure associated with each face of a Microstructure.
 */
template<typename Face>
struct MicrostructureFaceInfo
{
public:

	/// Bit-wise flags that can be set for a face in the microstructure mesh.
	enum FaceFlags {
		VISITED = (1<<0), 			//< Used by some algorithms as a mark faces as visited.
		IS_EVEN_FACE = (1<<1), 		//< Indicates that the face is the "even" one in a pair of opposite faces.
		IS_DISLOCATION = (1<<2), 	//< Indicates that the face is a virtual face associated with a dislocation line.
		IS_SLIP_SURFACE = (1<<3), 	//< Indicates that the face is part of a slip surface.
	};

public:

	/// Returns a pointer to this face's opposite face.
	MicrostructureBase::Face* oppositeFace() const { return _oppositeFace; }

	/// Sets the pointer to this face's opposite face. Use with care!
	void setOppositeFace(MicrostructureBase::Face* of) { _oppositeFace = of; }

	/// Returns whether this is the "even" face from the pair of two opposite faces.
	bool isEvenFace() const {
		OVITO_ASSERT(oppositeFace() != nullptr);
		OVITO_ASSERT(oppositeFace()->testFlag(IS_EVEN_FACE) != reinterpret_cast<const Face*>(this)->testFlag(IS_EVEN_FACE));
		return reinterpret_cast<const Face*>(this)->testFlag(IS_EVEN_FACE); 
	}

	/// Sets whether this is the "even" face in a pair of two opposite faces.
	void setEvenFace(bool b) { 
		if(b)
			reinterpret_cast<Face*>(this)->setFlag(IS_EVEN_FACE);
		else
			reinterpret_cast<Face*>(this)->clearFlag(IS_EVEN_FACE);
	}

	/// Returns the Burgers vector of the dislocation defect or the slip vector of the surface surface.
	const Vector3& burgersVector() const { return _burgersVector; }

	/// Sets the Burgers vector of the dislocation defect or the slip vector of the surface surface.
	void setBurgersVector(const Vector3& b) { _burgersVector = b; }

	/// Returns the cluster the dislocation/slip surface is embedded in.
	Cluster* cluster() const { return _cluster; }	

	/// Sets the cluster the dislocation/slip surface is embedded in.
	void setCluster(Cluster* cluster) { _cluster = cluster; }

	/// Returns whether this face is a virtual face associated with a dislocation line.
	bool isDislocationFace() const {
		return reinterpret_cast<const Face*>(this)->testFlag(IS_DISLOCATION);
	}

	/// Marks this face as a virtual face associated with a dislocation line.
	void setDislocationFace(bool b) {
		if(b)
			reinterpret_cast<Face*>(this)->setFlag(IS_DISLOCATION);
		else
			reinterpret_cast<Face*>(this)->clearFlag(IS_DISLOCATION);
	}

	/// Returns whether this face is part of a slip surface.
	bool isSlipSurfaceFace() const {
		return reinterpret_cast<const Face*>(this)->testFlag(IS_SLIP_SURFACE);
	}

	/// Marks this face as part of a slip surface.
	void setSlipSurfaceFace(bool b) {
		if(b)
			reinterpret_cast<Face*>(this)->setFlag(IS_SLIP_SURFACE);
		else
			reinterpret_cast<Face*>(this)->clearFlag(IS_SLIP_SURFACE);
	}

private:

	/// The face on the opposite side of the manifold.
	MicrostructureBase::Face* _oppositeFace = nullptr;

	/// The Burgers vector of the dislocation defect.
	Vector3 _burgersVector;

	/// The cluster the dislocation is embedded in.
	Cluster* _cluster;
};

/**
 * Geometric and topological data structure that describes a materials microstructure
 * consisting of domains (grains), domain boundaries, planar defects and surfaces
 * and line defects (dislocations).
 */
class OVITO_CRYSTALANALYSIS_EXPORT Microstructure : public MicrostructureBase
{
public:

	/// Constructor.
	Microstructure(std::shared_ptr<ClusterGraph> clusterGraph) : _clusterGraph(std::move(clusterGraph)) {}

	/// Copy constructor.
	Microstructure(const Microstructure& other);

	/// Returns a const-reference to the cluster graph.
	const std::shared_ptr<ClusterGraph>& clusterGraph() const { return _clusterGraph; }

	/// Create a dislocation line segment between two nodes.
	Edge* createDislocationSegment(Vertex* vertex1, Vertex* vertex2, const Vector3& burgersVector, Cluster* cluster);

	/// Merges virtual dislocation faces to build continuous lines from individual dislocation segments.
	void makeContinuousDislocationLines();

private:

	/// The associated cluster graph.
	const std::shared_ptr<ClusterGraph> _clusterGraph;
};

}	// End of namespace
}	// End of namespace
}	// End of namespace
