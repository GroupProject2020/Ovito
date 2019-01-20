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


#include <plugins/particles/Particles.h>
#include <core/dataset/pipeline/Modifier.h>
#include <core/dataset/pipeline/ModifierApplication.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Modify)

/**
 * \brief This modifier unwraps the positions of particles that have crossed a periodic boundary
 *        in order to generate continuous trajectories.
 */
class OVITO_PARTICLES_EXPORT UnwrapTrajectoriesModifier : public Modifier
{
	/// Give this modifier class its own metaclass.
	class UnwrapTrajectoriesModifierClass : public ModifierClass 
	{
	public:

		/// Inherit constructor from base class.
		using ModifierClass::ModifierClass;

		/// Asks the metaclass whether the modifier can be applied to the given input data.
		virtual bool isApplicableTo(const DataCollection& input) const override;
	};

	Q_OBJECT
	OVITO_CLASS_META(UnwrapTrajectoriesModifier, UnwrapTrajectoriesModifierClass)

	Q_CLASSINFO("DisplayName", "Unwrap trajectories");
	Q_CLASSINFO("ModifierCategory", "Modification");

public:

	/// Constructs a new instance of this class.
	Q_INVOKABLE UnwrapTrajectoriesModifier(DataSet* dataset) : Modifier(dataset) {}

	/// Modifies the input data.
	virtual Future<PipelineFlowState> evaluate(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input) override;

	/// Modifies the input data in an immediate, preliminary way.
	virtual void evaluatePreliminary(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state) override;

	/// Recalculates the information that is needed to unwrap particle coordinates.
	bool detectPeriodicCrossings(AsyncOperation&& operation);

private:

	/// Unwraps the current particle coordinates.
	void unwrapParticleCoordinates(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state);
};


/**
 * Used by the UnwrapTrajectoriesModifier to store the information for unfolding the particle trajectories.
 */
class OVITO_PARTICLES_EXPORT UnwrapTrajectoriesModifierApplication : public ModifierApplication
{
	OVITO_CLASS(UnwrapTrajectoriesModifierApplication)
	Q_OBJECT

public:

	/// Data structure holding the precomputed information that is needed to unwrap the particle trajectories.
	/// For each crossing of a particle through a periodic cell boundary, the map contains one entry specifying
	/// the particle's unique ID, the time of the crossing, the spatial dimension and the direction (positive or negative).
	using UnwrapData = std::unordered_multimap<qlonglong, std::tuple<TimePoint, qint8, qint16>>; 

	/// Constructor.
	Q_INVOKABLE UnwrapTrajectoriesModifierApplication(DataSet* dataset) : ModifierApplication(dataset), _unwrappedUpToTime(TimeNegativeInfinity()) {}

protected:

	/// Saves the class' contents to an output stream.
	virtual void saveToStream(ObjectSaveStream& stream, bool excludeRecomputableData) override;

	/// Loads the class' contents from an input stream.
	virtual void loadFromStream(ObjectLoadStream& stream) override;

private:

	/// Indicates the animation time up to which trajectories have been unwrapped already. 
	DECLARE_RUNTIME_PROPERTY_FIELD(TimePoint, unwrappedUpToTime, setUnwrappedUpToTime);

	/// The list of particle crossings through periodic cell boundaries.
	DECLARE_RUNTIME_PROPERTY_FIELD(UnwrapData, unwrapRecords, setUnwrapRecords);
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
