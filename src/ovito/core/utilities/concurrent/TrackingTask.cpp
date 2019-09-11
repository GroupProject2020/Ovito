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
