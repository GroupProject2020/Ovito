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

#pragma once


#include <plugins/grid/Grid.h>
#include <plugins/mesh/surface/SurfaceMeshData.h>
#include <core/utilities/concurrent/Task.h>

namespace Ovito { namespace Grid {

/**
* The Marching Cubes algorithm for constructing isosurfaces from grid data.
*/
class MarchingCubes
{
public:

    // Constructor
    MarchingCubes(SurfaceMeshData& outputMesh, int size_x, int size_y, int size_z, const FloatType* fielddata, size_t stride, bool lowerIsSolid);

    /// Returns the field value in a specific cube of the grid.
    /// Takes into account periodic boundary conditions.
    inline FloatType getFieldValue(int i, int j, int k) const {
        if(_pbcFlags[0]) {
            if(i == _size_x) i = 0;
        }
        else {
            if(i == 0 || i == _size_x) return std::numeric_limits<FloatType>::lowest();
            i--;
        }
        if(_pbcFlags[1]) {
            if(j == _size_y) j = 0;
        }
        else {
            if(j == 0 || j == _size_y) return std::numeric_limits<FloatType>::lowest();
            j--;
        }
        if(_pbcFlags[2]) {
            if(k == _size_z) k = 0;
        }
        else {
            if(k == 0 || k == _size_z) return std::numeric_limits<FloatType>::lowest();
            k--;
        }
        OVITO_ASSERT(i >= 0 && i < _data_size_x);
        OVITO_ASSERT(j >= 0 && j < _data_size_y);
        OVITO_ASSERT(k >= 0 && k < _data_size_z);
        return _data[(i + j*_data_size_x + k*_data_size_x*_data_size_y) * _dataStride];
    }

    bool generateIsosurface(FloatType iso, Task& task);

    /// Returns the generated surface mesh.
    const SurfaceMeshData& mesh() const { return _outputMesh; }

protected:

    /// Tessellates one cube.
    void processCube(int i, int j, int k);

    /// tTests if the components of the tessellation of the cube should be
    /// connected by the interior of an ambiguous face.
    bool testFace(char face);

    /// Tests if the components of the tessellation of the cube should be
    /// connected through the interior of the cube.
    bool testInterior(char s);

    /// Computes almost all the vertices of the mesh by interpolation along the cubes edges.
    void computeIntersectionPoints(FloatType iso, Task& promise);

    /// Adds triangles to the mesh.
    void addTriangle(int i, int j, int k, const char* trig, char n, HalfEdgeMesh::vertex_index v12 = HalfEdgeMesh::InvalidIndex);

    /// Adds a vertex on the current horizontal edge.
    HalfEdgeMesh::vertex_index createEdgeVertexX(int i, int j, int k, FloatType u) {
        OVITO_ASSERT(i >= 0 && i < _size_x);
        OVITO_ASSERT(j >= 0 && j < _size_y);
        OVITO_ASSERT(k >= 0 && k < _size_z);
        auto v = _outputMesh.createVertex(Point3(i + u - (_pbcFlags[0]?0:1), j - (_pbcFlags[1]?0:1), k - (_pbcFlags[2]?0:1)));
        _cubeVerts[(i + j*_size_x + k*_size_x*_size_y)*3 + 0] = v;
        return v;
    }

    /// Adds a vertex on the current longitudinal edge.
    HalfEdgeMesh::vertex_index createEdgeVertexY(int i, int j, int k, FloatType u) {
        OVITO_ASSERT(i >= 0 && i < _size_x);
        OVITO_ASSERT(j >= 0 && j < _size_y);
        OVITO_ASSERT(k >= 0 && k < _size_z);
        auto v = _outputMesh.createVertex(Point3(i - (_pbcFlags[0]?0:1), j + u - (_pbcFlags[1]?0:1), k - (_pbcFlags[2]?0:1)));
        _cubeVerts[(i + j*_size_x + k*_size_x*_size_y)*3 + 1] = v;
        return v;
    }

    /// Adds a vertex on the current vertical edge.
    HalfEdgeMesh::vertex_index createEdgeVertexZ(int i, int j, int k, FloatType u) {
        OVITO_ASSERT(i >= 0 && i < _size_x);
        OVITO_ASSERT(j >= 0 && j < _size_y);
        OVITO_ASSERT(k >= 0 && k < _size_z);
        auto v = _outputMesh.createVertex(Point3(i - (_pbcFlags[0]?0:1), j - (_pbcFlags[1]?0:1), k + u - (_pbcFlags[2]?0:1)));
        _cubeVerts[(i + j*_size_x + k*_size_x*_size_y)*3 + 2] = v;
        return v;
    }

    /// Adds a vertex inside the current cube.
    HalfEdgeMesh::vertex_index createCenterVertex(int i, int j, int k);

    /// Accesses the pre-computed vertex on a lower edge of a specific cube.
    HalfEdgeMesh::vertex_index getEdgeVert(int i, int j, int k, int axis) const {
        OVITO_ASSERT(i >= 0 && i <= _size_x);
        OVITO_ASSERT(j >= 0 && j <= _size_y);
        OVITO_ASSERT(k >= 0 && k <= _size_z);
        OVITO_ASSERT(axis >= 0 && axis < 3);
        if(i == _size_x) i = 0;
        if(j == _size_y) j = 0;
        if(k == _size_z) k = 0;
        return _cubeVerts[(i + j*_size_x + k*_size_x*_size_y)*3 + axis];
    }

protected:

	const std::array<bool,3> _pbcFlags; ///< PBC flags
    int _data_size_x;  ///< width  of the input data grid
    int _data_size_y;  ///< depth  of the input data grid
    int _data_size_z;  ///< height of the input data grid
    int _size_x;  ///< width  of the grid
    int _size_y;  ///< depth  of the grid
    int _size_z;  ///< height of the grid
    const FloatType* _data;  ///< implicit function values sampled on the grid
    size_t _dataStride;
    bool _lowerIsSolid; ///< Controls the inward/outward orientation of the created triangle surface.

    /// Vertices created along cube edges.
    std::vector<HalfEdgeMesh::vertex_index> _cubeVerts;

    FloatType     _cube[8];   ///< values of the implicit function on the active cube
    unsigned char _lut_entry; ///< cube sign representation in [0..255]
    unsigned char _case;      ///< case of the active cube in [0..15]
    unsigned char _config;    ///< configuration of the active cube
    unsigned char _subconfig; ///< subconfiguration of the active cube

    /// The generated surface mesh.
    SurfaceMeshData& _outputMesh;

#ifdef FLOATTYPE_FLOAT
    static constexpr FloatType _epsilon = FloatType(1e-12);
#else
    static constexpr FloatType _epsilon = FloatType(1e-18);
#endif
};

}	// End of namespace
}	// End of namespace
