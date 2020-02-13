////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2020 Alexander Stukowski
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

#pragma once


#include <ovito/core/Core.h>
#include <ovito/core/utilities/concurrent/Promise.h>
#include <ovito/core/dataset/scene/SceneNode.h>
#include <ovito/core/dataset/pipeline/PipelineFlowState.h>
#include <ovito/core/dataset/pipeline/PipelineObject.h>
#include <ovito/core/dataset/pipeline/PipelineCache.h>
#include <ovito/core/dataset/data/DataVis.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Scene)

/**
 * \brief A visual node in the 3d scene which manages a data pipeline.
 */
class OVITO_CORE_EXPORT PipelineSceneNode : public SceneNode
{
	Q_OBJECT
	OVITO_CLASS(PipelineSceneNode)

public:

	/// \brief Constructs an object node.
	Q_INVOKABLE explicit PipelineSceneNode(DataSet* dataset);

	/// \brief Destructor.
	virtual ~PipelineSceneNode();

	/// \brief Performs a synchronous evaluation of the pipeline yielding only preliminary results.
	const PipelineFlowState& evaluatePipelineSynchronous(bool includeVisElements);

	/// \brief Performs an asynchronous evaluation of the data pipeline.
	PipelineEvaluationFuture evaluatePipeline(const PipelineEvaluationRequest& request);

	/// \brief Performs an asynchronous evaluation of the data pipeline including the visualization elements.
	PipelineEvaluationFuture evaluateRenderingPipeline(const PipelineEvaluationRequest& request);

	/// Traverses the node's pipeline until the end and returns the object that generates the
	/// input data for the pipeline.
	PipelineObject* pipelineSource() const;

	/// Sets the data source of this node's pipeline, i.e., the object that provides the
	/// input data entering the pipeline.
	void setPipelineSource(PipelineObject* sourceObject);

	/// \brief Applies a modifier by appending it to the end of the node's data pipeline.
	/// \param modifier The modifier to be inserted into the data flow pipeline.
	/// \undoable
	ModifierApplication* applyModifier(Modifier* modifier);

	/// \brief Returns the title of this object.
	virtual QString objectTitle() const override;

	/// \brief Returns the bounding box of the scene node.
	/// \param time The time at which the bounding box should be computed.
	/// \return An world axis-aligned box.
	virtual Box3 localBoundingBox(TimePoint time, TimeInterval& validity) const override;

	/// \brief Deletes this node from the scene.
	virtual void deleteNode() override;

	/// Rescales the times of all animation keys from the old animation interval to the new interval.
	virtual void rescaleTime(const TimeInterval& oldAnimationInterval, const TimeInterval& newAnimationInterval) override;

	/// \brief Replaces the given visual element in this pipeline's output with an independent copy.
	DataVis* makeVisElementIndependent(DataVis* visElement);

	/// Returns the internal replacement for the given data vis element.
	/// If there is no replacement, the original vis element is returned.
	DataVis* getReplacementVisElement(DataVis* vis) const {
		OVITO_ASSERT(replacementVisElements().size() == replacedVisElements().size());
		OVITO_ASSERT(std::find(replacedVisElements().begin(), replacedVisElements().end(), nullptr) == replacedVisElements().end());
		OVITO_ASSERT(vis);
		int index = replacedVisElements().indexOf(vis);
		if(index >= 0)
			return replacementVisElements()[index];
		else
			return vis;
	}

protected:

	/// This method is called when a referenced object has changed.
	virtual bool referenceEvent(RefTarget* source, const ReferenceEvent& event) override;

	/// Is called when the value of a reference field of this object changes.
	virtual void referenceReplaced(const PropertyFieldDescriptor& field, RefTarget* oldTarget, RefTarget* newTarget) override;

	/// Is called when a RefTarget has been added to a VectorReferenceField of this RefMaker.
	virtual void referenceInserted(const PropertyFieldDescriptor& field, RefTarget* newTarget, int listIndex) override;

	/// Is called when a RefTarget has been added to a VectorReferenceField of this RefMaker.
	virtual void referenceRemoved(const PropertyFieldDescriptor& field, RefTarget* oldTarget, int listIndex) override;

	/// Is called when the value of a non-animatable property field of this RefMaker has changed.
	virtual void propertyChanged(const PropertyFieldDescriptor& field) override;

	/// Saves the class' contents to the given stream.
	virtual void saveToStream(ObjectSaveStream& stream, bool excludeRecomputableData) override;

	/// Loads the class' contents from the given stream.
	virtual void loadFromStream(ObjectLoadStream& stream) override;

	/// This method is called once for this object after it has been completely loaded from a stream.
	virtual void loadFromStreamComplete() override;

private:

	/// Invalidates the contents of the data pipeline cache.
	void invalidatePipelineCache(TimeInterval keepInterval = TimeInterval::empty(), bool resetSynchronousCache = false);

	/// Rebuilds the list of visual elements maintained by the scene node.
	void updateVisElementList(const PipelineFlowState& state);

	/// Helper function that recursively collects all visual elements attached to a
	/// data object and its children and stores them in an output vector.
	static void collectVisElements(const DataObject* dataObj, std::vector<DataVis*>& visElements);

	/// Computes the bounding box of a data object and all its sub-objects.
	void getDataObjectBoundingBox(TimePoint time, const DataObject* dataObj, const PipelineFlowState& state, TimeInterval& validity, Box3& bb, std::vector<const DataObject*>& objectStack) const;

	/// The terminal object of the pipeline that outputs the data to be rendered by this PipelineSceneNode.
	DECLARE_MODIFIABLE_REFERENCE_FIELD(PipelineObject, dataProvider, setDataProvider);

	/// The transient list of display objects that render the node's data in the viewports.
	/// This list is for internal caching purposes only and is rebuilt every time the node's
	/// pipeline is newly evaluated.
	DECLARE_VECTOR_REFERENCE_FIELD_FLAGS(DataVis, visElements, PROPERTY_FIELD_NEVER_CLONE_TARGET | PROPERTY_FIELD_NO_UNDO | PROPERTY_FIELD_NO_CHANGE_MESSAGE);

	/// List of weak references to visual elements coming from the pipeline which shall be replaced with
	/// independent versions owned by this node.
	DECLARE_VECTOR_REFERENCE_FIELD_FLAGS(DataVis, replacedVisElements, PROPERTY_FIELD_NEVER_CLONE_TARGET | PROPERTY_FIELD_NO_CHANGE_MESSAGE | PROPERTY_FIELD_WEAK_REF);

	/// Visual elements owned by the node which replace the ones produced by the pipeline.
	DECLARE_VECTOR_REFERENCE_FIELD_FLAGS(DataVis, replacementVisElements, PROPERTY_FIELD_NEVER_CLONE_TARGET | PROPERTY_FIELD_NO_CHANGE_MESSAGE);

	/// Activates the precomputation of the pipeline results for all animation frames.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(bool, pipelineTrajectoryCachingEnabled, setPipelineTrajectoryCachingEnabled, PROPERTY_FIELD_NO_UNDO | PROPERTY_FIELD_NO_CHANGE_MESSAGE);

	/// The cached output of the data pipeline (without the effect of visualization elements).
	PipelineCache _pipelineCache;

	/// The cached pipeline output including any output generated by asynchronous visualization elements.
	PipelineCache _pipelineRenderingCache;

	friend class PipelineCache;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
