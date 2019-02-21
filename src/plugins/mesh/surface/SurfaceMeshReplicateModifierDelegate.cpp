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

#include <plugins/mesh/Mesh.h>
#include <plugins/mesh/surface/SurfaceMesh.h>
#include <plugins/stdobj/simcell/SimulationCellObject.h>
#include <core/dataset/DataSet.h>
#include <core/dataset/pipeline/ModifierApplication.h>
#include "SurfaceMeshReplicateModifierDelegate.h"

namespace Ovito { namespace Mesh {

IMPLEMENT_OVITO_CLASS(SurfaceMeshReplicateModifierDelegate);

/******************************************************************************
* Determines whether this delegate can handle the given input data.
******************************************************************************/
bool SurfaceMeshReplicateModifierDelegate::OOMetaClass::isApplicableTo(const DataCollection& input) const
{
	return input.containsObject<SurfaceMesh>();
}

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
			// This code can only handle closed meshes at the moment.
			OVITO_ASSERT(mesh->isClosed());

			// Create a copy of the vertices sub-object.
			SurfaceMeshVertices* newVertices = newSurface->makeVerticesMutable();
			newVertices->makePropertiesMutable();

			// Replicate all vertex properties
			size_t oldVertexCount = newVertices->elementCount();
			size_t newVertexCount = oldVertexCount * numCopies;
			for(PropertyObject* property : newVertices->properties()) {
				// Translate existing vertex coordinate so that they form the first image at grid position (0,0,0).
				if(property->type() == SurfaceMeshVertices::PositionProperty && newImages.minc != Point3I::Origin()) {
					const Vector3 translation = simCell * Vector3(newImages.minc.x(), newImages.minc.y(), newImages.minc.z());
					for(Point3& p : property->point3Range())
						p += translation;
				}
				// Replicate property data N times.
				property->replicate(numCopies);
				// Shift vertex positions by the periodicity vector.
				if(property->type() == SurfaceMeshVertices::PositionProperty) {
					Point3* p = property->dataPoint3() + oldVertexCount;
					for(int imageX = 0; imageX < nPBC[0]; imageX++) {
						for(int imageY = 0; imageY < nPBC[1]; imageY++) {
							for(int imageZ = 0; imageZ < nPBC[2]; imageZ++) {
								if(imageX == 0 && imageY == 0 && imageZ == 0) continue;
								const Vector3 imageDelta = simCell * Vector3(imageX, imageY, imageZ);
								for(size_t i = 0; i < oldVertexCount; i++)
									*p++ += imageDelta;
							}
						}
					}
				}
			}

			// Add right number of new topology vertices.
			for(size_t i = oldVertexCount; i < newVertexCount; i++) {
				mesh->createVertex();
			}

			// Replicate faces.
			size_t oldFaceCount = mesh->faceCount();
			size_t newFaceCount = oldFaceCount * numCopies;
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
							mesh->createFace(newFaceVertices.begin(), newFaceVertices.end());
						}
						// Copy face connectivity.
						for(HalfEdgeMesh::face_index oldFace = 0; oldFace < oldFaceCount; oldFace++) {
							HalfEdgeMesh::face_index newFace = oldFace + imageIndexShift * oldFaceCount;
							HalfEdgeMesh::edge_index oldEdge = mesh->firstFaceEdge(oldFace);
							HalfEdgeMesh::edge_index newEdge = mesh->firstFaceEdge(newFace);
							do {
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
								oldEdge = mesh->nextFaceEdge(oldEdge);
								newEdge = mesh->nextFaceEdge(newEdge);
							}
							while(oldEdge != mesh->firstFaceEdge(oldFace));
						}
					}
				}
			}
			OVITO_ASSERT(mesh->faceCount() == newFaceCount);
			OVITO_ASSERT(mesh->isClosed());

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
						if(mesh->vertex2(mesh->oppositeEdge(edge)) != mesh->vertex1(edge)) {
//							OVITO_ASSERT((edge->oppositeEdge()->vertex2()->index() % oldVertexCount) == (edge->vertex1()->index() % oldVertexCount));
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
						edge = mesh->nextFaceEdge(edge);
					}
					while(edge != mesh->firstFaceEdge(face));
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
			newSurface->mutableDomain()->setCellMatrix(simCell);
		}
	}

	return PipelineStatus::Success;
}

}	// End of namespace
}	// End of namespace
