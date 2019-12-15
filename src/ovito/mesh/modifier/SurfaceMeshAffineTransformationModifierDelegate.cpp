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
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>
#include "SurfaceMeshAffineTransformationModifierDelegate.h"

namespace Ovito { namespace Mesh {

IMPLEMENT_OVITO_CLASS(SurfaceMeshAffineTransformationModifierDelegate);

/******************************************************************************
* Applies the modifier operation to the data in a pipeline flow state.
******************************************************************************/
PipelineStatus SurfaceMeshAffineTransformationModifierDelegate::apply(Modifier* modifier, PipelineFlowState& state, TimePoint time, ModifierApplication* modApp, const std::vector<std::reference_wrapper<const PipelineFlowState>>& additionalInputs)
{
	AffineTransformationModifier* mod = static_object_cast<AffineTransformationModifier>(modifier);

	AffineTransformation tm;
	if(mod->relativeMode())
		tm = mod->transformationTM();
	else
		tm = mod->targetCell() * state.expectObject<SimulationCellObject>()->cellMatrix().inverse();

	for(const DataObject* obj : state.data()->objects()) {
		if(const SurfaceMesh* existingSurface = dynamic_object_cast<SurfaceMesh>(obj)) {
			// Create a copy of the SurfaceMesh.
			SurfaceMesh* newSurface = state.makeMutable(existingSurface);
			// Create a copy of the vertices sub-object (no need to copy the topology when only moving vertices).
			SurfaceMeshVertices* newVertices = newSurface->makeVerticesMutable();
			// Create a copy of the vertex coordinates array.
			PropertyObject* positionProperty = newVertices->expectMutableProperty(SurfaceMeshVertices::PositionProperty);

			if(!mod->selectionOnly()) {
				// Apply transformation to the vertices coordinates.
				for(Point3& p : positionProperty->point3Range())
					p = tm * p;
			}
			else {
				if(const PropertyObject* selectionProperty = newVertices->getProperty(SurfaceMeshVertices::SelectionProperty)) {
					// Apply transformation only to the selected vertices.
					const int* s = selectionProperty->constDataInt();
					for(Point3& p : positionProperty->point3Range()) {
						if(*s++)
							p = tm * p;
					}
				}
			}

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
