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


#include <ovito/mesh/Mesh.h>
#include <ovito/mesh/surface/HalfEdgeMesh.h>
#include <ovito/mesh/surface/SurfaceMeshVertices.h>
#include <ovito/mesh/surface/SurfaceMeshFaces.h>
#include <ovito/mesh/surface/SurfaceMeshRegions.h>
#include <ovito/stdobj/properties/PropertyStorage.h>
#include <ovito/stdobj/simcell/SimulationCell.h>

namespace Ovito { namespace Mesh {

/**
 * Container data structure that represents a surface mesh consisting of a topology data structure and vertex coordinates.
 * The class is used in the implementation of algorithms to build up or operate on surface meshes.
§ */
class OVITO_MESH_EXPORT SurfaceMeshData
{
public:

    using size_type = HalfEdgeMesh::size_type;
    using vertex_index = HalfEdgeMesh::vertex_index;
    using edge_index = HalfEdgeMesh::edge_index;
    using face_index = HalfEdgeMesh::face_index;
    using region_index = int;

    /// Constructor creating an empty surface mesh.
    SurfaceMeshData(const SimulationCell& cell = {});

    /// Constructor that adopts the data from the given pipeline data object into this structure.
    explicit SurfaceMeshData(const SurfaceMesh* sm);

    /// Copies the data stored in this structure to the given pipeline data object.
    void transferTo(SurfaceMesh* sm) const;

    /// Returns the topology of the surface mesh.
    const HalfEdgeMeshPtr& topology() const { return _topology; }

    /// Returns the number of vertices in the mesh.
    size_type vertexCount() const { return topology()->vertexCount(); }

    /// Returns the number of faces in the mesh.
    size_type faceCount() const { return topology()->faceCount(); }

    /// Returns the number of half-edges in the mesh.
    size_type edgeCount() const { return topology()->edgeCount(); }

    /// Returns the number of spatial regions defined for the mesh.
    size_type regionCount() const { return _regionCount; }

    /// Returns the index of the space-filling spatial region.
    region_index spaceFillingRegion() const { return _spaceFillingRegion; }

    /// Sets the index of the space-filling spatial region.
    void setSpaceFillingRegion(region_index region) { _spaceFillingRegion = region; }

    /// Returns whether the "Region" face property is defined in this mesh.
    bool hasFaceRegions() const { return _faceRegions != nullptr; }

    /// Returns the spatial region which the given face belongs to.
    region_index faceRegion(face_index face) const { OVITO_ASSERT(face >= 0 && face < faceCount()); return faceRegions()[face]; }

    /// Sets the cluster a dislocation/slip face is embedded in.
    void setFaceRegion(face_index face, region_index region) { OVITO_ASSERT(face >= 0 && face < faceCount()); faceRegions()[face] = region; }

    /// Returns the spatial region which the given mesh edge belongs to.
    region_index edgeRegion(edge_index edge) const { return faceRegion(adjacentFace(edge)); }

    /// Returns the first edge from a vertex' list of outgoing half-edges.
    edge_index firstVertexEdge(vertex_index vertex) const { return topology()->firstVertexEdge(vertex); }

    /// Returns the half-edge following the given half-edge in the linked list of half-edges of a vertex.
    edge_index nextVertexEdge(edge_index edge) const { return topology()->nextVertexEdge(edge); }

    /// Returns the first half-edge from the linked-list of half-edges of a face.
    edge_index firstFaceEdge(face_index face) const { return topology()->firstFaceEdge(face); }

    /// Returns the list of first half-edges for each face.
    const std::vector<edge_index>& firstFaceEdges() const { return topology()->firstFaceEdges(); }

    /// Returns the opposite face of a face.
    face_index oppositeFace(face_index face) const { return topology()->oppositeFace(face); };

    /// Determines whether the given face is linked to an opposite face.
    bool hasOppositeFace(face_index face) const { return topology()->hasOppositeFace(face); };

    /// Returns the next half-edge following the given half-edge in the linked-list of half-edges of a face.
    edge_index nextFaceEdge(edge_index edge) const { return topology()->nextFaceEdge(edge); }

