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

#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include "MicrostructureObject.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

IMPLEMENT_OVITO_CLASS(MicrostructureObject);

/******************************************************************************
* Constructor that adopts the data fields from the given microstructure data object. 
******************************************************************************/
Microstructure::Microstructure(const MicrostructureObject* mo) :
    _isMutable(false),
    _topology(mo->topology()),
    _vertexCoords(const_pointer_cast<PropertyStorage>(mo->vertices()->getPropertyStorage(SurfaceMeshVertices::PositionProperty))),
    _burgersVectors(const_pointer_cast<PropertyStorage>(mo->faces()->getPropertyStorage(SurfaceMeshFaces::BurgersVectorProperty))),
    _faceTypes(const_pointer_cast<PropertyStorage>(mo->faces()->getPropertyStorage(SurfaceMeshFaces::FaceTypeProperty))),
    _faceRegions(const_pointer_cast<PropertyStorage>(mo->faces()->getPropertyStorage(SurfaceMeshFaces::RegionProperty))),
    _cell(mo->domain()->data())
{
}

/******************************************************************************
* Create a dislocation line segment between two nodal points.
******************************************************************************/
Microstructure::edge_index Microstructure::createDislocationSegment(vertex_index vertex1, vertex_index vertex2, const Vector3& burgersVector, int region)
{
    OVITO_ASSERT(_isMutable);
    face_index face1 = topology()->createFace({vertex1, vertex2});
    face_index face2 = topology()->createFace({vertex2, vertex1});
    topology()->linkOppositeEdges(topology()->firstFaceEdge(face1), topology()->firstFaceEdge(face2));
    topology()->linkOppositeFaces(face1, face2);
	setBurgersVector(face1,  burgersVector);
    setBurgersVector(face2, -burgersVector);
    setFaceRegion(face1, region);
    setFaceRegion(face2, region);
    setFaceType(face1, DISLOCATION);
    setFaceType(face2, DISLOCATION);
	return firstFaceEdge(face1);
}

/******************************************************************************
* Merges virtual dislocation faces to build continuous lines from individual 
* dislocation segments.
******************************************************************************/
void Microstructure::makeContinuousDislocationLines()
{
    OVITO_ASSERT(_isMutable);
    size_t joined = 0;

    // Process each vertex in the microstructure.
	auto vertexCount = topology()->vertexCount();
    for(vertex_index vertex = 0; vertex < vertexCount; vertex++) {
        
        // Look only for 2-nodes, which are part of continues dislocation lines.
        if(countDislocationArms(vertex) == 2) {

            // Gather the two dislocation arms.
            edge_index arm1 = HalfEdgeMesh::InvalidIndex;
            edge_index arm2 = HalfEdgeMesh::InvalidIndex;
            for(edge_index e = firstVertexEdge(vertex); e != HalfEdgeMesh::InvalidIndex; e = nextVertexEdge(e)) {
                if(isDislocationEdge(e)) {
                    if(!arm1) arm1 = e;
                    else arm2 = e;
                }
            }
            OVITO_ASSERT(arm1 != HalfEdgeMesh::InvalidIndex && arm2 != HalfEdgeMesh::InvalidIndex);

            // The segments of a continuous dislocation line must be embedded in the same crystallite.
            if(edgeRegion(arm1) == edgeRegion(arm2)) {
                
                // Verify that Burgers vector conservation is fulfilled at the 2-node.
                OVITO_ASSERT(burgersVector(arm1).equals(-burgersVector(arm2)));

                // These conditions must always be fulfilled:
                OVITO_ASSERT(vertex2(prevFaceEdge(arm1)) == vertex);
                OVITO_ASSERT(vertex2(prevFaceEdge(arm2)) == vertex);
                OVITO_ASSERT(adjacentFace(oppositeEdge(arm1)) == oppositeFace(adjacentFace(arm1)));
                OVITO_ASSERT(adjacentFace(oppositeEdge(arm2)) == oppositeFace(adjacentFace(arm2)));
                OVITO_ASSERT(vertex1(prevFaceEdge(arm1)) == vertex2(nextFaceEdge(oppositeEdge(arm1))));
                OVITO_ASSERT(vertex1(prevFaceEdge(arm2)) == vertex2(nextFaceEdge(oppositeEdge(arm2))));

                // Test if the two pairs of virtual faces have already been joined.
                if(adjacentFace(arm1) != adjacentFace(oppositeEdge(arm2))) {
                    
                    edge_index virtualArm1 = nextFaceEdge(oppositeEdge(arm1));
                    edge_index virtualArm2 = nextFaceEdge(oppositeEdge(arm2));

                    // Rewire first edge sequence at the vertex.
                    topology()->setNextFaceEdge(prevFaceEdge(arm1), virtualArm2);
                    topology()->setPrevFaceEdge(virtualArm2, prevFaceEdge(arm1));
                    topology()->setPrevFaceEdge(arm1, oppositeEdge(arm2));
                    topology()->setNextFaceEdge(oppositeEdge(arm2), arm1);

                    // Rewire second edge sequence at the vertex.
                    topology()->setNextFaceEdge(prevFaceEdge(arm2), virtualArm1);
                    topology()->setPrevFaceEdge(virtualArm1, prevFaceEdge(arm2));
                    topology()->setPrevFaceEdge(arm2, oppositeEdge(arm1));
                    topology()->setNextFaceEdge(oppositeEdge(arm1), arm2);

                    // Make sure the first edge of a face is always the one at the beginning of the 
                    // corresponding continuous dislocation line.
                    topology()->setFirstFaceEdge(adjacentFace(arm1), firstFaceEdge(adjacentFace(virtualArm2)));

                    // Transfer edges of the faces that are going to be removed to the remaining faces.
		            for(edge_index currentEdge = virtualArm2; currentEdge != arm1; currentEdge = nextFaceEdge(currentEdge)) {
			            topology()->setAdjacentFace(currentEdge, adjacentFace(arm1));
            		}
		            for(edge_index currentEdge = arm2; currentEdge != virtualArm1; currentEdge = nextFaceEdge(currentEdge)) {
			            topology()->setAdjacentFace(currentEdge, adjacentFace(oppositeEdge(arm1)));
            		}

                    // Delete one pair of faces from the mesh.
                    face_index delFace1 = adjacentFace(oppositeEdge(arm2));
                    face_index delFace2 = adjacentFace(arm2);
            		topology()->deleteFace(delFace1);
                    if(delFace2 == topology()->faceCount()) delFace2 = delFace1;
                    topology()->deleteFace(delFace2);
                    
                    joined++;
                }
            }
        }
    }
}

