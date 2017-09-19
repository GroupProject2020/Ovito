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


#include <plugins/mesh/Mesh.h>
#include <core/dataset/data/simcell/PeriodicDomainDataObject.h>
#include <core/dataset/data/simcell/SimulationCellObject.h>
#include <core/utilities/mesh/HalfEdgeMesh.h>

namespace Ovito { namespace Mesh {

/// Typically, surface meshes are shallow copied. That's why we use a shared_ptr to hold on to them.
using SurfaceMeshPtr = std::shared_ptr<HalfEdgeMesh<>>;

/// This pointer type is used to indicate that we only need read-only access to the mesh data.
using ConstSurfaceMeshPtr = std::shared_ptr<const HalfEdgeMesh<>>;	

/**
 * \brief A closed triangle mesh representing a surface.
 */
class OVITO_MESH_EXPORT SurfaceMesh : public PeriodicDomainDataObject
{
	Q_OBJECT
	OVITO_CLASS(SurfaceMesh)
	
public:

	/// \brief Constructor that creates an empty SurfaceMesh object.
	Q_INVOKABLE SurfaceMesh(DataSet* dataset);

	/// Returns the title of this object.
	virtual QString objectTitle() override { return tr("Surface mesh"); }

	/// \brief Returns whether this object, when returned as an editable sub-object by another object,
	///        should be displayed in the modification stack.
	///
	/// Return false because this object cannot be edited.
	virtual bool isSubObjectEditable() const override { return false; }

	/// Returns the data encapsulated by this object after making sure it is not shared with any other owners.
	const SurfaceMeshPtr& modifiableStorage();

	/// Determines if a spatial location is inside or oustide of the region enclosed by the surface.
	/// Return value:
	///     -1 : The point is inside the enclosed region
	///      0 : The point is right on the surface (approximately, within the given epsilon)
	///     +1 : The point is outside the enclosed region
	int locatePoint(const Point3& location, FloatType epsilon = FLOATTYPE_EPSILON) const;
	
	/// Fairs the triangle mesh stored in this object.
	bool smoothMesh(int numIterations, PromiseState& promise, FloatType k_PB = FloatType(0.1), FloatType lambda = FloatType(0.5)) {
		if(!domain() || !storage()) 
			return true;
		if(!smoothMesh(*modifiableStorage(), domain()->data(), numIterations, promise, k_PB, lambda))
			return false;
		notifyDependents(ReferenceEvent::TargetChanged);
		return true;
	}

	/// Fairs a triangle mesh.
	static bool smoothMesh(HalfEdgeMesh<>& mesh, const SimulationCell& cell, int numIterations, PromiseState& promise, FloatType k_PB = FloatType(0.1), FloatType lambda = FloatType(0.5));

	/// Static implementation function of the locatePoint() method.
	static int locatePointStatic(const Point3& location, const HalfEdgeMesh<>& mesh, const SimulationCell cell, bool isCompletelySolid, FloatType epsilon = FLOATTYPE_EPSILON);
		
protected:

	/// Performs one iteration of the smoothing algorithm.
	static void smoothMeshIteration(HalfEdgeMesh<>& mesh, FloatType prefactor, const SimulationCell& cell);

private:

	/// Indicates that the entire simulation cell is part of the solid region.
	DECLARE_RUNTIME_PROPERTY_FIELD(SurfaceMeshPtr, storage, setStorage);

	/// Indicates that the entire simulation cell is part of the solid region.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, isCompletelySolid, setIsCompletelySolid);

};

}	// End of namespace
}	// End of namespace