    /// Returns the previous half-edge preceding the given edge in the linked-list of half-edges of a face.
    edge_index prevFaceEdge(edge_index edge) const { return topology()->prevFaceEdge(edge); }

    /// Returns the first vertex from the contour of a face.
    vertex_index firstFaceVertex(face_index face) const { return topology()->firstFaceVertex(face); }

    /// Returns the second vertex from the contour of a face.
    vertex_index secondFaceVertex(face_index face) const { return topology()->secondFaceVertex(face); }

    /// Returns the third vertex from the contour of a face.
    vertex_index thirdFaceVertex(face_index face) const { return topology()->thirdFaceVertex(face); }

    /// Returns the second half-edge (following the first half-edge) from the linked-list of half-edges of a face.
    edge_index secondFaceEdge(face_index face) const { return topology()->secondFaceEdge(face); }

    /// Returns the vertex the given half-edge is originating from.
    vertex_index vertex1(edge_index edge) const { return topology()->vertex1(edge); }

    /// Returns the vertex the given half-edge is leading to.
    vertex_index vertex2(edge_index edge) const { return topology()->vertex2(edge); }

    /// Returns the face which is adjacent to the given half-edge.
    face_index adjacentFace(edge_index edge) const { return topology()->adjacentFace(edge); }

    /// Returns the opposite half-edge of the given edge.
    edge_index oppositeEdge(edge_index edge) const { return topology()->oppositeEdge(edge); }

    /// Returns whether the given half-edge has an opposite half-edge.
    bool hasOppositeEdge(edge_index edge) const { return topology()->hasOppositeEdge(edge); }

    /// Counts the number of outgoing half-edges adjacent to the given mesh vertex.
    size_type vertexEdgeCount(vertex_index vertex) const { return topology()->vertexEdgeCount(vertex); }

    /// Searches the half-edges of a face for one connecting the two given vertices.
    edge_index findEdge(face_index face, vertex_index v1, vertex_index v2) const { return topology()->findEdge(face, v1, v2); }

    /// Returns the next incident manifold when going around the given half-edge.
    edge_index nextManifoldEdge(edge_index edge) const { return topology()->nextManifoldEdge(edge); };

    /// Sets what is the next incident manifold when going around the given half-edge.
    void setNextManifoldEdge(edge_index edge, edge_index nextEdge) {
        OVITO_ASSERT(isTopologyMutable());
        topology()->setNextManifoldEdge(edge, nextEdge);
    }

    /// Determines the number of manifolds adjacent to a half-edge.
    int countManifolds(edge_index edge) const { return topology()->countManifolds(edge); }

    /// Returns the position of the i-th mesh vertex.
    const Point3& vertexPosition(vertex_index vertex) const {
        OVITO_ASSERT(vertex >= 0 && vertex < vertexCount());
        return vertexCoords()[vertex];
    }

    /// Sets the position of the i-th mesh vertex.
    void setVertexPosition(vertex_index vertex, const Point3& coords) {
        OVITO_ASSERT(vertex >= 0 && vertex < vertexCount());
        vertexCoords()[vertex] = coords;
    }

    /// Creates a new vertex at the given coordinates.
    vertex_index createVertex(const Point3& pos) {
        OVITO_ASSERT(isTopologyMutable());
        OVITO_ASSERT(areVertexPropertiesMutable());
        vertex_index vidx = topology()->createVertex();
        for(const auto& prop : _vertexProperties) {
            if(prop->grow(1))
                updateVertexPropertyPointers(prop);
        }
        vertexCoords()[vidx] = pos;
        return vidx;
    }

    /// Creates several new vertices and initializes their coordinates.
    template<typename CoordinatesIterator>
    void createVertices(CoordinatesIterator begin, CoordinatesIterator end) {
        OVITO_ASSERT(isTopologyMutable());
        OVITO_ASSERT(areVertexPropertiesMutable());
        size_type oldVertexCount = vertexCount();
        auto nverts = std::distance(begin, end);
        topology()->createVertices(nverts);
        for(const auto& prop : _vertexProperties) {
            if(prop->grow(nverts))
                updateVertexPropertyPointers(prop);
        }
	    std::copy(begin, end, vertexCoords() + oldVertexCount);
    }

