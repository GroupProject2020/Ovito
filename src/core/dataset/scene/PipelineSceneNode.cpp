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

#include <core/Core.h>
#include <core/dataset/scene/PipelineSceneNode.h>
#include <core/dataset/data/DataObject.h>
#include <core/dataset/data/TransformingDataVis.h>
#include <core/dataset/pipeline/PipelineObject.h>
#include <core/dataset/pipeline/ModifierApplication.h>
#include <core/dataset/DataSetContainer.h>
#include <core/dataset/animation/AnimationSettings.h>
#include <core/viewport/Viewport.h>
#include <core/oo/CloneHelper.h>

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
PipelineSceneNode::~PipelineSceneNode()
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
	_pipelinePreliminaryCache.clear();

	// Also mark the cached bounding box of this node as invalid.
	invalidateBoundingBox();
}

/******************************************************************************
* Asks the object for the preliminary results of the data pipeline.
******************************************************************************/
const PipelineFlowState& PipelineSceneNode::evaluatePipelinePreliminary(bool includeVisElements) 
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

		// Inject our unique copies of visual elements into the pipeline output.
		replaceVisualElements(_pipelinePreliminaryCache);
	}
	else {
		_pipelinePreliminaryCache.clear();
	}

	// The preliminary state cache is time-independent.
	_pipelinePreliminaryCache.setStateValidity(TimeInterval::infinite());

	return _pipelinePreliminaryCache;
}

/******************************************************************************
* Asks the node for the results of its data pipeline.
******************************************************************************/
SharedFuture<PipelineFlowState> PipelineSceneNode::evaluatePipeline(TimePoint time)
{
	// Check if we can immediately serve the request from the internal cache.
	if(_pipelineCache.contains(time))
		return _pipelineCache.getAt(time);

	// Without a data provider, we cannot serve any requests.
	if(!dataProvider())
		return Future<PipelineFlowState>::createImmediateEmplace();

	// Evaluate the pipeline and store the obtained results in the cache before returning them to the caller.
	return dataProvider()->evaluate(time)
		.then(executor(), [this, time](PipelineFlowState state) {

			// The pipeline should never return a state without proper validity interval.
			OVITO_ASSERT(state.stateValidity().contains(time));

			// Inject our unique copies of visual elements into the pipeline output.
			replaceVisualElements(state);

			// We maintain a data cache for the current animation time.
			if(_pipelineCache.insert(state, this)) {
				updateVisElementList(dataset()->animationSettings()->time());
			}

			// Simply forward the pipeline results to the caller by default.
			return std::move(state);
		});
}

/******************************************************************************
* Asks the node for the results of its data pipeline including the output of 
* asynchronous visualization elements.
******************************************************************************/
SharedFuture<PipelineFlowState> PipelineSceneNode::evaluateRenderingPipeline(TimePoint time)
{
	// Check if we can immediately serve the request from the internal cache.
	if(_pipelineRenderingCache.contains(time))
		return _pipelineRenderingCache.getAt(time);

	// Evaluate the pipeline and store the obtained results in the cache before returning them to the caller.
	return evaluatePipeline(time)
		.then(executor(), [this, time](const PipelineFlowState& state) {

			// Holds the results to be returned to the caller.
			Future<PipelineFlowState> results;

			// Give every visualization element the chance to apply an asynchronous data transformation.
			for(const auto& dataObj : state.objects()) {
				for(DataVis* vis : dataObj->visElements()) {
					OVITO_ASSERT(vis);
					if(!vis->isEnabled()) continue;
					if(TransformingDataVis* transformingVis = dynamic_object_cast<TransformingDataVis>(vis)) {
						if(!results.isValid()) {
							results = transformingVis->transformData(time, dataObj, PipelineFlowState(state), _pipelineRenderingCache.getStaleContents(), this);
						}
						else {
							results = results.then(transformingVis->executor(), [this, time, transformingVis, dataObj](PipelineFlowState&& state) {
								return transformingVis->transformData(time, dataObj, std::move(state), _pipelineRenderingCache.getStaleContents(), this);
							});
						}
						OVITO_ASSERT(results.isValid());
					}
				}
			}

			// Maintain a data cache for pipeline states.
			if(!results.isValid()) {
				// Immediate storage in the cache:
				_pipelineRenderingCache.insert(state, this);
				results = Future<PipelineFlowState>::createImmediate(state);
			}
			else {
				// Asynchronous storage in the cache:
				_pipelineRenderingCache.insert(results, state.stateValidity(), this);
				OVITO_ASSERT(results.isValid());
			}

			OVITO_ASSERT(results.isValid());
			return results;
		});
}

