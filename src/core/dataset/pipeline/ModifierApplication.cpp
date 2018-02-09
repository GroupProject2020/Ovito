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

#include <core/Core.h>
#include <core/dataset/DataSet.h>
#include <core/dataset/pipeline/ModifierApplication.h>
#include <core/dataset/pipeline/PipelineFlowState.h>
#include <core/dataset/scene/ObjectNode.h>
#include <core/dataset/animation/AnimationSettings.h>
#include <core/utilities/concurrent/PromiseWatcher.h>
#include <core/utilities/concurrent/Future.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Scene)

IMPLEMENT_OVITO_CLASS(ModifierApplication);
DEFINE_REFERENCE_FIELD(ModifierApplication, modifier);	
DEFINE_REFERENCE_FIELD(ModifierApplication, input);
SET_PROPERTY_FIELD_LABEL(ModifierApplication, modifier, "Modifier");
SET_PROPERTY_FIELD_LABEL(ModifierApplication, input, "Input");
SET_PROPERTY_FIELD_CHANGE_EVENT(ModifierApplication, input, ReferenceEvent::PipelineChanged);

/******************************************************************************
* Constructor.
******************************************************************************/
ModifierApplication::ModifierApplication(DataSet* dataset) : CachingPipelineObject(dataset)
{
}

/******************************************************************************
* Is called when a RefTarget referenced by this object has generated an event.
******************************************************************************/
bool ModifierApplication::referenceEvent(RefTarget* source, ReferenceEvent* event)
{
	if(event->type() == ReferenceEvent::TargetEnabledOrDisabled && source == modifier()) {

		if(modifier()->isEnabled() == false) {
			setStatus(PipelineStatus(PipelineStatus::Success, tr("Modifier is currently disabled.")));
		}

		// Propagate enabled/disabled notification events from the modifier.
		return true;
	}
	if(event->type() == ReferenceEvent::PipelineChanged && source == input()) {
		// Propagate pipeline changed events and updates to the preliminary state from upstream.
		return true;
	}
	else if(event->type() == ReferenceEvent::TargetChanged) {
		// Invalidate cached results when the modifier or the upstream pipeline change.
		invalidatePipelineCache();
		// Trigger a preliminary viewport update if desired by the modifier.
		if(source == modifier() && modifier()->performPreliminaryUpdateAfterChange()) {
			notifyDependents(ReferenceEvent::PreliminaryStateAvailable);
		}
	}
	else if(event->type() == ReferenceEvent::PreliminaryStateAvailable && source == input()) {
		// Inform modifier that the input state has changed.
		if(modifier()) 
			modifier()->notifyDependents(ReferenceEvent::ModifierInputChanged);
	}
	return CachingPipelineObject::referenceEvent(source, event);
}

/******************************************************************************
* Gets called when the data object of the node has been replaced.
******************************************************************************/
void ModifierApplication::referenceReplaced(const PropertyFieldDescriptor& field, RefTarget* oldTarget, RefTarget* newTarget)
{
	if(field == PROPERTY_FIELD(modifier)) {
		// Update the status of the Modifier when it is detached from the ModifierApplication.
		if(Modifier* oldMod = static_object_cast<Modifier>(oldTarget)) {
			oldMod->notifyDependents(ReferenceEvent::ObjectStatusChanged);
			oldMod->notifyDependents(ReferenceEvent::ModifierInputChanged);
		}
		if(Modifier* newMod = static_object_cast<Modifier>(newTarget)) {
			newMod->notifyDependents(ReferenceEvent::ObjectStatusChanged);
			newMod->notifyDependents(ReferenceEvent::ModifierInputChanged);
		}
	}
	else if(field == PROPERTY_FIELD(input) && modifier()) {
		// Update the status of the Modifier when ModifierApplication is inserted/removed into pipeline.
		modifier()->notifyDependents(ReferenceEvent::ModifierInputChanged);
	}

	CachingPipelineObject::referenceReplaced(field, oldTarget, newTarget);
}

