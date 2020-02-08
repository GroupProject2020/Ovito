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


#include <ovito/particles/Particles.h>
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/core/dataset/pipeline/Modifier.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Properties)

/**
 * \brief Smootly interpolates between snapshots of a particle system.
 */
class OVITO_PARTICLES_EXPORT InterpolateTrajectoryModifier : public Modifier
{
	/// Give this modifier class its own metaclass.
	class OOMetaClass : public Modifier::OOMetaClass
	{
	public:

		/// Inherit constructor from base metaclass.
		using Modifier::OOMetaClass::OOMetaClass;

		/// Asks the metaclass whether the modifier can be applied to the given input data.
		virtual bool isApplicableTo(const DataCollection& input) const override;
	};

	Q_OBJECT
	OVITO_CLASS_META(InterpolateTrajectoryModifier, OOMetaClass)

	Q_CLASSINFO("DisplayName", "Interpolate trajectory");
	Q_CLASSINFO("ModifierCategory", "Modification");

public:

	/// Constructor.
	Q_INVOKABLE InterpolateTrajectoryModifier(DataSet* dataset);

	/// Determines the time interval over which a computed pipeline state will remain valid.
	virtual TimeInterval validityInterval(const PipelineEvaluationRequest& request, const ModifierApplication* modApp) const override;

	/// Asks the modifier for the set of animation time intervals that should be cached by the downstream pipeline.
	virtual void inputCachingHints(TimeIntervalUnion& cachingIntervals, ModifierApplication* modApp) override;

	/// Is called by the ModifierApplication to let the modifier adjust the time interval of a TargetChanged event 
	/// received from the downstream pipeline before it is propagated to the upstream pipeline.
	virtual void restrictInputValidityInterval(TimeInterval& iv) const override;

	/// Modifies the input data.
	virtual Future<PipelineFlowState> evaluate(const PipelineEvaluationRequest& request, ModifierApplication* modApp, const PipelineFlowState& input) override;

	/// Modifies the input data synchronously.
	virtual void evaluateSynchronous(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state) override;

private:

	/// Computes the interpolated state from two input states.
	void interpolateState(PipelineFlowState& state1, const PipelineFlowState& state2, ModifierApplication* modApp, TimePoint time, TimePoint time1, TimePoint time2);

	/// Controls whether the minimum image convention is used during displacement calculation.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, useMinimumImageConvention, setUseMinimumImageConvention);
};

/**
 * This class is no longer used as 02/2020. It's only here for backward compatibility with files written by older OVITO versions.
 * The class can be removed in the future.
 */
class OVITO_PARTICLES_EXPORT InterpolateTrajectoryModifierApplication : public ModifierApplication
{
	Q_OBJECT
	OVITO_CLASS(InterpolateTrajectoryModifierApplication)

public:

	/// Constructor.
	Q_INVOKABLE InterpolateTrajectoryModifierApplication(DataSet* dataset) : ModifierApplication(dataset) {}
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
