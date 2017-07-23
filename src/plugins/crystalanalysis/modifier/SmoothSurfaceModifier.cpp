///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2013) Alexander Stukowski
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
#include <plugins/particles/objects/SurfaceMesh.h>
#include <plugins/particles/objects/SimulationCellObject.h>
#include <plugins/crystalanalysis/objects/slip_surface/SlipSurface.h>
#include <core/utilities/concurrent/Task.h>
#include <core/utilities/mesh/HalfEdgeMesh_EdgeCollapse.h>
#include <core/dataset/DataSetContainer.h>
#include "SmoothSurfaceModifier.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(SmoothSurfaceModifier, Modifier);
DEFINE_FLAGS_PROPERTY_FIELD(SmoothSurfaceModifier, smoothingLevel, "SmoothingLevel", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(SmoothSurfaceModifier, minEdgeLength, "MinEdgeLength", PROPERTY_FIELD_MEMORIZE);
SET_PROPERTY_FIELD_LABEL(SmoothSurfaceModifier, smoothingLevel, "Smoothing level");
SET_PROPERTY_FIELD_LABEL(SmoothSurfaceModifier, minEdgeLength, "Minimum edge length");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(SmoothSurfaceModifier, smoothingLevel, IntegerParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(SmoothSurfaceModifier, minEdgeLength, WorldParameterUnit, 0);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
SmoothSurfaceModifier::SmoothSurfaceModifier(DataSet* dataset) : Modifier(dataset), _smoothingLevel(8), _minEdgeLength(0)
{
	INIT_PROPERTY_FIELD(smoothingLevel);
	INIT_PROPERTY_FIELD(minEdgeLength);
}

/******************************************************************************
* Asks the modifier whether it can be applied to the given input data.
******************************************************************************/
bool SmoothSurfaceModifier::isApplicableTo(const PipelineFlowState& input)
{
	return (input.findObject<SurfaceMesh>() != nullptr) || (input.findObject<SlipSurface>() != nullptr);
}

/******************************************************************************
* This modifies the input object.
******************************************************************************/
PipelineStatus SmoothSurfaceModifier::modifyObject(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state)
{
	if(smoothingLevel() <= 0 && minEdgeLength() <= 0)
		return PipelineStatus::Success;

	// Get simulation cell geometry and periodic boundary flags.
	SimulationCell cell;
	if(SimulationCellObject* simulationCellObj = state.findObject<SimulationCellObject>())
		cell = simulationCellObj->data();
	else
		cell.setPbcFlags(false, false, false);

	// Helper function required by the mesh simplification routine.
	auto pointPointVector = [&cell](const Point3& p0, const Point3& p1) {
		return cell.wrapVector(p1 - p0);
	};

	CloneHelper cloneHelper;
	for(int index = 0; index < state.objects().size(); index++) {
		if(SurfaceMesh* inputSurface = dynamic_object_cast<SurfaceMesh>(state.objects()[index])) {
			OORef<SurfaceMesh> outputSurface = cloneHelper.cloneObject(inputSurface, false);
			SynchronousTask smoothingTask(dataset()->container()->taskManager());
			if(smoothingLevel() > 0) {
				outputSurface->smoothMesh(cell, smoothingLevel(), smoothingTask.promise());
			}
			if(minEdgeLength() > 0) {
				EdgeCollapseMeshSimplification<HalfEdgeMesh<>, decltype(pointPointVector)> simplification(*outputSurface->modifiableStorage(), pointPointVector);
				simplification.perform(minEdgeLength(), smoothingTask.promise());
				outputSurface->changed();
			}		
			state.replaceObject(inputSurface, outputSurface);
		}
		else if(SlipSurface* inputSurface = dynamic_object_cast<SlipSurface>(state.objects()[index])) {
			OORef<SlipSurface> outputSurface = cloneHelper.cloneObject(inputSurface, false);
			SynchronousTask smoothingTask(dataset()->container()->taskManager());
			if(smoothingLevel() > 0) {
				outputSurface->smoothMesh(cell, smoothingLevel(), smoothingTask.promise(), FloatType(0.1), FloatType(0.6));
			}
#if 0
			if(minEdgeLength() > 0) {
				EdgeCollapseMeshSimplification<SlipSurfaceData, decltype(pointPointVector)> simplification(*outputSurface->modifiableStorage(), pointPointVector);
				simplification.perform(minEdgeLength(), smoothingTask.promise());
				outputSurface->changed();
			}
#endif
			state.replaceObject(inputSurface, outputSurface);
		}
	}
	return PipelineStatus::Success;
}

}	// End of namespace
}	// End of namespace
}	// End of namespace
