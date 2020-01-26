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

#pragma once


#include <ovito/stdobj/StdObj.h>
#include <ovito/stdobj/simcell/SimulationCell.h>
#include <ovito/stdobj/properties/PropertyStorage.h>
#include <ovito/mesh/surface/SurfaceMeshData.h>
#include <ovito/core/utilities/concurrent/Task.h>
#include <ovito/delaunay/DelaunayTessellation.h>

#include <boost/functional/hash.hpp>

namespace Ovito { namespace Delaunay {

using namespace Ovito::Mesh;

/**
 * Constructs a SurfaceMesh structure from a DelaunayTessellation representing the separating surface manifold
 * between different spatial regions of the tessellation.
 */
template<bool FlipOrientation = false, bool CreateTwoSidedMesh = false, bool CreateDisconnectedRegions = false>
class ManifoldConstructionHelper
{
public:

	// A no-op face-preparation functor.
	struct DefaultPrepareMeshFaceFunc {
		void operator()(HalfEdgeMesh::face_index face,
				const std::array<size_t,3>& vertexIndices,
				const std::array<DelaunayTessellation::VertexHandle,3>& vertexHandles,
				DelaunayTessellation::CellHandle cell) {}
	};

	// A no-op vertex-preparation functor.
	struct DefaultPrepareMeshVertexFunc {
		void operator()(HalfEdgeMesh::vertex_index vertex, size_t particleIndex) {}
	};

	// A no-op manifold cross-linking functor.
	struct DefaultLinkManifoldsFunc {
		void operator()(HalfEdgeMesh::edge_index edge1, HalfEdgeMesh::edge_index edge2) {}
	};

public:

	/// Constructor.
	ManifoldConstructionHelper(DelaunayTessellation& tessellation, SurfaceMeshData& outputMesh, FloatType alpha,
			const PropertyStorage& positions) : _tessellation(tessellation), _mesh(outputMesh), _alpha(alpha), _positions(positions) {}

	/// This is the main function, which constructs the manifold triangle mesh.
	template<typename CellRegionFunc, typename PrepareMeshFaceFunc = DefaultPrepareMeshFaceFunc, typename PrepareMeshVertexFunc = DefaultPrepareMeshVertexFunc, typename LinkManifoldsFunc = DefaultLinkManifoldsFunc>
	bool construct(CellRegionFunc&& determineCellRegion, Task& promise,
			PrepareMeshFaceFunc&& prepareMeshFaceFunc = PrepareMeshFaceFunc(), PrepareMeshVertexFunc&& prepareMeshVertexFunc = PrepareMeshVertexFunc(), LinkManifoldsFunc&& linkManifoldsFunc = LinkManifoldsFunc())
	{
		// Algorithm is divided into several sub-steps.
		if(CreateDisconnectedRegions)
			promise.beginProgressSubStepsWithWeights({1,8,2,1});
		else
			promise.beginProgressSubStepsWithWeights({1,1,2});

		// Assign tetrahedra to spatial regions.
		if(!classifyTetrahedra(std::move(determineCellRegion), promise))
			return false;

		promise.nextProgressSubStep();

		// Aggregate connected tetrahedra into regions.
		if(CreateDisconnectedRegions) {

			// Create the "Region" face property in the output mesh.
			_mesh.createFaceProperty(SurfaceMeshFaces::RegionProperty);

			if(!formRegions(promise))
				return false;
			promise.nextProgressSubStep();
		}

		// Create triangle facets at interfaces between two different regions.
		if(!createInterfaceFacets(std::move(prepareMeshFaceFunc), std::move(prepareMeshVertexFunc), promise))
			return false;

		promise.nextProgressSubStep();

		// Connect triangles with one another to form a closed manifold.
		if(!linkHalfedges(std::move(linkManifoldsFunc), promise))
			return false;

		promise.endProgressSubSteps();

		return !promise.isCanceled();
	}

private:

