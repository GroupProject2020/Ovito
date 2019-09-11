///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2018) Alexander Stukowski
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
#include <ovito/core/utilities/concurrent/TaskManager.h>
#include <ovito/core/viewport/ViewportConfiguration.h>
#include <ovito/core/dataset/DataSetContainer.h>
#include <ovito/core/app/Application.h>

#ifdef Q_OS_UNIX
	#include <csignal>
#endif

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(Concurrency)

/******************************************************************************
* Initializes the task manager.
******************************************************************************/
TaskManager::TaskManager(DataSetContainer& owner) : _owner(owner)
{
	qRegisterMetaType<TaskPtr>("TaskPtr");
}

/******************************************************************************
* Destructor.
******************************************************************************/
TaskManager::~TaskManager()
{
    for(TaskWatcher* watcher : runningTasks()) {
        OVITO_ASSERT_MSG(watcher->isFinished() || watcher->isCanceled(), "TaskManager destructor",
                        "Some tasks are still in progress while destroying the TaskManager.");
    }
}

/******************************************************************************
* Registers a future's promise with the progress manager, which will display
* the progress of the background task in the main window.
******************************************************************************/
void TaskManager::registerFuture(const FutureBase& future)
{
	registerTask(future.task());
}

/******************************************************************************
* Registers a promise with the progress manager, which will display
* the progress of the background task in the main window.
******************************************************************************/
void TaskManager::registerPromise(const PromiseBase& promise)
{
	registerTask(promise.task());
}

/******************************************************************************
* Registers a promise with the task manager.
******************************************************************************/
void TaskManager::registerTask(const TaskPtr& task)
{
	// Execute the function call in the main thread.
	QMetaObject::invokeMethod(this, "addTaskInternal", Q_ARG(TaskPtr, task));
}

/******************************************************************************
* Registers a promise with the task manager.
******************************************************************************/
TaskWatcher* TaskManager::addTaskInternal(const TaskPtr& task)
{
	OVITO_ASSERT(!QCoreApplication::instance() || QThread::currentThread() == QCoreApplication::instance()->thread());

	// Check if the task has already been registered before.
    // In this case, a TaskWatcher must exist for the task that has been added as a child object to the TaskManager.
	for(QObject* childObject : children()) {
		if(TaskWatcher* watcher = qobject_cast<TaskWatcher*>(childObject)) {
			if(watcher->task() == task)
				return watcher;
		}
	}

	// Create a task watcher, which will generate start/stop notification signals.
	TaskWatcher* watcher = new TaskWatcher(this);
	connect(watcher, &TaskWatcher::started, this, &TaskManager::taskStartedInternal);
	connect(watcher, &TaskWatcher::finished, this, &TaskManager::taskFinishedInternal);

	// Activate the watcher.
	watcher->watch(task);
	return watcher;
}

/******************************************************************************
* Waits for the given future to be fulfilled and displays a modal progress
* dialog to show the progress.
******************************************************************************/
bool TaskManager::waitForFuture(const FutureBase& future)
{
	return waitForTask(future.task());
}

/******************************************************************************
* Is called when a task has started to run.
******************************************************************************/
void TaskManager::taskStartedInternal()
{
	TaskWatcher* watcher = static_cast<TaskWatcher*>(sender());
	_runningTaskStack.push_back(watcher);

	Q_EMIT taskStarted(watcher);
}

/******************************************************************************
* Is called when a task has finished.
******************************************************************************/
void TaskManager::taskFinishedInternal()
{
	TaskWatcher* watcher = static_cast<TaskWatcher*>(sender());

	OVITO_ASSERT(std::find(_runningTaskStack.begin(), _runningTaskStack.end(), watcher) != _runningTaskStack.end());
	_runningTaskStack.erase(std::find(_runningTaskStack.begin(), _runningTaskStack.end(), watcher));

	Q_EMIT taskFinished(watcher);

	watcher->deleteLater();
}

/******************************************************************************
* Cancels all running background tasks.
******************************************************************************/
void TaskManager::cancelAll()
{
	for(TaskWatcher* watcher : runningTasks()) {
		if(watcher->task()) {
			watcher->task()->cancel();
		}
	}
}

/******************************************************************************
* Cancels all running background tasks and waits for them to finish.
******************************************************************************/
void TaskManager::cancelAllAndWait()
{
	cancelAll();
	waitForAll();
}

/******************************************************************************
* Waits for all tasks to finish.
******************************************************************************/
void TaskManager::waitForAll()
{
	OVITO_ASSERT(!QCoreApplication::instance() || QThread::currentThread() == QCoreApplication::instance()->thread());
	do {
		QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
		QCoreApplication::sendPostedEvents(nullptr, OvitoObjectExecutor::workEventType());
	}
	while(!runningTasks().empty());
}

