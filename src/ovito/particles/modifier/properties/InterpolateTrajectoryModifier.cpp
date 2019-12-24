////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2017 Alexander Stukowski
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
#include <ovito/core/dataset/animation/AnimationSettings.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>
#include <ovito/core/dataset/pipeline/PipelineEvaluation.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include "InterpolateTrajectoryModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Properties)

IMPLEMENT_OVITO_CLASS(InterpolateTrajectoryModifier);
DEFINE_PROPERTY_FIELD(InterpolateTrajectoryModifier, useMinimumImageConvention);
SET_PROPERTY_FIELD_LABEL(InterpolateTrajectoryModifier, useMinimumImageConvention, "Use minimum image convention");

IMPLEMENT_OVITO_CLASS(InterpolateTrajectoryModifierApplication);
SET_MODIFIER_APPLICATION_TYPE(InterpolateTrajectoryModifier, InterpolateTrajectoryModifierApplication);

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
* Modifies the input data.
******************************************************************************/
Future<PipelineFlowState> InterpolateTrajectoryModifier::evaluate(const PipelineEvaluationRequest& request, ModifierApplication* modApp, const PipelineFlowState& input)
{
	int nextFrame;
	SharedFuture<PipelineFlowState> nextStateFuture;

	// Determine the current frame, preferably from the attribute stored with the pipeline flow state.
	// If the source frame attribute is not present, fall back to inferring it from the current animation time.
	int currentFrame = input.data() ? input.data()->sourceFrame() : -1;
	if(currentFrame < 0)
		currentFrame = modApp->animationTimeToSourceFrame(request.time());

	// If we are exactly on a source frame, there is no need to interpolate between two frames.
	if(modApp->sourceFrameToAnimationTime(currentFrame) == request.time()) {
		nextFrame = currentFrame;
		nextStateFuture = input;
	}
	else {
		nextFrame = currentFrame + 1;

		// Obtain the subsequent input frame.
		// Check if we already have the state in our cache available.
		if(InterpolateTrajectoryModifierApplication* myModApp = dynamic_object_cast<InterpolateTrajectoryModifierApplication>(modApp)) {
			if(!myModApp->frameCache().isEmpty() && myModApp->frameCache().data()->sourceFrame() == nextFrame) {
				nextStateFuture = myModApp->frameCache();
			}
		}

		// Need to request next frame from the pipeline if not in cache.
		if(!nextStateFuture.isValid()) {
			nextStateFuture = modApp->evaluateInput(PipelineEvaluationRequest(modApp->sourceFrameToAnimationTime(nextFrame)));
		}
	}

	// Wait for the reference configuration to become available.
	return nextStateFuture.then(executor(), [this, time = request.time(), modApp, state = input, nextFrame](const PipelineFlowState& nextState) mutable {
		if(InterpolateTrajectoryModifierApplication* myModApp = dynamic_object_cast<InterpolateTrajectoryModifierApplication>(modApp)) {
			UndoSuspender noUndo(this);

			// Make sure the obtained reference configuration is valid and ready to use.
			if(nextState.status().type() == PipelineStatus::Error)
				throwException(tr("Input state is not available: %1").arg(nextState.status().text()));

			if(!nextState.isEmpty() && nextState.data()->sourceFrame() == nextFrame) {
				// Cache the next source frame in the ModifierApplication.
				myModApp->updateFrameCache(nextState);

				// Perform the actual interpolation.
				evaluatePreliminary(time, modApp, state);
				return std::move(state);
			}

			myModApp->invalidateFrameCache();
		}
		state.intersectStateValidity(time);
		return std::move(state);
	});
}

