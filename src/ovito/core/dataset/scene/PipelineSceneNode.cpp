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

#include <ovito/core/Core.h>
#include <ovito/core/dataset/scene/PipelineSceneNode.h>
#include <ovito/core/dataset/data/DataObject.h>
#include <ovito/core/dataset/data/TransformingDataVis.h>
#include <ovito/core/dataset/pipeline/PipelineObject.h>
#include <ovito/core/dataset/pipeline/PipelineEvaluation.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>
#include <ovito/core/dataset/DataSetContainer.h>
#include <ovito/core/dataset/animation/AnimationSettings.h>
#include <ovito/core/viewport/Viewport.h>
#include <ovito/core/oo/CloneHelper.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Scene)

IMPLEMENT_OVITO_CLASS(PipelineSceneNode);
DEFINE_REFERENCE_FIELD(PipelineSceneNode, dataProvider);
DEFINE_REFERENCE_FIELD(PipelineSceneNode, visElements);
DEFINE_REFERENCE_FIELD(PipelineSceneNode, replacedVisElements);
DEFINE_REFERENCE_FIELD(PipelineSceneNode, replacementVisElements);
SET_PROPERTY_FIELD_LABEL(PipelineSceneNode, dataProvider, "Pipeline object");
SET_PROPERTY_FIELD_CHANGE_EVENT(PipelineSceneNode, dataProvider, ReferenceEvent::PipelineChanged);

/******************************************************************************
* Constructor.
******************************************************************************/
PipelineSceneNode::PipelineSceneNode(DataSet* dataset) : SceneNode(dataset)
{
}

/******************************************************************************
* Destructor.
******************************************************************************/
PipelineSceneNode::~PipelineSceneNode() // NOLINT
{
}

/******************************************************************************
* Invalidates the data pipeline cache of the object node.
******************************************************************************/
void PipelineSceneNode::invalidatePipelineCache()
{
	// Invalidate data caches.
	_pipelineCache.invalidate(false);
	_pipelineRenderingCache.invalidate(true);	// Do not completely discard these cached objects,
												// because we might be able to re-use the transformed data objects.
	_pipelinePreliminaryCache.reset();

	// Also mark the cached bounding box of this node as invalid.
	invalidateBoundingBox();
}

/******************************************************************************
* Asks the object for the preliminary results of the data pipeline.
******************************************************************************/
const PipelineFlowState& PipelineSceneNode::evaluatePipelinePreliminary(bool includeVisElements) const
{
	TimePoint time = dataset()->animationSettings()->time();

	// First check if our real caches can serve the request.
	if(includeVisElements) {
		if(_pipelineRenderingCache.contains(time))
			return _pipelineRenderingCache.getAt(time);
	}
	else {
		if(_pipelineCache.contains(time))
			return _pipelineCache.getAt(time);
	}

	// If not, check if our preliminary state cache is filled.
	if(_pipelinePreliminaryCache.stateValidity().contains(time)) {
		return _pipelinePreliminaryCache;
	}

	// If not, update the preliminary state cache from the pipeline.
	if(dataProvider()) {
		_pipelinePreliminaryCache = dataProvider()->evaluatePreliminary();
	}
	else {
		_pipelinePreliminaryCache.reset();
	}

	// The preliminary state cache is time-independent.
	_pipelinePreliminaryCache.setStateValidity(TimeInterval::infinite());

	return _pipelinePreliminaryCache;
}

