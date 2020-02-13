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
#include <ovito/core/dataset/pipeline/ModifierApplication.h>
#include <ovito/core/dataset/pipeline/PipelineEvaluation.h>
#include "InterpolateTrajectoryModifier.h"

namespace Ovito { namespace Particles {

IMPLEMENT_OVITO_CLASS(InterpolateTrajectoryModifier);
DEFINE_PROPERTY_FIELD(InterpolateTrajectoryModifier, useMinimumImageConvention);
SET_PROPERTY_FIELD_LABEL(InterpolateTrajectoryModifier, useMinimumImageConvention, "Use minimum image convention");

// This class can be removed in a future version of OVITO:
IMPLEMENT_OVITO_CLASS(InterpolateTrajectoryModifierApplication);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
InterpolateTrajectoryModifier::InterpolateTrajectoryModifier(DataSet* dataset) : Modifier(dataset),
	_useMinimumImageConvention(true)
{
}

/******************************************************************************
* Asks the modifier whether it can be applied to the given input data.
******************************************************************************/
bool InterpolateTrajectoryModifier::OOMetaClass::isApplicableTo(const DataCollection& input) const
{
	return input.containsObject<ParticlesObject>();
}

/******************************************************************************
* Determines the time interval over which a computed pipeline state will remain valid.
******************************************************************************/
TimeInterval InterpolateTrajectoryModifier::validityInterval(const PipelineEvaluationRequest& request, const ModifierApplication* modApp) const
{
	TimeInterval iv = Modifier::validityInterval(request, modApp);
	// Interpolation results will only be valid for the duration of the current frame.
	iv.intersect(request.time());
	return iv;
}

/******************************************************************************
* Asks the modifier for the set of animation time intervals that should be 
* cached by the downstream pipeline.
******************************************************************************/
void InterpolateTrajectoryModifier::inputCachingHints(TimeIntervalUnion& cachingIntervals, ModifierApplication* modApp)
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
		TimePoint newStartTime = modApp->sourceFrameToAnimationTime(startFrame);
		TimePoint newEndTime = modApp->sourceFrameToAnimationTime(endFrame);
		OVITO_ASSERT(newStartTime <= iv.start());
		OVITO_ASSERT(newEndTime >= iv.end());
		cachingIntervals.add(TimeInterval(newStartTime, newEndTime));
	}
}

/******************************************************************************
* Is called by the ModifierApplication to let the modifier adjust the 
* time interval of a TargetChanged event received from the downstream pipeline 
* before it is propagated to the upstream pipeline.
******************************************************************************/
void InterpolateTrajectoryModifier::restrictInputValidityInterval(TimeInterval& iv) const
{
	Modifier::restrictInputValidityInterval(iv);

	// If the downstream pipeline changes, all computed output frames of the modifier become invalid.
	iv.setEmpty();
}

/******************************************************************************
* Modifies the input data.
******************************************************************************/
Future<PipelineFlowState> InterpolateTrajectoryModifier::evaluate(const PipelineEvaluationRequest& request, ModifierApplication* modApp, const PipelineFlowState& input)
{
	// Determine the current frame, preferably from the attribute stored with the pipeline flow state.
	// If the source frame attribute is not present, fall back to inferring it from the current animation time.
	int currentFrame = input.data() ? input.data()->sourceFrame() : -1;
	if(currentFrame < 0)
		currentFrame = modApp->animationTimeToSourceFrame(request.time());
	TimePoint time1 = modApp->sourceFrameToAnimationTime(currentFrame);

	// If we are exactly on a source frame, there is no need to interpolate between two consecutive frames.
	if(time1 == request.time()) {
		// The validity of the resulting state is restricted to the current animation time.
		PipelineFlowState output = input;
		output.intersectStateValidity(time1);
		return output;
	}

	// Perform interpolation between two consecutive frames.
	int nextFrame = currentFrame + 1;
	TimePoint time2 = modApp->sourceFrameToAnimationTime(nextFrame);

	// Obtain the subsequent input frame by evaluating the downstream pipeline.
	PipelineEvaluationRequest frameRequest = request;
	frameRequest.setTime(time2);
	inputCachingHints(frameRequest.modifiableCachingIntervals(), modApp);

	// Wait for the reference configuration to become available.
	return modApp->evaluateInput(frameRequest)
			.then(executor(), [this, time = request.time(), modApp, state = input, time1, time2](const PipelineFlowState& nextState) mutable {

		interpolateState(state, nextState, modApp, time, time1, time2);
		return std::move(state);
	});
}

/******************************************************************************
* Modifies the input data synchronously.
******************************************************************************/
void InterpolateTrajectoryModifier::evaluateSynchronous(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state)
{
	// Determine the current frame, preferably from the attribute stored with the pipeline flow state.
	// If the source frame attribute is not present, fall back to inferring it from the current animation time.
	int currentFrame = state.data() ? state.data()->sourceFrame() : -1;
	if(currentFrame < 0)
		currentFrame = modApp->animationTimeToSourceFrame(time);
	TimePoint time1 = modApp->sourceFrameToAnimationTime(currentFrame);

	// If we are exactly on a source frame, there is no need to interpolate between two consecutive frames.
	if(time1 == time) {
		// The validity of the resulting state is restricted to the current animation time.
		state.intersectStateValidity(time);
		return;
	}

	// Perform interpolation between two consecutive frames.
	int nextFrame = currentFrame + 1;
	TimePoint time2 = modApp->sourceFrameToAnimationTime(nextFrame);

	// Get the second frame.
	const PipelineFlowState& state2 = modApp->evaluateInputSynchronous(time2); 

	// Perform the actual interpolation calculation.
	interpolateState(state, state2, modApp, time, time1, time2);
}

/******************************************************************************
* Computes the interpolated state from two input states.
******************************************************************************/
void InterpolateTrajectoryModifier::interpolateState(PipelineFlowState& state1, const PipelineFlowState& state2, ModifierApplication* modApp, TimePoint time, TimePoint time1, TimePoint time2)
{
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
	ConstPropertyAccess<Point3> posProperty1 = particles1->expectProperty(ParticlesObject::PositionProperty);
	ConstPropertyAccess<Point3> posProperty2 = particles2->expectProperty(ParticlesObject::PositionProperty);

	ConstPropertyAccess<qlonglong> idProperty1 = particles1->getProperty(ParticlesObject::IdentifierProperty);
	ConstPropertyAccess<qlonglong> idProperty2 = particles2->getProperty(ParticlesObject::IdentifierProperty);
	ParticlesObject* outputParticles = state1.makeMutable(particles1);
	PropertyAccess<Point3> outputPositions = outputParticles->createProperty(ParticlesObject::PositionProperty, true);
	if(idProperty1 && idProperty2 && idProperty1.size() == idProperty2.size() && !boost::equal(idProperty1, idProperty2)) {

		// Build ID-to-index map.
		std::unordered_map<qlonglong,int> idmap;
		int index = 0;
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

}	// End of namespace
}	// End of namespace