	/// Assigns each tetrahedron to a region.
	template<typename CellRegionFunc>
	bool classifyTetrahedra(CellRegionFunc&& determineCellRegion, Task& promise)
	{
		promise.setProgressValue(0);
		promise.setProgressMaximum(_tessellation.numberOfTetrahedra());

		_numInteriorCells = 0;
		size_t progressCounter = 0;
		_mesh.setSpaceFillingRegion(HalfEdgeMesh::InvalidIndex);
		bool spaceFillingRegionUndetermined = true;
		bool isSpaceFilling = true;
		for(DelaunayTessellation::CellIterator cellIter = _tessellation.begin_cells(); cellIter != _tessellation.end_cells(); ++cellIter) {
			DelaunayTessellation::CellHandle cell = *cellIter;

			// Update progress indicator.
			if(!promise.setProgressValueIntermittent(progressCounter++))
				return false;

			// Alpha shape criterion: This determines whether the Delaunay tetrahedron is part of the solid region.
			bool isInterior = _tessellation.isValidCell(cell) && _tessellation.alphaTest(cell, _alpha);

			int region = HalfEdgeMesh::InvalidIndex;
			if(isInterior) {
				region = determineCellRegion(cell);
				OVITO_ASSERT(region >= 0 || region == HalfEdgeMesh::InvalidIndex);
				OVITO_ASSERT(!CreateDisconnectedRegions || region <= 0);
				OVITO_ASSERT(CreateDisconnectedRegions || region < _mesh.regionCount());
			}
			_tessellation.setUserField(cell, region);

			if(!_tessellation.isGhostCell(cell)) {
				if(spaceFillingRegionUndetermined) {
					_mesh.setSpaceFillingRegion(region);
					spaceFillingRegionUndetermined = false;
				}
				else {
					if(isSpaceFilling && _mesh.spaceFillingRegion() != region) {
						_mesh.setSpaceFillingRegion(HalfEdgeMesh::InvalidIndex);
						isSpaceFilling = false;
					}
				}
			}

			if(region != HalfEdgeMesh::InvalidIndex && !_tessellation.isGhostCell(cell)) {
				_tessellation.setCellIndex(cell, _numInteriorCells++);
			}
			else {
				_tessellation.setCellIndex(cell, -1);
			}
		}

		return !promise.isCanceled();
	}

	/// Computes the volume of a Delaunay tetrahedron.
	FloatType cellVolume(DelaunayTessellation::CellHandle cell) const
	{
		Point3 p0 = _tessellation.vertexPosition(_tessellation.cellVertex(cell, 0));
		Vector3 ad = _tessellation.vertexPosition(_tessellation.cellVertex(cell, 1)) - p0;
		Vector3 bd = _tessellation.vertexPosition(_tessellation.cellVertex(cell, 2)) - p0;
		Vector3 cd = _tessellation.vertexPosition(_tessellation.cellVertex(cell, 3)) - p0;
		return std::abs(ad.dot(cd.cross(bd))) / FloatType(6);
	}

