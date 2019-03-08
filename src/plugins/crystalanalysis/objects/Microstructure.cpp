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
#include "Microstructure.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

IMPLEMENT_OVITO_CLASS(Microstructure);

/******************************************************************************
* Constructor creating an empty microstructure.
******************************************************************************/
MicrostructureData::MicrostructureData(const SimulationCell& cell) : SurfaceMeshData(cell)
{
    createFaceProperty(SurfaceMeshFaces::BurgersVectorProperty);
    createFaceProperty(SurfaceMeshFaces::FaceTypeProperty);
    createRegionProperty(SurfaceMeshRegions::PhaseProperty);
    OVITO_ASSERT(burgersVectors());
    OVITO_ASSERT(faceTypes());
}

/******************************************************************************
* Constructor that adopts the data from the given pipeline data object into
* this structure.
******************************************************************************/
MicrostructureData::MicrostructureData(const SurfaceMesh* mo) : SurfaceMeshData(mo)
{
    OVITO_ASSERT(faceTypes());
    OVITO_ASSERT(burgersVectors());
}

/******************************************************************************
* Create a dislocation line segment between two nodal points.
******************************************************************************/
MicrostructureData::edge_index MicrostructureData::createDislocationSegment(vertex_index vertex1, vertex_index vertex2, const Vector3& burgersVector, region_index region)
{
    face_index face1 = createFace({vertex1, vertex2}, region, DISLOCATION,  burgersVector);
    face_index face2 = createFace({vertex2, vertex1}, region, DISLOCATION, -burgersVector);
    // Note: We are intentionally linking only one pair of opposite half-edges here.
    // The other two face edges remain without an opposite edge partner
    // to mark them as as virtual dislocation segments, which exist only to close the face boundaries.
    linkOppositeEdges(firstFaceEdge(face1), firstFaceEdge(face2));
    topology()->linkOppositeFaces(face1, face2);
    return firstFaceEdge(face1);
}

/******************************************************************************
* Merges virtual dislocation faces to build continuous lines from individual
* dislocation segments.
******************************************************************************/
void MicrostructureData::makeContinuousDislocationLines()
{
    size_t joined = 0;

    // Process each vertex in the microstructure.
    auto vertexCount = this->vertexCount();
    for(vertex_index vertex = 0; vertex < vertexCount; vertex++) {

        // Specifically look for 2-nodes which are part of continuous dislocation lines.
        int armCount = 0;
        edge_index arms[3];
        for(edge_index e = firstVertexEdge(vertex); e != HalfEdgeMesh::InvalidIndex; e = nextVertexEdge(e)) {
            if(isPhysicalDislocationEdge(e)) {
                arms[armCount++] = e;
                if(armCount == 3) break;
            }
        }
        if(armCount == 2) {
            edge_index arm1 = arms[0];
            edge_index arm2 = arms[1];

            // The segments of a continuous dislocation line must be embedded in the same crystallite.
            if(edgeRegion(arm1) == edgeRegion(arm2)) {

                // Verify that Burgers vector conservation is fulfilled at the 2-node.
                OVITO_ASSERT(burgersVector(adjacentFace(arm1)).equals(-burgersVector(adjacentFace(arm2))));

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

                    face_index delFace1 = adjacentFace(oppositeEdge(arm2));
                    face_index delFace2 = adjacentFace(arm2);
                    face_index keepFace1 = adjacentFace(arm1);
                    face_index keepFace2 = adjacentFace(oppositeEdge(arm1));
                    OVITO_ASSERT(oppositeFace(delFace1) == delFace2);
                    OVITO_ASSERT(burgersVector(delFace1).equals(-burgersVector(delFace2)));
                    OVITO_ASSERT(oppositeFace(keepFace1) == keepFace2);
                    OVITO_ASSERT(burgersVector(keepFace1).equals(-burgersVector(keepFace2)));

                    // Make sure the first edge of a face is always the one at the beginning of the
                    // corresponding continuous dislocation line.
                    topology()->setFirstFaceEdge(keepFace1, firstFaceEdge(adjacentFace(virtualArm2)));

                    // Transfer edges of the faces that are going to be removed to the remaining faces.
                    for(edge_index currentEdge = virtualArm2; currentEdge != arm1; currentEdge = nextFaceEdge(currentEdge)) {
                        topology()->setAdjacentFace(currentEdge, keepFace1);
                    }
                    for(edge_index currentEdge = arm2; currentEdge != virtualArm1; currentEdge = nextFaceEdge(currentEdge)) {
                        topology()->setAdjacentFace(currentEdge, keepFace2);
                    }

                    // Delete one pair of faces from the mesh.
                    topology()->setFirstFaceEdge(delFace1, HalfEdgeMesh::InvalidIndex);
                    topology()->setFirstFaceEdge(delFace2, HalfEdgeMesh::InvalidIndex);
                    topology()->unlinkFromOppositeFace(delFace1);

                    // Make sure we delete the faces in an ordered fashion, starting from the back.
                    if(delFace1 < delFace2) std::swap(delFace1, delFace2);
                    deleteFace(delFace1);
                    deleteFace(delFace2);

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
void MicrostructureData::makeSlipSurfaces()
{
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
                MicrostructureData::face_index neighborFace = adjacentFace(oppositeEdge(edge));
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
