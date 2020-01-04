////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2013 Alexander Stukowski
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

#include <ovito/core/Core.h>
#include <ovito/core/rendering/SceneRenderer.h>
#include <ovito/core/rendering/RenderSettings.h>
#include <ovito/core/dataset/scene/SceneNode.h>
#include <ovito/core/dataset/scene/RootSceneNode.h>
#include <ovito/core/dataset/pipeline/PipelineObject.h>
#include <ovito/core/dataset/pipeline/PipelineEvaluation.h>
#include <ovito/core/dataset/pipeline/Modifier.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/dataset/DataSetContainer.h>
#include <ovito/core/utilities/concurrent/AsyncOperation.h>

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
	return { renderSettings()->outputImageWidth(), renderSettings()->outputImageHeight() };
}

/******************************************************************************
* Computes the bounding box of the entire scene to be rendered.
******************************************************************************/
Box3 SceneRenderer::computeSceneBoundingBox(TimePoint time, const ViewProjectionParameters& params, Viewport* vp, AsyncOperation& operation)
{
	OVITO_CHECK_OBJECT_POINTER(renderDataset()); // startRender() must be called first.

	try {
		_sceneBoundingBox.setEmpty();
		_isBoundingBoxPass = true;
		_time = time;
		_viewport = vp;
		setProjParams(params);

		// Perform bounding box rendering pass.
		if(renderScene(operation)) {

			// Include other visual content that is only visible in the interactive viewports.
			if(isInteractive())
				renderInteractiveContent();

			// Include three-dimensional content from viewport overlys in the bounding box.
			if(vp && (!isInteractive() || vp->renderPreviewMode())) {
				for(ViewportOverlay* layer : vp->underlays()) {
					if(layer->isEnabled())
						layer->render3D(vp, time, this, operation);
				}
				for(ViewportOverlay* layer : vp->overlays()) {
					if(layer->isEnabled())
						layer->render3D(vp, time, this, operation);
				}
			}
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
bool SceneRenderer::renderScene(AsyncOperation& operation)
{
	OVITO_CHECK_OBJECT_POINTER(renderDataset());

	if(RootSceneNode* rootNode = renderDataset()->sceneRoot()) {
		// Recursively render all scene nodes.
		return renderNode(rootNode, operation);
	}

	return true;
}

/******************************************************************************
* Render a scene node (and all its children).
******************************************************************************/
bool SceneRenderer::renderNode(SceneNode* node, AsyncOperation& operation)
{
    OVITO_CHECK_OBJECT_POINTER(node);

    // Set up transformation matrix.
	TimeInterval interval;
	const AffineTransformation& nodeTM = node->getWorldTransform(time(), interval);
	setWorldTransform(nodeTM);

	if(PipelineSceneNode* pipeline = dynamic_object_cast<PipelineSceneNode>(node)) {

		// Do not render node if it is the view node of the viewport or
		// if it is the target of the view node.
		if(!viewport() || !viewport()->viewNode() || (viewport()->viewNode() != node && viewport()->viewNode()->lookatTargetNode() != node)) {

			// Evaluate data pipeline of object node and render the results.
			PipelineEvaluationFuture pipelineEvaluation(time());
			if(waitForLongOperationsEnabled()) {
				pipelineEvaluation.execute(pipeline, true);
				if(!operation.waitForFuture(pipelineEvaluation))
					return false;

				// After the rendering process has been temporarily interrupted above, rendering is resumed now.
				// Give the renderer the opportunity to restore any state that must be active (e.g. the active OpenGL context).
				resumeRendering();
			}
			const PipelineFlowState& state = pipelineEvaluation.isValid() ?
												pipelineEvaluation.result() :
												pipeline->evaluatePipelinePreliminary(true);

			// Invoke all vis elements of all data objects in the pipeline state.
			std::vector<const DataObject*> objectStack;
			if(!state.isEmpty())
				renderDataObject(state.data(), pipeline, state, objectStack);
			OVITO_ASSERT(objectStack.empty());
		}
	}

	// Render trajectory when node transformation is animated.
	if(isInteractive() && !isPicking()) {
		renderNodeTrajectory(node);
	}

	// Render child nodes.
	for(SceneNode* child : node->children()) {
		if(!renderNode(child, operation))
			return false;
	}

	return !operation.isCanceled();
}

/******************************************************************************
* Renders a data object and all its sub-objects.
******************************************************************************/
void SceneRenderer::renderDataObject(const DataObject* dataObj, const PipelineSceneNode* pipeline, const PipelineFlowState& state, std::vector<const DataObject*>& objectStack)
{
	bool isOnStack = false;

	// Call all vis elements of the data object.
	for(DataVis* vis : dataObj->visElements()) {
		// Let the PipelineSceneNode substitude the vis element with another one.
		vis = pipeline->getReplacementVisElement(vis);
		if(vis->isEnabled()) {
			// Push the data object onto the stack.
			if(!isOnStack) {
				objectStack.push_back(dataObj);
				isOnStack = true;
			}
			try {
				// Let the vis element do the rendering.
				vis->render(time(), objectStack, state, this, pipeline);
			}
			catch(const Exception& ex) {
				ex.logError();
			}
		}
	}

	// Recursively visit the sub-objects of the data object and render them as well.
	dataObj->visitSubObjects([&](const DataObject* subObject) {
		// Push the data object onto the stack.
		if(!isOnStack) {
			objectStack.push_back(dataObj);
			isOnStack = true;
		}
		renderDataObject(subObject, pipeline, state, objectStack);
		return false;
	});

	// Pop the data object from the stack.
	if(isOnStack) {
		objectStack.pop_back();
	}
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
	if(viewport() && viewport()->viewNode() == node) return;

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
			bb.addPoints(trajectory);
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
	renderDataset()->sceneRoot()->visitObjectNodes([this, renderOverlay](PipelineSceneNode* pipeline) -> bool {
		renderModifiers(pipeline, renderOverlay);
		return true;
	});
}

/******************************************************************************
* Renders the visual representation of the modifiers in a pipeline.
******************************************************************************/
void SceneRenderer::renderModifiers(PipelineSceneNode* pipeline, bool renderOverlay)
{
	ModifierApplication* modApp = dynamic_object_cast<ModifierApplication>(pipeline->dataProvider());
	while(modApp) {
		Modifier* mod = modApp->modifier();

		// Setup local transformation.
		TimeInterval interval;
		setWorldTransform(pipeline->getWorldTransform(time(), interval));

		// Render modifier.
		mod->renderModifierVisual(time(), pipeline, modApp, this, renderOverlay);

		// Traverse up the pipeline.
		modApp = dynamic_object_cast<ModifierApplication>(modApp->input());
	}
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