/******************************************************************************
* Asks the node for the results of its data pipeline.
******************************************************************************/
SharedFuture<PipelineFlowState> PipelineSceneNode::evaluatePipeline(const PipelineEvaluationRequest& request) const
{
	// Check if we can immediately serve the request from the internal cache.
	if(_pipelineCache.contains(request.time()))
		return _pipelineCache.getAt(request.time());

	// Without a data provider, we cannot serve any requests.
	if(!dataProvider())
		return Future<PipelineFlowState>::createImmediateEmplace();

	// Evaluate the pipeline and store the obtained results in the cache before returning them to the caller.
	return dataProvider()->evaluate(request)
		.then(executor(), [this, time=request.time()](PipelineFlowState state) {
			UndoSuspender noUndo(this);

			// The pipeline should never return a state without proper validity interval.
			OVITO_ASSERT(state.stateValidity().contains(time));

			// We maintain a data cache for the current animation time.
			if(_pipelineCache.insert(state, this)) {
				const_cast<PipelineSceneNode*>(this)->updateVisElementList(dataset()->animationSettings()->time());
			}

			// Simply forward the pipeline results to the caller by default.
			return std::move(state);
		});
}

/******************************************************************************
* Asks the node for the results of its data pipeline including the output of
* asynchronous visualization elements.
******************************************************************************/
SharedFuture<PipelineFlowState> PipelineSceneNode::evaluateRenderingPipeline(const PipelineEvaluationRequest& request) const
{
	// Check if we can immediately serve the request from the internal cache.
	if(_pipelineRenderingCache.contains(request.time()))
		return _pipelineRenderingCache.getAt(request.time());

	// Evaluate the pipeline and store the obtained results in the cache before returning them to the caller.
	return evaluatePipeline(request)
		.then(executor(), [this, request](const PipelineFlowState& state) {
			UndoSuspender noUndo(this);

			// Holds the results to be returned to the caller.
			Future<PipelineFlowState> results;

			// Give every visualization element the opportunity to apply an asynchronous data transformation.
			if(!state.isEmpty()) {
				for(const auto& dataObj : state.data()->objects()) {
					for(DataVis* vis : dataObj->visElements()) {
						if(TransformingDataVis* transformingVis = dynamic_object_cast<TransformingDataVis>(vis)) {
							if(transformingVis->isEnabled()) {
								if(!results.isValid()) {
									results = transformingVis->transformData(request, dataObj, PipelineFlowState(state), _pipelineRenderingCache.getStaleContents());
								}
								else {
									results = results.then(transformingVis->executor(), [request, transformingVis, dataObj, pipeline = OORef<PipelineSceneNode>(this)](PipelineFlowState&& state) {
										UndoSuspender noUndo(transformingVis);
										return transformingVis->transformData(request, dataObj, std::move(state), pipeline->_pipelineRenderingCache.getStaleContents());
									});
								}
								OVITO_ASSERT(results.isValid());
							}
						}
					}
				}
			}

			// Maintain the cache with rendering pipeline outputs.
			if(!results.isValid()) {
				// Immediate storage in the cache:
				_pipelineRenderingCache.insert(state, this);
				results = Future<PipelineFlowState>::createImmediate(state);
			}
			else {
				// Deferred storage in the cache as soon as the transforming vis elements have done their job:
				_pipelineRenderingCache.insert(results, state.stateValidity(), this);
				OVITO_ASSERT(results.isValid());
			}

			OVITO_ASSERT(results.isValid());
			return results;
		});
}


/******************************************************************************
* Helper function that recursively collects all visual elements attached to a
* data object and its children and stores them in an output vector.
******************************************************************************/
void PipelineSceneNode::collectVisElements(const DataObject* dataObj, std::vector<DataVis*>& visElements)
{
	for(DataVis* vis : dataObj->visElements()) {
		if(std::find(visElements.begin(), visElements.end(), vis) == visElements.end())
			visElements.push_back(vis);
	}

	dataObj->visitSubObjects([&visElements](const DataObject* subObject) {
		collectVisElements(subObject, visElements);
		return false;
	});
}

