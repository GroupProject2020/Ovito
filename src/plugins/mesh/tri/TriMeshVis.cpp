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

#include <plugins/mesh/Mesh.h>
#include <plugins/mesh/tri/TriMeshObject.h>
#include <core/rendering/SceneRenderer.h>
#include <core/utilities/units/UnitsManager.h>
#include "TriMeshVis.h"

namespace Ovito { namespace Mesh {

IMPLEMENT_OVITO_CLASS(TriMeshVis);
DEFINE_PROPERTY_FIELD(TriMeshVis, color);
DEFINE_REFERENCE_FIELD(TriMeshVis, transparencyController);
SET_PROPERTY_FIELD_LABEL(TriMeshVis, color, "Display color");
SET_PROPERTY_FIELD_LABEL(TriMeshVis, transparencyController, "Transparency");
SET_PROPERTY_FIELD_UNITS_AND_RANGE(TriMeshVis, transparencyController, PercentParameterUnit, 0, 1);

/******************************************************************************
* Constructor.
******************************************************************************/
TriMeshVis::TriMeshVis(DataSet* dataset) : DataVis(dataset),
	_color(0.85, 0.85, 1)
{
	setTransparencyController(ControllerManager::createFloatController(dataset));
}

/******************************************************************************
* Computes the bounding box of the object.
******************************************************************************/
Box3 TriMeshVis::boundingBox(TimePoint time, DataObject* dataObject, PipelineSceneNode* contextNode, const PipelineFlowState& flowState, TimeInterval& validityInterval)
{
	// Compute bounding box.
	if(OORef<TriMeshObject> triMeshObj = dataObject->convertTo<TriMeshObject>(time))
		return triMeshObj->mesh().boundingBox();
	else
		return Box3();
}

/******************************************************************************
* Lets the vis element render a data object.
******************************************************************************/
void TriMeshVis::render(TimePoint time, DataObject* dataObject, const PipelineFlowState& flowState, SceneRenderer* renderer, PipelineSceneNode* contextNode)
{
	if(!renderer->isBoundingBoxPass()) {
		// Do we have to re-create the geometry buffer from scratch?
		bool recreateBuffer = !_buffer || !_buffer->isValid(renderer);

		FloatType transp = 0;
		TimeInterval iv;
		if(transparencyController()) transp = transparencyController()->getFloatValue(time, iv);
		ColorA color_mesh(color(), FloatType(1) - transp);

		// Do we have to update contents of the geometry buffer?
		bool updateContents = _geometryCacheHelper.updateState(dataObject, color_mesh) || recreateBuffer;

		// Re-create the geometry buffer if necessary.
		if(recreateBuffer)
			_buffer = renderer->createMeshPrimitive();

		// Update buffer contents.
		if(updateContents) {
			OORef<TriMeshObject> triMeshObj = dataObject->convertTo<TriMeshObject>(time);
			if(triMeshObj)
				_buffer->setMesh(triMeshObj->mesh(), color_mesh);
			else
				_buffer->setMesh(TriMesh(), ColorA(1,1,1,1));
		}

		renderer->beginPickObject(contextNode);
		_buffer->render(renderer);
		renderer->endPickObject();
	}
	else {
		// Add mesh to bounding box.
		TimeInterval validityInterval;
		renderer->addToLocalBoundingBox(boundingBox(time, dataObject, contextNode, flowState, validityInterval));
	}
}

}	// End of namespace
}	// End of namespace
