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

struct MicrostructureFaceInfo;	// defined below
struct MicrostructureEdgeInfo;	// defined below
using MicrostructureBase = HalfEdgeMesh<MicrostructureEdgeInfo, MicrostructureFaceInfo, EmptyHalfEdgeMeshStruct>;

/**
 * Extension data structure associated with each half-edge of a Microstructure.
 */
class MicrostructureEdgeInfo
{
public:

private:

	/// Pointer to the next manifold sharing this edge.
	MicrostructureBase::Edge* _nextManifoldEdge = nullptr;
};

/**
 * Extension data structure associated with each face of a Microstructure.
 */
struct MicrostructureFaceInfo
{
public:

	/// Returns a pointer to this face's opposite face.
	MicrostructureBase::Face* oppositeFace() const { return _oppositeFace; }

	/// Sets the pointer to this face's opposite face. Use with care!
	void setOppositeFace(MicrostructureBase::Face* of) { _oppositeFace = of; }

	/// Returns the Burgers vector of the dislocation defect or the slip vector of the surface surface.
	const Vector3& burgersVector() const { return _burgersVector; }

	/// Sets the Burgers vector of the dislocation defect or the slip vector of the surface surface.
	void setBurgersVector(const Vector3& b) { _burgersVector = b; }

	/// Returns the cluster the dislocation/slip surface is embedded in.
	Cluster* cluster() const { return _cluster; }	

	/// Sets the cluster the dislocation/slip surface is embedded in.
	void setCluster(Cluster* cluster) { _cluster = cluster; }	

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

	enum FaceFlags {
		FACE_IS_DISLOCATION = (1<<0)
	};

public:

	/// Constructor.
	Microstructure(std::shared_ptr<ClusterGraph> clusterGraph) : _clusterGraph(std::move(clusterGraph)) {}

	/// Returns a const-reference to the cluster graph.
	const std::shared_ptr<ClusterGraph>& clusterGraph() const { return _clusterGraph; }

private:

	/// The associated cluster graph.
	const std::shared_ptr<ClusterGraph> _clusterGraph;
};

}	// End of namespace
}	// End of namespace
}	// End of namespace