/******************************************************************************
* Rebuilds the list of visual elements maintained by the scene node.
******************************************************************************/
void PipelineSceneNode::updateVisElementList(TimePoint time)
{
	const PipelineFlowState& state = _pipelineCache.getAt(time);

	// Collect all visual elements from the current pipeline state.
	std::vector<DataVis*> newVisElements;
	if(state.data())
		collectVisElements(state.data(), newVisElements);

	// Perform the replacement of vis elements.
	if(!replacedVisElements().empty()) {
		for(DataVis*& vis : newVisElements)
			vis = getReplacementVisElement(vis);
	}

	// To maintain a stable ordering, first discard those elements from the old list which are not in the new list.
	for(int i = visElements().size() - 1; i >= 0; i--) {
		DataVis* vis = visElements()[i];
		if(std::find(newVisElements.begin(), newVisElements.end(), vis) == newVisElements.end()) {
			_visElements.remove(this, PROPERTY_FIELD(visElements), i);
		}
	}

	// Now add any new visual elements to the end of the list.
	for(DataVis* vis : newVisElements) {
		OVITO_CHECK_OBJECT_POINTER(vis);
		if(!visElements().contains(vis))
			_visElements.push_back(this, PROPERTY_FIELD(visElements), vis);
	}
}

/******************************************************************************
* This method is called when a referenced object has changed.
******************************************************************************/
bool PipelineSceneNode::referenceEvent(RefTarget* source, const ReferenceEvent& event)
{
	if(source == dataProvider()) {
		if(event.type() == ReferenceEvent::TargetChanged) {
			invalidatePipelineCache();
		}
		else if(event.type() == ReferenceEvent::TargetDeleted) {
			invalidatePipelineCache();

			// Data provider has been deleted -> delete node as well.
			if(!dataset()->undoStack().isUndoingOrRedoing())
				deleteNode();
		}
		else if(event.type() == ReferenceEvent::TitleChanged) {
			notifyDependents(ReferenceEvent::TitleChanged);
		}
		else if(event.type() == ReferenceEvent::PipelineChanged || event.type() == ReferenceEvent::AnimationFramesChanged) {
			// Forward pipeline changed events from the pipeline.
			return true;
		}
		else if(event.type() == ReferenceEvent::PreliminaryStateAvailable) {
			// Invalidate our preliminary state cache.
			_pipelinePreliminaryCache.reset();
		}
	}
	else if(_visElements.contains(source)) {
		if(event.type() == ReferenceEvent::TargetChanged) {

			// Update cached bounding box when visual element parameters change.
			invalidateBoundingBox();

			// Invalidate the rendering pipeline cache whenever an asynchronous visual element changes.
			if(dynamic_object_cast<TransformingDataVis>(source)) {
				// Do not completely discard these cached objects, because we might be able to re-use the transformed data objects.
				_pipelineRenderingCache.invalidate(true);

				// Trigger a pipeline re-evaluation.
				notifyTargetChanged(&PROPERTY_FIELD(visElements));
			}
			else {
				// Trigger an immediate viewport repaint without pipeline re-evaluation.
				notifyDependents(ReferenceEvent::PreliminaryStateAvailable);
			}
		}
	}
	return SceneNode::referenceEvent(source, event);
}

/******************************************************************************
* Gets called when the data object of the node has been replaced.
******************************************************************************/
void PipelineSceneNode::referenceReplaced(const PropertyFieldDescriptor& field, RefTarget* oldTarget, RefTarget* newTarget)
{
	if(field == PROPERTY_FIELD(dataProvider)) {
		invalidatePipelineCache();

		// The animation length might have changed when an object has been replaced in the scene.
		if(!isBeingLoaded())
			notifyDependents(ReferenceEvent::AnimationFramesChanged);
	}
	SceneNode::referenceReplaced(field, oldTarget, newTarget);
}

/******************************************************************************
* Saves the class' contents to the given stream.
******************************************************************************/
void PipelineSceneNode::saveToStream(ObjectSaveStream& stream, bool excludeRecomputableData)
{
	SceneNode::saveToStream(stream, excludeRecomputableData);
	stream.beginChunk(0x01);
	// For future use...
	stream.endChunk();
}

/******************************************************************************
* Loads the class' contents from the given stream.
******************************************************************************/
void PipelineSceneNode::loadFromStream(ObjectLoadStream& stream)
{
	SceneNode::loadFromStream(stream);
	stream.expectChunk(0x01);
	// For future use...
	stream.closeChunk();
}

