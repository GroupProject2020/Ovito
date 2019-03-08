///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2018) Alexander Stukowski
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
#include <plugins/crystalanalysis/objects/Microstructure.h>
#include <plugins/stdobj/simcell/SimulationCellObject.h>
#include <core/dataset/DataSet.h>
#include <core/dataset/pipeline/ModifierApplication.h>
#include "MicrostructureReplicateModifierDelegate.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

IMPLEMENT_OVITO_CLASS(MicrostructureReplicateModifierDelegate);

/******************************************************************************
* Determines whether this delegate can handle the given input data.
******************************************************************************/
bool MicrostructureReplicateModifierDelegate::OOMetaClass::isApplicableTo(const DataCollection& input) const
{
	return input.containsObject<Microstructure>();
}

/******************************************************************************
* Applies the modifier operation to the data in a pipeline flow state.
******************************************************************************/
PipelineStatus MicrostructureReplicateModifierDelegate::apply(Modifier* modifier, PipelineFlowState& state, TimePoint time, ModifierApplication* modApp, const std::vector<std::reference_wrapper<const PipelineFlowState>>& additionalInputs)
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

	for(const DataObject* obj : state.data()->objects()) {
		if(const Microstructure* existingMicrostructure = dynamic_object_cast<Microstructure>(obj)) {
			// For replication, a domain is required.
			if(!existingMicrostructure->domain()) continue;
			AffineTransformation simCell = existingMicrostructure->domain()->cellMatrix();
			auto pbcFlags = existingMicrostructure->domain()->pbcFlags();
			AffineTransformation inverseSimCell;
			if(!simCell.inverse(inverseSimCell))
				continue;

			// Create the output copy of the input microstructure.
			Microstructure* newMicrostructure = state.makeMutable(existingMicrostructure);
			MicrostructurePtr microstructure = newMicrostructure->modifiableStorage();

			// Shift existing vertices so that they form the first image at grid position (0,0,0).
			const Vector3 imageDelta = simCell * Vector3(newImages.minc.x(), newImages.minc.y(), newImages.minc.z());
			if(!imageDelta.isZero()) {
				for(Microstructure::Vertex* vertex : microstructure->vertices()) {
					vertex->setPos(vertex->pos() + imageDelta);
				}
			}

			// Replicate vertices.
			microstructure->reserveVertices(microstructure->vertexCount() * numCopies);
			size_t oldVertexCount = microstructure->vertexCount();
			for(int imageX = 0; imageX < nPBC[0]; imageX++) {
				for(int imageY = 0; imageY < nPBC[1]; imageY++) {
					for(int imageZ = 0; imageZ < nPBC[2]; imageZ++) {
						if(imageX == 0 && imageY == 0 && imageZ == 0) continue;
						// Shift vertex positions by the periodicity vector.
						const Vector3 imageDelta = simCell * Vector3(imageX, imageY, imageZ);
						for(size_t i = 0; i < oldVertexCount; i++) {
							microstructure->createVertex(microstructure->vertex(i)->pos() + imageDelta);
						}
					}
				}
			}
			OVITO_ASSERT(microstructure->vertexCount() == oldVertexCount * numCopies);

			// Replicate faces.
			microstructure->reserveFaces(microstructure->faceCount() * numCopies);
			size_t oldFaceCount = microstructure->faceCount();
			std::vector<Microstructure::Vertex*> newFaceVertices;
			for(int imageX = 0; imageX < nPBC[0]; imageX++) {
				for(int imageY = 0; imageY < nPBC[1]; imageY++) {
					for(int imageZ = 0; imageZ < nPBC[2]; imageZ++) {
						if(imageX == 0 && imageY == 0 && imageZ == 0) continue;
						size_t imageIndexShift = (imageX * nPBC[1] * nPBC[2]) + (imageY * nPBC[2]) + imageZ;
						// Copy faces.
						for(size_t i = 0; i < oldFaceCount; i++) {
							newFaceVertices.clear();
							Microstructure::Face* face = microstructure->face(i);
							Microstructure::Edge* edge = face->edges();
							OVITO_ASSERT(face->index() == i);
							do {
								size_t newVertexIndex = edge->vertex1()->index() + imageIndexShift * oldVertexCount;
								newFaceVertices.push_back(microstructure->vertex(newVertexIndex));
								edge = edge->nextFaceEdge();
							}
							while(edge != face->edges());
							Microstructure::Face* newface = microstructure->createFace(newFaceVertices.begin(), newFaceVertices.end());
							newface->setBurgersVector(face->burgersVector());
							newface->setCluster(face->cluster());
							newface->setEvenFace(face->isEvenFace());
							newface->setDislocationFace(face->isDislocationFace());
							newface->setSlipSurfaceFace(face->isSlipSurfaceFace());
						}
						// Copy face connectivity.
						for(size_t i = 0; i < oldFaceCount; i++) {
							Microstructure::Face* oldFace = microstructure->face(i);
							OVITO_ASSERT(oldFace->index() == i);
							Microstructure::Face* newFace = microstructure->face(i + imageIndexShift * oldFaceCount);
							if(oldFace->oppositeFace()) {
								newFace->setOppositeFace(microstructure->face(oldFace->oppositeFace()->index() + imageIndexShift * oldFaceCount));
							}
							Microstructure::Edge* oldEdge = oldFace->edges();
							Microstructure::Edge* newEdge = newFace->edges();
							do {
								if(oldEdge->oppositeEdge()) {
									size_t oppositeFaceIndex = oldEdge->oppositeEdge()->face()->index();
									OVITO_ASSERT(microstructure->face(oppositeFaceIndex) == oldEdge->oppositeEdge()->face());
									oppositeFaceIndex += imageIndexShift * oldFaceCount;
									Microstructure::Edge* newOppositeEdge = microstructure->face(oppositeFaceIndex)->findEdge(newEdge->vertex2(), newEdge->vertex1());
									OVITO_ASSERT(newOppositeEdge);
									if(!newEdge->oppositeEdge()) {
										newEdge->linkToOppositeEdge(newOppositeEdge);
									}
									else {
										OVITO_ASSERT(newEdge->oppositeEdge() == newOppositeEdge);
									}
								}
								if(oldEdge->nextManifoldEdge()) {
									size_t nextManifoldFaceIndex = oldEdge->nextManifoldEdge()->face()->index();
									OVITO_ASSERT(microstructure->face(nextManifoldFaceIndex) == oldEdge->nextManifoldEdge()->face());
									nextManifoldFaceIndex += imageIndexShift * oldFaceCount;
									Microstructure::Edge* newManifoldEdge = microstructure->face(nextManifoldFaceIndex)->findEdge(newEdge->vertex1(), newEdge->vertex2());
									OVITO_ASSERT(newManifoldEdge);
									newEdge->setNextManifoldEdge(newManifoldEdge);
								}
								oldEdge = oldEdge->nextFaceEdge();
								newEdge = newEdge->nextFaceEdge();
							}
							while(oldEdge != oldFace->edges());
						}
					}
				}
			}
			OVITO_ASSERT(microstructure->faceCount() == oldFaceCount * numCopies);

			if(pbcFlags[0] || pbcFlags[1] || pbcFlags[2]) {
				// Unwrap faces that cross a periodic boundary of the original cell.
				for(Microstructure::Face* face : microstructure->faces()) {
					Microstructure::Edge* edge = face->edges();
					Microstructure::Vertex* v1 = microstructure->vertex(edge->vertex1()->index() % oldVertexCount);
					Vector3I imageShift = Vector3I::Zero();
					do {
						Microstructure::Vertex* v2 = microstructure->vertex(edge->vertex2()->index() % oldVertexCount);
						Vector3 delta = inverseSimCell * (v2->pos() - v1->pos());
						for(size_t dim = 0; dim < 3; dim++) {
							if(pbcFlags[dim])
								imageShift[dim] -= (int)std::floor(delta[dim] + FloatType(0.5));
						}
						if(imageShift != Vector3I::Zero()) {
							size_t imageIndex = edge->vertex2()->index() / oldVertexCount;
							Point3I image(imageIndex / nPBC[1] / nPBC[2], (imageIndex / nPBC[2]) % nPBC[1], imageIndex % nPBC[2]);
							Point3I newImage(SimulationCell::modulo(image[0] + imageShift[0], nPBC[0]),
											SimulationCell::modulo(image[1] + imageShift[1], nPBC[1]),
											SimulationCell::modulo(image[2] + imageShift[2], nPBC[2]));
							size_t newImageIndex = (newImage[0] * nPBC[1] * nPBC[2]) + (newImage[1] * nPBC[2]) + newImage[2];
							Microstructure::Vertex* new_v2 = microstructure->vertex(newImageIndex * oldVertexCount + v2->index());
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
				for(Microstructure::Face* face : microstructure->faces()) {
					// Check if link to opposite face is consistent.
					Microstructure::Edge* edge = face->edges();
					OVITO_ASSERT(edge != nullptr);
					if(face->oppositeFace() && !face->oppositeFace()->findEdge(edge->vertex2(), edge->vertex1())) {
						// Link is not consistent, find the right partner face among the periodic images of the original partner face.
						size_t oppositeFaceIndex = face->oppositeFace()->index() % oldFaceCount;
						face->setOppositeFace(nullptr);
						for(size_t i = 0; i < numCopies; i++) {
							Microstructure::Face* newOppositeFace = microstructure->face(oppositeFaceIndex + i * oldFaceCount);
							if(newOppositeFace->findEdge(edge->vertex2(), edge->vertex1())) {
								face->setOppositeFace(newOppositeFace);
								break;
							}
						}
						OVITO_ASSERT(face->oppositeFace() != nullptr);
						face->oppositeFace()->setOppositeFace(face);
					}
					// Repair connectivity of face edges.
					do {
						// Check if edge is still linked to the correct opposite edge.
						if(edge->oppositeEdge() && edge->oppositeEdge()->vertex2() != edge->vertex1()) {
							// If opposite edge link is inconsistent, find new partner edge.
							OVITO_ASSERT((edge->oppositeEdge()->vertex2()->index() % oldVertexCount) == (edge->vertex1()->index() % oldVertexCount));
							size_t oppositeFaceIndex = edge->oppositeEdge()->face()->index() % oldFaceCount;
							edge->setOppositeEdge(nullptr);
							for(size_t i = 0; i < numCopies; i++) {
								Microstructure::Edge* newOppositeEdge = microstructure->face(oppositeFaceIndex + i * oldFaceCount)->findEdge(edge->vertex2(), edge->vertex1());
								if(newOppositeEdge) {
									edge->setOppositeEdge(newOppositeEdge);
									break;
								}
							}
							OVITO_ASSERT(edge->oppositeEdge());
							OVITO_ASSERT(edge->oppositeEdge()->vertex2() == edge->vertex1());
						}
						if(edge->nextManifoldEdge() && edge->nextManifoldEdge()->vertex2() != edge->vertex2()) {
							OVITO_ASSERT((edge->nextManifoldEdge()->vertex2()->index() % oldVertexCount) == (edge->vertex2()->index() % oldVertexCount));
							size_t nextManifoldFaceIndex = edge->nextManifoldEdge()->face()->index() % oldFaceCount;
							edge->setNextManifoldEdge(nullptr);
							for(size_t i = 0; i < numCopies; i++) {
								Microstructure::Edge* newManifoldEdge = microstructure->face(nextManifoldFaceIndex + i * oldFaceCount)->findEdge(edge->vertex1(), edge->vertex2());
								if(newManifoldEdge) {
									edge->setNextManifoldEdge(newManifoldEdge);
									break;
								}
							}
							OVITO_ASSERT(edge->nextManifoldEdge());
							OVITO_ASSERT(edge->nextManifoldEdge()->vertex1() == edge->vertex1());
							OVITO_ASSERT(edge->nextManifoldEdge()->vertex2() == edge->vertex2());
						}
						edge = edge->nextFaceEdge();
					}
					while(edge != face->edges());
				}
			}

			// Extend the periodic domain the microstructure is embedded in.
			simCell.translation() += (FloatType)newImages.minc.x() * simCell.column(0);
			simCell.translation() += (FloatType)newImages.minc.y() * simCell.column(1);
			simCell.translation() += (FloatType)newImages.minc.z() * simCell.column(2);
			simCell.column(0) *= (newImages.sizeX() + 1);
			simCell.column(1) *= (newImages.sizeY() + 1);
			simCell.column(2) *= (newImages.sizeZ() + 1);
			newMicrostructure->mutableDomain()->setCellMatrix(simCell);

			microstructure->makeContinuousDislocationLines();
		}
	}

	return PipelineStatus::Success;
}

}	// End of namespace
}	// End of namespace
}	// End of namespace