	/// Aggregates adjacent Delaunay tetrahedra into spatial regions.
	bool formRegions(Task& promise)
	{
		promise.beginProgressSubStepsWithWeights({2,3,1});

		// Create a lookup map that allows retreiving the primary image of a Delaunay cell for a triangular face formed by three particles.
		if(!createCellMap(promise))
			return false;

		// Make sure no regions have been defined so far.
		OVITO_ASSERT(_mesh.regionCount() == 0);

		// Create the output property arrays for the identified regions.
 		_mesh.createRegionProperty(SurfaceMeshRegions::VolumeProperty, true);

		std::deque<size_t> toProcess;

		// Loop over all cells to cluster them.
		promise.nextProgressSubStep();
		promise.setProgressMaximum(_tessellation.numberOfTetrahedra());
		for(DelaunayTessellation::CellIterator cellIter = _tessellation.begin_cells(); cellIter != _tessellation.end_cells() && !promise.isCanceled(); ++cellIter) {
			DelaunayTessellation::CellHandle cell = *cellIter;

			// Skip outside cells and cells that have already been assigned to a cluster.
			if(_tessellation.getUserField(cell) != 0)
				continue;

			// Skip ghost cells.
			if(_tessellation.isGhostCell(cell))
				continue;

			// Start a new cluster.
			int currentCluster = _mesh.regionCount() + 1;
			OVITO_ASSERT(currentCluster >= 1);
			double regionVolume = 0;

			// Now recursively iterate over all neighbors of the seed cell and add them to the current cluster.
			toProcess.push_back(cell);
			_tessellation.setUserField(cell, currentCluster);
			do {
				if(promise.isCanceled())
					return false;

				DelaunayTessellation::CellHandle currentCell = toProcess.front();
				toProcess.pop_front();
				if(!promise.incrementProgressValue())
					break;

				// Add the volume of the current cell to the total region volume.
				regionVolume += cellVolume(currentCell);

				// Loop over the 4 facets of the cell
				for(int f = 0; f < 4; f++) {

					// Get the 3 vertices of the facet.
					// Note that we reverse their order to find the opposite face.
					std::array<size_t, 3> vertices;
					for(int v = 0; v < 3; v++)
						vertices[v] = _tessellation.vertexIndex(_tessellation.cellVertex(currentCell, DelaunayTessellation::cellFacetVertexIndex(f, 2-v)));

					// Bring vertices into a well-defined order, which can be used as lookup key to find the adjacent tetrahedron.
					reorderFaceVertices(vertices);

					// Look up the neighboring Delaunay cell.
					auto neighborCell = _cellLookupMap.find(vertices);
					if(neighborCell != _cellLookupMap.end()) {
						// Add adjecent cell to the deque if it has not been processed yet.
						if(_tessellation.getUserField(neighborCell->second) == 0) {
							toProcess.push_back(neighborCell->second);
							_tessellation.setUserField(neighborCell->second, currentCluster);
						}
					}
				}
			}
			while(!toProcess.empty());

			// Create a spatial region in the output mesh.
			_mesh.createRegion(0, regionVolume);
		}
		promise.nextProgressSubStep();

		if(_mesh.regionCount() > 0) {
			// Shift interior region IDs to start at index 0.
			for(DelaunayTessellation::CellIterator cellIter = _tessellation.begin_cells(); cellIter != _tessellation.end_cells(); ++cellIter) {
				int region = _tessellation.getUserField(*cellIter);
				if(region > 0)
					_tessellation.setUserField(*cellIter, region - 1);
			}

			// Copy assigned region IDs from primary tetrahedra to ghost tetrahedra.
			promise.setProgressMaximum(_tessellation.numberOfTetrahedra());
			for(DelaunayTessellation::CellIterator cellIter = _tessellation.begin_cells(); cellIter != _tessellation.end_cells(); ++cellIter) {
				DelaunayTessellation::CellHandle cell = *cellIter;
				if(_tessellation.isGhostCell(cell) && _tessellation.getUserField(cell) == 0) {
					if(!promise.setProgressValueIntermittent(cell))
						break;

					// Get the 3 vertices of the first face of the tet.
					std::array<size_t, 3> vertices;
					for(int v = 0; v < 3; v++)
						vertices[v] = _tessellation.vertexIndex(_tessellation.cellVertex(cell, DelaunayTessellation::cellFacetVertexIndex(0, v)));

					// Bring vertices into a well-defined order, which can be used as lookup key.
					reorderFaceVertices(vertices);

					// Find the primary tet whose face connects the same three particles.
					auto neighborCell = _cellLookupMap.find(vertices);
					if(neighborCell != _cellLookupMap.end()) {
						_tessellation.setUserField(cell, _tessellation.getUserField(neighborCell->second));
					}
				}
			}
		}
		promise.endProgressSubSteps();

		return true;
	}

