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

#include <ovito/mesh/Mesh.h>
#include <ovito/mesh/surface/SurfaceMesh.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>
#include "SurfaceMeshSliceModifierDelegate.h"

namespace Ovito { namespace Mesh {

IMPLEMENT_OVITO_CLASS(SurfaceMeshSliceModifierDelegate);

/******************************************************************************
* Performs the actual rejection of particles.
******************************************************************************/
PipelineStatus SurfaceMeshSliceModifierDelegate::apply(Modifier* modifier, PipelineFlowState& state, TimePoint time, ModifierApplication* modApp, const std::vector<std::reference_wrapper<const PipelineFlowState>>& additionalInputs)
{
	SliceModifier* mod = static_object_cast<SliceModifier>(modifier);
	if(mod->createSelection())
		return PipelineStatus::Success;

	// Obtain modifier parameter values.
	Plane3 plane;
	FloatType sliceWidth;
	std::tie(plane, sliceWidth) = mod->slicingPlane(time, state.mutableStateValidity());

	for(const DataObject* obj : state.data()->objects()) {
		if(const SurfaceMesh* inputMesh = dynamic_object_cast<SurfaceMesh>(obj)) {
			SurfaceMesh* outputMesh = state.makeMutable(inputMesh);
			QVector<Plane3> planes = outputMesh->cuttingPlanes();
			if(sliceWidth <= 0) {
				planes.push_back(plane);
			}
			else {
				planes.push_back(Plane3( plane.normal,  plane.dist + sliceWidth/2));
				planes.push_back(Plane3(-plane.normal, -plane.dist + sliceWidth/2));
			}
			outputMesh->setCuttingPlanes(std::move(planes));
		}
	}

	return PipelineStatus::Success;
}

}	// End of namespace
}	// End of namespace
