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
#include <ovito/stdobj/simcell/PeriodicDomainDataObject.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/mesh/surface/SurfaceMeshData.h>
#include "SurfaceMeshVertices.h"
#include "SurfaceMeshFaces.h"
#include "SurfaceMeshRegions.h"

namespace Ovito { namespace Mesh {

/**
 * \brief A closed mesh representing a surface, i.e. a two-dimensional manifold.
 */
class OVITO_MESH_EXPORT SurfaceMesh : public PeriodicDomainDataObject
{
	Q_OBJECT
	OVITO_CLASS(SurfaceMesh)

public:

	/// Constructor creating an empty SurfaceMesh object.
	Q_INVOKABLE SurfaceMesh(DataSet* dataset, const QString& title = QString());

	/// Returns the display title of this object.
	virtual QString objectTitle() const override {
		if(!title().isEmpty()) return title();
		else if(!identifier().isEmpty()) return identifier();
		else return tr("Surface mesh");
	}

	/// Makes sure that the data structures of the surface mesh are valid and all vertex and face properties
	/// are consistent with the topology of the mesh. If this is not the case, the method throws an exception.
	void verifyMeshIntegrity() const;

	/// Returns the topology data after making sure it is not shared with any other owners.
	const HalfEdgeMeshPtr& modifiableTopology();

	/// Duplicates the SurfaceMeshVertices sub-object if it is shared with other surface meshes.
	/// After this method returns, the sub-object is exclusively owned by the container and
	/// can be safely modified without unwanted side effects.
	SurfaceMeshVertices* makeVerticesMutable() {
		OVITO_ASSERT(vertices());
	    return makeMutable(vertices());
	}

	/// Duplicates the SurfaceMeshFaces sub-object if it is shared with other surface meshes.
	/// After this method returns, the sub-object is exclusively owned by the container and
	/// can be safely modified without unwanted side effects.
	SurfaceMeshFaces* makeFacesMutable() {
		OVITO_ASSERT(faces());
	    return makeMutable(faces());
	}

	/// Duplicates the SurfaceMeshRegions sub-object if it is shared with other surface meshes.
	/// After this method returns, the sub-object is exclusively owned by the container and
	/// can be safely modified without unwanted side effects.
	SurfaceMeshRegions* makeRegionsMutable() {
		OVITO_ASSERT(regions());
	    return makeMutable(regions());
	}

	/// Determines which spatial region contains the given location in space.
	boost::optional<SurfaceMeshData::region_index> locatePoint(const Point3& location, FloatType epsilon = FLOATTYPE_EPSILON) const;

private:

	/// The assigned title of the mesh, which is displayed in the user interface.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(QString, title, setTitle);

	/// The data structure storing the topology of the surface mesh.
	DECLARE_RUNTIME_PROPERTY_FIELD(HalfEdgeMeshPtr, topology, setTopology);

	/// The container holding the mesh vertex properties.
	DECLARE_MODIFIABLE_REFERENCE_FIELD(SurfaceMeshVertices, vertices, setVertices);

	/// The container holding the mesh face properties.
	DECLARE_MODIFIABLE_REFERENCE_FIELD(SurfaceMeshFaces, faces, setFaces);

	/// The container holding the properties of the volumetric regions enclosed by the mesh.
	DECLARE_MODIFIABLE_REFERENCE_FIELD(SurfaceMeshRegions, regions, setRegions);

	/// If the mesh has zero faces and is embedded in a fully periodic domain,
	/// this indicates the volumetric region that fills the entire space.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, spaceFillingRegion, setSpaceFillingRegion);
};

}	// End of namespace
}	// End of namespace
