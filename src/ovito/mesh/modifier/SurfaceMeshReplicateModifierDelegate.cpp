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

#include <ovito/mesh/Mesh.h>
#include <ovito/mesh/surface/SurfaceMesh.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>
#include "SurfaceMeshReplicateModifierDelegate.h"

namespace Ovito { namespace Mesh {

IMPLEMENT_OVITO_CLASS(SurfaceMeshReplicateModifierDelegate);

/******************************************************************************
* Applies the modifier operation to the data in a pipeline flow state.
******************************************************************************/
PipelineStatus SurfaceMeshReplicateModifierDelegate::apply(Modifier* modifier, PipelineFlowState& state, TimePoint time, ModifierApplication* modApp, const std::vector<std::reference_wrapper<const PipelineFlowState>>& additionalInputs)
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
		if(const SurfaceMesh* existingSurface = dynamic_object_cast<SurfaceMesh>(obj)) {
			// For replication, a domain is always required.
			if(!existingSurface->domain()) continue;
			AffineTransformation simCell = existingSurface->domain()->cellMatrix();
			auto pbcFlags = existingSurface->domain()->pbcFlags();
			AffineTransformation inverseSimCell;
			if(!simCell.inverse(inverseSimCell))
				continue;

			// Make sure surface mesh is in a good state.
			existingSurface->verifyMeshIntegrity();

			// Create a copy of the input surface.
			SurfaceMesh* newSurface = state.makeMutable(existingSurface);

			// Create a copy of the mesh topology.
			HalfEdgeMeshPtr mesh = newSurface->modifiableTopology();

			// Create a copy of the vertices sub-object.
			SurfaceMeshVertices* newVertices = newSurface->makeVerticesMutable();

			// Extend the property arrays.
			size_t oldVertexCount = newVertices->elementCount();
			size_t newVertexCount = oldVertexCount * numCopies;
			newVertices->replicate(numCopies);

			// Shift vertex positions by the periodicity vector.
			PropertyObject* positionProperty = newVertices->expectMutableProperty(SurfaceMeshVertices::PositionProperty);
			Point3* p = positionProperty->dataPoint3();
			for(int imageX = newImages.minc.x(); imageX <= newImages.maxc.x(); imageX++) {
				for(int imageY = newImages.minc.y(); imageY <= newImages.maxc.y(); imageY++) {
					for(int imageZ = newImages.minc.z(); imageZ <= newImages.maxc.z(); imageZ++) {
						const Vector3 imageDelta = simCell * Vector3(imageX, imageY, imageZ);
						for(size_t i = 0; i < oldVertexCount; i++)
							*p++ += imageDelta;
					}
				}
			}

			// Create a copy of the faces sub-object.
			SurfaceMeshFaces* newFaces = newSurface->makeFacesMutable();

			// Replicate all face properties
			size_t oldFaceCount = newFaces->elementCount();
			size_t newFaceCount = oldFaceCount * numCopies;
			newFaces->replicate(numCopies);

			// Add right number of new vertices to the topology.
			for(size_t i = oldVertexCount; i < newVertexCount; i++) {
				mesh->createVertex();
			}

			// Replicate topology faces.
			std::vector<HalfEdgeMesh::vertex_index> newFaceVertices;
			for(int imageX = 0; imageX < nPBC[0]; imageX++) {
				for(int imageY = 0; imageY < nPBC[1]; imageY++) {
					for(int imageZ = 0; imageZ < nPBC[2]; imageZ++) {
						if(imageX == 0 && imageY == 0 && imageZ == 0) continue;
						size_t imageIndexShift = (imageX * nPBC[1] * nPBC[2]) + (imageY * nPBC[2]) + imageZ;
						// Copy faces.
						for(HalfEdgeMesh::face_index face = 0; face < oldFaceCount; face++) {
							newFaceVertices.clear();
							HalfEdgeMesh::edge_index edge = mesh->firstFaceEdge(face);
							do {
								HalfEdgeMesh::vertex_index newVertexIndex = mesh->vertex1(edge) + imageIndexShift * oldVertexCount;
								newFaceVertices.push_back(newVertexIndex);
								edge = mesh->nextFaceEdge(edge);
							}
							while(edge != mesh->firstFaceEdge(face));
							mesh->createFaceAndEdges(newFaceVertices.begin(), newFaceVertices.end());
						}
						// Copy face connectivity.
						for(HalfEdgeMesh::face_index oldFace = 0; oldFace < oldFaceCount; oldFace++) {
							HalfEdgeMesh::face_index newFace = oldFace + imageIndexShift * oldFaceCount;
							HalfEdgeMesh::edge_index oldEdge = mesh->firstFaceEdge(oldFace);
							HalfEdgeMesh::edge_index newEdge = mesh->firstFaceEdge(newFace);
							do {
								if(mesh->hasOppositeEdge(oldEdge)) {
									HalfEdgeMesh::face_index oppositeFaceIndex = mesh->adjacentFace(mesh->oppositeEdge(oldEdge));
									oppositeFaceIndex += imageIndexShift * oldFaceCount;
									HalfEdgeMesh::edge_index newOppositeEdge = mesh->findEdge(oppositeFaceIndex, mesh->vertex2(newEdge), mesh->vertex1(newEdge));
									OVITO_ASSERT(newOppositeEdge != HalfEdgeMesh::InvalidIndex);
									if(!mesh->hasOppositeEdge(newEdge)) {
										mesh->linkOppositeEdges(newEdge, newOppositeEdge);
									}
									else {
										OVITO_ASSERT(mesh->oppositeEdge(newEdge) == newOppositeEdge);
									}
								}
								if(mesh->nextManifoldEdge(oldEdge) != HalfEdgeMesh::InvalidIndex) {
									HalfEdgeMesh::face_index nextManifoldFaceIndex = mesh->adjacentFace(mesh->nextManifoldEdge(oldEdge));
									nextManifoldFaceIndex += imageIndexShift * oldFaceCount;
									HalfEdgeMesh::edge_index newManifoldEdge = mesh->findEdge(nextManifoldFaceIndex, mesh->vertex1(newEdge), mesh->vertex2(newEdge));
									OVITO_ASSERT(newManifoldEdge != HalfEdgeMesh::InvalidIndex);
									mesh->setNextManifoldEdge(newEdge, newManifoldEdge);
								}
								oldEdge = mesh->nextFaceEdge(oldEdge);
								newEdge = mesh->nextFaceEdge(newEdge);
							}
							while(oldEdge != mesh->firstFaceEdge(oldFace));
						}
					}
				}
			}
			OVITO_ASSERT(mesh->faceCount() == newFaceCount);

			if(pbcFlags[0] || pbcFlags[1] || pbcFlags[2]) {
				ConstPropertyPtr vertexCoords = newVertices->getPropertyStorage(SurfaceMeshVertices::PositionProperty);
				// Unwrap faces that crossed a periodic boundary in the original cell.
				for(HalfEdgeMesh::face_index face = 0; face < newFaceCount; face++) {
					HalfEdgeMesh::edge_index edge = mesh->firstFaceEdge(face);
					HalfEdgeMesh::vertex_index v1 = mesh->vertex1(edge);
					HalfEdgeMesh::vertex_index v1wrapped = v1 % oldVertexCount;
					Vector3I imageShift = Vector3I::Zero();
					do {
						HalfEdgeMesh::vertex_index v2 = mesh->vertex2(edge);
						HalfEdgeMesh::vertex_index v2wrapped = v2 % oldVertexCount;
						Vector3 delta = inverseSimCell * (vertexCoords->getPoint3(v2wrapped) - vertexCoords->getPoint3(v1wrapped));
						for(size_t dim = 0; dim < 3; dim++) {
							if(pbcFlags[dim])
								imageShift[dim] -= (int)std::floor(delta[dim] + FloatType(0.5));
						}
						if(imageShift != Vector3I::Zero()) {
							size_t imageIndex = v2 / oldVertexCount;
							Point3I image(imageIndex / nPBC[1] / nPBC[2], (imageIndex / nPBC[2]) % nPBC[1], imageIndex % nPBC[2]);
							Point3I newImage(SimulationCell::modulo(image[0] + imageShift[0], nPBC[0]),
											SimulationCell::modulo(image[1] + imageShift[1], nPBC[1]),
											SimulationCell::modulo(image[2] + imageShift[2], nPBC[2]));
							size_t newImageIndex = (newImage[0] * nPBC[1] * nPBC[2]) + (newImage[1] * nPBC[2]) + newImage[2];
							HalfEdgeMesh::vertex_index new_v2 = v2wrapped + newImageIndex * oldVertexCount;
							mesh->transferFaceBoundaryToVertex(edge, new_v2);
						}
						v1 = v2;
						v1wrapped = v2wrapped;
						edge = mesh->nextFaceEdge(edge);
					}
					while(edge != mesh->firstFaceEdge(face));
				}

				// Since faces that cross a periodic boundary can end up in different images,
				// we now need to "repair" the face connectivity.
				for(HalfEdgeMesh::face_index face = 0; face < newFaceCount; face++) {
					HalfEdgeMesh::edge_index edge = mesh->firstFaceEdge(face);
					do {
						if(mesh->hasOppositeEdge(edge) && mesh->vertex2(mesh->oppositeEdge(edge)) != mesh->vertex1(edge)) {
							HalfEdgeMesh::face_index oppositeFaceIndex = mesh->adjacentFace(mesh->oppositeEdge(edge)) % oldFaceCount;
							mesh->setOppositeEdge(edge, HalfEdgeMesh::InvalidIndex);
							for(size_t i = 0; i < numCopies; i++) {
								HalfEdgeMesh::edge_index newOppositeEdge = mesh->findEdge(oppositeFaceIndex + i * oldFaceCount, mesh->vertex2(edge), mesh->vertex1(edge));
								if(newOppositeEdge != HalfEdgeMesh::InvalidIndex) {
									mesh->setOppositeEdge(edge, newOppositeEdge);
									break;
								}
							}
							OVITO_ASSERT(mesh->hasOppositeEdge(edge));
							OVITO_ASSERT(mesh->vertex2(mesh->oppositeEdge(edge)) == mesh->vertex1(edge));
						}
						if(mesh->nextManifoldEdge(edge) != HalfEdgeMesh::InvalidIndex && mesh->vertex2(mesh->nextManifoldEdge(edge)) != mesh->vertex2(edge)) {
							HalfEdgeMesh::face_index nextManifoldFaceIndex = mesh->adjacentFace(mesh->nextManifoldEdge(edge)) % oldFaceCount;
							mesh->setNextManifoldEdge(edge, HalfEdgeMesh::InvalidIndex);
							for(size_t i = 0; i < numCopies; i++) {
								HalfEdgeMesh::edge_index newNextManifoldEdge = mesh->findEdge(nextManifoldFaceIndex + i * oldFaceCount, mesh->vertex1(edge), mesh->vertex2(edge));
								if(newNextManifoldEdge != HalfEdgeMesh::InvalidIndex) {
									mesh->setNextManifoldEdge(edge, newNextManifoldEdge);
									break;
								}
							}
							OVITO_ASSERT(mesh->nextManifoldEdge(edge)!= HalfEdgeMesh::InvalidIndex);
							OVITO_ASSERT(mesh->vertex1(mesh->nextManifoldEdge(edge)) == mesh->vertex1(edge));
							OVITO_ASSERT(mesh->vertex2(mesh->nextManifoldEdge(edge)) == mesh->vertex2(edge));
						}
						edge = mesh->nextFaceEdge(edge);
					}
					while(edge != mesh->firstFaceEdge(face));
				}
			}

			// Extend the periodic domain the surface is embedded in.
			simCell.translation() += (FloatType)newImages.minc.x() * simCell.column(0);
			simCell.translation() += (FloatType)newImages.minc.y() * simCell.column(1);
			simCell.translation() += (FloatType)newImages.minc.z() * simCell.column(2);
			simCell.column(0) *= (newImages.sizeX() + 1);
			simCell.column(1) *= (newImages.sizeY() + 1);
			simCell.column(2) *= (newImages.sizeZ() + 1);
			newSurface->mutableDomain()->setCellMatrix(simCell);
		}
	}

	return PipelineStatus::Success;
}

}	// End of namespace
}	// End of namespace
