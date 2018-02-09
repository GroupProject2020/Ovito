///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2017) Alexander Stukowski
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
#include <core/dataset/scene/ObjectNode.h>
#include <core/dataset/data/DataObject.h>
#include <core/dataset/pipeline/PipelineObject.h>
#include <core/dataset/pipeline/ModifierApplication.h>
#include <core/viewport/Viewport.h>
#include <core/dataset/DataSetContainer.h>
#include <core/dataset/animation/AnimationSettings.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Scene)

IMPLEMENT_OVITO_CLASS(ObjectNode);
DEFINE_REFERENCE_FIELD(ObjectNode, dataProvider);
DEFINE_REFERENCE_FIELD(ObjectNode, displayObjects);
SET_PROPERTY_FIELD_LABEL(ObjectNode, dataProvider, "Pipeline object");
SET_PROPERTY_FIELD_LABEL(ObjectNode, displayObjects, "Display objects");
SET_PROPERTY_FIELD_CHANGE_EVENT(ObjectNode, dataProvider, ReferenceEvent::PipelineChanged);

/******************************************************************************
* Constructor.
******************************************************************************/
ObjectNode::ObjectNode(DataSet* dataset) : SceneNode(dataset)
{
}

/******************************************************************************
* Destructor.
******************************************************************************/
ObjectNode::~ObjectNode()
{
}

/******************************************************************************
* Invalidates the data pipeline cache of the object node.
******************************************************************************/
void ObjectNode::invalidatePipelineCache()
{
	// Invalidate data caches.
	_pipelineDataCache.invalidate(false);
	_pipelineDisplayCache.invalidate(true);	// Do not completely discard these cached objects, 
											// because we might be able to re-use the transformed data objects. 
	_pipelinePreliminaryCache.clear();

	// Also mark the cached bounding box of this node as invalid.
	invalidateBoundingBox();
}

/******************************************************************************
* Asks the object for the preliminary results of the data pipeline.
******************************************************************************/
const PipelineFlowState& ObjectNode::evaluatePipelinePreliminary(bool includeDisplayObjects) 
{
	TimePoint time = dataset()->animationSettings()->time();

	// First check if our real caches can serve the request.
	if(includeDisplayObjects) {
		if(_pipelineDisplayCache.contains(time))
			return _pipelineDisplayCache.getAt(time);
	}
	else {
		if(_pipelineDataCache.contains(time))
			return _pipelineDataCache.getAt(time);
	}

	// If not, check if our preliminary state cache is filled.
	if(_pipelinePreliminaryCache.stateValidity().contains(time)) {
		return _pipelinePreliminaryCache;
	}

	// If not, update the preliminary state cache from the pipeline.
	if(dataProvider())
		_pipelinePreliminaryCache = dataProvider()->evaluatePreliminary();
	else
		_pipelinePreliminaryCache.clear();

	// The preliminary state cache is time-independent.
	_pipelinePreliminaryCache.setStateValidity(TimeInterval::infinite());

	return _pipelinePreliminaryCache;
}

/******************************************************************************
* Asks the node for the results of its data pipeline.
******************************************************************************/
SharedFuture<PipelineFlowState> ObjectNode::evaluatePipeline(TimePoint time)
{
	// Check if we can immediately serve the request from the internal cache.
	if(_pipelineDataCache.contains(time))
		return _pipelineDataCache.getAt(time);

	// Without a data provider, we cannot serve any requests.
	if(!dataProvider())
		return Future<PipelineFlowState>::createImmediateEmplace();

	// Evaluate the pipeline and store the obtained results in the cache before returning them to the caller.
	return dataProvider()->evaluate(time)
		.then(executor(), [this, time](const PipelineFlowState& state) {

			// The pipeline should never return a state without proper validity interval.
			OVITO_ASSERT(state.stateValidity().contains(time));

			// We maintain a data cache for the current animation time.
			if(_pipelineDataCache.insert(state, this)) {
				updateDisplayObjectList(dataset()->animationSettings()->time());
			}

			// Simply forward the pipeline results to the caller by default.
			return state;
		});
}