    /// Deletes a vertex from the mesh.
    /// This method assumes that the vertex is not connected to any part of the mesh.
    void deleteVertex(vertex_index vertex) {
        OVITO_ASSERT(isTopologyMutable());
        OVITO_ASSERT(areVertexPropertiesMutable());
        if(vertex < vertexCount() - 1) {
            // Move the last vertex to the index of the vertex being deleted.
            for(const auto& prop : _vertexProperties) {
                OVITO_ASSERT(prop->size() == vertexCount());
                std::memcpy(prop->dataAt(vertex), prop->constDataAt(prop->size() - 1), prop->stride());
            }
        }
        // Truncate the vertex property arrays.
        for(const auto& prop : _vertexProperties) {
            prop->truncate(1);
        }
        topology()->deleteVertex(vertex);
    }

    /// Creates a new face, and optionally also the half-edges surrounding it.
    /// Returns the index of the new face.
    face_index createFace(std::initializer_list<vertex_index> vertices, region_index faceRegion = HalfEdgeMesh::InvalidIndex) {
        return createFace(vertices.begin(), vertices.end(), faceRegion);
    }

    /// Creates a new face, and optionally also the half-edges surrounding it.
    /// Returns the index of the new face.
    template<typename VertexIterator>
    face_index createFace(VertexIterator begin, VertexIterator end, region_index faceRegion = HalfEdgeMesh::InvalidIndex) {
        OVITO_ASSERT(isTopologyMutable());
        OVITO_ASSERT(areFacePropertiesMutable());
        face_index fidx = (begin == end) ? topology()->createFace() : topology()->createFaceAndEdges(begin, end);
        for(const auto& prop : _faceProperties) {
            if(prop->grow(1))
                updateFacePropertyPointers(prop);
        }
        if(_faceRegions) {
            _faceRegions[fidx] = faceRegion;
        }
        return fidx;
    }

    /// Splits a face along the edge given by the second vertices of two of its border edges.
    edge_index splitFace(edge_index edge1, edge_index edge2);

    /// Deletes a face from the mesh.
    /// A hole in the mesh will be left behind at the location of the deleted face.
    /// The half-edges of the face are also disconnected from their respective opposite half-edges and deleted by this method.
    void deleteFace(face_index face) {
        OVITO_ASSERT(isTopologyMutable());
        OVITO_ASSERT(areFacePropertiesMutable());
        if(face < faceCount() - 1) {
            // Move the last face to the index of the face being deleted.
            for(const auto& prop : _faceProperties) {
                OVITO_ASSERT(prop->size() == faceCount());
                std::memcpy(prop->dataAt(face), prop->constDataAt(prop->size() - 1), prop->stride());
            }
        }
        // Truncate the face property arrays.
        for(const auto& prop : _faceProperties) {
            prop->truncate(1);
        }
        topology()->deleteFace(face);
    }

    /// Creates a new half-edge between two vertices and adjacent to the given face.
    /// Returns the index of the new half-edge.
    edge_index createEdge(vertex_index vertex1, vertex_index vertex2, face_index face) {
        OVITO_ASSERT(isTopologyMutable());
        return topology()->createEdge(vertex1, vertex2, face);
    }

    /// Creates a new half-edge connecting the two vertices of an existing edge in reverse direction
    /// and which is adjacent to the given face. Returns the index of the new half-edge.
    edge_index createOppositeEdge(edge_index edge, face_index face) {
        OVITO_ASSERT(isTopologyMutable());
        return topology()->createOppositeEdge(edge, face);
    }

    /// Inserts a new vertex in the midle of an existing edge.
    vertex_index splitEdge(edge_index edge, const Point3& pos) {
        OVITO_ASSERT(isTopologyMutable());
        vertex_index new_v = createVertex(pos);
        topology()->splitEdge(edge, new_v);
        return new_v;
    }

