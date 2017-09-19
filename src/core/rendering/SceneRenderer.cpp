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

#include <core/Core.h>
#include <core/rendering/SceneRenderer.h>
#include <core/rendering/RenderSettings.h>
#include <core/dataset/scene/SceneNode.h>
#include <core/dataset/scene/SceneRoot.h>
#include <core/dataset/pipeline/PipelineObject.h>
#include <core/dataset/pipeline/Modifier.h>
#include <core/dataset/pipeline/ModifierApplication.h>
#include <core/dataset/DataSet.h>
#include <core/dataset/DataSetContainer.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Rendering)

IMPLEMENT_OVITO_CLASS(SceneRenderer);
IMPLEMENT_OVITO_CLASS(ObjectPickInfo);

/******************************************************************************
* Constructor.
******************************************************************************/
SceneRenderer::SceneRenderer(DataSet* dataset) : RefTarget(dataset)
{
}

/******************************************************************************
* Returns the final size of the rendered image in pixels.
******************************************************************************/
QSize SceneRenderer::outputSize() const
{
	return QSize(renderSettings()->outputImageWidth(), renderSettings()->outputImageHeight());
}

/******************************************************************************
* Computes the bounding box of the entire scene to be rendered.
******************************************************************************/
Box3 SceneRenderer::computeSceneBoundingBox(TimePoint time, const ViewProjectionParameters& params, Viewport* vp, const PromiseBase& promise)
{
	OVITO_CHECK_OBJECT_POINTER(renderDataset()); // startRender() must be called first.

	try {
		_sceneBoundingBox.setEmpty();
		_isBoundingBoxPass = true;
		_time = time;
		_viewport = vp;
		setProjParams(params);

		// Perform bounding box rendering pass.
		if(renderScene(promise)) {

			// Take into acocunt additional visual content that is only visible in the interactive viewports.
			if(isInteractive())
				renderInteractiveContent();
		}

		_isBoundingBoxPass = false;
	}
	catch(...) {
		_isBoundingBoxPass = false;
		throw;
	}

	return _sceneBoundingBox;
}

/******************************************************************************
* Renders all nodes in the scene
******************************************************************************/
bool SceneRenderer::renderScene(const PromiseBase& promise)
{
	OVITO_CHECK_OBJECT_POINTER(renderDataset());

	if(SceneRoot* rootNode = renderDataset()->sceneRoot()) {
		// Recursively render all scene nodes.
		return renderNode(rootNode, promise);
	}

	return true;
}

/******************************************************************************
* Render a scene node (and all its children).
******************************************************************************/
bool SceneRenderer::renderNode(SceneNode* node, const PromiseBase& promise)
{
    OVITO_CHECK_OBJECT_POINTER(node);

    // Set up transformation matrix.
	TimeInterval interval;
	const AffineTransformation& nodeTM = node->getWorldTransform(time(), interval);
	setWorldTransform(nodeTM);

	if(ObjectNode* objNode = dynamic_object_cast<ObjectNode>(node)) {

		// Do not render node if it is the view node of the viewport or
		// if it is the target of the view node.
		if(!viewport() || !viewport()->viewNode() || (viewport()->viewNode() != objNode && viewport()->viewNode()->lookatTargetNode() != objNode)) {

			// Evaluate data pipeline of object node and render the results.
			SharedFuture<PipelineFlowState> pipelineStateFuture;
			if(!isInteractive()) {
				pipelineStateFuture = objNode->evaluateRenderingPipeline(time());
				if(!dataset()->container()->taskManager().waitForTask(pipelineStateFuture))
					return false;
			}
			const PipelineFlowState& state = pipelineStateFuture.isValid() ? 
												pipelineStateFuture.result() : 
												objNode->evaluatePipelinePreliminary(true);

			// Render every display object of every data object in the pipeline state.
			for(const auto& dataObj : state.objects()) {
				for(DisplayObject* displayObj : dataObj->displayObjects()) {
					OVITO_ASSERT(displayObj);
					if(displayObj->isEnabled()) {
						displayObj->render(time(), dataObj, state, this, objNode);
					}
				}	
			}
		}
	}

	// Render trajectory when node transformation is animated.
	if(isInteractive() && !isPicking()) {
		renderNodeTrajectory(node);
	}

	// Render child nodes.
	for(SceneNode* child : node->children()) {
		if(!renderNode(child, promise))
			return false;
	}

	return !promise.isCanceled();
}

