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
#include "MainThreadTask.h"
#include "Future.h"
#include "TaskManager.h"
#include "TaskWatcher.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(Concurrency)

bool MainThreadTask::setProgressValue(qlonglong value)
{
	// Yield control to the event loop to process user interface events.
	// This is necessary so that the user can interrupt the running opertion.
	_taskManager.processEvents();

    return ProgressiveTask::setProgressValue(value);
}

bool MainThreadTask::incrementProgressValue(qlonglong increment)
{
	// Yield control to the event loop to process user interface events.
	// This is necessary so that the user can interrupt the running opertion.
	_taskManager.processEvents();

	return ProgressiveTask::incrementProgressValue(increment);
}

void MainThreadTask::setProgressText(const QString& progressText)
{
	ProgressiveTask::setProgressText(progressText);

	// Yield control to the event loop to process user interface events.
	// This is necessary so that the user can interrupt the running opertion.
	_taskManager.processEvents();
}

Promise<> MainThreadTask::createSubTask()
{
	OVITO_ASSERT(isStarted());
	OVITO_ASSERT(!isFinished());

	// Create a new promise for the sub-operation.
	Promise<> subOperation = _taskManager.createMainThreadOperation<>(true);

	// Ensure that the sub-operation gets canceled together with the parent operation.
	TaskWatcher* parentOperationWatcher = _taskManager.addTaskInternal(shared_from_this());
	TaskWatcher* subOperationWatcher = _taskManager.addTaskInternal(subOperation.task());
	QObject::connect(parentOperationWatcher, &TaskWatcher::canceled, subOperationWatcher, &TaskWatcher::cancel);
	QObject::connect(subOperationWatcher, &TaskWatcher::canceled, parentOperationWatcher, &TaskWatcher::cancel);

	return subOperation;
}

/// Blocks execution until the given future reaches the completed state.
bool MainThreadTask::waitForFuture(const FutureBase& future)
{
	if(!_taskManager.waitForTask(future.task(), shared_from_this())) {
		cancel();
		return false;
	}
	return true;
}


OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
