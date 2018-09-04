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
#include <plugins/stdobj/simcell/SimulationCellObject.h>
#include <core/dataset/pipeline/ModifierApplication.h>
#include <core/dataset/DataSet.h>
#include "DislocationSliceModifierDelegate.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

IMPLEMENT_OVITO_CLASS(DislocationSliceModifierDelegate);

/******************************************************************************
* Performs the actual rejection of particles.
******************************************************************************/
PipelineStatus DislocationSliceModifierDelegate::apply(Modifier* modifier, const PipelineFlowState& input, PipelineFlowState& output, TimePoint time, ModifierApplication* modApp, const std::vector<std::reference_wrapper<const PipelineFlowState>>& additionalInputs)
{
	SliceModifier* mod = static_object_cast<SliceModifier>(modifier);
	if(mod->createSelection()) 
		return PipelineStatus::Success;
		
	// Obtain modifier parameter values. 
	Plane3 plane;
	FloatType sliceWidth;
	std::tie(plane, sliceWidth) = mod->slicingPlane(time, output.mutableStateValidity());
	
	for(const DataObject* obj : output.objects()) {
		if(const DislocationNetworkObject* inputDislocations = dynamic_object_cast<DislocationNetworkObject>(obj)) {
			QVector<Plane3> planes = inputDislocations->cuttingPlanes();
			if(sliceWidth <= 0) {
				planes.push_back(plane);
			}
			else {
				planes.push_back(Plane3(plane.normal, plane.dist + sliceWidth/2));
				planes.push_back(Plane3(-plane.normal, -plane.dist + sliceWidth/2));
			}
			DislocationNetworkObject* outputDislocations = output.makeMutable(inputDislocations);
			outputDislocations->setCuttingPlanes(std::move(planes));
		}
	}

	return PipelineStatus::Success;
}

}	// End of namespace
}	// End of namespace
}	// End of namespace