/******************************************************************************
* Gets the trajectory of motion of a node.
******************************************************************************/
std::vector<Point3> SceneRenderer::getNodeTrajectory(SceneNode* node)
{
	std::vector<Point3> vertices;
	Controller* ctrl = node->transformationController();
	if(ctrl && ctrl->isAnimated()) {
		AnimationSettings* animSettings = node->dataset()->animationSettings();
		int firstFrame = animSettings->firstFrame();
		int lastFrame = animSettings->lastFrame();
		OVITO_ASSERT(lastFrame >= firstFrame);
		vertices.resize(lastFrame - firstFrame + 1);
		auto v = vertices.begin();
		for(int frame = firstFrame; frame <= lastFrame; frame++) {
			TimeInterval iv;
			const Vector3& pos = node->getWorldTransform(animSettings->frameToTime(frame), iv).translation();
			*v++ = Point3::Origin() + pos;
		}
	}
	return vertices;
}

/******************************************************************************
* Renders the trajectory of motion of a node in the interactive viewports.
******************************************************************************/
void SceneRenderer::renderNodeTrajectory(SceneNode* node)
{
	if(viewport()->viewNode() == node) return;

	std::vector<Point3> trajectory = getNodeTrajectory(node);
	if(!trajectory.empty()) {
		setWorldTransform(AffineTransformation::Identity());

		if(!isBoundingBoxPass()) {
			std::shared_ptr<MarkerPrimitive> frameMarkers = createMarkerPrimitive(MarkerPrimitive::DotShape);
			frameMarkers->setCount(trajectory.size());
			frameMarkers->setMarkerPositions(trajectory.data());
			frameMarkers->setMarkerColor(ColorA(1, 1, 1));
			frameMarkers->render(this);

			std::vector<Point3> lineVertices((trajectory.size()-1) * 2);
			if(!lineVertices.empty()) {
				for(size_t index = 0; index < trajectory.size(); index++) {
					if(index != 0)
						lineVertices[index*2 - 1] = trajectory[index];
					if(index != trajectory.size() - 1)
						lineVertices[index*2] = trajectory[index];
				}
				std::shared_ptr<LinePrimitive> trajLine = createLinePrimitive();
				trajLine->setVertexCount(lineVertices.size());
				trajLine->setVertexPositions(lineVertices.data());
				trajLine->setLineColor(ColorA(1.0f, 0.8f, 0.4f));
				trajLine->render(this);
			}
		}
		else {
			Box3 bb;
			bb.addPoints(trajectory.data(), trajectory.size());
			addToLocalBoundingBox(bb);
		}
	}
}

/******************************************************************************
* Renders the visual representation of the modifiers.
******************************************************************************/
void SceneRenderer::renderModifiers(bool renderOverlay)
{
	// Visit all objects in the scene.
	renderDataset()->sceneRoot()->visitObjectNodes([this, renderOverlay](ObjectNode* objNode) -> bool {
		renderModifiers(objNode, renderOverlay);
		return true;
	});
}

/******************************************************************************
* Renders the visual representation of the modifiers.
******************************************************************************/
void SceneRenderer::renderModifiers(ObjectNode* objNode, bool renderOverlay)
{
	ModifierApplication* modApp = dynamic_object_cast<ModifierApplication>(objNode->dataProvider());
	while(modApp) {

		Modifier* mod = modApp->modifier();

		// Setup object node transformation.
		TimeInterval interval;
		setWorldTransform(objNode->getWorldTransform(time(), interval));

		// Render modifier.
		mod->renderModifierVisual(time(), objNode, modApp, this, renderOverlay);

		modApp = dynamic_object_cast<ModifierApplication>(modApp->input());
	}
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
