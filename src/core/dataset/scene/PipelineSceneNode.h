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

#pragma once


#include <core/Core.h>
#include <core/utilities/concurrent/Promise.h>
#include <core/dataset/scene/SceneNode.h>
#include <core/dataset/pipeline/PipelineFlowState.h>
#include <core/dataset/pipeline/PipelineObject.h>
#include <core/dataset/pipeline/PipelineCache.h>
#include <core/dataset/data/DataVis.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Scene)

/**
 * \brief A node in the scene that represents an object.
 */
class OVITO_CORE_EXPORT PipelineSceneNode : public SceneNode
{
	Q_OBJECT
	OVITO_CLASS(PipelineSceneNode)

public:

	/// \brief Constructs an object node.
	Q_INVOKABLE PipelineSceneNode(DataSet* dataset);

	/// \brief Destructor.
	virtual ~PipelineSceneNode();

	/// Traverses the node's pipeline until the end and returns the object that generates the 
	/// input data for the pipeline.
	PipelineObject* pipelineSource() const;

	/// Sets the data source of this node's pipeline, i.e., the object that provides the
	/// input data entering the pipeline.
	void setPipelineSource(PipelineObject* sourceObject);

	/// \brief Asks the node for the results of its data pipeline.
	/// \param time Specifies at which animation time the pipeline should be evaluated.
	SharedFuture<PipelineFlowState> evaluatePipeline(TimePoint time);

	/// \brief Asks the node for the results of its data pipeline including the output of asynchronous visualization elements.
	/// \param time Specifies at which animation time the pipeline should be evaluated.
	SharedFuture<PipelineFlowState> evaluateRenderingPipeline(TimePoint time);

	/// \brief Requests preliminary results from the data pipeline.
	const PipelineFlowState& evaluatePipelinePreliminary(bool includeVisElements);

	/// \brief Applies a modifier by appending it to the end of the node's data pipeline.
	/// \param mod The modifier to be inserted into the data flow pipeline.
	/// \undoable
	void applyModifier(Modifier* mod);

	/// \brief Returns the title of this object.
	virtual QString objectTitle() override;

	/// \brief Returns the bounding box of the scene node.
	/// \param time The time at which the bounding box should be computed.
	/// \return An world axis-aligned box.
	virtual Box3 localBoundingBox(TimePoint time, TimeInterval& validity) override;
	
	/// \brief Deletes this node from the scene.
	virtual void deleteNode() override;

	/// \brief Replaces the given visual element in this pipeline's output with a unique copy.
	void makeVisElementUnique(DataVis* visElement);

protected:

	/// This method is called when a referenced object has changed.
	virtual bool referenceEvent(RefTarget* source, const ReferenceEvent& event) override;

	/// Is called when the value of a reference field of this object changes.
	virtual void referenceReplaced(const PropertyFieldDescriptor& field, RefTarget* oldTarget, RefTarget* newTarget) override;

	/// Saves the class' contents to the given stream.
	virtual void saveToStream(ObjectSaveStream& stream, bool excludeRecomputableData) override;

	/// Loads the class' contents from the given stream.
	virtual void loadFromStream(ObjectLoadStream& stream) override;

	/// Invalidates the data pipeline cache of the scene node.
	void invalidatePipelineCache();

	/// Rebuilds the list of visual elements maintained by the scene node.
	void updateVisElementList(TimePoint time);

	/// Replaces upstream visual elements with our own unique copies.
	void replaceVisualElements(PipelineFlowState& state);

private:

	/// The terminal object of the pipeline that outputs the data to be rendered by this PipelineSceneNode.
	DECLARE_MODIFIABLE_REFERENCE_FIELD(PipelineObject, dataProvider, setDataProvider);

	/// The transient list of display objects that render the node's data in the viewports. 
	/// This list is for internal caching purposes only and is rebuilt every time the node's 
	/// pipeline is newly evaluated.
	DECLARE_VECTOR_REFERENCE_FIELD_FLAGS(DataVis, visElements, PROPERTY_FIELD_NEVER_CLONE_TARGET | PROPERTY_FIELD_NO_UNDO | PROPERTY_FIELD_NO_CHANGE_MESSAGE);

	/// The cached results from the data pipeline.
	PipelineCache _pipelineCache;

	/// The cached results from the data pipeline including the output of asynchronous visualization elements.
	PipelineCache _pipelineRenderingCache;

	/// The cached results from a preliminary pipeline evaluation.
	PipelineFlowState _pipelinePreliminaryCache;

	/// The mapping of upstream visual elements to unique copies of the visual elements.
	std::map<QPointer<DataVis>, OORef<DataVis>> _uniqueVisElementMapping;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
