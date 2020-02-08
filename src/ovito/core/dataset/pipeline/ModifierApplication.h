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
#include "Modifier.h"
#include "CachingPipelineObject.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Scene)

/**
 * \brief Represents the application of a Modifier in a data pipeline.
 *
 * Modifiers can be shared by multiple data pipelines.
 * For every use of a Modifier instance in a pipeline, a ModifierApplication
 * is created.
 *
 * \sa Modifier
 */
class OVITO_CORE_EXPORT ModifierApplication : public CachingPipelineObject
{
	Q_OBJECT
	OVITO_CLASS(ModifierApplication)

public:

	/// Registry for modifier application classes.
	class OVITO_CORE_EXPORT Registry : private std::map<OvitoClassPtr, OvitoClassPtr>
	{
	public:
		void registerModAppClass(OvitoClassPtr modifierClass, OvitoClassPtr modAppClass) {
			insert(std::make_pair(modifierClass, modAppClass));
		}
		OvitoClassPtr getModAppClass(OvitoClassPtr modifierClass) const {
			auto entry = find(modifierClass);
			if(entry != end()) return entry->second;
			else return nullptr;
		}
	};

	/// Returns the global class registry, which allows looking up the ModifierApplication subclass for a Modifier subclass.
	static Registry& registry();

public:

	/// \brief Constructs a modifier application.
	Q_INVOKABLE explicit ModifierApplication(DataSet* dataset);

	/// \brief Determines the time interval over which a computed pipeline state will remain valid.
	virtual TimeInterval validityInterval(const PipelineEvaluationRequest& request) const override;

	/// \brief Asks the object for the result of the upstream data pipeline.
	SharedFuture<PipelineFlowState> evaluateInput(const PipelineEvaluationRequest& request);

	/// \brief Requests the preliminary computation results from the upstream data pipeline.
	PipelineFlowState evaluateInputSynchronous(TimePoint time) const { return input() ? input()->evaluateSynchronous(time) : PipelineFlowState(); }

	/// \brief Asks the object for the result of the data pipeline.
	virtual SharedFuture<PipelineFlowState> evaluate(const PipelineEvaluationRequest& request) override;

	/// \brief Returns the current status of the pipeline object.
	virtual PipelineStatus status() const override;

	/// \brief Returns the number of animation frames this pipeline object can provide.
	virtual int numberOfSourceFrames() const override;

	/// \brief Given an animation time, computes the source frame to show.
	virtual int animationTimeToSourceFrame(TimePoint time) const override;

	/// \brief Given a source frame index, returns the animation time at which it is shown.
	virtual TimePoint sourceFrameToAnimationTime(int frame) const override;

	/// \brief Returns the human-readable labels associated with the animation frames (e.g. the simulation timestep numbers).
	virtual QMap<int, QString> animationFrameLabels() const override;

	/// \brief Traverses the pipeline from this modifier application up to the source and
	/// returns the source object that generates the input data for the pipeline.
	PipelineObject* pipelineSource() const;

	/// \brief Returns the title of this modifier application.
	virtual QString objectTitle() const override {
		// Inherit title from modifier.
		if(modifier()) return modifier()->objectTitle();
		else return CachingPipelineObject::objectTitle();
	}

protected:

	/// \brief Asks the object for the result of the data pipeline.
	virtual Future<PipelineFlowState> evaluateInternal(const PipelineEvaluationRequest& request) override;

	/// \brief Lets the pipeline stage compute a preliminary result in a synchronous fashion.
	virtual PipelineFlowState evaluateInternalSynchronous(TimePoint time) override;

	/// \brief Decides whether a preliminary viewport update is performed after this pipeline object has been
	///        evaluated but before the rest of the pipeline is complete.
	virtual bool performPreliminaryUpdateAfterEvaluation() override {
		return CachingPipelineObject::performPreliminaryUpdateAfterEvaluation() && (!modifier() || modifier()->performPreliminaryUpdateAfterEvaluation());
	}

	/// Sends an event to all dependents of this RefTarget.
	virtual void notifyDependentsImpl(const ReferenceEvent& event) override;

	/// \brief Is called when a RefTarget referenced by this object has generated an event.
	virtual bool referenceEvent(RefTarget* source, const ReferenceEvent& event) override;

	/// Is called when the value of a reference field of this object changes.
	virtual void referenceReplaced(const PropertyFieldDescriptor& field, RefTarget* oldTarget, RefTarget* newTarget) override;

private:

	/// Provides the input to which the modifier is applied.
	DECLARE_MODIFIABLE_REFERENCE_FIELD_FLAGS(PipelineObject, input, setInput, PROPERTY_FIELD_NEVER_CLONE_TARGET);

	/// The modifier that is inserted into the pipeline.
	DECLARE_MODIFIABLE_REFERENCE_FIELD_FLAGS(Modifier, modifier, setModifier, PROPERTY_FIELD_NEVER_CLONE_TARGET | PROPERTY_FIELD_OPEN_SUBEDITOR);

	/// Indicates whether the modifier of this ModifierApplication is currently being evaluated.
	int _numEvaluationsInProgress = 0;
};

/// This macro assigns a ModifierApplication-derived class to a Modifier-derived class.
#define SET_MODIFIER_APPLICATION_TYPE(ModifierClass, ModifierApplicationClass) \
	static const int __modAppSetter##ModifierClass = (Ovito::ModifierApplication::registry().registerModAppClass(&ModifierClass::OOClass(), &ModifierApplicationClass::OOClass()), 0);


OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