    /// Defines a new spatial region.
    region_index createRegion(int phase = 0, FloatType volume = 0, FloatType surfaceArea = 0) {
        OVITO_ASSERT(areRegionPropertiesMutable());
        vertex_index ridx = _regionCount++;
        for(const auto& prop : _regionProperties) {
            if(prop->grow(1))
                updateRegionPropertyPointers(prop);
        }
        if(_regionPhases) {
            _regionPhases[ridx] = phase;
        }
        if(_regionVolumes) {
            _regionVolumes[ridx] = volume;
        }
        if(_regionSurfaceAreas) {
            _regionSurfaceAreas[ridx] = surfaceArea;
        }
        return ridx;
    }

    /// Defines an array of new spatial regions.
    region_index createRegions(size_type numRegions) {
        OVITO_ASSERT(areRegionPropertiesMutable());
        vertex_index ridx = _regionCount;
        _regionCount += numRegions;
        for(const auto& prop : _regionProperties) {
            if(prop->grow(numRegions))
                updateRegionPropertyPointers(prop);
        }
        return ridx;
    }

    /// Deletes a region from the mesh.
    /// This method assumes that the region is not referenced by any other part of the mesh.
    void deleteRegion(region_index region) {
        OVITO_ASSERT(isTopologyMutable());
        OVITO_ASSERT(areRegionPropertiesMutable());
        OVITO_ASSERT(areFacePropertiesMutable());
        OVITO_ASSERT(region >= 0 && region < regionCount());
        OVITO_ASSERT(std::none_of(topology()->begin_faces(), topology()->end_faces(), [&](auto face) { return faceRegion(face) == region; } ));
        if(region < regionCount() - 1) {
            // Move the last region to the index of the region being deleted.
            for(const auto& prop : _regionProperties) {
                OVITO_ASSERT(prop->size() == regionCount());
                std::memcpy(prop->dataAt(region), prop->constDataAt(prop->size() - 1), prop->stride());
            }
            // Update the faces that belong to the moved region.
            if(hasFaceRegions())
                std::replace(_faceRegions, _faceRegions + faceCount(), regionCount() - 1, region);
        }
        // Truncate the region property arrays.
        for(const auto& prop : _regionProperties) {
            prop->truncate(1);
        }
        _regionCount--;
    }

    /// Links two opposite half-edges together.
    void linkOppositeEdges(edge_index edge1, edge_index edge2) {
        OVITO_ASSERT(isTopologyMutable());
        topology()->linkOppositeEdges(edge1, edge2);
    }

    /// Transforms all vertices of the mesh with the given affine transformation matrix.
    void transformVertices(const AffineTransformation tm) {
        OVITO_ASSERT(isVertexPropertyMutable(SurfaceMeshVertices::PositionProperty));
        for(Point3& p : boost::make_iterator_range(vertexCoords(), vertexCoords() + vertexCount())) {
            p = tm * p;
        }
    }

    /// Returns the simulation box the surface mesh is embedded in.
    const SimulationCell& cell() const { return _cell; }

    /// Returns a mutable reference to the simulation box the surface mesh is embedded in.
    SimulationCell& cell() { return _cell; }

    /// Returns the vector corresponding to an half-edge of the surface mesh.
    Vector3 edgeVector(edge_index edge) const {
        return cell().wrapVector(vertexPosition(vertex2(edge)) - vertexPosition(vertex1(edge)));
    }

    /// Flips the orientation of all faces in the mesh.
    void flipFaces() {
        OVITO_ASSERT(isTopologyMutable());
        topology()->flipFaces();
    }

    /// Tries to wire each half-edge with its opposite (reverse) half-edge.
    /// Returns true if every half-edge has an opposite half-edge, i.e. if the mesh
    /// is closed after this method returns.
    bool connectOppositeHalfedges() {
        OVITO_ASSERT(isTopologyMutable());
        return topology()->connectOppositeHalfedges();
    }