/******************************************************************************
* Asks the node for the results of its data pipeline including 
* the effect of display objects.
******************************************************************************/
SharedFuture<PipelineFlowState> ObjectNode::evaluateRenderingPipeline(TimePoint time)
{
	// Check if we can immediately serve the request from the internal cache.
	if(_pipelineDisplayCache.contains(time))
		return _pipelineDisplayCache.getAt(time);

	// Evaluate the pipeline and store the obtained results in the cache before returning them to the caller.
	return evaluatePipeline(time)
		.then(executor(), [this, time](const PipelineFlowState& state) {

			// Holds the results to be returned to the caller.
			Future<PipelineFlowState> results;

			// Give every display object the chance to apply an asynchronous data transformation.
			for(const auto& dataObj : state.objects()) {
				for(DisplayObject* displayObj : dataObj->displayObjects()) {
					OVITO_ASSERT(displayObj);
					if(displayObj->isEnabled() && displayObj->doesPerformDataTransformation()) {
						if(!results.isValid()) {
							results = displayObj->transformData(time, dataObj, PipelineFlowState(state), _pipelineDisplayCache.getStaleContents(), this);
						}
						else {
							results = results.then(displayObj->executor(), [this, time, displayObj, dataObj](PipelineFlowState&& state) {
								return displayObj->transformData(time, dataObj, std::move(state), _pipelineDisplayCache.getStaleContents(), this);
							});
						}
						OVITO_ASSERT(results.isValid());
					}
				}
			}

			// Maintain a data cache for pipeline states.
			if(!results.isValid()) {
				// Immediate storage in the cache:
				_pipelineDisplayCache.insert(state, this);
				results = Future<PipelineFlowState>::createImmediate(state);
			}
			else {
				// Asynchronous storage in the cache:
				_pipelineDisplayCache.insert(results, state.stateValidity(), this);
				OVITO_ASSERT(results.isValid());
			}

			OVITO_ASSERT(results.isValid());
			return results;
		});
}

/******************************************************************************
* Rebuilds the list of display objects maintained by the node.
******************************************************************************/
void ObjectNode::updateDisplayObjectList(TimePoint time)
{
	const PipelineFlowState& state = _pipelineDataCache.getAt(time);

	// First discard those display objects which are no longer needed.
	for(int i = displayObjects().size() - 1; i >= 0; i--) {
		DisplayObject* displayObj = displayObjects()[i];
		// Check if the display object is still being referenced by any of the objects
		// that left the pipeline.
		if(std::none_of(state.objects().begin(), state.objects().end(),
				[displayObj](DataObject* obj) { return obj->displayObjects().contains(displayObj); })) {
			_displayObjects.remove(this, PROPERTY_FIELD(displayObjects), i);
		}
	}

	// Now add any new display objects.
	for(const auto& dataObj : state.objects()) {
		for(DisplayObject* displayObj : dataObj->displayObjects()) {
			OVITO_CHECK_OBJECT_POINTER(displayObj);
			if(displayObjects().contains(displayObj) == false)
				_displayObjects.push_back(this, PROPERTY_FIELD(displayObjects), displayObj);
		}
	}
}

