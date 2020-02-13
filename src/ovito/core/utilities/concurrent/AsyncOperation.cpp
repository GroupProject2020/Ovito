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
#include "AsyncOperation.h"
#include "TaskManager.h"
#include "TaskWatcher.h"

namespace Ovito {

/******************************************************************************
* Constructor creating a new operation, registering it with the given 
* task manager, and putting into the 'started' state.
******************************************************************************/
AsyncOperation::AsyncOperation(TaskManager& taskManager) : Promise(taskManager.createMainThreadOperation<>(true))
{
}

/******************************************************************************
* Returns the TaskWatcher automatically created by the TaskManager for this operation.
******************************************************************************/
TaskWatcher* AsyncOperation::watcher()
{
    OVITO_ASSERT(isValid());
	OVITO_ASSERT(!isFinished());
	OVITO_ASSERT(task()->taskManager() != nullptr);

    return task()->taskManager()->addTaskInternal(task());
}

/******************************************************************************
* Creates a child operation that executes within the context of this parent 
* operation. In case the child task gets canceled, this parent task gets 
* canceled too --and vice versa.
******************************************************************************/
AsyncOperation AsyncOperation::createSubOperation()
{
    OVITO_ASSERT(isValid());
	OVITO_ASSERT(isStarted());
	OVITO_ASSERT(!isFinished());
	OVITO_ASSERT(task()->taskManager() != nullptr);

	// Create the sub-operation object.
	AsyncOperation subOperation(*task()->taskManager());

	// Ensure that the sub-operation gets canceled together with the parent operation.
	TaskWatcher* parentOperationWatcher = watcher();
	TaskWatcher* subOperationWatcher = subOperation.watcher();
	QObject::connect(parentOperationWatcher, &TaskWatcher::canceled, subOperationWatcher, &TaskWatcher::cancel);
	QObject::connect(subOperationWatcher, &TaskWatcher::canceled, parentOperationWatcher, &TaskWatcher::cancel);

	return subOperation;
}

/******************************************************************************
* Creates a special async operation that can be used just for signaling the 
* completion of an operation and which is not registered with a task manager.
******************************************************************************/
AsyncOperation AsyncOperation::createSignalOperation(bool startedState, TaskManager* taskManager)
{
	return AsyncOperation(std::make_shared<Task>(Task::State(startedState ? Task::Started : Task::NoState), taskManager));
}

}	// End of namespace