    /// Duplicates those vertices which are shared by more than one manifold.
    /// The method may only be called on a closed mesh.
    /// Returns the number of vertices that were duplicated by the method.
    size_type makeManifold() {
        OVITO_ASSERT(isTopologyMutable());
        OVITO_ASSERT(areVertexPropertiesMutable());
        return topology()->makeManifold([this](HalfEdgeMesh::vertex_index copiedVertex) {
            // Duplicate the property data of the copied vertex.
            for(const auto& prop : _vertexProperties) {
                if(prop->grow(1))
                    updateVertexPropertyPointers(prop);
                std::memcpy(prop->dataAt(prop->size() - 1), prop->constDataAt(copiedVertex), prop->stride());
            }
        });
    }

	/// Fairs the surface mesh.
	bool smoothMesh(int numIterations, Task& task, FloatType k_PB = FloatType(0.1), FloatType lambda = FloatType(0.5));

	/// Determines which spatial region contains the given point in space.
	/// Returns no result if the point is exactly on a region boundary.
	boost::optional<region_index> locatePoint(const Point3& location, FloatType epsilon = FLOATTYPE_EPSILON, const boost::dynamic_bitset<>& faceSubset = boost::dynamic_bitset<>()) const;

    /// Returns one of the standard vertex properties (or null if the property is not defined).
    PropertyPtr vertexProperty(SurfaceMeshVertices::Type ptype) const {
        for(const auto& property : _vertexProperties)
            if(property->type() == ptype)
                return property;
        return {};
    }

    /// Returns one of the standard face properties (or null if the property is not defined).
    PropertyPtr faceProperty(SurfaceMeshFaces::Type ptype) const {
        for(const auto& property : _faceProperties)
            if(property->type() == ptype)
                return property;
        return {};
    }

    /// Returns one of the standard spatial region properties (or null if the property is not defined).
    PropertyPtr regionProperty(SurfaceMeshRegions::Type ptype) const {
        for(const auto& property : _regionProperties)
            if(property->type() == ptype)
                return property;
        return {};
    }

    /// Adds a new standard vertex property to the mesh.
    PropertyPtr createVertexProperty(SurfaceMeshVertices::Type ptype, bool initialize = false) {
        OVITO_ASSERT(ptype != SurfaceMeshVertices::UserProperty);
        // Check if already exists.
        PropertyPtr prop = vertexProperty(ptype);
        if(!prop) {
            prop = SurfaceMeshVertices::OOClass().createStandardStorage(vertexCount(), ptype, initialize);
            addVertexProperty(prop);
        }
        return prop;
    }

	/// Adds a new user property to the mesh vertices.
	PropertyPtr createVertexProperty(int dataType, size_t componentCount, size_t stride, const QString& name, bool initializeMemory, QStringList componentNames = QStringList(), SurfaceMeshVertices::Type type = SurfaceMeshVertices::UserProperty) {
        PropertyPtr prop = std::make_shared<PropertyStorage>(vertexCount(), dataType, componentCount, stride, name, initializeMemory, type, std::move(componentNames));
        addVertexProperty(prop);
        return prop;
    }

    /// Adds a new standard face property to the mesh.
    PropertyPtr createFaceProperty(SurfaceMeshFaces::Type ptype, bool initialize = false) {
        OVITO_ASSERT(ptype != SurfaceMeshFaces::UserProperty);
        // Check if already exists.
        PropertyPtr prop = faceProperty(ptype);
        if(!prop) {
            prop = SurfaceMeshFaces::OOClass().createStandardStorage(faceCount(), ptype, initialize);
            addFaceProperty(prop);
        }
        return prop;
    }

	/// Adds a new user property to the mesh faces.
	PropertyPtr createFaceProperty(int dataType, size_t componentCount, size_t stride, const QString& name, bool initializeMemory, QStringList componentNames = QStringList(), SurfaceMeshFaces::Type type = SurfaceMeshFaces::UserProperty) {
        PropertyPtr prop = std::make_shared<PropertyStorage>(faceCount(), dataType, componentCount, stride, name, initializeMemory, type, std::move(componentNames));
        addFaceProperty(prop);
        return prop;
    }

