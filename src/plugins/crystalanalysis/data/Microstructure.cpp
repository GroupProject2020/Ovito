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
#include "Microstructure.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

/******************************************************************************
* Copy constructor.
******************************************************************************/
Microstructure::Microstructure(const Microstructure& other) : MicrostructureBase(other), _clusterGraph(other.clusterGraph())
{
    // The base class constructor has already copied the face and vertex lists.
    // Here we only have to copy the additional data associated with indiviual faces 
    // and vertices.

    // Copy additional face data.
    auto source_face = other.faces().begin();
    for(auto& dest_face : faces()) {
        dest_face->setBurgersVector((*source_face)->burgersVector());
        dest_face->setCluster((*source_face)->cluster());
        dest_face->setDislocationFace((*source_face)->isDislocationFace());
        dest_face->setOppositeFace(faces()[(*source_face)->oppositeFace()->index()]);
    }
}

/******************************************************************************
* Create a dislocation line segment between two nodes.
******************************************************************************/
Microstructure::Edge* Microstructure::createDislocationSegment(Vertex* vertex1, Vertex* vertex2, const Vector3& burgersVector, Cluster* cluster)
{
    Face* face1 = createFace({vertex1, vertex2});
    Face* face2 = createFace({vertex2, vertex1});
    face1->setEvenFace(true);
    face1->edges()->linkToOppositeEdge(face2->edges());
    face1->setOppositeFace(face2);
    face2->setOppositeFace(face1);
    face1->setBurgersVector(burgersVector);
    face2->setBurgersVector(-burgersVector);
    face1->setCluster(cluster);
    face2->setCluster(cluster);
    face1->setDislocationFace(true);
    face2->setDislocationFace(true);
    OVITO_ASSERT(face1->edges()->vertex1() == vertex1);
    OVITO_ASSERT(face1->edges()->vertex2() == vertex2);
    OVITO_ASSERT(face2->edges()->vertex1() == vertex2);
    OVITO_ASSERT(face2->edges()->vertex2() == vertex1);
}

/******************************************************************************
* Merges virtual dislocation faces to build continuous lines from individual dislocation segments.
******************************************************************************/
void Microstructure::makeContinuousDislocationLines()
{
    size_t joined = 0;

    // Process each vertex in the microstructure.
    for(Vertex* vertex : vertices()) {
        
        // Look only for 2-nodes, which are part of continues dislocation lines.
        if(vertex->countDislocationArms() == 2) {

            // Gather the two dislocation arms.
            Edge* arm1 = nullptr;
            Edge* arm2 = nullptr;
            for(Edge* e = vertex->edges(); e != nullptr; e = e->nextVertexEdge()) {
                if(e->isDislocation()) {
                    if(!arm1) arm1 = e;
                    else arm2 = e;
                }
            }
            OVITO_ASSERT(arm1 && arm2);

            // All segments of a continuous dislocation line must be embedded in the same crystal.
            if(arm1->cluster() == arm2->cluster()) {
                
                // Verify that Burgers vector conservation is fulfilled at the 2-node.
                OVITO_ASSERT(arm1->burgersVector().equals(-arm2->burgersVector()));

                // These conditions must always be fulfilled:
                OVITO_ASSERT(arm1->prevFaceEdge()->vertex2() == vertex);
                OVITO_ASSERT(arm2->prevFaceEdge()->vertex2() == vertex);
                OVITO_ASSERT(arm1->oppositeEdge()->face() == arm1->face()->oppositeFace());
                OVITO_ASSERT(arm2->oppositeEdge()->face() == arm2->face()->oppositeFace());
                OVITO_ASSERT(arm1->prevFaceEdge()->vertex1() == arm1->oppositeEdge()->nextFaceEdge()->vertex2());
                OVITO_ASSERT(arm2->prevFaceEdge()->vertex1() == arm2->oppositeEdge()->nextFaceEdge()->vertex2());

                // Test if the two pairs of virtual faces have already been joined before.
                if(arm1->face() != arm2->oppositeEdge()->face()) {
                    
                    Edge* virtualArm1 = arm1->oppositeEdge()->nextFaceEdge();
                    Edge* virtualArm2 = arm2->oppositeEdge()->nextFaceEdge();

                    // Rewire first edge sequence at the vertex.
                    arm1->prevFaceEdge()->setNextFaceEdge(virtualArm2);
                    virtualArm2->setPrevFaceEdge(arm1->prevFaceEdge());
                    arm1->setPrevFaceEdge(arm2->oppositeEdge());
                    arm2->oppositeEdge()->setNextFaceEdge(arm1);

                    // Rewire second edge sequence at the vertex.
                    arm2->prevFaceEdge()->setNextFaceEdge(virtualArm1);
                    virtualArm1->setPrevFaceEdge(arm2->prevFaceEdge());
                    arm2->setPrevFaceEdge(arm1->oppositeEdge());
                    arm1->oppositeEdge()->setNextFaceEdge(arm2);

                    // Make sure the first edge of a face is always the one at the beginning of the 
                    // corresponding continuous dislocation line.
                    arm1->face()->setEdges(virtualArm2->face()->edges());

                    // Mark one pair of faces for deletion.
            		arm2->oppositeEdge()->face()->markForDeletion();
                    arm2->face()->markForDeletion();
                    
                    // Transfer edges of the faces that are going to be removed to the remaining faces.
		            for(Edge* currentEdge = virtualArm2; currentEdge != arm1; currentEdge = currentEdge->nextFaceEdge()) {
			            currentEdge->setFace(arm1->face());
            		}
		            for(Edge* currentEdge = arm2; currentEdge != virtualArm1; currentEdge = currentEdge->nextFaceEdge()) {
			            currentEdge->setFace(arm1->oppositeEdge()->face());
            		}

                    joined++;
                }
            }
        }
    }

    // Delete the faces from the mesh that have been marked for deletion above.
    removeMarkedFaces();
}


}	// End of namespace
}	// End of namespace
}	// End of namespace