	/// Creates a lookup map that allows to retreive the primary Delaunay cell image that belongs to a
	/// triangular face formed by three particles.
	bool createCellMap(Task& promise)
	{
		promise.setProgressMaximum(_tessellation.numberOfTetrahedra());
		for(DelaunayTessellation::CellIterator cellIter = _tessellation.begin_cells(); cellIter != _tessellation.end_cells(); ++cellIter) {
			DelaunayTessellation::CellHandle cell = *cellIter;

			// Skip cells that belong to the exterior region.
			if(_tessellation.getUserField(cell) == HalfEdgeMesh::InvalidIndex)
				continue;

			// Skip ghost cells.
			if(_tessellation.isGhostCell(cell))
				continue;

			// Update progress indicator.
			if(!promise.setProgressValueIntermittent(cell))
				break;

			// Loop over the 4 facets of the cell.
			for(int f = 0; f < 4; f++) {

				// Get the 3 vertices of the facet.
				std::array<size_t, 3> vertices;
				for(int v = 0; v < 3; v++)
					vertices[v] = _tessellation.vertexIndex(_tessellation.cellVertex(cell, DelaunayTessellation::cellFacetVertexIndex(f, v)));

				// Bring vertices into a well-defined order, which can be used as lookup key.
				reorderFaceVertices(vertices);

				// Each key in the map should be unique.
				OVITO_ASSERT(_cellLookupMap.find(vertices) == _cellLookupMap.end());

				// Add facet and its adjacent cell to the loopup map.
				_cellLookupMap.emplace(vertices, cell);
			}
		}
		return !promise.isCanceled();
	}

	/// Constructs the triangle facets that separate different regions in the tetrahedral mesh.
	template<typename PrepareMeshFaceFunc, typename PrepareMeshVertexFunc>
	bool createInterfaceFacets(PrepareMeshFaceFunc&& prepareMeshFaceFunc, PrepareMeshVertexFunc&& prepareMeshVertexFunc, Task& promise)
	{
		// Stores the triangle mesh vertices created for the vertices of the tetrahedral mesh.
		std::vector<HalfEdgeMesh::vertex_index> vertexMap(_positions.size(), HalfEdgeMesh::InvalidIndex);
		_tetrahedraFaceList.clear();
		_faceLookupMap.clear();

		promise.setProgressValue(0);
		promise.setProgressMaximum(_numInteriorCells);

		for(DelaunayTessellation::CellIterator cellIter = _tessellation.begin_cells(); cellIter != _tessellation.end_cells(); ++cellIter) {
			DelaunayTessellation::CellHandle cell = *cellIter;

			// Look for interior and local tetrahedra.
			if(_tessellation.getCellIndex(cell) == -1) continue;
			int interiorRegion = _tessellation.getUserField(cell);
			OVITO_ASSERT(interiorRegion != HalfEdgeMesh::InvalidIndex);

			// Update progress indicator.
			if(!promise.setProgressValueIntermittent(_tessellation.getCellIndex(cell)))
				return false;

			Point3 unwrappedVerts[4];
			for(int i = 0; i < 4; i++)
				unwrappedVerts[i] = _tessellation.vertexPosition(_tessellation.cellVertex(cell, i));

			// Check validity of tessellation.
			Vector3 ad = unwrappedVerts[0] - unwrappedVerts[3];
			Vector3 bd = unwrappedVerts[1] - unwrappedVerts[3];
			Vector3 cd = unwrappedVerts[2] - unwrappedVerts[3];
			if(_tessellation.simCell().isWrappedVector(ad) || _tessellation.simCell().isWrappedVector(bd) || _tessellation.simCell().isWrappedVector(cd))
				throw Exception("Cannot construct manifold. Simulation cell length is too small for the given probe sphere radius parameter.");

			// Iterate over the four faces of the tetrahedron cell.
			_tessellation.setCellIndex(cell, -1);
			for(int f = 0; f < 4; f++) {

				// Check if the adjacent tetrahedron belongs to a different region.
				std::pair<DelaunayTessellation::CellHandle,int> mirrorFacet = _tessellation.mirrorFacet(cell, f);
				DelaunayTessellation::CellHandle adjacentCell = mirrorFacet.first;
				if(_tessellation.getUserField(adjacentCell) == interiorRegion) {
					continue;
				}

				// Create the three vertices of the face or use existing output vertices.
				std::array<HalfEdgeMesh::vertex_index,3> facetVertices;
				std::array<DelaunayTessellation::VertexHandle,3> vertexHandles;
				std::array<size_t,3> vertexIndices;
				for(int v = 0; v < 3; v++) {
					vertexHandles[v] = _tessellation.cellVertex(cell, DelaunayTessellation::cellFacetVertexIndex(f, FlipOrientation ? v : (2-v)));
					size_t vertexIndex = vertexIndices[v] = _tessellation.vertexIndex(vertexHandles[v]);
					OVITO_ASSERT(vertexIndex < vertexMap.size());
					if(vertexMap[vertexIndex] == HalfEdgeMesh::InvalidIndex) {
						vertexMap[vertexIndex] = _mesh.createVertex(_positions[vertexIndex]);
						prepareMeshVertexFunc(vertexMap[vertexIndex], vertexIndex);
					}
					facetVertices[v] = vertexMap[vertexIndex];
				}

				// Create a new triangle facet.
				HalfEdgeMesh::face_index face = _mesh.createFace(facetVertices.begin(), facetVertices.end(), interiorRegion);

				// Tell client code about the new facet.
				prepareMeshFaceFunc(face, vertexIndices, vertexHandles, cell);

				// Create additional face for exterior region if requested.
				if(CreateTwoSidedMesh && _tessellation.getUserField(adjacentCell) == HalfEdgeMesh::InvalidIndex) {

					// Build face vertex list.
					std::reverse(std::begin(vertexHandles), std::end(vertexHandles));
					std::array<size_t,3> reverseVertexIndices;
					for(int v = 0; v < 3; v++) {
						vertexHandles[v] = _tessellation.cellVertex(adjacentCell, DelaunayTessellation::cellFacetVertexIndex(mirrorFacet.second, FlipOrientation ? v : (2-v)));
						size_t vertexIndex = reverseVertexIndices[v] = _tessellation.vertexIndex(vertexHandles[v]);
						OVITO_ASSERT(vertexIndex < vertexMap.size());
						OVITO_ASSERT(vertexMap[vertexIndex] != HalfEdgeMesh::InvalidIndex);
						facetVertices[v] = vertexMap[vertexIndex];
					}

					// Create a new triangle facet.
					HalfEdgeMesh::face_index oppositeFace = _mesh.createFace(facetVertices.begin(), facetVertices.end(), HalfEdgeMesh::InvalidIndex);

					// Tell client code about the new facet.
					prepareMeshFaceFunc(oppositeFace, reverseVertexIndices, vertexHandles, adjacentCell);

					// Insert new facet into lookup map.
					reorderFaceVertices(reverseVertexIndices);
					_faceLookupMap.emplace(reverseVertexIndices, oppositeFace);
				}

				// Insert new facet into lookup map.
				reorderFaceVertices(vertexIndices);
				_faceLookupMap.emplace(vertexIndices, face);

				// Insert into contiguous list of tetrahedron faces.
				if(_tessellation.getCellIndex(cell) == -1) {
					_tessellation.setCellIndex(cell, _tetrahedraFaceList.size());
					_tetrahedraFaceList.push_back(std::array<HalfEdgeMesh::face_index, 4>{{ HalfEdgeMesh::InvalidIndex, HalfEdgeMesh::InvalidIndex, HalfEdgeMesh::InvalidIndex, HalfEdgeMesh::InvalidIndex }});
				}
				_tetrahedraFaceList[_tessellation.getCellIndex(cell)][f] = face;
			}
		}

		return !promise.isCanceled();
	}

