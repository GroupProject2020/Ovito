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
#include <core/dataset/data/DisplayObject.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Scene)

/**
 * \brief A node in the scene that represents an object.
 */
class OVITO_CORE_EXPORT ObjectNode : public SceneNode
{
	Q_OBJECT
	OVITO_CLASS(ObjectNode)

public:

	/// \brief Constructs an object node.
	Q_INVOKABLE ObjectNode(DataSet* dataset);

	/// \brief Destructor.
	virtual ~ObjectNode();

	/// \brief Returns the data source of this node's pipeline, i.e., the object that provides the
	///        input data entering the pipeline.
	PipelineObject* sourceObject() const;

	/// \brief Sets the data source of this node's pipeline, i.e., the object that provides the
	///        input data entering the pipeline.
	void setSourceObject(PipelineObject* sourceObject);

	/// \brief Asks the node for the results of its data pipeline.
	/// \param time Specifies at which animation time the pipeline should be evaluated.
	SharedFuture<PipelineFlowState> evaluatePipeline(TimePoint time);

	/// \brief Asks the node for the results of its data pipeline including the effect of display objects.
	/// \param time Specifies at which animation time the pipeline should be evaluated.
	SharedFuture<PipelineFlowState> evaluateRenderingPipeline(TimePoint time);

	/// \brief Asks the object for the preliminary results of the data pipeline.
	const PipelineFlowState& evaluatePipelinePreliminary(bool includeDisplayObjects);

	/// \brief Applies a modifier by appending it to the end of the node's data pipeline.
	/// \param mod The modifier to be inserted into the data flow pipeline.
	/// \undoable
	void applyModifier(Modifier* mod);

	/// \brief Returns the title of this object.
	virtual QString objectTitle() override;

	/// \brief Returns the bounding box of the scene.
	/// \param time The time at which the bounding box should be computed.
	/// \return An world axis-aligned box that contains the bounding boxes of all child nodes.
	virtual Box3 localBoundingBox(TimePoint time, TimeInterval& validity) override;
	
protected:

	/// This method is called when a referenced object has changed.
	virtual bool referenceEvent(RefTarget* source, ReferenceEvent* event) override;

	/// Is called when the value of a reference field of this object changes.
	virtual void referenceReplaced(const PropertyFieldDescriptor& field, RefTarget* oldTarget, RefTarget* newTarget) override;

	/// Saves the class' contents to the given stream.
	virtual void saveToStream(ObjectSaveStream& stream, bool excludeRecomputableData) override;

	/// Loads the class' contents from the given stream.
	virtual void loadFromStream(ObjectLoadStream& stream) override;

	/// Invalidates the data pipeline cache of the object node.
	void invalidatePipelineCache();

	/// Rebuilds the list of display objects maintained by the node.
	void updateDisplayObjectList(TimePoint time);

private:

	/// The object that generates the data to be displayed by this ObjectNode.
	DECLARE_MODIFIABLE_REFERENCE_FIELD(PipelineObject, dataProvider, setDataProvider);

	/// The transient list of display objects that render the node's data in the viewports. 
	/// This list is for internal caching purposes only and is rebuilt every time the node's 
	/// pipeline is newly evaluated.
	DECLARE_VECTOR_REFERENCE_FIELD_FLAGS(DisplayObject, displayObjects, PROPERTY_FIELD_NEVER_CLONE_TARGET | PROPERTY_FIELD_NO_UNDO | PROPERTY_FIELD_NO_CHANGE_MESSAGE);

	/// The cached results from the data pipeline at the current animation time.
	PipelineCache _pipelineDataCache;

	/// The cached results from the data pipeline at the current animation time including the effect of display objects.
	PipelineCache _pipelineDisplayCache;

	/// The cached results from an preliminary pipeline evaluation.
	PipelineFlowState _pipelinePreliminaryCache;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace


