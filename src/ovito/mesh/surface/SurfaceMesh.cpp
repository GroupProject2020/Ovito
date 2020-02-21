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

#include <ovito/mesh/Mesh.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include "SurfaceMesh.h"
#include "SurfaceMeshVis.h"

namespace Ovito { namespace Mesh {

IMPLEMENT_OVITO_CLASS(SurfaceMesh);
DEFINE_PROPERTY_FIELD(SurfaceMesh, topology);
DEFINE_PROPERTY_FIELD(SurfaceMesh, spaceFillingRegion);
DEFINE_REFERENCE_FIELD(SurfaceMesh, vertices);
DEFINE_REFERENCE_FIELD(SurfaceMesh, faces);
DEFINE_REFERENCE_FIELD(SurfaceMesh, regions);
SET_PROPERTY_FIELD_LABEL(SurfaceMesh, vertices, "Vertices");
SET_PROPERTY_FIELD_LABEL(SurfaceMesh, faces, "Faces");
SET_PROPERTY_FIELD_LABEL(SurfaceMesh, regions, "Regions");

/******************************************************************************
* Constructs an empty surface mesh object.
******************************************************************************/
SurfaceMesh::SurfaceMesh(DataSet* dataset, const QString& title) : PeriodicDomainDataObject(dataset, title),
	_spaceFillingRegion(0)
{
	// Attach a visualization element for rendering the surface mesh.
	addVisElement(new SurfaceMeshVis(dataset));

	// Create the sub-object for storing the vertex properties.
	setVertices(new SurfaceMeshVertices(dataset));

	// Create the sub-object for storing the face properties.
	setFaces(new SurfaceMeshFaces(dataset));

	// Create the sub-object for storing the region properties.
	setRegions(new SurfaceMeshRegions(dataset));
}

/******************************************************************************
* Checks if the surface mesh is valid and all vertex and face properties
* are consistent with the topology of the mesh. If this is not the case,
* the method throws an exception.
******************************************************************************/
void SurfaceMesh::verifyMeshIntegrity() const
{
	if(!topology())
		throwException(tr("Surface mesh has no topology object attached."));

	if(!vertices())
		throwException(tr("Surface mesh has no vertex properties container attached."));
	if(!vertices()->getProperty(SurfaceMeshVertices::PositionProperty))
		throwException(tr("Invalid data structure. Surface mesh is missing the position vertex property."));
	if(topology()->vertexCount() != vertices()->elementCount())
		throwException(tr("Length of vertex property arrays of surface mesh do not match number of vertices in the mesh topology."));

	if(!faces())
		throwException(tr("Surface mesh has no face properties container attached."));
	if(!faces()->properties().empty() && topology()->faceCount() != faces()->elementCount())
		throwException(tr("Length of face property arrays of surface mesh do not match number of faces in the mesh topology."));

	if(!regions())
		throwException(tr("Surface mesh has no region properties container attached."));

	if(spaceFillingRegion() != HalfEdgeMesh::InvalidIndex && spaceFillingRegion() < 0)
		throwException(tr("Space filling region ID set for surface mesh must not be negative."));

	vertices()->verifyIntegrity();
	faces()->verifyIntegrity();
	regions()->verifyIntegrity();
}

/******************************************************************************
* Returns the topology data after making sure it is not shared with other owners.
******************************************************************************/
const HalfEdgeMeshPtr& SurfaceMesh::modifiableTopology()
{
	// Copy data if there is more than one reference to the storage.
	OVITO_ASSERT(topology());
	OVITO_ASSERT(topology().use_count() >= 1);
	if(topology().use_count() > 1)
		_topology.mutableValue() = std::make_shared<HalfEdgeMesh>(*topology());
	OVITO_ASSERT(topology().use_count() == 1);
	return topology();
}

/******************************************************************************
* Determines which spatial region contains the given point in space.
* Returns -1 if the point is exactly on a region boundary.
******************************************************************************/
boost::optional<SurfaceMeshData::region_index> SurfaceMesh::locatePoint(const Point3& location, FloatType epsilon) const
{
	verifyMeshIntegrity();
	return SurfaceMeshData(this).locatePoint(location, epsilon);
}

}	// End of namespace
}	// End of namespace
