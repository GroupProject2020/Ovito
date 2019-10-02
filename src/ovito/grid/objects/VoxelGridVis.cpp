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

#include <ovito/grid/Grid.h>
#include <ovito/grid/objects/VoxelGrid.h>
#include <ovito/core/rendering/SceneRenderer.h>
#include <ovito/core/rendering/MeshPrimitive.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/dataset/data/VersionedDataObjectRef.h>
#include <ovito/core/utilities/mesh/TriMesh.h>
#include "VoxelGridVis.h"

namespace Ovito { namespace Grid {

IMPLEMENT_OVITO_CLASS(VoxelGridVis);
DEFINE_REFERENCE_FIELD(VoxelGridVis, transparencyController);
DEFINE_PROPERTY_FIELD(VoxelGridVis, highlightGridLines);
SET_PROPERTY_FIELD_LABEL(VoxelGridVis, transparencyController, "Transparency");
SET_PROPERTY_FIELD_LABEL(VoxelGridVis, highlightGridLines, "Highlight grid lines");
SET_PROPERTY_FIELD_UNITS_AND_RANGE(VoxelGridVis, transparencyController, PercentParameterUnit, 0, 1);

/******************************************************************************
* Constructor.
******************************************************************************/
VoxelGridVis::VoxelGridVis(DataSet* dataset) : DataVis(dataset),
	_highlightGridLines(true)
{
	setTransparencyController(ControllerManager::createFloatController(dataset));
}

/******************************************************************************
* Computes the bounding box of the displayed data.
******************************************************************************/
Box3 VoxelGridVis::boundingBox(TimePoint time, const std::vector<const DataObject*>& objectStack, const PipelineSceneNode* contextNode, const PipelineFlowState& flowState, TimeInterval& validityInterval)
{
	if(const VoxelGrid* gridObj = dynamic_object_cast<VoxelGrid>(objectStack.back())) {
		if(gridObj->domain()) {
			AffineTransformation matrix = gridObj->domain()->cellMatrix();
			if(gridObj->domain()->is2D()) {
				matrix.column(2).setZero();
			}
			return Box3(Point3(0), Point3(1)).transformed(matrix);
		}
	}
	return {};
}

/******************************************************************************
* Lets the visualization element render the data object.
******************************************************************************/
void VoxelGridVis::render(TimePoint time, const std::vector<const DataObject*>& objectStack, const PipelineFlowState& flowState, SceneRenderer* renderer, const PipelineSceneNode* contextNode)
{
	// Check if this is just the bounding box computation pass.
	if(renderer->isBoundingBoxPass()) {
		TimeInterval validityInterval;
		renderer->addToLocalBoundingBox(boundingBox(time, objectStack, contextNode, flowState, validityInterval));
		return;
	}

	// Get the grid object being rendered.
	const VoxelGrid* gridObj = dynamic_object_cast<VoxelGrid>(objectStack.back());
	if(!gridObj) return;

	// Throws an exception if the input data structure is corrupt.
	gridObj->verifyIntegrity();

	// Look for 'Color' voxel property.
	const PropertyObject* colorProperty = gridObj->getProperty(VoxelGrid::ColorProperty);
	ConstPropertyPtr colorArray = colorProperty ? colorProperty->storage() : nullptr;

	// The key type used for caching the geometry primitive:
	using CacheKey = std::tuple<
		CompatibleRendererGroup,	// The scene renderer
		VersionedDataObjectRef,		// The voxel grid object
		VersionedDataObjectRef,		// Color property
		FloatType,					// Transparency
		bool						// Grid line highlighting
	>;

	// The values stored in the vis cache.
	struct CacheValue {
		std::shared_ptr<MeshPrimitive> volumeFaces;
	};

	FloatType transp = 0;
	TimeInterval iv;
	if(transparencyController()) {
		transp = transparencyController()->getFloatValue(time, iv);
		if(transp >= 1.0) return;
	}
	FloatType alpha = FloatType(1) - transp;

	// Look up the rendering primitive in the vis cache.
	auto& primitives = dataset()->visCache().get<CacheValue>(CacheKey(renderer, gridObj, colorProperty, transp, highlightGridLines()));

	// Check if we already have valid rendering primitives that are up to date.
	if(!primitives.volumeFaces || !primitives.volumeFaces->isValid(renderer)) {
		primitives.volumeFaces = renderer->createMeshPrimitive();
		if(gridObj->domain()) {
			TriMesh mesh;
			mesh.setHasFaceColors(colorArray != nullptr);
			VoxelGrid::GridDimensions gridDims = gridObj->shape();

			// Helper function that creates the mesh vertices and faces for one side of the grid volume.
			auto createFacesForSide = [&](size_t dim1, size_t dim2, int dim3, bool oppositeSide) {
				int nx = gridDims[dim1] + 1;
				int ny = gridDims[dim2] + 1;
				int baseVertexCount = mesh.vertexCount();
				int baseFaceCount = mesh.faceCount();
				mesh.setVertexCount(baseVertexCount + nx * ny);
				mesh.setFaceCount(baseFaceCount + 2 * (nx-1) * (ny-1));
				Vector3 dx = gridObj->domain()->cellMatrix().column(dim1) / gridDims[dim1];
				Vector3 dy = gridObj->domain()->cellMatrix().column(dim2) / gridDims[dim2];
				Point3 origin = Point3::Origin() + gridObj->domain()->cellMatrix().translation();
				if(oppositeSide) origin += gridObj->domain()->cellMatrix().column(dim3);
				auto vertex = mesh.vertices().begin() + baseVertexCount;
				auto face = mesh.faces().begin() + baseFaceCount;
				ColorA* faceColor = nullptr;
				if(colorArray) faceColor = mesh.faceColors().data() + baseFaceCount;
				for(int iy = 0; iy < ny; iy++) {
					for(int ix = 0; ix < nx; ix++) {
						*vertex++ = origin + (ix * dx) + (iy * dy);
						if(ix+1 < nx && iy+1 < ny) {
							face->setVertices(baseVertexCount + iy * nx + ix, baseVertexCount + iy * nx + ix+1, baseVertexCount + (iy+1) * nx + ix+1);
							face->setEdgeVisibility(true, true, false);
							++face;
							face->setVertices(baseVertexCount + iy * nx + ix, baseVertexCount + (iy+1) * nx + ix+1, baseVertexCount + (iy+1) * nx + ix);
							face->setEdgeVisibility(false, true, true);
							++face;
							if(colorArray) {
								size_t coords[3];
								coords[dim1] = ix;
								coords[dim2] = iy;
								coords[dim3] = 0;
								const Color& c = colorArray->getColor(gridObj->voxelIndex(coords[0], coords[1], coords[2]));
								*faceColor++ = ColorA(c, alpha);
								*faceColor++ = ColorA(c, alpha);
							}
						}
					}
				}
				OVITO_ASSERT(vertex == mesh.vertices().end());
				OVITO_ASSERT(face == mesh.faces().end());
			};

			createFacesForSide(0, 1, 2, false);
			if(!gridObj->domain()->is2D()) {
				createFacesForSide(0, 1, 2, true);
				createFacesForSide(1, 2, 0, false);
				createFacesForSide(1, 2, 0, true);
				createFacesForSide(2, 0, 1, false);
				createFacesForSide(2, 0, 1, true);
			}
			primitives.volumeFaces->setMesh(std::move(mesh), ColorA(1,1,1,alpha), highlightGridLines());
			primitives.volumeFaces->setCullFaces(false);
		}
	}

	renderer->beginPickObject(contextNode);
	primitives.volumeFaces->render(renderer);
	renderer->endPickObject();
}

}	// End of namespace
}	// End of namespace
