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


#include <plugins/particles/Particles.h>
#include <plugins/particles/objects/ParticlesObject.h>
#include <core/dataset/pipeline/Modifier.h>
#include <core/dataset/pipeline/ModifierApplication.h>

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

	/// Modifies the input data.
	virtual Future<PipelineFlowState> evaluate(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input) override;
		
	/// Modifies the input data in an immediate, preliminary way.
	virtual void evaluatePreliminary(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state) override;

private:

	/// Controls whether the minimum image convention is used during displacement calculation.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, useMinimumImageConvention, setUseMinimumImageConvention);
};

/**
 * Used by the InterpolateTrajectoryModifier to cache the input state(s).
 */
class OVITO_PARTICLES_EXPORT InterpolateTrajectoryModifierApplication : public ModifierApplication
{
	Q_OBJECT
	OVITO_CLASS(InterpolateTrajectoryModifierApplication)

public:

	/// Constructor.
	Q_INVOKABLE InterpolateTrajectoryModifierApplication(DataSet* dataset) : ModifierApplication(dataset) {}

	/// Clears the stored source frame.
	void invalidateFrameCache() { _frameCache.reset(); }

	/// Replaces the cached source frame.
	void updateFrameCache(const PipelineFlowState& state) { _frameCache = state; }

	/// Returns the stored source frame.
	const PipelineFlowState& frameCache() const { return _frameCache; }
	
protected:

	/// Is called when a RefTarget referenced by this object has generated an event.
	virtual bool referenceEvent(RefTarget* source, const ReferenceEvent& event) override;
	
private:

	/// The cached source frame.
	PipelineFlowState _frameCache;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace


