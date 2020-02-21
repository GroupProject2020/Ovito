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

#include <ovito/particles/Particles.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/stdobj/properties/PropertyAccess.h>
#include <ovito/core/dataset/animation/AnimationSettings.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>
#include <ovito/core/dataset/pipeline/PipelineEvaluation.h>
#include <ovito/core/utilities/units/UnitsManager.h>
#include "SmoothTrajectoryModifier.h"

namespace Ovito { namespace Particles {

IMPLEMENT_OVITO_CLASS(SmoothTrajectoryModifier);
DEFINE_PROPERTY_FIELD(SmoothTrajectoryModifier, useMinimumImageConvention);
DEFINE_PROPERTY_FIELD(SmoothTrajectoryModifier, smoothingWindowSize);
SET_PROPERTY_FIELD_LABEL(SmoothTrajectoryModifier, useMinimumImageConvention, "Use minimum image convention");
SET_PROPERTY_FIELD_LABEL(SmoothTrajectoryModifier, smoothingWindowSize, "Smoothing window size");
SET_PROPERTY_FIELD_UNITS_AND_RANGE(SmoothTrajectoryModifier, smoothingWindowSize, IntegerParameterUnit, 1, 200);

// This class can be removed in a future version of OVITO:
IMPLEMENT_OVITO_CLASS(InterpolateTrajectoryModifierApplication);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
SmoothTrajectoryModifier::SmoothTrajectoryModifier(DataSet* dataset) : Modifier(dataset),
	_useMinimumImageConvention(true),
	_smoothingWindowSize(1)
{
}

/******************************************************************************
* Asks the modifier whether it can be applied to the given input data.
******************************************************************************/
bool SmoothTrajectoryModifier::OOMetaClass::isApplicableTo(const DataCollection& input) const
{
	return input.containsObject<ParticlesObject>();
}

/******************************************************************************
* Determines the time interval over which a computed pipeline state will remain valid.
******************************************************************************/
TimeInterval SmoothTrajectoryModifier::validityInterval(const PipelineEvaluationRequest& request, const ModifierApplication* modApp) const
{
	TimeInterval iv = Modifier::validityInterval(request, modApp);
	// Interpolation results will only be valid for the duration of the current frame.
	iv.intersect(request.time());
	return iv;
}

/******************************************************************************
* Asks the modifier for the set of animation time intervals that should be 
* cached by the upstream pipeline.
******************************************************************************/
void SmoothTrajectoryModifier::inputCachingHints(TimeIntervalUnion& cachingIntervals, ModifierApplication* modApp)
{
	Modifier::inputCachingHints(cachingIntervals, modApp);

	TimeIntervalUnion originalIntervals = cachingIntervals;
	for(const TimeInterval& iv : originalIntervals) {
		// Round interval start down to the previous animation frame.
		// Round interval end up to the next animation frame.
		int startFrame = modApp->animationTimeToSourceFrame(iv.start());
		int endFrame = modApp->animationTimeToSourceFrame(iv.end());
		if(modApp->sourceFrameToAnimationTime(endFrame) < iv.end())
			endFrame++;
		startFrame -= (smoothingWindowSize() - 1) / 2;
		endFrame += smoothingWindowSize() / 2;
		TimePoint newStartTime = modApp->sourceFrameToAnimationTime(startFrame);
		TimePoint newEndTime = modApp->sourceFrameToAnimationTime(endFrame);
		OVITO_ASSERT(newStartTime <= iv.start());
		OVITO_ASSERT(newEndTime >= iv.end());
		cachingIntervals.add(TimeInterval(newStartTime, newEndTime));
	}
}

/******************************************************************************
* Is called by the ModifierApplication to let the modifier adjust the 
* time interval of a TargetChanged event received from the upstream pipeline 
* before it is propagated to the downstream pipeline.
******************************************************************************/
void SmoothTrajectoryModifier::restrictInputValidityInterval(TimeInterval& iv) const
{
	Modifier::restrictInputValidityInterval(iv);

	// If the upstream pipeline changes, all computed output frames of the modifier become invalid.
	iv.setEmpty();
}

/******************************************************************************
* Modifies the input data.
******************************************************************************/
Future<PipelineFlowState> SmoothTrajectoryModifier::evaluate(const PipelineEvaluationRequest& request, ModifierApplication* modApp, const PipelineFlowState& input)
{
	// Determine the current frame, preferably from the attribute stored with the pipeline flow state.
	// If the source frame attribute is not present, fall back to inferring it from the current animation time.
	int currentFrame = input.data() ? input.data()->sourceFrame() : -1;
	if(currentFrame < 0)
		currentFrame = modApp->animationTimeToSourceFrame(request.time());
	TimePoint time1 = modApp->sourceFrameToAnimationTime(currentFrame);

	// If we are exactly on a source frame, there is no need to interpolate between frames.
	if(time1 == request.time() && smoothingWindowSize() <= 1) {
		// The validity of the resulting state is restricted to the current animation time.
		PipelineFlowState output = input;
		output.intersectStateValidity(time1);
		return output;
	}

	if(smoothingWindowSize() == 1) {
		// Perform interpolation between two consecutive frames.
		int nextFrame = currentFrame + 1;
		TimePoint time2 = modApp->sourceFrameToAnimationTime(nextFrame);

		// Obtain the subsequent input frame by evaluating the upstream pipeline.
		PipelineEvaluationRequest frameRequest = request;
		frameRequest.setTime(time2);

		// Wait for the second frame to become available.
		return modApp->evaluateInput(frameRequest)
			.then(executor(), [this, time = request.time(), modApp, state = input, time1, time2](const PipelineFlowState& nextState) mutable {
				// Compute interpolated state.
				interpolateState(state, nextState, modApp, time, time1, time2);
				return std::move(state);
			});
	}
	else {
		// Perform averaging of several frames. Determine frame interval first.
		int startFrame = currentFrame - (smoothingWindowSize() - 1) / 2;
		int endFrame = currentFrame + smoothingWindowSize() / 2;

		// Prepare the upstream pipeline request.
		PipelineEvaluationRequest frameRequest = request;
		frameRequest.setTime(modApp->sourceFrameToAnimationTime(startFrame));

		// List of animation times at which to evaluate the upstream pipeline.
		std::vector<TimePoint> otherTimes;
		otherTimes.reserve(endFrame - startFrame);
		for(int frame = startFrame; frame <= endFrame; frame++) {
			if(frame != currentFrame)
				otherTimes.push_back(modApp->sourceFrameToAnimationTime(frame));
		}

		// Obtain the range of input frames from the upstream pipeline.
		return modApp->evaluateInputMultiple(frameRequest, std::move(otherTimes))
			.then(executor(), false, [this, state = input, modApp, time = request.time()](const std::vector<PipelineFlowState>& otherStates) mutable {
				// Compute smoothed state.
				averageState(state, otherStates, modApp, time);
				return std::move(state);
			});
	}
}

/******************************************************************************
* Modifies the input data synchronously.
******************************************************************************/
void SmoothTrajectoryModifier::evaluateSynchronous(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state)
{
	// Determine the current frame, preferably from the attribute stored with the pipeline flow state.
	// If the source frame attribute is not present, fall back to inferring it from the current animation time.
	int currentFrame = state.data() ? state.data()->sourceFrame() : -1;
	if(currentFrame < 0)
		currentFrame = modApp->animationTimeToSourceFrame(time);
	TimePoint time1 = modApp->sourceFrameToAnimationTime(currentFrame);

	// If we are exactly on a source frame, there is no need to interpolate between two consecutive frames.
	if(time1 == time && smoothingWindowSize() <= 1) {
		// The validity of the resulting state is restricted to the current animation time.
		state.intersectStateValidity(time);
		return;
	}

	if(smoothingWindowSize() == 1) {
		// Perform interpolation between two consecutive frames.
		int nextFrame = currentFrame + 1;
		TimePoint time2 = modApp->sourceFrameToAnimationTime(nextFrame);

		// Get the second frame.
		const PipelineFlowState& state2 = modApp->evaluateInputSynchronous(time2); 

		// Perform the actual interpolation calculation.
		interpolateState(state, state2, modApp, time, time1, time2);
	}
	else {
		// Perform averaging of several frames. Determine frame interval.
		int startFrame = currentFrame - (smoothingWindowSize() - 1) / 2;
		int endFrame = currentFrame + smoothingWindowSize() / 2;

		// Obtain the range of input frames from the upstream pipeline.
		std::vector<PipelineFlowState> otherStates;
		otherStates.reserve(endFrame - startFrame);
		for(int frame = startFrame; frame <= endFrame; frame++) {
			if(frame != currentFrame) {
				TimePoint time2 = modApp->sourceFrameToAnimationTime(frame);
				otherStates.push_back(modApp->evaluateInputSynchronous(time2));
			}
		}

		// Compute smoothed state.
		averageState(state, otherStates, modApp, time);
	}
}

/******************************************************************************
* Computes the interpolated state between two input states.
******************************************************************************/
void SmoothTrajectoryModifier::interpolateState(PipelineFlowState& state1, const PipelineFlowState& state2, ModifierApplication* modApp, TimePoint time, TimePoint time1, TimePoint time2)
{
	OVITO_ASSERT(!dataset()->undoStack().isRecording());

	// Make sure the obtained reference configuration is valid and ready to use.
	if(state2.status().type() == PipelineStatus::Error)
		throwException(tr("Input state for frame %1 is not available: %2").arg(modApp->animationTimeToSourceFrame(time2)).arg(state2.status().text()));

	OVITO_ASSERT(time2 > time1);
	FloatType t = (FloatType)(time - time1) / (time2 - time1);
	if(t < 0) t = 0;
	else if(t > 1) t = 1;

	const SimulationCellObject* cell1 = state1.getObject<SimulationCellObject>();
	const SimulationCellObject* cell2 = state2.getObject<SimulationCellObject>();

	// Interpolate particle positions.
	const ParticlesObject* particles1 = state1.expectObject<ParticlesObject>();
	const ParticlesObject* particles2 = state2.getObject<ParticlesObject>();
	if(!particles2 || particles1->elementCount() != particles2->elementCount())
		throwException(tr("Cannot interpolate between consecutive simulation frames, because they contain different numbers of particles."));
	particles1->verifyIntegrity();
	particles2->verifyIntegrity();
	ConstPropertyAccess<Point3> posProperty2 = particles2->expectProperty(ParticlesObject::PositionProperty);
	ConstPropertyAccess<qlonglong> idProperty1 = particles1->getProperty(ParticlesObject::IdentifierProperty);
	ConstPropertyAccess<qlonglong> idProperty2 = particles2->getProperty(ParticlesObject::IdentifierProperty);
	ParticlesObject* outputParticles = state1.makeMutable(particles1);
	PropertyAccess<Point3> outputPositions = outputParticles->createProperty(ParticlesObject::PositionProperty, true);
	if(idProperty1 && idProperty2 && !boost::equal(idProperty1, idProperty2)) {

		// Build ID-to-index map.
		std::unordered_map<qlonglong,size_t> idmap;
		size_t index = 0;
		for(auto id : idProperty2) {
			if(!idmap.insert(std::make_pair(id,index)).second)
				throwException(tr("Detected duplicate particle ID: %1. Cannot interpolate trajectories in this case.").arg(id));
			index++;
		}

		if(useMinimumImageConvention() && cell1 != nullptr) {
			SimulationCell cell = cell1->data();
			auto id = idProperty1.cbegin();
			for(Point3& p1 : outputPositions) {
				auto mapEntry = idmap.find(*id);
				if(mapEntry == idmap.end())
					throwException(tr("Cannot interpolate between consecutive frames, because the identity of particles changes between frames."));
				Vector3 delta = cell.wrapVector(posProperty2[mapEntry->second] - p1);
				p1 += delta * t;
				++id;
			}
		}
		else {
			auto id = idProperty1.cbegin();
			for(Point3& p1 : outputPositions) {
				auto mapEntry = idmap.find(*id);
				if(mapEntry == idmap.end())
					throwException(tr("Cannot interpolate between consecutive frames, because the identity of particles changes between frames."));
				p1 += (posProperty2[mapEntry->second] - p1) * t;
				++id;
			}
		}
	}
	else {
		const Point3* p2 = posProperty2.cbegin();
		if(useMinimumImageConvention() && cell1 != nullptr) {
			SimulationCell cell = cell1->data();
			for(Point3& p1 : outputPositions) {
				Vector3 delta = cell.wrapVector((*p2++) - p1);
				p1 += delta * t;
			}
		}
		else {
			for(Point3& p1 : outputPositions) {
				p1 += ((*p2++) - p1) * t;
			}
		}
	}

	// Interpolate simulation cell vectors.
	if(cell1 && cell2) {
		SimulationCellObject* outputCell = state1.expectMutableObject<SimulationCellObject>();
		const AffineTransformation& cellMat1 = cell1->cellMatrix();
		const AffineTransformation delta = cell2->cellMatrix() - cellMat1;
		outputCell->setCellMatrix(cellMat1 + delta * t);
	}

	// The validity of the interpolated state is restricted to the current animation time.
	state1.intersectStateValidity(time);
}

/******************************************************************************
* Computes the averaged state from several input states.
******************************************************************************/
void SmoothTrajectoryModifier::averageState(PipelineFlowState& state1, const std::vector<PipelineFlowState>& otherStates, ModifierApplication* modApp, TimePoint time)
{
	OVITO_ASSERT(!dataset()->undoStack().isRecording());

	// Get particle positions and simulation cell of the central frame.
	const SimulationCellObject* cell1 = state1.getObject<SimulationCellObject>();
	const ParticlesObject* particles1 = state1.expectObject<ParticlesObject>();
	particles1->verifyIntegrity();
	ConstPropertyAccessAndRef<Point3> posProperty1 = particles1->expectProperty(ParticlesObject::PositionProperty);
	ConstPropertyAccess<qlonglong> idProperty1 = particles1->getProperty(ParticlesObject::IdentifierProperty);

	// Create a modifiable copy of the particle coordinates array.
	ParticlesObject* outputParticles = state1.makeMutable(particles1);
	PropertyAccess<Point3> outputPositions = outputParticles->createProperty(ParticlesObject::PositionProperty, true);

	// For interpolating the simulation cell vectors.
	AffineTransformation averageCellMat;
	if(cell1)
		averageCellMat = cell1->cellMatrix();

	// Iterate over all frames in the averging window (except the central frame).
	FloatType weight = FloatType(1) / (1 + otherStates.size());
	for(const PipelineFlowState& state2 : otherStates) {

		// Make sure the obtained reference configuration is valid and ready to use.
		if(state2.status().type() == PipelineStatus::Error)
			throwException(tr("Input state for trajectory smoothing is not available: %1").arg(state2.status().text()));

		const ParticlesObject* particles2 = state2.getObject<ParticlesObject>();
		if(!particles2 || particles1->elementCount() != particles2->elementCount())
			throwException(tr("Cannot smooth trajectory, because number of particles varies between consecutive simulation frames."));
		particles2->verifyIntegrity();
		ConstPropertyAccess<Point3> posProperty2 = particles2->expectProperty(ParticlesObject::PositionProperty);
		ConstPropertyAccess<qlonglong> idProperty2 = particles2->getProperty(ParticlesObject::IdentifierProperty);

		// Sum up cell vectors.
		const SimulationCellObject* cell2 = cell1 ? state2.expectObject<SimulationCellObject>() : nullptr;
		if(cell2)
			averageCellMat += cell2->cellMatrix();

		if(idProperty1 && idProperty2 && !boost::equal(idProperty1, idProperty2)) {

			// Build ID-to-index map.
			std::unordered_map<qlonglong,size_t> idmap;
			size_t index = 0;
			for(auto id : idProperty2) {
				if(!idmap.insert(std::make_pair(id,index)).second)
					throwException(tr("Detected duplicate particle ID: %1. Cannot smooth trajectories in this case.").arg(id));
				index++;
			}

			const Point3* p1 = posProperty1.cbegin();
			if(useMinimumImageConvention() && cell2 != nullptr) {
				SimulationCell cell = cell2->data();
				auto id = idProperty1.cbegin();
				for(Point3& pout : outputPositions) {
					auto mapEntry = idmap.find(*id);
					if(mapEntry == idmap.end())
						throwException(tr("Cannot smooth trajectories, because the set of particles doesn't remain the same from frame to frame."));
					pout += cell.wrapVector(posProperty2[mapEntry->second] - (*p1++)) * weight;
					++id;
				}
			}
			else {
				auto id = idProperty1.cbegin();
				for(Point3& pout : outputPositions) {
					auto mapEntry = idmap.find(*id);
					if(mapEntry == idmap.end())
						throwException(tr("Cannot smooth trajectories, because the set of particles doesn't remain the same from frame to frame."));
					pout += (posProperty2[mapEntry->second] - (*p1++)) * weight;
					++id;
				}
			}
		}
		else {
			const Point3* p1 = posProperty1.cbegin();
			const Point3* p2 = posProperty2.cbegin();
			if(useMinimumImageConvention() && cell2 != nullptr) {
				SimulationCell cell = cell2->data();
				for(Point3& pout : outputPositions) {
					pout += cell.wrapVector((*p2++) - (*p1++)) * weight;
				}
			}
			else {
				for(Point3& pout : outputPositions) {
					pout += ((*p2++) - (*p1++)) * weight;
				}
			}
		}
	}

	// Compute average of simulation cell vectors.
	if(cell1) {
		SimulationCellObject* outputCell = state1.expectMutableObject<SimulationCellObject>();
		outputCell->setCellMatrix(averageCellMat * weight);
	}

	// The validity of the interpolated state is restricted to the current animation time.
	state1.intersectStateValidity(time);
}

}	// End of namespace
}	// End of namespace
