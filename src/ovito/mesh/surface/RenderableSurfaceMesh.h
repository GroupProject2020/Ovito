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


#include <ovito/mesh/Mesh.h>
#include <ovito/core/dataset/data/TransformedDataObject.h>
#include <ovito/core/utilities/mesh/TriMesh.h>

namespace Ovito { namespace Mesh {

/**
 * \brief A non-periodic triangle mesh that is generated from a periodic SurfaceMesh.
 */
class OVITO_MESH_EXPORT RenderableSurfaceMesh : public TransformedDataObject
{
	Q_OBJECT
	OVITO_CLASS(RenderableSurfaceMesh)
	Q_CLASSINFO("DisplayName", "Renderable surface mesh");

public:

	/// \brief Standard constructor.
	Q_INVOKABLE RenderableSurfaceMesh(DataSet* dataset) : TransformedDataObject(dataset) {}

	/// \brief Initialization constructor.
	RenderableSurfaceMesh(TransformingDataVis* creator, const DataObject* sourceData, TriMesh surfaceMesh, TriMesh capPolygonsMesh);

private:

	/// The surface part of the mesh.
	DECLARE_RUNTIME_PROPERTY_FIELD(TriMesh, surfaceMesh, setSurfaceMesh);

	/// The cap polygon part of the mesh.
	DECLARE_RUNTIME_PROPERTY_FIELD(TriMesh, capPolygonsMesh, setCapPolygonsMesh);

	/// The material colors assigned to the surface mesh (optional).
	DECLARE_RUNTIME_PROPERTY_FIELD(std::vector<ColorA>, materialColors, setMaterialColors);

	/// The mapping of triangles of the renderable surface mesh to the original mesh (optional).
	DECLARE_RUNTIME_PROPERTY_FIELD(std::vector<size_t>, originalFaceMap, setOriginalFaceMap);
};

}	// End of namespace
}	// End of namespace
