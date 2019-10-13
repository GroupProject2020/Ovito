////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2018 Alexander Stukowski
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
#include <ovito/mesh/tri/TriMeshObject.h>
#include <ovito/core/rendering/SceneRenderer.h>
#include <ovito/core/rendering/MeshPrimitive.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/dataset/data/VersionedDataObjectRef.h>
#include <ovito/core/utilities/units/UnitsManager.h>
#include "TriMeshVis.h"

namespace Ovito { namespace Mesh {

IMPLEMENT_OVITO_CLASS(TriMeshVis);
DEFINE_PROPERTY_FIELD(TriMeshVis, color);
DEFINE_REFERENCE_FIELD(TriMeshVis, transparencyController);
DEFINE_PROPERTY_FIELD(TriMeshVis, highlightEdges);
SET_PROPERTY_FIELD_LABEL(TriMeshVis, color, "Display color");
SET_PROPERTY_FIELD_LABEL(TriMeshVis, transparencyController, "Transparency");
SET_PROPERTY_FIELD_LABEL(TriMeshVis, highlightEdges, "Highlight edges");
SET_PROPERTY_FIELD_UNITS_AND_RANGE(TriMeshVis, transparencyController, PercentParameterUnit, 0, 1);

/******************************************************************************
* Constructor.
******************************************************************************/
TriMeshVis::TriMeshVis(DataSet* dataset) : DataVis(dataset),
	_color(0.85, 0.85, 1),
	_highlightEdges(false)
{
	setTransparencyController(ControllerManager::createFloatController(dataset));
}

/******************************************************************************
* Computes the bounding box of the object.
******************************************************************************/
Box3 TriMeshVis::boundingBox(TimePoint time, const std::vector<const DataObject*>& objectStack, const PipelineSceneNode* contextNode, const PipelineFlowState& flowState, TimeInterval& validityInterval)
{
	// Compute bounding box.
	if(const TriMeshObject* triMeshObj = dynamic_object_cast<TriMeshObject>(objectStack.back())) {
		if(triMeshObj->mesh())
			return triMeshObj->mesh()->boundingBox();
	}
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
			ColorA,						// Display color
			bool						// Edge highlighting
		>;

		FloatType transp = 0;
		TimeInterval iv;
		if(transparencyController()) transp = transparencyController()->getFloatValue(time, iv);
		ColorA color_mesh(color(), FloatType(1) - transp);

		// Lookup the rendering primitive in the vis cache.
		auto& meshPrimitive = dataset()->visCache().get<std::shared_ptr<MeshPrimitive>>(CacheKey(renderer, objectStack.back(), color_mesh, highlightEdges()));

		// Check if we already have a valid rendering primitive that is up to date.
		if(!meshPrimitive || !meshPrimitive->isValid(renderer)) {
			meshPrimitive = renderer->createMeshPrimitive();
			const TriMeshObject* triMeshObj = dynamic_object_cast<TriMeshObject>(objectStack.back());
			if(triMeshObj && triMeshObj->mesh())
				meshPrimitive->setMesh(*triMeshObj->mesh(), color_mesh, highlightEdges());
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