	HalfEdgeMesh::face_index findAdjacentFace(DelaunayTessellation::CellHandle cell, int f, int e)
	{
		int vertexIndex1, vertexIndex2;
		if(!FlipOrientation) {
			vertexIndex1 = DelaunayTessellation::cellFacetVertexIndex(f, 2-e);
			vertexIndex2 = DelaunayTessellation::cellFacetVertexIndex(f, (4-e)%3);
		}
		else {
			vertexIndex1 = DelaunayTessellation::cellFacetVertexIndex(f, (e+1)%3);
			vertexIndex2 = DelaunayTessellation::cellFacetVertexIndex(f, e);
		}
		DelaunayTessellation::FacetCirculator circulator_start = _tessellation.incident_facets(cell, vertexIndex1, vertexIndex2, cell, f);
		DelaunayTessellation::FacetCirculator circulator = circulator_start;
		OVITO_ASSERT((*circulator).first == cell);
		OVITO_ASSERT((*circulator).second == f);
		--circulator;
		OVITO_ASSERT(circulator != circulator_start);
		int region = _tessellation.getUserField(cell);
		do {
			// Look for the first cell while going around the edge that belongs to a different region.
			if(_tessellation.getUserField((*circulator).first) != region)
				break;
			--circulator;
		}
		while(circulator != circulator_start);
		OVITO_ASSERT(circulator != circulator_start);

		// Get the current adjacent cell, which is part of the same region as the first tet.
		std::pair<DelaunayTessellation::CellHandle,int> mirrorFacet = _tessellation.mirrorFacet(*circulator);
		OVITO_ASSERT(_tessellation.getUserField(mirrorFacet.first) == region);

		HalfEdgeMesh::face_index adjacentFace = findCellFace(mirrorFacet);
		if(adjacentFace == HalfEdgeMesh::InvalidIndex)
			throw Exception("Cannot construct mesh for this input dataset. Adjacent cell face not found.");
		return adjacentFace;
	}

