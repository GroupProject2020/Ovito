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

#include <plugins/mesh/Mesh.h>
#include <plugins/mesh/surface/SurfaceMesh.h>
#include <core/dataset/data/simcell/SimulationCellObject.h>
#include <core/dataset/pipeline/InputHelper.h>
#include <core/dataset/pipeline/OutputHelper.h>
#include <core/dataset/DataSet.h>
#include "SurfaceMeshReplicateModifierDelegate.h"

namespace Ovito { namespace Mesh {

IMPLEMENT_OVITO_CLASS(SurfaceMeshReplicateModifierDelegate);

/******************************************************************************
* Determines whether this delegate can handle the given input data.
******************************************************************************/
bool SurfaceMeshReplicateModifierDelegate::OOMetaClass::isApplicableTo(const PipelineFlowState& input) const
{
	return input.findObject<SurfaceMesh>() != nullptr;
}

/******************************************************************************
* Applies the modifier operation to the data in a pipeline flow state.
******************************************************************************/
PipelineStatus SurfaceMeshReplicateModifierDelegate::apply(Modifier* modifier, const PipelineFlowState& input, PipelineFlowState& output, TimePoint time, ModifierApplication* modApp)
{
	ReplicateModifier* mod = static_object_cast<ReplicateModifier>(modifier);
	
	std::array<int,3> nPBC;
	nPBC[0] = std::max(mod->numImagesX(),1);
	nPBC[1] = std::max(mod->numImagesY(),1);
	nPBC[2] = std::max(mod->numImagesZ(),1);

	size_t numCopies = nPBC[0] * nPBC[1] * nPBC[2];
	if(numCopies <= 1)
		return PipelineStatus::Success;

	Box3I newImages = mod->replicaRange();

	InputHelper ih(dataset(), input);
	OutputHelper oh(dataset(), output);

	for(DataObject* obj : output.objects()) {
		if(SurfaceMesh* existingSurface = dynamic_object_cast<SurfaceMesh>(obj)) {
			// For replication, a domain is required.
			if(!existingSurface->domain()) continue;
			AffineTransformation simCell = existingSurface->domain()->cellMatrix();
			auto pbcFlags = existingSurface->domain()->pbcFlags();
			AffineTransformation inverseSimCell;
			if(!simCell.inverse(inverseSimCell))
				continue;
			
			// Create the output copy of the input surface.
			SurfaceMesh* newSurface = oh.cloneIfNeeded(existingSurface);
			SurfaceMeshPtr mesh = newSurface->modifiableStorage();
			OVITO_ASSERT(mesh->isClosed());

			// Shift existing vertices so that they form the first image at grid position (0,0,0).
			const Vector3 imageDelta = simCell * Vector3(newImages.minc.x(), newImages.minc.y(), newImages.minc.z());
			if(!imageDelta.isZero()) {
				for(HalfEdgeMesh<>::Vertex* vertex : mesh->vertices()) {
					vertex->setPos(vertex->pos() + imageDelta);
				}
			}

			// Replicate vertices.
			mesh->reserveVertices(mesh->vertexCount() * numCopies);
			size_t oldVertexCount = mesh->vertexCount();
			for(int imageX = 0; imageX < nPBC[0]; imageX++) {
				for(int imageY = 0; imageY < nPBC[1]; imageY++) {
					for(int imageZ = 0; imageZ < nPBC[2]; imageZ++) {
						if(imageX == 0 && imageY == 0 && imageZ == 0) continue;
						// Shift vertex positions by the periodicity vector.
						const Vector3 imageDelta = simCell * Vector3(imageX, imageY, imageZ);
						for(size_t i = 0; i < oldVertexCount; i++) {
							mesh->createVertex(mesh->vertex(i)->pos() + imageDelta);
						}
					}
				}
			}
			OVITO_ASSERT(mesh->vertexCount() == oldVertexCount * numCopies);

			// Replicate faces.
			mesh->reserveFaces(mesh->faceCount() * numCopies);
			size_t oldFaceCount = mesh->faceCount();
			std::vector<HalfEdgeMesh<>::Vertex*> newFaceVertices;
			for(int imageX = 0; imageX < nPBC[0]; imageX++) {
				for(int imageY = 0; imageY < nPBC[1]; imageY++) {
					for(int imageZ = 0; imageZ < nPBC[2]; imageZ++) {
						if(imageX == 0 && imageY == 0 && imageZ == 0) continue;
						size_t imageIndexShift = (imageX * nPBC[1] * nPBC[2]) + (imageY * nPBC[2]) + imageZ;
						// Copy faces.
						for(size_t i = 0; i < oldFaceCount; i++) {
							newFaceVertices.clear();
							HalfEdgeMesh<>::Face* face = mesh->face(i);
							HalfEdgeMesh<>::Edge* edge = face->edges();
							OVITO_ASSERT(face->index() == i);
							do {
								size_t newVertexIndex = edge->vertex1()->index() + imageIndexShift * oldVertexCount;
								newFaceVertices.push_back(mesh->vertex(newVertexIndex));
								edge = edge->nextFaceEdge();
							}
							while(edge != face->edges());
							mesh->createFace(newFaceVertices.begin(), newFaceVertices.end());
						}
						// Copy face connectivity.
						for(size_t i = 0; i < oldFaceCount; i++) {
							HalfEdgeMesh<>::Face* oldFace = mesh->face(i);
							HalfEdgeMesh<>::Face* newFace = mesh->face(i + imageIndexShift * oldFaceCount);
							HalfEdgeMesh<>::Edge* oldEdge = oldFace->edges();
							HalfEdgeMesh<>::Edge* newEdge = newFace->edges();
							do {
								size_t oppositeFaceIndex = oldEdge->oppositeEdge()->face()->index();
								OVITO_ASSERT(mesh->face(oppositeFaceIndex) == oldEdge->oppositeEdge()->face());
								oppositeFaceIndex += imageIndexShift * oldFaceCount;
								HalfEdgeMesh<>::Edge* newOppositeEdge = mesh->face(oppositeFaceIndex)->findEdge(newEdge->vertex2(), newEdge->vertex1());
								OVITO_ASSERT(newOppositeEdge);
								if(!newEdge->oppositeEdge()) {
									newEdge->linkToOppositeEdge(newOppositeEdge);
								}
								else {
									OVITO_ASSERT(newEdge->oppositeEdge() == newOppositeEdge);
								}
								oldEdge = oldEdge->nextFaceEdge();
								newEdge = newEdge->nextFaceEdge();
							}
							while(oldEdge != oldFace->edges());
						}
					}
				}
			}
			OVITO_ASSERT(mesh->faceCount() == oldFaceCount * numCopies);
			OVITO_ASSERT(mesh->isClosed());

			if(pbcFlags[0] || pbcFlags[1] || pbcFlags[2]) {
				// Unwrap faces that crossed a periodic boundary in the original cell.
				for(HalfEdgeMesh<>::Face* face : mesh->faces()) {
					HalfEdgeMesh<>::Edge* edge = face->edges();
					HalfEdgeMesh<>::Vertex* v1 = mesh->vertex(edge->vertex1()->index() % oldVertexCount);
					Vector3I imageShift = Vector3I::Zero();
					do {
						HalfEdgeMesh<>::Vertex* v2 = mesh->vertex(edge->vertex2()->index() % oldVertexCount);
						Vector3 delta = inverseSimCell * (v2->pos() - v1->pos());
						for(size_t dim = 0; dim < 3; dim++) {
							if(pbcFlags[dim])
								imageShift[dim] -= (int)std::floor(delta[dim] + FloatType(0.5));
						}
						if(!imageShift.isZero()) {
							size_t imageIndex = edge->vertex2()->index() / oldVertexCount;
							Point3I image(imageIndex / nPBC[1] / nPBC[2], (imageIndex / nPBC[2]) % nPBC[1], imageIndex % nPBC[2]);
							Point3I newImage(SimulationCell::modulo(image[0] + imageShift[0], nPBC[0]),
											SimulationCell::modulo(image[1] + imageShift[1], nPBC[1]),
											SimulationCell::modulo(image[2] + imageShift[2], nPBC[2]));
							size_t newImageIndex = (newImage[0] * nPBC[1] * nPBC[2]) + (newImage[1] * nPBC[2]) + newImage[2];
							HalfEdgeMesh<>::Vertex* new_v2 = mesh->vertex(newImageIndex * oldVertexCount + v2->index());
							if(new_v2 != edge->vertex2()) {
								edge->vertex2()->transferEdgeToVertex(edge->nextFaceEdge(), new_v2, false);
								edge->setVertex2(new_v2);
								OVITO_ASSERT(edge->vertex2() == new_v2);
							}
						}
						v1 = v2;
						edge = edge->nextFaceEdge();
					}
					while(edge != face->edges());
				}

				// Since faces that cross a periodic boundary can end up in different images,
				// we now need to "repair" the face connectivity.
				for(HalfEdgeMesh<>::Face* face : mesh->faces()) {
					HalfEdgeMesh<>::Edge* edge = face->edges();
					do {
						if(edge->oppositeEdge()->vertex2() != edge->vertex1()) {
							OVITO_ASSERT((edge->oppositeEdge()->vertex2()->index() % oldVertexCount) == (edge->vertex1()->index() % oldVertexCount));
							size_t oppositeFaceIndex = edge->oppositeEdge()->face()->index() % oldFaceCount;
							edge->setOppositeEdge(nullptr);
							for(size_t i = 0; i < numCopies; i++) {
								HalfEdgeMesh<>::Edge* newOppositeEdge = mesh->face(oppositeFaceIndex + i * oldFaceCount)->findEdge(edge->vertex2(), edge->vertex1());														
								if(newOppositeEdge) {
									edge->setOppositeEdge(newOppositeEdge);
									break;
								}
							}
							OVITO_ASSERT(edge->oppositeEdge());
							OVITO_ASSERT(edge->oppositeEdge()->vertex2() == edge->vertex1());
						}
						edge = edge->nextFaceEdge();
					}
					while(edge != face->edges());
				}
			}
			OVITO_ASSERT(mesh->isClosed());
			
			// Extend the periodic domain the surface is embedded in.
			simCell.translation() += (FloatType)newImages.minc.x() * simCell.column(0);
			simCell.translation() += (FloatType)newImages.minc.y() * simCell.column(1);
			simCell.translation() += (FloatType)newImages.minc.z() * simCell.column(2);
			simCell.column(0) *= (newImages.sizeX() + 1);
			simCell.column(1) *= (newImages.sizeY() + 1);
			simCell.column(2) *= (newImages.sizeZ() + 1);
			newSurface->domain()->setCellMatrix(simCell);
		}
	}

	return PipelineStatus::Success;
}

}	// End of namespace
}	// End of namespace
