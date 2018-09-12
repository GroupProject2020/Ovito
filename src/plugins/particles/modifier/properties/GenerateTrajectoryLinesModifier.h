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

#pragma once


#include <plugins/particles/Particles.h>
#include <plugins/particles/objects/TrajectoryVis.h>
#include <plugins/particles/objects/TrajectoryObject.h>
#include <core/dataset/pipeline/Modifier.h>
#include <core/dataset/pipeline/ModifierApplication.h>

namespace Ovito { namespace Particles {

/**
 * \brief Generates trajectory lines for particles.
 */
class OVITO_PARTICLES_EXPORT GenerateTrajectoryLinesModifier : public Modifier
{
	/// Give this modifier class its own metaclass.
	class GenerateTrajectoryLinesModifierClass : public ModifierClass 
	{
	public:

		/// Inherit constructor from base class.
		using ModifierClass::ModifierClass;

		/// Asks the metaclass whether the modifier can be applied to the given input data.
		virtual bool isApplicableTo(const DataCollection& input) const override;
	};
	
	Q_OBJECT
	OVITO_CLASS_META(GenerateTrajectoryLinesModifier, GenerateTrajectoryLinesModifierClass)
	Q_CLASSINFO("DisplayName", "Generate particle trajectories");
	Q_CLASSINFO("ModifierCategory", "Visualization");

public:

	/// \brief Constructor.
	Q_INVOKABLE GenerateTrajectoryLinesModifier(DataSet* dataset);

	/// Modifies the input data in an immediate, preliminary way.
	virtual void evaluatePreliminary(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state) override;

	/// Returns the the custom time interval.
	TimeInterval customInterval() const { return TimeInterval(_customIntervalStart, _customIntervalEnd); }

	/// Updates the stored trajectories from the source particle object.
	bool generateTrajectories(TaskManager& taskManager);

private:

	/// Controls which particles trajectories are created for.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, onlySelectedParticles, setOnlySelectedParticles);

	/// Controls whether the created trajectories span the entire animation interval or a sub-interval.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, useCustomInterval, setUseCustomInterval);

	/// The start of the custom time interval.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(TimePoint, customIntervalStart, setCustomIntervalStart);

	/// The end of the custom time interval.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(TimePoint, customIntervalEnd, setCustomIntervalEnd);

	/// The sampling frequency for creating trajectories.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, everyNthFrame, setEveryNthFrame);

	/// Controls whether trajectories are unwrapped when crossing periodic boundaries.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, unwrapTrajectories, setUnwrapTrajectories);

	/// The vis element for rendering the trajectory lines.
	DECLARE_MODIFIABLE_REFERENCE_FIELD_FLAGS(TrajectoryVis, trajectoryVis, setTrajectoryVis, PROPERTY_FIELD_DONT_PROPAGATE_MESSAGES | PROPERTY_FIELD_MEMORIZE | PROPERTY_FIELD_OPEN_SUBEDITOR);
};

/**
 * Used by the GenerateTrajectoryLinesModifier to store the generated trajeectory lines.
 */
class OVITO_PARTICLES_EXPORT GenerateTrajectoryLinesModifierApplication : public ModifierApplication
{
	OVITO_CLASS(GenerateTrajectoryLinesModifierApplication)
	Q_OBJECT

public:

	/// Constructor.
	Q_INVOKABLE GenerateTrajectoryLinesModifierApplication(DataSet* dataset) : ModifierApplication(dataset) {}

private:

	/// The cached trajectory line data.
	DECLARE_MODIFIABLE_REFERENCE_FIELD_FLAGS(TrajectoryObject, trajectoryData, setTrajectoryData, PROPERTY_FIELD_NEVER_CLONE_TARGET | PROPERTY_FIELD_NO_SUB_ANIM);
};

}	// End of namespace
}	// End of namespace