	template<typename LinkManifoldsFunc>
	bool linkHalfedges(LinkManifoldsFunc&& linkManifoldsFunc, Task& promise)
	{
		promise.setProgressValue(0);
		promise.setProgressMaximum(_tetrahedraFaceList.size());

		auto tet = _tetrahedraFaceList.cbegin();
		for(DelaunayTessellation::CellIterator cellIter = _tessellation.begin_cells(); cellIter != _tessellation.end_cells(); ++cellIter) {
			DelaunayTessellation::CellHandle cell = *cellIter;

			// Look for tetrahedra with at least one face.
			if(_tessellation.getCellIndex(cell) == -1) continue;

			// Update progress indicator.
			if(!promise.setProgressValueIntermittent(_tessellation.getCellIndex(cell)))
				return false;

			for(int f = 0; f < 4; f++) {
				HalfEdgeMesh::face_index facet = (*tet)[f];
				if(facet == HalfEdgeMesh::InvalidIndex) continue;

				// Link within manifold.
				HalfEdgeMesh::edge_index edge = _mesh.firstFaceEdge(facet);
				for(int e = 0; e < 3; e++, edge = _mesh.nextFaceEdge(edge)) {
					if(_mesh.hasOppositeEdge(edge)) continue;
					HalfEdgeMesh::face_index oppositeFace = findAdjacentFace(cell, f, e);
					HalfEdgeMesh::edge_index oppositeEdge = _mesh.findEdge(oppositeFace, _mesh.vertex2(edge), _mesh.vertex1(edge));
					if(oppositeEdge == HalfEdgeMesh::InvalidIndex)
						throw Exception("Cannot construct mesh for this input dataset. Opposite half-edge not found.");
					_mesh.linkOppositeEdges(edge, oppositeEdge);
				}

				if(CreateTwoSidedMesh) {
					std::pair<DelaunayTessellation::CellHandle,int> oppositeFacet = _tessellation.mirrorFacet(cell, f);
					OVITO_ASSERT(_tessellation.getUserField(oppositeFacet.first) != _tessellation.getUserField(cell));
					HalfEdgeMesh::face_index outerFacet = findCellFace(oppositeFacet);
					OVITO_ASSERT(outerFacet != HalfEdgeMesh::InvalidIndex);

					// Link across manifolds
					HalfEdgeMesh::edge_index edge1 = _mesh.firstFaceEdge(facet);
					for(int e1 = 0; e1 < 3; e1++, edge1 = _mesh.nextFaceEdge(edge1)) {
						bool found = false;
						for(HalfEdgeMesh::edge_index edge2 = _mesh.firstFaceEdge(outerFacet); ; edge2 = _mesh.nextFaceEdge(edge2)) {
							if(_mesh.vertex1(edge2) == _mesh.vertex2(edge1)) {
								OVITO_ASSERT(_mesh.vertex2(edge2) == _mesh.vertex1(edge1));
								linkManifoldsFunc(edge1, edge2);
								found = true;
								break;
							}
						}
						OVITO_ASSERT(found);
					}

					if(_tessellation.getUserField(oppositeFacet.first) == 0) {
						// Link within opposite manifold.
						HalfEdgeMesh::edge_index edge = _mesh.firstFaceEdge(outerFacet);
						for(int e = 0; e < 3; e++, edge = _mesh.nextFaceEdge(edge)) {
							if(_mesh.hasOppositeEdge(edge)) continue;
							HalfEdgeMesh::face_index oppositeFace = findAdjacentFace(oppositeFacet.first, oppositeFacet.second, e);
							HalfEdgeMesh::edge_index oppositeEdge = _mesh.findEdge(oppositeFace, _mesh.vertex2(edge), _mesh.vertex1(edge));
							if(oppositeEdge == HalfEdgeMesh::InvalidIndex)
								throw Exception("Cannot construct mesh for this input dataset. Opposite half-edge1 not found.");
							_mesh.linkOppositeEdges(edge, oppositeEdge);
						}
					}
				}
			}
			++tet;
		}
		OVITO_ASSERT(tet == _tetrahedraFaceList.cend());
		OVITO_ASSERT(_mesh.topology()->isClosed());
		return !promise.isCanceled();
	}