/*************************************************************************************
* Aligns the orientation of slip facets and builds contiguous two-dimensional manifolds 
* of maximum extent, i.e. slip surfaces with constant slip vector.
**************************************************************************************/
void Microstructure::makeSlipSurfaces()
{
    OVITO_ASSERT(_isMutable);
#if 0
	// We assume in the following that every slip facet half-edge has an opposite half-edge.

	// Reset flags.
	for(face_index face : faces()) {
		if(face->isSlipSurfaceFace())
			face->setEvenFace(false);
	}
	
	// Build contiguous surfaces with constant slip vector.
	std::deque<face_index> toVisit;
	auto faceCount = topology()->faceCount();
	for(face_index seedFace = 0; seedFace < faceCount; seedFace++) {

		// Find a first slip surface face which hasn't been aligned yet.
		if(!isSlipSurfaceFace(seedFace)) continue;
		if(seedFace->isEvenFace() || seedFace->oppositeFace()->isEvenFace()) continue;

		// Starting at the current seed face, recursively visit all neighboring faces
		// and align them. Stop at triple lines and slip surface boundaries.
		seedFace->setEvenFace(true);
		toVisit.push_back(seedFace);
		do {
			face_index face = toVisit.front();
			toVisit.pop_front();
			edge_index edge = firstFaceEdge(face);
			do {
				OVITO_ASSERT(hasOppositeEdge(edge));
				Microstructure::face_index neighborFace = adjacentFace(oppositeEdge(edge));
				OVITO_ASSERT(neighborFace->isSlipSurfaceFace());
				if(!neighborFace->isEvenFace() && !neighborFace->oppositeFace()->isEvenFace()) {
					if(burgersVector(neighborFace).equals(burgersVector(face)) && neighborFace->cluster() == face->cluster()) {
						neighborFace->setEvenFace(true);
						toVisit.push_back(neighborFace);
					}
				}
				edge = edge->nextFaceEdge();
			}
			while(edge != face->edges());
		}
		while(!toVisit.empty());
	}
#endif
}

}	// End of namespace
}	// End of namespace
}	// End of namespace
