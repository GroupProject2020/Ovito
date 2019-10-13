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

#include <ovito/core/Core.h>
#include "TrackingTask.h"
#include "Future.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(Concurrency)

#ifdef OVITO_DEBUG
TrackingTask::~TrackingTask()
{
	// Shared states must always end up in the finished state.
	OVITO_ASSERT(isFinished());
	OVITO_ASSERT(!trackedState() || trackedState()->isFinished());
	OVITO_ASSERT(!_nextInList);
}
#endif

void TrackingTask::setTrackedState(TaskDependency&& state)
{
	// The function may only be called once.
	OVITO_ASSERT(!trackedState());
	OVITO_ASSERT(state.get());
	_trackedState.swap(state);

//	qDebug() << "State" << this << "is now tracking state" << trackedState().get();

	// Track it.
	trackedState()->registerTracker(this);

	// Propagate cancelation state.
	if(!trackedState()->isCanceled() && this->isCanceled())
		trackedState()->cancel();

	// Our reference to the fulfilled creator state is no longer needed.
	_creatorState.reset();
}

void TrackingTask::cancel() noexcept
{
	if(!isCanceled()) {
		Task::cancel();
		if(trackedState())
			trackedState()->cancel();
		setStarted();
		setFinished();
	}
}

void TrackingTask::setFinished()
{
	// Our reference to the fulfilled creator state is no longer needed.
	_creatorState.reset();
	Task::setFinished();
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