/******************************************************************************
* Rebuilds the list of visual elements maintained by the scene node.
******************************************************************************/
void PipelineSceneNode::updateVisElementList(TimePoint time)
{
	const PipelineFlowState& state = _pipelineCache.getAt(time);

	// First, discard those elements which are no longer needed.
	for(int i = visElements().size() - 1; i >= 0; i--) {
		DataVis* vis = visElements()[i];
		// Check if the element is still being referenced by any of the data objects
		// that came out of the pipeline.
		if(std::none_of(state.objects().begin(), state.objects().end(),
				[vis](DataObject* obj) { return obj->visElements().contains(vis); })) {
			_visElements.remove(this, PROPERTY_FIELD(visElements), i);
		}
	}

	// Now add any new visual elements.
	for(const auto& dataObj : state.objects()) {
		for(DataVis* vis : dataObj->visElements()) {
			OVITO_CHECK_OBJECT_POINTER(vis);
			if(visElements().contains(vis) == false)
				_visElements.push_back(this, PROPERTY_FIELD(visElements), vis);
		}
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
		else if(event.type() == ReferenceEvent::PipelineChanged) {
			// Forward pipeline changed events from the pipeline.
			return true;
		}
		else if(event.type() == ReferenceEvent::PreliminaryStateAvailable) {
			// Invalidate our preliminary state cache.
			_pipelinePreliminaryCache.clear();
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
QString PipelineSceneNode::objectTitle()
{
	// If a name has been assigned to this node, return it as the node's display title.
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
void PipelineSceneNode::applyModifier(Modifier* modifier)
{
	OVITO_ASSERT(modifier);

	OORef<ModifierApplication> modApp = modifier->createModifierApplication();
	modApp->setModifier(modifier);
	modApp->setInput(dataProvider());
	modifier->initializeModifier(modApp);
	setDataProvider(modApp);
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
Box3 PipelineSceneNode::localBoundingBox(TimePoint time, TimeInterval& validity)
{
	const PipelineFlowState& state = evaluatePipelinePreliminary(true);

	// Let display objects compute bounding boxes of data objects.
	Box3 bb;
	for(DataObject* dataObj : state.objects()) {
		for(DataVis* vis : dataObj->visElements()) {
			if(vis && vis->isEnabled())
				bb.addBox(vis->boundingBox(time, dataObj, this, state, validity));
		}
	}
	validity.intersect(state.stateValidity());
	return bb;
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
		if(dataset()->undoStack().isUndoingOrRedoing() == false) {
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

/******************************************************************************
* Replaces upstream visual elements with our own unique copies.
******************************************************************************/
void PipelineSceneNode::replaceVisualElements(PipelineFlowState& state)
{
	OVITO_ASSERT(replacedVisElements().size() == replacementVisElements().size());
	OVITO_ASSERT(std::find(replacedVisElements().begin(), replacedVisElements().end(), nullptr) == replacedVisElements().end());

	if(replacedVisElements().empty())
		return;	// Nothing to do.

	CloneHelper cloneHelper;
	for(DataObject* dataObj : state.objects()) {

		// Skip data objects having no visual elements.
		if(dataObj->visElements().empty()) continue;

		// In the data object's list of visual elements, replace those
		// for which the pipeline node manages its own copy.
		QVector<DataVis*> visualElements = dataObj->visElements();
		bool didReplace = false;
		for(auto& vis : visualElements) {
			OVITO_ASSERT(vis != nullptr);
			int index = replacedVisElements().indexOf(vis);
			if(index >= 0) {
				vis = replacementVisElements()[index];
				didReplace = true;
			}
		}

		if(didReplace) {
			// Clone the data object so that we can replace its visual elements.
			if(dataObj->numberOfStrongReferences() > 1) {
				OORef<DataObject> clone = cloneHelper.cloneObject(dataObj, false);
				if(!state.replaceObject(dataObj, clone)) 
					continue;
				dataObj = clone;
			}
			// Assign the new visual element list to the data object.
			dataObj->setVisElements(std::move(visualElements));
		}
	}	
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