/******************************************************************************
* Modifies the input data in an immediate, preliminary way.
******************************************************************************/
void InterpolateTrajectoryModifier::evaluatePreliminary(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state)
{
	// Determine the current frame, preferably from the attribute stored with the pipeline flow state.
	// If the source frame attribute is not present, fall back to inferring it from the current animation time.
	int currentFrame = state.data() ? state.data()->sourceFrame() : -1;
	if(currentFrame < 0)
		currentFrame = modApp->animationTimeToSourceFrame(time);

	// If we are exactly on a source frame, there is no need to interpolate between two frames.
	if(modApp->sourceFrameToAnimationTime(currentFrame) == time) {
		state.intersectStateValidity(time);
		return;
	}

	// Retrieve the state of the second frame stored in the ModifierApplication.
	int nextFrame = currentFrame + 1;
	InterpolateTrajectoryModifierApplication* myModApp = dynamic_object_cast<InterpolateTrajectoryModifierApplication>(modApp);
	if(!myModApp || myModApp->frameCache().isEmpty() || myModApp->frameCache().data()->sourceFrame() != nextFrame)
		throwException(tr("No frame state stored."));

	const PipelineFlowState& secondState = myModApp->frameCache();
	TimePoint time1 = modApp->sourceFrameToAnimationTime(currentFrame);
	TimePoint time2 = modApp->sourceFrameToAnimationTime(nextFrame);
	FloatType t = (FloatType)(time - time1) / (time2 - time1);
	if(t < 0) t = 0;
	else if(t > 1) t = 1;

	const SimulationCellObject* cell1 = state.getObject<SimulationCellObject>();
	const SimulationCellObject* cell2 = secondState.getObject<SimulationCellObject>();

	// Interpolate particle positions.
	const ParticlesObject* particles1 = state.expectObject<ParticlesObject>();
	const ParticlesObject* particles2 = secondState.getObject<ParticlesObject>();
	if(!particles2 || particles1->elementCount() != particles2->elementCount())
		throwException(tr("Cannot interpolate between consecutive simulation frames, because they contain different numbers of particles."));
	const PropertyObject* posProperty1 = particles1->expectProperty(ParticlesObject::PositionProperty);
	const PropertyObject* posProperty2 = particles2->expectProperty(ParticlesObject::PositionProperty);

	const PropertyObject* idProperty1 = particles1->getProperty(ParticlesObject::IdentifierProperty);
	const PropertyObject* idProperty2 = particles2->getProperty(ParticlesObject::IdentifierProperty);
	ParticlesObject* outputParticles = state.makeMutable(particles1);
	PropertyObject* outputPositions = outputParticles->createProperty(ParticlesObject::PositionProperty, true);
	if(idProperty1 && idProperty2 && idProperty1->size() == idProperty2->size() &&
			!boost::equal(idProperty1->crange<qlonglong>(), idProperty2->crange<qlonglong>())) {

		// Build ID-to-index map.
		std::unordered_map<qlonglong,int> idmap;
		int index = 0;
		for(auto id : idProperty2->crange<qlonglong>()) {
			if(!idmap.insert(std::make_pair(id,index)).second)
				throwException(tr("Detected duplicate particle ID: %1. Cannot interpolate trajectories in this case.").arg(id));
			index++;
		}

		if(useMinimumImageConvention() && cell1 != nullptr) {
			SimulationCell cell = cell1->data();
			auto id = idProperty1->cdata<qlonglong>();
			for(Point3& p1 : outputPositions->range<Point3>()) {
				auto mapEntry = idmap.find(*id);
				if(mapEntry == idmap.end())
					throwException(tr("Cannot interpolate between consecutive frames, because the identity of particles changes between frames."));
				Vector3 delta = cell.wrapVector(posProperty2->get<Point3>(mapEntry->second) - p1);
				p1 += delta * t;
				++id;
			}
		}
		else {
			auto id = idProperty1->cdata<qlonglong>();
			for(Point3& p1 : outputPositions->range<Point3>()) {
				auto mapEntry = idmap.find(*id);
				if(mapEntry == idmap.end())
					throwException(tr("Cannot interpolate between consecutive frames, because the identity of particles changes between frames."));
				p1 += (posProperty2->get<Point3>(mapEntry->second) - p1) * t;
				++id;
			}
		}
	}
	else {
		const Point3* p2 = posProperty2->cdata<Point3>();
		if(useMinimumImageConvention() && cell1 != nullptr) {
			SimulationCell cell = cell1->data();
			for(Point3& p1 : outputPositions->range<Point3>()) {
				Vector3 delta = cell.wrapVector((*p2++) - p1);
				p1 += delta * t;
			}
		}
		else {
			for(Point3& p1 : outputPositions->range<Point3>()) {
				p1 += ((*p2++) - p1) * t;
			}
		}
	}

	// Interpolate simulation cell vectors.
	if(cell1 && cell2) {
		SimulationCellObject* outputCell = state.expectMutableObject<SimulationCellObject>();
		const AffineTransformation& cellMat1 = cell1->cellMatrix();
		const AffineTransformation delta = cell2->cellMatrix() - cellMat1;
		outputCell->setCellMatrix(cellMat1 + delta * t);
	}

	state.intersectStateValidity(time);
}

/******************************************************************************
* Is called when a RefTarget referenced by this object has generated an event.
******************************************************************************/
bool InterpolateTrajectoryModifierApplication::referenceEvent(RefTarget* source, const ReferenceEvent& event)
{
	if(event.type() == ReferenceEvent::TargetChanged) {
		invalidateFrameCache();
	}
	return ModifierApplication::referenceEvent(source, event);
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