	HalfEdgeMesh::face_index findCellFace(const std::pair<DelaunayTessellation::CellHandle,int>& facet)
	{
		// If the cell is a ghost cell, find the corresponding real cell first.
		auto cell = facet.first;
		if(_tessellation.getCellIndex(cell) != -1) {
			OVITO_ASSERT(_tessellation.getCellIndex(cell) >= 0 && _tessellation.getCellIndex(cell) < _tetrahedraFaceList.size());
			return _tetrahedraFaceList[_tessellation.getCellIndex(cell)][facet.second];
		}
		else {
			std::array<size_t,3> faceVerts;
			for(size_t i = 0; i < 3; i++) {
				int vertexIndex = DelaunayTessellation::cellFacetVertexIndex(facet.second, FlipOrientation ? i : (2-i));
				faceVerts[i] = _tessellation.vertexIndex(_tessellation.cellVertex(cell, vertexIndex));
			}
			reorderFaceVertices(faceVerts);
			auto iter = _faceLookupMap.find(faceVerts);
			if(iter != _faceLookupMap.end())
				return iter->second;
			else
				return HalfEdgeMesh::InvalidIndex;
		}
	}

	static void reorderFaceVertices(std::array<size_t,3>& vertexIndices) {
#if !defined(Q_OS_MACOS) && !defined(Q_OS_WASM)
		// Shift the order of vertices so that the smallest index is at the front.
		std::rotate(std::begin(vertexIndices), std::min_element(std::begin(vertexIndices), std::end(vertexIndices)), std::end(vertexIndices));
#else
		// Workaround for compiler bug in Xcode 10.0. Clang hangs when compiling the code above with -O2/-O3 flag.
		auto min_index = std::min_element(vertexIndices.begin(), vertexIndices.end()) - vertexIndices.begin();
		std::rotate(vertexIndices.begin(), vertexIndices.begin() + min_index, vertexIndices.end());
#endif
	}

private:

	/// The tetrahedral tessellation.
	DelaunayTessellation& _tessellation;

	/// The squared probe sphere radius used to classify tetrahedra as open or solid.
	FloatType _alpha;

	/// Counts the number of tetrehedral cells that belong to the solid region.
	size_t _numInteriorCells = 0;

	/// The input particle positions.
	ConstPropertyAccess<Point3> _positions;

	/// The output mesh topology.
	SurfaceMeshData& _mesh;

	/// Stores the faces of the local tetrahedra that have a least one facet for which a triangle has been created.
	std::vector<std::array<HalfEdgeMesh::face_index, 4>> _tetrahedraFaceList;

	/// This map allows to lookup output mesh faces based on their vertices.
#if 0
	std::unordered_map<std::array<size_t,3>, HalfEdgeMesh::face_index, boost::hash<std::array<size_t,3>>> _faceLookupMap;
#else
	std::map<std::array<size_t,3>, HalfEdgeMesh::face_index> _faceLookupMap;
#endif

	/// This map allows to lookup the tetrahedron that is adjacent to a given triangular face.
#if 0
	std::unordered_map<std::array<size_t,3>, DelaunayTessellation::CellHandle, boost::hash<std::array<size_t,3>>> _cellLookupMap;
#else
	std::map<std::array<size_t,3>, DelaunayTessellation::CellHandle> _cellLookupMap;
#endif
};

}	// End of namespace
}	// End of namespace