/******************************************************************************
* Returns the title of this object.
******************************************************************************/
QString PipelineSceneNode::objectTitle() const
{
	// If a user-defined name has been assigned to this node, return it as the node's display title.
	if(!nodeName().isEmpty())
		return nodeName();

	// Otherwise, use the title of the node's data source object.
	if(PipelineObject* source = pipelineSource())
		return source->objectTitle();

	// Fall back to default behavior.
	return SceneNode::objectTitle();
}

/******************************************************************************
* Applies a modifier by appending it to the end of the node's modification
* pipeline.
******************************************************************************/
ModifierApplication* PipelineSceneNode::applyModifier(Modifier* modifier)
{
	OVITO_ASSERT(modifier);

	OORef<ModifierApplication> modApp = modifier->createModifierApplication();
	modApp->setModifier(modifier);
	modApp->setInput(dataProvider());
	modifier->initializeModifier(modApp);
	setDataProvider(modApp);
	return modApp;
}

/******************************************************************************
* Traverses the node's pipeline until the end and returns the object that
* generates the input data for the pipeline.
******************************************************************************/
PipelineObject* PipelineSceneNode::pipelineSource() const
{
	if(ModifierApplication* modApp = dynamic_object_cast<ModifierApplication>(dataProvider()))
		return modApp->pipelineSource();
	else
		return dataProvider();
}

/******************************************************************************
* Sets the data source of this node's pipeline, i.e., the object that provides the
* input data entering the pipeline.
******************************************************************************/
void PipelineSceneNode::setPipelineSource(PipelineObject* sourceObject)
{
	ModifierApplication* modApp = dynamic_object_cast<ModifierApplication>(dataProvider());
	if(!modApp) {
		setDataProvider(sourceObject);
	}
	else {
		for(;;) {
			if(ModifierApplication* modApp2 = dynamic_object_cast<ModifierApplication>(modApp->input()))
				modApp = modApp2;
			else
				break;
		}
		modApp->setInput(sourceObject);
	}
	OVITO_ASSERT(this->pipelineSource() == sourceObject);
}

/******************************************************************************
* Computes the bounding box of the scene node in local coordinates.
******************************************************************************/
Box3 PipelineSceneNode::localBoundingBox(TimePoint time, TimeInterval& validity) const
{
	const PipelineFlowState& state = evaluatePipelinePreliminary(true);

	// Let visual elements compute the bounding boxes of the data objects.
	Box3 bb;
	std::vector<const DataObject*> objectStack;
	if(state.data())
		getDataObjectBoundingBox(time, state.data(), state, validity, bb, objectStack);
	OVITO_ASSERT(objectStack.empty());
	validity.intersect(state.stateValidity());
	return bb;
}

