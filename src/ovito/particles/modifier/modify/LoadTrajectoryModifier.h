///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2019) Alexander Stukowski
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


#include <ovito/particles/Particles.h>
#include <ovito/core/dataset/pipeline/Modifier.h>
#include <ovito/core/dataset/pipeline/PipelineObject.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Modify)

/**
 * \brief Loads particle trajectories from a separate file and injects them into the modification pipeline.
 */
class OVITO_PARTICLES_EXPORT LoadTrajectoryModifier : public Modifier
{
	/// Give this modifier class its own metaclass.
	class LoadTrajectoryModifierClass : public ModifierClass
	{
	public:

		/// Inherit constructor from base metaclass.
		using ModifierClass::ModifierClass;

		/// Asks the metaclass whether the modifier can be applied to the given input data.
		virtual bool isApplicableTo(const DataCollection& input) const override;
	};

	Q_OBJECT
	OVITO_CLASS_META(LoadTrajectoryModifier, LoadTrajectoryModifierClass)

	Q_CLASSINFO("DisplayName", "Load trajectory");
	Q_CLASSINFO("ModifierCategory", "Modification");

public:

	/// Constructor.
	Q_INVOKABLE LoadTrajectoryModifier(DataSet* dataset);

	/// Modifies the input data.
	virtual Future<PipelineFlowState> evaluate(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input) override;

	/// Returns the number of animation frames this modifier can provide.
	virtual int numberOfSourceFrames(int inputFrames) const override {
		return trajectorySource() ? trajectorySource()->numberOfSourceFrames() : inputFrames;
	}

	/// Given an animation time, computes the source frame to show.
	virtual int animationTimeToSourceFrame(TimePoint time, int inputFrame) const override {
		return trajectorySource() ? trajectorySource()->animationTimeToSourceFrame(time) : inputFrame;
	}

	/// Given a source frame index, returns the animation time at which it is shown.
	virtual TimePoint sourceFrameToAnimationTime(int frame, TimePoint inputTime) const override {
		return trajectorySource() ? trajectorySource()->sourceFrameToAnimationTime(frame) : inputTime;
	}

protected:

	/// \brief Is called when a RefTarget referenced by this object has generated an event.
	virtual bool referenceEvent(RefTarget* source, const ReferenceEvent& event) override;

	/// Is called when the value of a reference field of this object changes.
	virtual void referenceReplaced(const PropertyFieldDescriptor& field, RefTarget* oldTarget, RefTarget* newTarget) override;

private:

	/// The source for trajectory data.
	DECLARE_MODIFIABLE_REFERENCE_FIELD_FLAGS(PipelineObject, trajectorySource, setTrajectorySource, PROPERTY_FIELD_NO_SUB_ANIM);
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
