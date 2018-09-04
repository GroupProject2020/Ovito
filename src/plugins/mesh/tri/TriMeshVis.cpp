///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2018) Alexander Stukowski
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
#include <core/rendering/MeshPrimitive.h>
#include <core/dataset/DataSet.h>
#include <core/dataset/data/VersionedDataObjectRef.h>
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
Box3 TriMeshVis::boundingBox(TimePoint time, const std::vector<const DataObject*>& objectStack, const PipelineSceneNode* contextNode, const PipelineFlowState& flowState, TimeInterval& validityInterval)
{
	// Compute bounding box.
	if(const TriMeshObject* triMeshObj = dynamic_object_cast<TriMeshObject>(objectStack.back()))
		return triMeshObj->mesh().boundingBox();
	else
		return Box3();
}

/******************************************************************************
* Lets the vis element render a data object.
******************************************************************************/
void TriMeshVis::render(TimePoint time, const std::vector<const DataObject*>& objectStack, const PipelineFlowState& flowState, SceneRenderer* renderer, const PipelineSceneNode* contextNode)
{
	if(!renderer->isBoundingBoxPass()) {

		// The key type used for caching the rendering primitive:
		using CacheKey = std::tuple<
			CompatibleRendererGroup,	// The scene renderer
			VersionedDataObjectRef,		// Mesh object
			ColorA						// Display color
		>;

		FloatType transp = 0;
		TimeInterval iv;
		if(transparencyController()) transp = transparencyController()->getFloatValue(time, iv);
		ColorA color_mesh(color(), FloatType(1) - transp);

		// Lookup the rendering primitive in the vis cache.
		auto& meshPrimitive = dataset()->visCache().get<std::shared_ptr<MeshPrimitive>>(CacheKey(renderer, objectStack.back(), color_mesh));

		// Check if we already have a valid rendering primitive that is up to date.
		if(!meshPrimitive || !meshPrimitive->isValid(renderer)) {
			meshPrimitive = renderer->createMeshPrimitive();
			if(const TriMeshObject* triMeshObj = dynamic_object_cast<TriMeshObject>(objectStack.back()))
				meshPrimitive->setMesh(triMeshObj->mesh(), color_mesh);
			else
				meshPrimitive->setMesh(TriMesh(), ColorA(1,1,1,1));
		}

		renderer->beginPickObject(contextNode);
		meshPrimitive->render(renderer);
		renderer->endPickObject();
	}
	else {
		// Add mesh to bounding box.
		TimeInterval validityInterval;
		renderer->addToLocalBoundingBox(boundingBox(time, objectStack, contextNode, flowState, validityInterval));
	}
}

}	// End of namespace
}	// End of namespace
