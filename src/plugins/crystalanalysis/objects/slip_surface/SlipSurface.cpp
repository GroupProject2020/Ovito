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

#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include <core/utilities/concurrent/ParallelFor.h>
#include "SlipSurface.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

IMPLEMENT_OVITO_CLASS(SlipSurface);
DEFINE_PROPERTY_FIELD(SlipSurface, storage);

/******************************************************************************
* Constructor.
******************************************************************************/
SlipSurface::SlipSurface(DataSet* dataset) : PeriodicDomainDataObject(dataset)
{
}

/******************************************************************************
* Returns the data encapsulated by this object after making sure it is not 
* shared with other owners.
******************************************************************************/
const std::shared_ptr<SlipSurfaceData>& SlipSurface::modifiableStorage() 
{
	// Copy data storage on write if there is more than one reference to the storage.
	OVITO_ASSERT(storage());
	OVITO_ASSERT(storage().use_count() >= 1);
	if(storage().use_count() > 1)
		_storage.mutableValue() = std::make_shared<SlipSurfaceData>(*storage());
	OVITO_ASSERT(storage().use_count() == 1);
	return storage();
}

/******************************************************************************
* Fairs a closed triangle mesh.
******************************************************************************/
bool SlipSurface::smoothMesh(SlipSurfaceData& mesh, const SimulationCell& cell, int numIterations, PromiseBase& promise, FloatType k_PB, FloatType lambda)
{
	// This is the implementation of the mesh smoothing algorithm:
	//
	// Gabriel Taubin
	// A Signal Processing Approach To Fair Surface Design
	// In SIGGRAPH 95 Conference Proceedings, pages 351-358 (1995)

	FloatType mu = FloatType(1) / (k_PB - FloatType(1)/lambda);
	promise.setProgressMaximum(numIterations);

	for(int iteration = 0; iteration < numIterations; iteration++) {
		if(!promise.setProgressValue(iteration))
			return false;
		smoothMeshIteration(mesh, lambda, cell);
		smoothMeshIteration(mesh, mu, cell);
	}

	return !promise.isCanceled();
}

/******************************************************************************
* Performs one iteration of the smoothing algorithm.
******************************************************************************/
void SlipSurface::smoothMeshIteration(SlipSurfaceData& mesh, FloatType prefactor, const SimulationCell& cell)
{
	const AffineTransformation absoluteToReduced = cell.inverseMatrix();
	const AffineTransformation reducedToAbsolute = cell.matrix();

	// Compute displacement for each vertex.
	std::vector<Vector3> displacements(mesh.vertexCount());
	parallelFor(mesh.vertexCount(), [&mesh, &displacements, prefactor, cell, absoluteToReduced](int index) {
		SlipSurfaceData::Vertex* vertex = mesh.vertex(index);
		Vector3 d = Vector3::Zero();

		for(SlipSurfaceData::Edge* edge = vertex->edges(); edge != nullptr; edge = edge->nextVertexEdge()) {
			d += cell.wrapVector(edge->vertex2()->pos() - vertex->pos());
		}
		if(vertex->edges() != nullptr)
			d *= (prefactor / vertex->numEdges());

		displacements[index] = d;
	});

	// Apply computed displacements.
	auto d = displacements.cbegin();
	for(SlipSurfaceData::Vertex* vertex : mesh.vertices())
		vertex->pos() += *d++;
}

}	// End of namespace
}	// End of namespace
}	// End of namespace
