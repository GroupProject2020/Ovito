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

#pragma once


#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include <plugins/stdobj/simcell/SimulationCell.h>
#include <plugins/stdobj/properties/PropertyStorage.h>
#include <plugins/mesh/surface/SurfaceMeshData.h>
#include <core/utilities/concurrent/Task.h>
#include <plugins/crystalanalysis/util/DelaunayTessellation.h>

#include <boost/functional/hash.hpp>

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

/**
 * Constructs a closed manifold which separates different regions
 * in a tetrahedral mesh.
 */
template<bool FlipOrientation = false, bool CreateTwoSidedMesh = false>
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

	// A no-op manifold cross-linking functor.
	struct DefaultLinkManifoldsFunc {
		void operator()(HalfEdgeMesh::edge_index edge1, HalfEdgeMesh::edge_index edge2) {}
	};

public:

	/// Constructor.
	ManifoldConstructionHelper(DelaunayTessellation& tessellation, SurfaceMeshData& outputMesh, FloatType alpha,
			const PropertyStorage& positions) : _tessellation(tessellation), _mesh(outputMesh), _alpha(alpha), _positions(positions) {}

	/// This is the main function, which constructs the manifold triangle mesh.
	template<typename CellRegionFunc, typename PrepareMeshFaceFunc = DefaultPrepareMeshFaceFunc, typename LinkManifoldsFunc = DefaultLinkManifoldsFunc>
	bool construct(CellRegionFunc&& determineCellRegion, Task& promise,
			PrepareMeshFaceFunc&& prepareMeshFaceFunc = PrepareMeshFaceFunc(), LinkManifoldsFunc&& linkManifoldsFunc = LinkManifoldsFunc())
	{
		// Algorithm is divided into several sub-steps.
		// Assign weights to sub-steps according to estimated runtime.
		promise.beginProgressSubStepsWithWeights({ 1, 1, 1 });

		/// Assign tetrahedra to regions.
		if(!classifyTetrahedra(std::move(determineCellRegion), promise))
			return false;

		promise.nextProgressSubStep();

		// Create triangle facets at interfaces between two different regions.
		if(!createInterfaceFacets(std::move(prepareMeshFaceFunc), promise))
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

		_numSolidCells = 0;
		_mesh.setSpaceFillingRegion(-1);
		size_t progressCounter = 0;
		for(DelaunayTessellation::CellIterator cellIter = _tessellation.begin_cells(); cellIter != _tessellation.end_cells(); ++cellIter) {
			DelaunayTessellation::CellHandle cell = *cellIter;

			// Update progress indicator.
			if(!promise.setProgressValueIntermittent(progressCounter++))
				return false;

			// Alpha shape criterion: This determines whether the Delaunay tetrahedron is part of the solid region.
			bool isSolid = _tessellation.isValidCell(cell) && _tessellation.alphaTest(cell, _alpha);

			int region = 0;
			if(isSolid) {
				region = determineCellRegion(cell);
				OVITO_ASSERT(region >= 0);
			}
			_tessellation.setUserField(cell, region);

			if(!_tessellation.isGhostCell(cell)) {
				if(_mesh.spaceFillingRegion() == -1) _mesh.setSpaceFillingRegion(region);
				else if(_mesh.spaceFillingRegion() != region) _mesh.setSpaceFillingRegion(0);
			}

			if(region != 0 && !_tessellation.isGhostCell(cell)) {
				_tessellation.setCellIndex(cell, _numSolidCells++);
			}
			else {
				_tessellation.setCellIndex(cell, -1);
			}
		}
		if(_mesh.spaceFillingRegion() == -1) _mesh.setSpaceFillingRegion(0);

		return !promise.isCanceled();
	}

	/// Constructs the triangle facets that separate different regions in the tetrahedral mesh.
	template<typename PrepareMeshFaceFunc>
	bool createInterfaceFacets(PrepareMeshFaceFunc&& prepareMeshFaceFunc, Task& promise)
	{
		// Stores the triangle mesh vertices created for the vertices of the tetrahedral mesh.
		std::vector<HalfEdgeMesh::vertex_index> vertexMap(_positions.size(), HalfEdgeMesh::InvalidIndex);
		_tetrahedraFaceList.clear();
		_faceLookupMap.clear();

		promise.setProgressValue(0);
		promise.setProgressMaximum(_numSolidCells);

		for(DelaunayTessellation::CellIterator cellIter = _tessellation.begin_cells(); cellIter != _tessellation.end_cells(); ++cellIter) {
			DelaunayTessellation::CellHandle cell = *cellIter;

			// Look for solid and local tetrahedra.
			if(_tessellation.getCellIndex(cell) == -1) continue;
			int solidRegion = _tessellation.getUserField(cell);
			OVITO_ASSERT(solidRegion != 0);

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
				if(_tessellation.getUserField(adjacentCell) == solidRegion) {
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
						vertexMap[vertexIndex] = _mesh.createVertex(_positions.getPoint3(vertexIndex));
					}
					facetVertices[v] = vertexMap[vertexIndex];
				}

				// Create a new triangle facet.
				HalfEdgeMesh::face_index face = _mesh.createFace(facetVertices.begin(), facetVertices.end());

				// Tell client code about the new facet.
				prepareMeshFaceFunc(face, vertexIndices, vertexHandles, cell);

				// Create additional face for exterior region if requested.
				if(CreateTwoSidedMesh && _tessellation.getUserField(adjacentCell) == 0) {

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
					HalfEdgeMesh::face_index oppositeFace = _mesh.createFace(facetVertices.begin(), facetVertices.end());

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
#if !defined(Q_OS_MACOS)
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
	size_t _numSolidCells = 0;

	/// The input particle positions.
	const PropertyStorage& _positions;

	/// The output mesh topology.
	SurfaceMeshData& _mesh;

	/// Stores the faces of the local tetrahedra that have a least one facet for which a triangle has been created.
	std::vector<std::array<HalfEdgeMesh::face_index, 4>> _tetrahedraFaceList;

	/// This map allows to lookup faces based on their vertices.
	std::unordered_map<std::array<size_t,3>, HalfEdgeMesh::face_index, boost::hash<std::array<size_t,3>>> _faceLookupMap;
};

}	// End of namespace
}	// End of namespace
}	// End of namespace