    /// Removes a property from the faces of this mesh.
    void removeFaceProperty(const PropertyStorage* property) {
        OVITO_ASSERT(property);
        auto iter = _faceProperties.begin();
        for(; iter != _faceProperties.end(); ++iter)
            if(iter->get() == property) break;
        OVITO_ASSERT(iter != _faceProperties.end());
        OVITO_ASSERT(property->size() == faceCount());
        updateFacePropertyPointers(*iter, false);
        _faceProperties.erase(iter);
    }

    /// Adds a new standard property to the spatial regions of the mesh.
    PropertyPtr createRegionProperty(SurfaceMeshRegions::Type ptype, bool initialize = false) {
        OVITO_ASSERT(ptype != SurfaceMeshRegions::UserProperty);
        // Check if already exists.
        PropertyPtr prop = regionProperty(ptype);
        if(!prop) {
            prop = SurfaceMeshRegions::OOClass().createStandardStorage(regionCount(), ptype, initialize);
            addRegionProperty(prop);
        }
        return prop;
    }

	/// Adds a new user property to the mesh regions.
	PropertyPtr createRegionProperty(int dataType, size_t componentCount, size_t stride, const QString& name, bool initializeMemory, QStringList componentNames = QStringList(), SurfaceMeshRegions::Type type = SurfaceMeshRegions::UserProperty) {
        PropertyPtr prop = std::make_shared<PropertyStorage>(regionCount(), dataType, componentCount, stride, name, initializeMemory, type, std::move(componentNames));
        addRegionProperty(prop);
        return prop;
    }

    /// Adds a mesh vertex property array to the list of vertex properties.
    void addVertexProperty(PropertyPtr property) {
        OVITO_ASSERT(property);
        OVITO_ASSERT(std::find(_vertexProperties.cbegin(), _vertexProperties.cend(), property) == _vertexProperties.cend());
        OVITO_ASSERT(property->type() == SurfaceMeshVertices::UserProperty || std::none_of(_vertexProperties.cbegin(), _vertexProperties.cend(), [t = property->type()](const PropertyPtr& p) { return p->type() == t; }));
        OVITO_ASSERT(property->size() == vertexCount());
        updateVertexPropertyPointers(property);
        _vertexProperties.push_back(std::move(property));
    }

    /// Adds a mesh face property array to the list of face properties.
    void addFaceProperty(PropertyPtr property) {
        OVITO_ASSERT(property);
        OVITO_ASSERT(std::find(_faceProperties.cbegin(), _faceProperties.cend(), property) == _faceProperties.cend());
        OVITO_ASSERT(property->type() == SurfaceMeshFaces::UserProperty || std::none_of(_faceProperties.cbegin(), _faceProperties.cend(), [t = property->type()](const PropertyPtr& p) { return p->type() == t; }));
        OVITO_ASSERT(property->size() == faceCount());
        updateFacePropertyPointers(property);
        _faceProperties.push_back(std::move(property));
    }

    /// Adds a property array to the list of region properties.
    void addRegionProperty(PropertyPtr property) {
        OVITO_ASSERT(property);
        OVITO_ASSERT(std::find(_regionProperties.cbegin(), _regionProperties.cend(), property) == _regionProperties.cend());
        OVITO_ASSERT(property->type() == SurfaceMeshRegions::UserProperty || std::none_of(_regionProperties.cbegin(), _regionProperties.cend(), [t = property->type()](const PropertyPtr& p) { return p->type() == t; }));
        OVITO_ASSERT(property->size() == regionCount());
        updateRegionPropertyPointers(property);
        _regionProperties.push_back(std::move(property));
    }

    /// Constructs the convex hull from a set of points and adds the resulting polyhedron to the mesh.
    void constructConvexHull(std::vector<Point3> vecs);

    /// Joins adjacent faces that are coplanar.
    void joinCoplanarFaces(FloatType thresholdAngle = qDegreesToRadians(0.01));