/******************************************************************************
* Sends an event to all dependents of this RefTarget.
******************************************************************************/
void ModifierApplication::notifyDependentsImpl(ReferenceEvent& event)
{
	if(event.type() == ReferenceEvent::ObjectStatusChanged) {
		// When this ModifierApplication's status changes, the status of the 
		// referenced Modifier does potentially change as well.
		if(modifier())
			modifier()->notifyDependents(ReferenceEvent::ObjectStatusChanged);
	}
	else if(event.type() == ReferenceEvent::TargetChanged) {
		// Invalidate cached results when this modifier application changes.
		invalidatePipelineCache();
	}
	CachingPipelineObject::notifyDependentsImpl(event);
}

/******************************************************************************
* Returns the current status of the pipeline object.
******************************************************************************/
PipelineStatus ModifierApplication::status() const
{
	PipelineStatus status = CachingPipelineObject::status();
	if(_numEvaluationsInProgress > 0) status.setType(PipelineStatus::Pending);
	return status;
}

/******************************************************************************
* Asks the object for the result of the upstream data pipeline.
******************************************************************************/
SharedFuture<PipelineFlowState> ModifierApplication::evaluateInput(TimePoint time)
{
	// Without a data source, this ModifierApplication doesn't produce any data.
	if(!input())
		return PipelineFlowState();

	// Request the input data.
//	qDebug() << "ModifierApplication::evaluateInput requesting input from" << input();
	return input()->evaluate(time);
}

/******************************************************************************
* Asks the object for the result of the data pipeline.
******************************************************************************/
Future<PipelineFlowState> ModifierApplication::evaluateInternal(TimePoint time)
{
//	qDebug() << "ModifierApplication::evaluateInternal modifier=" << modifier() << "time" << time << "this=" << this;

	// Obtain input data and pass it on to the modifier.
	return evaluateInput(time)
		.then(executor(), [this, time](PipelineFlowState inputData) -> Future<PipelineFlowState> {
//			qDebug() << "ModifierApplication::evaluateInternal received input for modifier=" << modifier() << inputData.stateValidity();

			// Clear the status of the input unless it is an error.
			if(inputData.status().type() != PipelineStatus::Error) {
				OVITO_ASSERT(inputData.status().type() != PipelineStatus::Pending);
				inputData.setStatus(PipelineStatus());
			}

			// Without a modifier, this ModifierApplication becomes a no-op.
			// The same is true when the Modifier is disabled.
			if(!modifier() || modifier()->isEnabled() == false)
				return inputData;

			// We don't want to create any undo records while performing the data modifications.
			UndoSuspender noUndo(this);

			Future<PipelineFlowState> future;
			try {
				// Let the modifier do its job.
				future = modifier()->evaluate(time, this, inputData);
			}
			catch(...) {
				future = Future<PipelineFlowState>::createFailed(std::current_exception());
			}

			// Change status to 'in progress' during long-running modifier evaluation.
			if(!future.isFinished()) {
				if(_numEvaluationsInProgress++ == 0)
					notifyDependents(ReferenceEvent::ObjectStatusChanged);
				// Reset the pending status after the Future is fulfilled.
				future.finally(executor(), [this]() {
					OVITO_ASSERT(_numEvaluationsInProgress > 0);
					if(--_numEvaluationsInProgress == 0)
						notifyDependents(ReferenceEvent::ObjectStatusChanged);
				});
			}

//			qDebug() << "ModifierApplication::evaluateInternal received future from modifier" << future.sharedState().get() << "isfinished=" << future.isFinished() << "iscanceled:" << future.isCanceled();

			// Post-process the modifier results before returning them to the caller.
			//
			//  - Turn any exception that was thrown during modifier evaluation into a 
			//    valid pipeline state with an error code.
			//
			//  - Restrict the validity interval of the returned state to the validity interval of the modifier.
			//
			return future.then_future(executor(), [this, time, inputData = std::move(inputData)](Future<PipelineFlowState> future) mutable {
//				qDebug() << "ModifierApplication::evaluateInternal: post-processing results of future" << future.sharedState().get() << "from modifier" << modifier() << "canceled=" << future.isCanceled();
				OVITO_ASSERT(future.isFinished());
				OVITO_ASSERT(!future.isCanceled());
				try {
					try {
						PipelineFlowState state = future.result();
						if(modifier()) state.intersectStateValidity(modifier()->modifierValidity(time));
						if(inputData.status().type() != PipelineStatus::Error)
							setStatus(state.status());
						else
							setStatus(PipelineStatus());
						return state;
					}
					catch(const Exception&) {
						throw;
					}
					catch(const std::bad_alloc&) {
						throwException(tr("Not enough memory."));
					}
					catch(const std::exception& ex) {
						qWarning() << "WARNING: Modifier" << modifier() << "has thrown a non-standard exception:" << ex.what();
						OVITO_ASSERT(false);
						throwException(tr("Exception: %1").arg(QString::fromLatin1(ex.what())));
					}
				}
				catch(Exception& ex) {
					setStatus(PipelineStatus(PipelineStatus::Error, ex.messages().join(QChar('\n'))));
					ex.prependGeneralMessage(tr("Modifier '%1' reported:").arg(modifier()->objectTitle()));
					inputData.setStatus(PipelineStatus(PipelineStatus::Error, ex.messages().join(QChar(' '))));
					return std::move(inputData);
				}
				catch(...) {
					OVITO_ASSERT_MSG(false, "ModifierApplication::evaluate()", "Caught an unexpected exception type during modifier evaluation.");
					PipelineStatus status(PipelineStatus::Error, tr("Unknown exception caught during evaluation of modifier '%1'.").arg(modifier()->objectTitle()));
					setStatus(status);
					inputData.setStatus(status);
					return std::move(inputData);
				}
			});
		});
}

