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
#include <ovito/stdobj/simcell/SimulationCell.h>
#include <ovito/core/dataset/pipeline/Modifier.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>
#include <ovito/core/utilities/concurrent/AsyncOperation.h>

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
	virtual Future<PipelineFlowState> evaluate(const PipelineEvaluationRequest& request, ModifierApplication* modApp, const PipelineFlowState& input) override;

	/// Modifies the input data synchronously.
	virtual void evaluateSynchronous(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state) override;
};

/**
 * Used by the UnwrapTrajectoriesModifier to store the information for unfolding the particle trajectories.
 */
class OVITO_PARTICLES_EXPORT UnwrapTrajectoriesModifierApplication : public ModifierApplication
{
	Q_OBJECT
	OVITO_CLASS(UnwrapTrajectoriesModifierApplication)

public:

	/// Data structure holding the precomputed information that is needed to unwrap the particle trajectories.
	/// For each crossing of a particle through a periodic cell boundary, the map contains one entry specifying
	/// the particle's unique ID, the time of the crossing, the spatial dimension and the direction (positive or negative).
	using UnwrapData = std::unordered_multimap<qlonglong, std::tuple<TimePoint, qint8, qint16>>;

	/// Data structure holding the precomputed information that is needed to undo flipping of sheared simulation cells in LAMMPS.
	using UnflipData = std::vector<std::pair<TimePoint, std::array<int,3>>>;

	/// Constructor.
	Q_INVOKABLE UnwrapTrajectoriesModifierApplication(DataSet* dataset) : ModifierApplication(dataset) {}

	/// Indicates the animation time up to which trajectories have already been unwrapped.
	TimePoint unwrappedUpToTime() const { return _unwrappedUpToTime; }

	/// Returns the list of particle crossings through periodic cell boundaries.
	const UnwrapData& unwrapRecords() const { return _unwrapRecords; }

	/// Returns the list of detected cell flips.
	const UnflipData& unflipRecords() const { return _unflipRecords; }

	/// Processes all frames of the input trajectory to detect periodic crossings of the particles.
	SharedFuture<> detectPeriodicCrossings(TimePoint time);

	/// Unwraps the current particle coordinates.
	void unwrapParticleCoordinates(TimePoint time, PipelineFlowState& state);

protected:

	/// Saves the class' contents to an output stream.
	virtual void saveToStream(ObjectSaveStream& stream, bool excludeRecomputableData) override;

	/// Loads the class' contents from an input stream.
	virtual void loadFromStream(ObjectLoadStream& stream) override;

	/// \brief Is called when a RefTarget referenced by this object has generated an event.
	virtual bool referenceEvent(RefTarget* source, const ReferenceEvent& event) override;

	/// Is called when the value of a reference field of this object changes.
	virtual void referenceReplaced(const PropertyFieldDescriptor& field, RefTarget* oldTarget, RefTarget* newTarget) override;

	/// Requests the next trajectory frame from the downstream pipeline.
	void fetchNextFrame();

	/// Calculates the information that is needed to unwrap particle coordinates.
	void processNextFrame(int frame, TimePoint time, const PipelineFlowState& state);

private:

	/// The operation that processes all trajectory frames in the background to detect periodic crossings of particles.
	AsyncOperation _unwrapOperation;

	/// Indicates the animation time up to which trajectories have been unwrapped already.
	TimePoint _unwrappedUpToTime = TimeNegativeInfinity();

	/// The list of particle crossings through periodic cell boundaries.
	UnwrapData _unwrapRecords;

	/// The list of detected cell flips.
	UnflipData _unflipRecords;

	/// Working data used during processing of the input trajectory.
	std::unordered_map<qlonglong,Point3> _previousPositions;
	SimulationCell _previousCell;
	std::array<int,3> _currentFlipState{{0,0,0}};
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
