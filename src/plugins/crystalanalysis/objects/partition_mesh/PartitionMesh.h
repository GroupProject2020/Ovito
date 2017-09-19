///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2016) Alexander Stukowski
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
#include <core/utilities/mesh/HalfEdgeMesh.h>
#include <core/utilities/concurrent/PromiseState.h>
#include <core/dataset/data/simcell/PeriodicDomainDataObject.h>
#include <core/dataset/data/simcell/SimulationCellObject.h>

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

struct PartitionMeshFace;	// defined below

struct PartitionMeshEdge
{
	/// Pointer to the next manifold sharing this edge.
	HalfEdgeMesh<PartitionMeshEdge, PartitionMeshFace, EmptyHalfEdgeMeshStruct>::Edge* nextManifoldEdge = nullptr;
};

struct PartitionMeshFace
{
	/// The face on the opposite side of the manifold.
	HalfEdgeMesh<PartitionMeshEdge, PartitionMeshFace, EmptyHalfEdgeMeshStruct>::Face* oppositeFace = nullptr;

	/// The region to which this face belongs.
	int region;
};

using PartitionMeshData = HalfEdgeMesh<PartitionMeshEdge, PartitionMeshFace, EmptyHalfEdgeMeshStruct>;

/**
 * \brief A closed triangle mesh representing the outer surfaces and the inner interfaces of a microstructure.
 */
class OVITO_CRYSTALANALYSIS_EXPORT PartitionMesh : public PeriodicDomainDataObject
{
public:

	/// \brief Constructor that creates an empty PartitionMesh object.
	Q_INVOKABLE PartitionMesh(DataSet* dataset);

	/// Returns the title of this object.
	virtual QString objectTitle() override { return tr("Microstructure mesh"); }

	/// \brief Returns whether this object, when returned as an editable sub-object by another object,
	///        should be displayed in the modification stack.
	///
	/// Return false because this object cannot be edited.
	virtual bool isSubObjectEditable() const override { return false; }

	/// Returns the data encapsulated by this object after making sure it is not shared with other owners.
	const std::shared_ptr<PartitionMeshData>& modifiableStorage();
	
	/// Fairs a triangle mesh.
	static bool smoothMesh(PartitionMeshData& mesh, const SimulationCell& cell, int numIterations, PromiseBase& promise, FloatType k_PB = FloatType(0.1), FloatType lambda = FloatType(0.5));

protected:

	/// Performs one iteration of the smoothing algorithm.
	static void smoothMeshIteration(PartitionMeshData& mesh, FloatType prefactor, const SimulationCell& cell);

private:

	/// Indicates that the entire simulation cell is part of one region without boundaries.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, spaceFillingRegion, setSpaceFillingRegion);

	/// The internal data.
	DECLARE_RUNTIME_PROPERTY_FIELD(std::shared_ptr<PartitionMeshData>, storage, setStorage);

	Q_OBJECT
	OVITO_CLASS
};

}	// End of namespace
}	// End of namespace
}	// End of namespace