    /// Triangulates the polygonal faces of this mesh and outputs the results as a TriMesh object.
    void convertToTriMesh(TriMesh& outputMesh, bool smoothShading, const boost::dynamic_bitset<>& faceSubset = boost::dynamic_bitset<>{}, std::vector<size_t>* originalFaceMap = nullptr, bool autoGenerateOppositeFaces = false) const;

    /// Swaps the contents of two surface meshes.
    void swap(SurfaceMeshData& other);

    /// Computes the unit normal vector of a mesh face.
    Vector3 computeFaceNormal(face_index face) const;

    /// Makes a copy of the topology data structure if it is currently referenced by other owners. 
    void makeTopologyMutable() {
        if(!isTopologyMutable())
            _topology = std::make_shared<HalfEdgeMesh>(*topology());
        OVITO_ASSERT(isTopologyMutable());
    }

    /// Makes a deep copy of every vertex property array that is are currently referenced by other owners. 
    void makeVerticesMutable() {
        for(PropertyPtr& property : _vertexProperties) {
            PropertyStorage::makeMutable(property);
            updateVertexPropertyPointers(property);
        }
        OVITO_ASSERT(areVertexPropertiesMutable());
    }

    /// Makes a deep copy of every face property array that is are currently referenced by other owners. 
    void makeFacesMutable() {
        for(PropertyPtr& property : _faceProperties) {
            PropertyStorage::makeMutable(property);
            updateFacePropertyPointers(property);
        }
        OVITO_ASSERT(areFacePropertiesMutable());
    }

    /// Makes a deep copy of every region property array that is are currently referenced by other owners. 
    void makeRegionsMutable() {
        for(PropertyPtr& property : _regionProperties) {
            PropertyStorage::makeMutable(property);
            updateRegionPropertyPointers(property);
        }
        OVITO_ASSERT(areRegionPropertiesMutable());
    }

protected:

    void updateVertexPropertyPointers(const PropertyPtr& property, bool inserted = true) {
        if(property->type() == SurfaceMeshVertices::PositionProperty)
            _vertexCoords = inserted ? property->dataPoint3() : nullptr;
    }

    void updateFacePropertyPointers(const PropertyPtr& property, bool inserted = true) {
        switch(property->type()) {
        case SurfaceMeshFaces::RegionProperty:
            _faceRegions = inserted ? property->dataInt() : nullptr;
            break;
        case SurfaceMeshFaces::BurgersVectorProperty:
            _burgersVectors = inserted ? property->dataVector3() : nullptr;
            break;
        case SurfaceMeshFaces::CrystallographicNormalProperty:
            _crystallographicNormals = inserted ? property->dataVector3() : nullptr;
            break;
        case SurfaceMeshFaces::FaceTypeProperty:
            _faceTypes = inserted ? property->dataInt() : nullptr;
            break;
        }
    }

    void updateRegionPropertyPointers(const PropertyPtr& property, bool inserted = true) {
        if(property->type() == SurfaceMeshRegions::PhaseProperty)
            _regionPhases = inserted ? property->dataInt() : nullptr;
        else if(property->type() == SurfaceMeshRegions::VolumeProperty)
            _regionVolumes = inserted ? property->dataFloat() : nullptr;
        else if(property->type() == SurfaceMeshRegions::SurfaceAreaProperty)
            _regionSurfaceAreas = inserted ? property->dataFloat() : nullptr;
    }

    /// Returns whether the mesh topology may be safely modified without unwanted side effects.
    bool isTopologyMutable() const {
        return topology().use_count() == 1;
    }

    /// Returns whether the properties of the mesh vertices may be safely modified without unwanted side effects.
    bool areVertexPropertiesMutable() const {
        return std::all_of(_vertexProperties.begin(), _vertexProperties.end(), [](const PropertyPtr& p) { return p.use_count() == 1; });
    }

    /// Returns whether the properties of the mesh faces may be safely modified without unwanted side effects.
    bool areFacePropertiesMutable() const {
        return std::all_of(_faceProperties.begin(), _faceProperties.end(), [](const PropertyPtr& p) { return p.use_count() == 1; });
    }

