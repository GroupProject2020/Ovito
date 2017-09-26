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

#include <plugins/mesh/Mesh.h>
#include <core/dataset/DataSet.h>
#include <plugins/stdobj/simcell/SimulationCellObject.h>
#include <plugins/stdobj/util/InputHelper.h>
#include <plugins/stdobj/util/OutputHelper.h>
#include "SurfaceMeshAffineTransformationModifierDelegate.h"

namespace Ovito { namespace Mesh {

IMPLEMENT_OVITO_CLASS(SurfaceMeshAffineTransformationModifierDelegate);

/******************************************************************************
* Applies the modifier operation to the data in a pipeline flow state.
******************************************************************************/
PipelineStatus SurfaceMeshAffineTransformationModifierDelegate::apply(Modifier* modifier, const PipelineFlowState& input, PipelineFlowState& output, TimePoint time, ModifierApplication* modApp)
{
	AffineTransformationModifier* mod = static_object_cast<AffineTransformationModifier>(modifier);
	if(mod->selectionOnly())
		return PipelineStatus::Success;

	InputHelper ih(dataset(), input);
	OutputHelper oh(dataset(), output);
	
	AffineTransformation tm;
	if(mod->relativeMode())
		tm = mod->transformationTM();
	else
		tm = mod->targetCell() * ih.expectSimulationCell()->cellMatrix().inverse();
	
	for(DataObject* obj : output.objects()) {
		if(SurfaceMesh* existingSurface = dynamic_object_cast<SurfaceMesh>(obj)) {
			SurfaceMesh* newSurface = oh.cloneIfNeeded(existingSurface);
			// Apply transformation to the vertices of the surface mesh.
			for(HalfEdgeMesh<>::Vertex* vertex : newSurface->modifiableStorage()->vertices())
				vertex->pos() = tm * vertex->pos();
			// Apply transformation to the cutting planes attached to the surface mesh.
			QVector<Plane3> cuttingPlanes = newSurface->cuttingPlanes();
			for(Plane3& plane : cuttingPlanes)
				plane = tm * plane;
			newSurface->setCuttingPlanes(std::move(cuttingPlanes));
		}
	}
	
	return PipelineStatus::Success;
}

}	// End of namespace
}	// End of namespace
