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

#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include "NodalDislocationNetwork.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

/******************************************************************************
* Creates a new dislocation node.
******************************************************************************/
NodalDislocationNetwork::Node* NodalDislocationNetwork::createNode()
{
	Node* node = _nodePool.construct();
	node->index = _nodes.size();
	_nodes.push_back(node);
	return node;
}

/******************************************************************************
* Creates a dislocation segment and a reverse segment between two dislocation nodes.
******************************************************************************/
NodalDislocationNetwork::Segment* NodalDislocationNetwork::createSegmentPair(Node* node1, Node* node2, const ClusterVector& burgersVector)
{
	Segment* segment1 = _segmentPool.construct();
	Segment* segment2 = _segmentPool.construct();
	_segments.push_back(segment1);
	_segments.push_back(segment2);
	segment1->burgersVector = burgersVector;
	segment2->burgersVector = -burgersVector;
	segment1->node2 = node2;
	segment2->node2 = node1;
	segment1->reverseSegment = segment2;
	segment2->reverseSegment = segment1;
	segment1->nextNodeSegment = node1->segments;
	node1->segments = segment1;
	segment2->nextNodeSegment = node2->segments;
	node2->segments = segment2;
	return segment1;
}

}	// End of namespace
}	// End of namespace
}	// End of namespace
