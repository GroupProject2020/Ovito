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
DEFINE_PROPERTY_FIELD(VoxelGridVis, interpolateColors);
SET_PROPERTY_FIELD_LABEL(VoxelGridVis, transparencyController, "Transparency");
SET_PROPERTY_FIELD_LABEL(VoxelGridVis, highlightGridLines, "Highlight grid lines");
SET_PROPERTY_FIELD_LABEL(VoxelGridVis, interpolateColors, "Interpolate colors");
SET_PROPERTY_FIELD_UNITS_AND_RANGE(VoxelGridVis, transparencyController, PercentParameterUnit, 0, 1);

/******************************************************************************
* Constructor.
******************************************************************************/
VoxelGridVis::VoxelGridVis(DataSet* dataset) : DataVis(dataset),
	_highlightGridLines(true),
	_interpolateColors(false)
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
		bool,						// Grid line highlighting
		bool						// Interpolate colors
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
	auto& primitives = dataset()->visCache().get<CacheValue>(CacheKey(renderer, gridObj, colorProperty, transp, highlightGridLines(), interpolateColors()));

	// Check if we already have valid rendering primitives that are up to date.
	if(!primitives.volumeFaces || !primitives.volumeFaces->isValid(renderer)) {
		primitives.volumeFaces = renderer->createMeshPrimitive();
		if(gridObj->domain()) {
			TriMesh mesh;
			if(colorArray) {
				if(interpolateColors())
					mesh.setHasVertexColors(true);
				else
					mesh.setHasFaceColors(true);
			}
			VoxelGrid::GridDimensions gridDims = gridObj->shape();
			std::array<bool, 3> pbcFlags = gridObj->domain()->pbcFlags();

			// Helper function that creates the mesh vertices and faces for one side of the grid volume.
			auto createFacesForSide = [&](size_t dim1, size_t dim2, int dim3, bool oppositeSide) {
				int nx = gridDims[dim1] + 1;
				int ny = gridDims[dim2] + 1;
				size_t coords[3];
				coords[dim3] = (oppositeSide && !pbcFlags[dim3]) ? (gridDims[dim3]-1) : 0;
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
				ColorA* vertexColor = nullptr;
				if(colorArray) {
					if(interpolateColors())
						vertexColor = mesh.vertexColors().data() + baseVertexCount;
					else
						faceColor = mesh.faceColors().data() + baseFaceCount;
				}
				for(int iy = 0; iy < ny; iy++) {
					for(int ix = 0; ix < nx; ix++) {
						*vertex++ = origin + (ix * dx) + (iy * dy);
						if(vertexColor) {
							// Compute the color at the current vertex, which is the average from the
							// colors of the four adjacent voxel facets.
							Color interpolatedColor(0,0,0);
							int numNeighbors = 0;
							for(int niy = iy-1; niy <= iy; niy++) {
								if(niy < 0) {
									if(pbcFlags[dim2]) coords[dim2] = ny-2; else continue;
								}
								else if(niy > ny-2) {
									if(pbcFlags[dim2]) coords[dim2] = 0; else continue;
								}
								else coords[dim2] = niy;
								for(int nix = ix-1; nix <= ix; nix++) {
									if(nix < 0) {
										if(pbcFlags[dim1]) coords[dim1] = nx-2; else continue;
									}
									else if(nix > nx-2) {
										if(pbcFlags[dim1]) coords[dim1] = 0; else continue;
									}
									else coords[dim1] = nix;
									interpolatedColor += colorArray->get<Color>(gridObj->voxelIndex(coords[0], coords[1], coords[2]));
									numNeighbors++;
								}
							}
							OVITO_ASSERT(numNeighbors >= 1);
							*vertexColor++ = ColorA((FloatType(1) / numNeighbors) * interpolatedColor, alpha);
						}
						if(ix+1 < nx && iy+1 < ny) {
							face->setVertices(baseVertexCount + iy * nx + ix, baseVertexCount + iy * nx + ix+1, baseVertexCount + (iy+1) * nx + ix+1);
							face->setEdgeVisibility(true, true, false);
							++face;
							face->setVertices(baseVertexCount + iy * nx + ix, baseVertexCount + (iy+1) * nx + ix+1, baseVertexCount + (iy+1) * nx + ix);
							face->setEdgeVisibility(false, true, true);
							++face;
							if(faceColor) {
								coords[dim1] = ix;
								coords[dim2] = iy;
								const Color& c = colorArray->get<Color>(gridObj->voxelIndex(coords[0], coords[1], coords[2]));
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