/******************************************************************************
* This method is called when a referenced object has changed.
******************************************************************************/
bool ObjectNode::referenceEvent(RefTarget* source, ReferenceEvent* event)
{
	if(source == dataProvider()) {
		if(event->type() == ReferenceEvent::TargetChanged) {
			invalidatePipelineCache();
		}
		else if(event->type() == ReferenceEvent::TargetDeleted) {
			invalidatePipelineCache();

			// Data provider has been deleted -> delete node as well.
			if(!dataset()->undoStack().isUndoingOrRedoing())
				deleteNode();
		}
		else if(event->type() == ReferenceEvent::TitleChanged) {
			notifyDependents(ReferenceEvent::TitleChanged);
		}
		else if(event->type() == ReferenceEvent::PipelineChanged) {
			// Forward pipeline changed events from the pipeline.
			return true;
		}
		else if(event->type() == ReferenceEvent::PreliminaryStateAvailable) {
			// Invalidate our preliminary state cache.
			_pipelinePreliminaryCache.clear();
		}
	}
	else if(_displayObjects.contains(source)) {
		if(event->type() == ReferenceEvent::TargetChanged) {
			
			// Update cached bounding box when display settings change.
			invalidateBoundingBox();

			if(static_object_cast<DisplayObject>(source)->doesPerformDataTransformation()) {
				
				// Invalidate the display pipeline cache whenever an asynchronous display object changes.
				// Do not completely discard these cached objects, because we might be able to re-use the transformed data objects. 
				_pipelineDisplayCache.invalidate(true);

				// Trigger a pipeline re-evaluation.
				notifyDependents(ReferenceEvent::TargetChanged);
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
void ObjectNode::referenceReplaced(const PropertyFieldDescriptor& field, RefTarget* oldTarget, RefTarget* newTarget)
{
	if(field == PROPERTY_FIELD(dataProvider)) {
		invalidatePipelineCache();
	}
	SceneNode::referenceReplaced(field, oldTarget, newTarget);
}

/******************************************************************************
* Saves the class' contents to the given stream.
******************************************************************************/
void ObjectNode::saveToStream(ObjectSaveStream& stream, bool excludeRecomputableData)
{
	SceneNode::saveToStream(stream, excludeRecomputableData);
	stream.beginChunk(0x01);
	// For future use...
	stream.endChunk();
}

/******************************************************************************
* Loads the class' contents from the given stream.
******************************************************************************/
void ObjectNode::loadFromStream(ObjectLoadStream& stream)
{
	SceneNode::loadFromStream(stream);
	stream.expectChunk(0x01);
	// For future use...
	stream.closeChunk();
}

/******************************************************************************
* Returns the title of this object.
******************************************************************************/
QString ObjectNode::objectTitle()
{
	// If a name has been assigned to this node, return it as the node's display title.
	if(!nodeName().isEmpty())
		return nodeName();

	// Otherwise, use the title of the node's data source object.
	if(PipelineObject* sourceObj = sourceObject())
		return sourceObj->objectTitle();

	// Fall back to default behavior.
	return SceneNode::objectTitle();
}

/******************************************************************************
* Applies a modifier by appending it to the end of the node's modification
* pipeline.
******************************************************************************/
void ObjectNode::applyModifier(Modifier* modifier)
{
	OVITO_ASSERT(modifier);

	OORef<ModifierApplication> modApp = modifier->createModifierApplication();
	modApp->setInput(dataProvider());
	modifier->initializeModifier(modApp);
	setDataProvider(modApp);
}

/******************************************************************************
* Returns the modification pipeline source object, i.e., the input of this
* node's modification pipeline.
******************************************************************************/
PipelineObject* ObjectNode::sourceObject() const
{
	PipelineObject* obj = dataProvider();
	while(obj) {
		if(ModifierApplication* modApp = dynamic_object_cast<ModifierApplication>(obj))
			obj = modApp->input();
		else
			break;
	}
	return obj;
}

/******************************************************************************
* Sets the data source of this node's pipeline, i.e., the object that provides the
* input data entering the pipeline.
******************************************************************************/
void ObjectNode::setSourceObject(PipelineObject* sourceObject)
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
	OVITO_ASSERT(this->sourceObject() == sourceObject);
}

/******************************************************************************
* Returns the bounding box of the object node in local coordinates.
******************************************************************************/
Box3 ObjectNode::localBoundingBox(TimePoint time, TimeInterval& validity)
{
	const PipelineFlowState& state = evaluatePipelinePreliminary(true);

	// Let display objects compute bounding boxes of data objects.
	Box3 bb;
	for(DataObject* dataObj : state.objects()) {
		for(DisplayObject* displayObj : dataObj->displayObjects()) {
			if(displayObj && displayObj->isEnabled())
				bb.addBox(displayObj->boundingBox(time, dataObj, this, state, validity));
		}
	}
	validity.intersect(state.stateValidity());
	return bb;
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
