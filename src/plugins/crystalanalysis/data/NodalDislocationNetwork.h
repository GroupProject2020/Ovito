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
#include <plugins/stdobj/simcell/SimulationCell.h>
#include "ClusterVector.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

/**
 * This class stores a network of dislocation in a node-based representation.
 */
class NodalDislocationNetwork
{
public:

	struct Segment; // Forward declaration

	struct Node
	{
		Point3 pos;
		Segment* segments = nullptr;
		int index;
	};
	
	struct Segment
	{
		Node* node2;
		Node* node1() const { 
			OVITO_ASSERT(reverseSegment != nullptr);
			return reverseSegment->node2; 
		}
		ClusterVector burgersVector = Vector3::Zero();
		Segment* nextNodeSegment;
		Segment* reverseSegment;
		int info;
	};

public:

	/// Constructor.
	NodalDislocationNetwork(std::shared_ptr<ClusterGraph> clusterGraph) : _clusterGraph(std::move(clusterGraph)) {}

	/// Returns a const-reference to the cluster graph.
	const std::shared_ptr<ClusterGraph>& clusterGraph() const { return _clusterGraph; }

	/// Returns the list of dislocation segments.
	const std::vector<Segment*>& segments() const { return _segments; }

	/// Creates a dislocation segment and a reverse segment between two dislocation nodes.
	Segment* createSegmentPair(Node* node1, Node* node2, const ClusterVector& burgersVector);

	/// Returns the list of nodes.
	const std::vector<Node*>& nodes() const { return _nodes; }

	/// Creates a new dislocation node.
	Node* createNode();
	
private:

	/// The associated cluster graph.
	const std::shared_ptr<ClusterGraph> _clusterGraph;

	// Used for allocating nodes.
	MemoryPool<Node> _nodePool;

	// Used for allocating segments.
	MemoryPool<Segment> _segmentPool;

	/// The list of dislocation nodes.
	std::vector<Node*> _nodes;

	/// The list of dislocation segments.
	std::vector<Segment*> _segments;	
};

}	// End of namespace
}	// End of namespace
}	// End of namespace