/******************************************************************************
* Computes the bounding box of a data object and all its sub-objects.
******************************************************************************/
void PipelineSceneNode::getDataObjectBoundingBox(TimePoint time, const DataObject* dataObj, const PipelineFlowState& state, TimeInterval& validity, Box3& bb, std::vector<const DataObject*>& objectStack) const
{
	bool isOnStack = false;

	// Call all vis elements of the data object.
	for(DataVis* vis : dataObj->visElements()) {
		// Let the PipelineSceneNode substitude the vis element with another one.
		vis = getReplacementVisElement(vis);
		if(vis->isEnabled()) {
			// Push the data object onto the stack.
			if(!isOnStack) {
				objectStack.push_back(dataObj);
				isOnStack = true;
			}
			try {
				// Let the vis element do the rendering.
				bb.addBox(vis->boundingBox(time, objectStack, this, state, validity));
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
		getDataObjectBoundingBox(time, subObject, state, validity, bb, objectStack);
		return false;
	});

	// Pop the data object from the stack.
	if(isOnStack) {
		objectStack.pop_back();
	}
}

/******************************************************************************
* Deletes this node from the scene.
******************************************************************************/
void PipelineSceneNode::deleteNode()
{
	// Remove pipeline.
	setDataProvider(nullptr);

	// Discard transient references to visual elements.
	_visElements.clear(this, PROPERTY_FIELD(visElements));

	SceneNode::deleteNode();
}

/******************************************************************************
* Is called when a RefTarget has been added to a VectorReferenceField of this RefMaker.
******************************************************************************/
void PipelineSceneNode::referenceInserted(const PropertyFieldDescriptor& field, RefTarget* newTarget, int listIndex)
{
	if(field == PROPERTY_FIELD(replacementVisElements)) {
		invalidatePipelineCache();
	}
	SceneNode::referenceInserted(field, newTarget, listIndex);
}

/******************************************************************************
* Is called when a RefTarget has been added to a VectorReferenceField of this RefMaker.
******************************************************************************/
void PipelineSceneNode::referenceRemoved(const PropertyFieldDescriptor& field, RefTarget* oldTarget, int listIndex)
{
	if(field == PROPERTY_FIELD(replacedVisElements) && !isAboutToBeDeleted()) {
		// If an upstream vis element is being removed from the list, because the weakly referenced vis element is being deleted,
		// then also discard our corresponding replacement element managed by the node.
		if(!dataset()->undoStack().isUndoingOrRedoing()) {
			OVITO_ASSERT(replacedVisElements().size() + 1 == replacementVisElements().size());
			_replacementVisElements.remove(this, PROPERTY_FIELD(replacementVisElements), listIndex);
		}
		invalidatePipelineCache();
	}
	SceneNode::referenceRemoved(field, oldTarget, listIndex);
}

/******************************************************************************
* This method is called once for this object after it has been completely
* loaded from a stream.
******************************************************************************/
void PipelineSceneNode::loadFromStreamComplete()
{
	SceneNode::loadFromStreamComplete();

	// Remove null entries from the replacedVisElements list due to expired weak references.
	for(int i = replacedVisElements().size() - 1; i >= 0; i--) {
		if(replacedVisElements()[i] == nullptr) {
			_replacedVisElements.remove(this, PROPERTY_FIELD(replacedVisElements), i);
		}
	}
	OVITO_ASSERT(replacedVisElements().size() == replacementVisElements().size());
	OVITO_ASSERT(!dataset()->undoStack().isRecording());
}

/******************************************************************************
* Replaces the given visual element in this pipeline's output with an
* independent copy.
******************************************************************************/
DataVis* PipelineSceneNode::makeVisElementIndependent(DataVis* visElement)
{
	OVITO_ASSERT(visElement != nullptr);
	OVITO_ASSERT(!replacedVisElements().contains(visElement));
	OVITO_ASSERT(replacedVisElements().size() == replacementVisElements().size());

	OORef<DataVis> clonedVisElement;
	{
		UndoSuspender noUndo(this);

		// Clone the visual element.
		CloneHelper cloneHelper;
		clonedVisElement = cloneHelper.cloneObject(visElement, true);
	}
	if(dataset()->undoStack().isRecording())
		dataset()->undoStack().push(std::make_unique<TargetChangedUndoOperation>(this));

	// Put the copy into our mapping table, which will subsequently be applied
	// after every pipeline evaluation to replace the upstream visual element
	// with our local copy.
	int index = replacementVisElements().indexOf(visElement);
	if(index == -1) {
		_replacedVisElements.push_back(this, PROPERTY_FIELD(replacedVisElements), visElement);
		_replacementVisElements.push_back(this, PROPERTY_FIELD(replacementVisElements), clonedVisElement);
	}
	else {
		_replacementVisElements.set(this, PROPERTY_FIELD(replacementVisElements), index, clonedVisElement);
	}
	OVITO_ASSERT(replacedVisElements().size() == replacementVisElements().size());

	if(dataset()->undoStack().isRecording())
		dataset()->undoStack().push(std::make_unique<TargetChangedRedoOperation>(this));

	notifyTargetChanged();

	return clonedVisElement;
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
