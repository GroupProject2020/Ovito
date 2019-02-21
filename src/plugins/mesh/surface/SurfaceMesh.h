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


#include <plugins/mesh/Mesh.h>
#include <plugins/stdobj/simcell/PeriodicDomainDataObject.h>
#include <plugins/stdobj/simcell/SimulationCellObject.h>
#include <plugins/mesh/halfedge/HalfEdgeMesh.h>
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

	/// \brief Constructor that creates an empty SurfaceMesh object.
	Q_INVOKABLE SurfaceMesh(DataSet* dataset, const QString& title = QString());

	/// Returns the title of this object.
	virtual QString objectTitle() const override {
		if(!title().isEmpty()) return title();
		else if(!identifier().isEmpty()) return identifier();
		else return tr("Surface mesh");
	}

	/// Checks if the surface mesh is valid and all vertex and face properties 
	/// are consistent with the topology of the mesh. If this is not the case, 
	/// the method throws an exception. 
	void verifyMeshIntegrity() const;

	/// Returns the topology data after making sure it is not shared with any other owners.
	const HalfEdgeMeshPtr& modifiableTopology();

	/// Duplicates the SurfaceMeshVertices sub-object if it is shared with other surface meshes.
	/// After this method returns, the sub-object is exclusively owned by the container and 
	/// can be safely modified without unwanted side effects.
	SurfaceMeshVertices* makeVerticesMutable();

	/// Duplicates the SurfaceMeshFaces sub-object if it is shared with other surface meshes.
	/// After this method returns, the sub-object is exclusively owned by the container and 
	/// can be safely modified without unwanted side effects.
	SurfaceMeshFaces* makeFacesMutable();

	/// Fairs the triangle mesh stored in this object.
	bool smoothMesh(int numIterations, PromiseState& promise, FloatType k_PB = FloatType(0.1), FloatType lambda = FloatType(0.5)) {
		if(!domain() || !topology() || !vertices())
			return true;
		PropertyObject* vertexCoords = makeVerticesMutable()->expectMutableProperty(SurfaceMeshVertices::PositionProperty); 
		if(!vertexCoords)
			return true;
		if(!smoothMesh(*topology(), *vertexCoords->modifiableStorage(), domain()->data(), numIterations, promise, k_PB, lambda))
			return false;
		notifyTargetChanged();
		return true;
	}

	/// Fairs a triangle mesh.
	static bool smoothMesh(const HalfEdgeMesh& mesh, PropertyStorage& vertexCoords, const SimulationCell& cell, int numIterations, PromiseState& promise, FloatType k_PB = FloatType(0.1), FloatType lambda = FloatType(0.5));

	/// Determines which spatial region contains the given point in space.
	/// Returns -1 if the point is exactly on a region boundary.
	int locatePoint(const Point3& location, FloatType epsilon = FLOATTYPE_EPSILON) const;
	
	/// Static implementation function of the locatePoint() method.
	static int locatePointStatic(const Point3& location, const HalfEdgeMesh& mesh, const PropertyStorage& vertexCoords, const SimulationCell cell, int spaceFillingRegion, const ConstPropertyPtr& faceRegions = nullptr, FloatType epsilon = FLOATTYPE_EPSILON);

protected:

	/// Performs one iteration of the smoothing algorithm.
	static void smoothMeshIteration(const HalfEdgeMesh& mesh, PropertyStorage& vertexCoords, FloatType prefactor, const SimulationCell& cell);

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