/******************************************************************************
* This should be called whenever a local event handling loop is entered.
******************************************************************************/
void TaskManager::startLocalEventHandling()
{
	OVITO_ASSERT_MSG(!QCoreApplication::instance() || QThread::currentThread() == QCoreApplication::instance()->thread(), "TaskManager::startLocalEventHandling", "Function may only be called from the main thread.");

	_inLocalEventLoop++;
}

/*******************************
***********************************************
* This should be called whenever a local event handling loop is left.
******************************************************************************/
void TaskManager::stopLocalEventHandling()
{
	OVITO_ASSERT_MSG(!QCoreApplication::instance() || QThread::currentThread() == QCoreApplication::instance()->thread(), "TaskManager::stopLocalEventHandling", "Function may only be called from the main thread.");
	OVITO_ASSERT(_inLocalEventLoop > 0);

	_inLocalEventLoop--;
}

/******************************************************************************
* Waits for the given task to finish.
******************************************************************************/
bool TaskManager::waitForTask(const TaskPtr& task, const TaskPtr& dependentTask)
{
	OVITO_ASSERT_MSG(!QCoreApplication::instance() || QThread::currentThread() == QCoreApplication::instance()->thread(), "TaskManager::waitForTask", "Function may be called only from the main thread.");

	// Before entering the local event loop, check if the task has already finished.
	if(task->isFinished()) {
		return !task->isCanceled();
	}
	if(dependentTask && dependentTask->isCanceled()) {
		return false;
	}

	// Make sure this method is not called while rendering a viewport.
	// Qt doesn't allow a local event loops during paint event processing.
	if(DataSet* dataset = _owner.currentSet()) {
		if(dataset->viewportConfig()->isRendering()) {
			qWarning() << "WARNING: Do not call TaskManager::waitForTask() during interactive viewport rendering!";
			task->setException(std::make_exception_ptr(Exception(tr("This operation is not permitted during interactive viewport rendering. "
				"Note that certain long-running operations, e.g. I/O operations or complex computations, cannot be performed while viewport rendering is in progress. "), dataset)));
			return !task->isCanceled();;
		}
	}

	// Register the task in case it hasn't been registered with this TaskManager yet.
	TaskWatcher* watcher = addTaskInternal(task);

	// Start a local event loop and wait for the task to generate a signal when it finishes.
	QEventLoop eventLoop;
	connect(watcher, &TaskWatcher::finished, &eventLoop, &QEventLoop::quit);

	// Stop the event loop when the dependent task gets canceled.
	if(dependentTask) {
		TaskWatcher* dependentWatcher = addTaskInternal(dependentTask);
		connect(dependentWatcher, &TaskWatcher::canceled, watcher, [dependentTask, watcher]() {
			watcher->task()->cancelIfSingleFutureLeft();
		});
		connect(dependentWatcher, &TaskWatcher::canceled, &eventLoop, &QEventLoop::quit);
	}

#ifdef Q_OS_UNIX
	// Boolean flag which is set by the POSIX signal handler when user
	// presses Ctrl+C to interrupt the program.
	static QAtomicInt userInterrupt;
	userInterrupt.storeRelease(0);

	// Install POSIX signal handler to catch Ctrl+C key signal.
	static QEventLoop* activeEventLoop = nullptr;
	activeEventLoop = &eventLoop;
	auto oldSignalHandler = ::signal(SIGINT, [](int) {
		userInterrupt.storeRelease(1);
		if(activeEventLoop) {
			QMetaObject::invokeMethod(activeEventLoop, "quit");
		}
	});
#endif

	// If this method was called as part of a script, temporarily switch to interactive mode now since
	// the user may perform actions in the user interface while the local event loop is active.
	bool wasCalledFromScript = (Application::instance()->executionContext() == Application::ExecutionContext::Scripting);
	if(wasCalledFromScript)
		Application::instance()->switchExecutionContext(Application::ExecutionContext::Interactive);

	startLocalEventHandling();
	eventLoop.exec();
	stopLocalEventHandling();

	// Restore previous execution context state.
	if(wasCalledFromScript)
		Application::instance()->switchExecutionContext(Application::ExecutionContext::Scripting);

#ifdef Q_OS_UNIX
	::signal(SIGINT, oldSignalHandler);
	activeEventLoop = nullptr;
	if(userInterrupt.load()) {
		cancelAll();
		return false;
	}
#endif

	if(dependentTask && dependentTask->isCanceled()) {
		return false;
	}

	if(!task->isFinished()) {
		qWarning() << "Warning: TaskManager::waitForTask() returning with an unfinished promise state (canceled=" << task->isCanceled() << ")";
		task->cancel();
	}

	return !task->isCanceled();
}

/******************************************************************************
* Process events from the event queue when the tasks manager has started
* a local event loop. Otherwise do nothing and let the main event loop
* do the processing.
******************************************************************************/
void TaskManager::processEvents()
{
	if(_inLocalEventLoop)
		QCoreApplication::processEvents();
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
