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

#include <ovito/core/Core.h>
#include "MainThreadTask.h"
#include "Future.h"
#include "TaskManager.h"
#include "TaskWatcher.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(Concurrency)

bool MainThreadTask::setProgressValue(qlonglong value)
{
	// Yield control to the event loop to process user interface events.
	// This is necessary so that the user can interrupt the running operation.
	taskManager()->processEvents();

    return ProgressiveTask::setProgressValue(value);
}

bool MainThreadTask::incrementProgressValue(qlonglong increment)
{
	// Yield control to the event loop to process user interface events.
	// This is necessary so that the user can interrupt the running operation.
	taskManager()->processEvents();

	return ProgressiveTask::incrementProgressValue(increment);
}

void MainThreadTask::setProgressText(const QString& progressText)
{
	ProgressiveTask::setProgressText(progressText);

	// Yield control to the event loop to process user interface events.
	// This is necessary so that the user can interrupt the running operation.
	taskManager()->processEvents();
}

Promise<> MainThreadTask::createSubTask()
{
	OVITO_ASSERT(isStarted());
	OVITO_ASSERT(!isFinished());

	// Create a new promise for the sub-operation.
	Promise<> subOperation = taskManager()->createMainThreadOperation<>(true);

	// Ensure that the sub-operation gets canceled together with the parent operation.
	TaskWatcher* parentOperationWatcher = taskManager()->addTaskInternal(shared_from_this());
	TaskWatcher* subOperationWatcher = taskManager()->addTaskInternal(subOperation.task());
	QObject::connect(parentOperationWatcher, &TaskWatcher::canceled, subOperationWatcher, &TaskWatcher::cancel);
	QObject::connect(subOperationWatcher, &TaskWatcher::canceled, parentOperationWatcher, &TaskWatcher::cancel);

	return subOperation;
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