/******************************************************************************
* Returns the results of an immediate and preliminary evaluation of the data pipeline.
******************************************************************************/
PipelineFlowState ModifierApplication::evaluatePreliminary()
{
	// Use our real state cache if it is up to date.
	PipelineFlowState state = CachingPipelineObject::evaluatePreliminary();
	if(state.stateValidity().contains(dataset()->animationSettings()->time())) {
//		qDebug() << "ModifierApplication::evaluatePreliminary() " << this << "for modifier" << modifier() << "returning cached state (validity=" << state.stateValidity() << "current time=" << dataset()->animationSettings()->time() << ")";
		return state;
	}

	// If not, ask the modifier to perform a preliminary evaluation.
	if(modifier() && input()) {
		UndoSuspender noUndo(this);
		// First get the preliminary results from the upstream pipeline.
		state = input()->evaluatePreliminary();
		try {
//			qDebug() << "ModifierApplication::evaluatePreliminary() for modifier" << modifier() << " evaluating modifier preliminarily on state with" << state.objects().size() << "objects";
			// Apply modifier:
			if(modifier()->isEnabled())
				state = modifier()->evaluatePreliminary(dataset()->animationSettings()->time(), this, state);
		}
		catch(const Exception& ex) {
			// Turn exceptions thrown during modifier evaluation into an error pipeline state.
			state.setStatus(PipelineStatus(PipelineStatus::Error, ex.messages().join(QStringLiteral(": "))));
		}
		catch(const std::bad_alloc&) {
			// Turn exceptions thrown during modifier evaluation into an error pipeline state.
			state.setStatus(PipelineStatus(PipelineStatus::Error, tr("Not enough memory.")));
		}
		catch(const std::exception& ex) {
			qWarning() << "WARNING: Modifier" << modifier() << "has thrown a non-standard exception:" << ex.what();
			OVITO_ASSERT(false);
			state.setStatus(PipelineStatus(PipelineStatus::Error, tr("Exception: %1").arg(QString::fromLatin1(ex.what()))));
		}
		catch(...) {
			OVITO_ASSERT_MSG(false, "ModifierApplication::evaluatePreliminary()", "Caught an unexpected exception type during preliminary modifier evaluation.");
			// Turn exceptions thrown during modifier evaluation into an error pipeline state.
			state.setStatus(PipelineStatus(PipelineStatus::Error, tr("Unknown exception caught during evaluation of modifier '%1'.").arg(modifier()->objectTitle())));
		}
	}

	return state;
}

/******************************************************************************
* Given an animation time, computes the source frame to show.
******************************************************************************/
int ModifierApplication::animationTimeToSourceFrame(TimePoint time) const
{
	if(input()) 
		return input()->animationTimeToSourceFrame(time);
	return CachingPipelineObject::animationTimeToSourceFrame(time);
}

/******************************************************************************
* Given a source frame index, returns the animation time at which it is shown.
******************************************************************************/
TimePoint ModifierApplication::sourceFrameToAnimationTime(int frame) const
{
	if(input())
		return input()->sourceFrameToAnimationTime(frame);
	return CachingPipelineObject::sourceFrameToAnimationTime(frame);
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