    /// Returns whether the properties of the mesh regions may be safely modified without unwanted side effects.
    bool areRegionPropertiesMutable() const {
        return std::all_of(_regionProperties.begin(), _regionProperties.end(), [](const PropertyPtr& p) { return p.use_count() == 1; });
    }

    /// Returns whether the given property of the mesh vertices may be safely modified without unwanted side effects.
    bool isVertexPropertyMutable(SurfaceMeshVertices::Type ptype) const {
        for(const auto& property : _vertexProperties)
            if(property->type() == ptype)
                return property.use_count() == 1;
        return false;
    }

    /// Returns whether the given property of the mesh faces may be safely modified without unwanted side effects.
    bool isFacePropertyMutable(SurfaceMeshFaces::Type ptype) const {
        for(const auto& property : _faceProperties)
            if(property->type() == ptype)
                return property.use_count() == 1;
        return false;
    }

    /// Returns whether the given property of the spatial regions may be safely modified without unwanted side effects.
    bool isRegionPropertyMutable(SurfaceMeshRegions::Type ptype) const {
        for(const auto& property : _regionProperties)
            if(property->type() == ptype)
                return property.use_count() == 1;
        return false;
    }

    /// Returns the cached raw pointer to the per-vertex mesh coordinates.
    Point3* vertexCoords() const { OVITO_ASSERT(_vertexCoords != nullptr); return _vertexCoords; }

    /// Returns the cached raw pointer to the per-face region IDs.
    int* faceRegions() const { OVITO_ASSERT(_faceRegions != nullptr); return _faceRegions; }

    /// Returns the cached raw pointer to the per-face Burgers vectors.
    Vector3* burgersVectors() const { OVITO_ASSERT(_burgersVectors != nullptr); return _burgersVectors; }

    /// Returns the cached raw pointer to the per-region phase information.
    int* regionPhases() const { OVITO_ASSERT(_regionPhases != nullptr); return _regionPhases; }

    /// Returns the cached raw pointer to the per-face crystallographic normal vectors.
    Vector3* crystallographicNormals() const { OVITO_ASSERT(_crystallographicNormals != nullptr); return _crystallographicNormals; }

    /// Returns the cached raw pointer to the per-face type values.
    int* faceTypes() const { OVITO_ASSERT(_faceTypes != nullptr); return _faceTypes; }

private:

    HalfEdgeMeshPtr _topology;                  ///< Holds the mesh topology of the surface mesh.
    SimulationCell _cell;                       ///< The simulation cell the microstructure is embedded in.
    std::vector<PropertyPtr> _vertexProperties;	///< List of all property arrays associated with the vertices of the mesh.
    std::vector<PropertyPtr> _faceProperties;   ///< List of all property arrays associated with the faces of the mesh.
    std::vector<PropertyPtr> _regionProperties; ///< List of all property arrays associated with the volumetric domains of the mesh.
    size_type _regionCount = 0;                 ///< The number of spatial regions that have been defined.
    region_index _spaceFillingRegion = HalfEdgeMesh::InvalidIndex;      ///< The index of the space-filling spatial region.
    Point3* _vertexCoords = nullptr;            ///< Pointer to the per-vertex mesh coordinates.
    int* _faceRegions = nullptr;                ///< Pointer to the per-face region information.
	Vector3* _burgersVectors = nullptr;         ///< Pointer to the per-face Burgers vector information.
	Vector3* _crystallographicNormals = nullptr;///< Pointer to the per-face crystallographic normal information.
	int* _faceTypes = nullptr;                  ///< Pointer to the per-face type information.
	int* _regionPhases = nullptr;               ///< Pointer to the per-region phase information.
	FloatType* _regionVolumes = nullptr;        ///< Pointer to the per-region volume information.
	FloatType* _regionSurfaceAreas = nullptr;   ///< Pointer to the per-region surface area information.
};

}	// End of namespace
}	// End of namespace
