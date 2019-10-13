////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2017 Alexander Stukowski
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
