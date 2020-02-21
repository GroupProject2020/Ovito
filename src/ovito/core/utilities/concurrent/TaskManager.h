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

#pragma once


#include <ovito/core/Core.h>
#include "Task.h"

#ifndef OVITO_DISABLE_THREADING
	#include <QThreadPool>
#endif

namespace Ovito {

/**
 * \brief Manages the background tasks.
 */
class OVITO_CORE_EXPORT TaskManager : public QObject
{
	Q_OBJECT

public:

	/// Constructor.
	TaskManager(DataSetContainer* datasetContainer = nullptr);

	/// Destructor.
	~TaskManager();

	/// \brief Returns the dataset container this task manager belongs to.
	/// \return The dataset container that owns this task manager; or nullptr if the task manager doesn't belong to any dataset container.
	DataSetContainer* datasetContainer() const { return _datasetContainer; }

    /// \brief Returns the watchers for all currently running tasks.
    /// \return A list of TaskWatcher objects, one for each registered task that is currently in the 'started' state.
    /// \note This method is *not* thread-safe and may only be called from the main thread.
	const std::vector<TaskWatcher*>& runningTasks() const { return _runningTaskStack; }

	/// \brief Executes an asynchronous task in a background thread.
	///
	/// This function is thread-safe. It returns a Future that is fulfilled when the task completed.
	template<class TaskType>
	auto runTaskAsync(const std::shared_ptr<TaskType>& task) {
		OVITO_ASSERT(task);
		// Associate the task with this TaskManager.
		registerTask(task);
#ifndef OVITO_DISABLE_THREADING
		// Submit the task for execution in a background thread.
		QThreadPool::globalInstance()->start(task.get());
#else
		// If multi-threading is not available, run the task in the main thread as soon as execution returns to the event loop.
		QTimer::singleShot(0, this, [task]() { task->run(); });
#endif
		return task->future();
	}

    /// \brief Registers a future with the TaskManager, which will subsequently track the progress of the associated operation.
    /// \param future The Future whose shared state should be registered.
    /// \note This function is thread-safe.
    /// \sa registerTask()
    /// \sa FutureBase::task()
    void registerFuture(const FutureBase& future);

    /// \brief Registers a promise with the TaskManager, which will subsequently track the progress of the associated operation.
    /// \param promise The Promise whose shared state should be registered.
    /// \note This function is thread-safe.
    /// \sa registerTask()
    /// \sa PromiseBase::task()
    void registerPromise(const PromiseBase& promise);

    /// \brief Registers a Task with the TaskManager, which will subsequently track the progress of the associated operation.
    /// \note This function is thread-safe.
	void registerTask(const TaskPtr& task);

    /// \brief Waits for the given future to be fulfilled and displays a modal progress dialog to show the progress.
    /// \return False if the operation has been cancelled by the user.
    ///
    /// This function must be called from the main thread.
    bool waitForFuture(const FutureBase& future);

    /// \brief Waits for the given task to finish.
	/// \param task The task for which to wait.
	/// \param dependentTask Optionally another task that is waiting for \a task. The method will return early if the dependent task has been canceled.
	/// \return false if either \a task or \a dependentTask have been canceled.
    bool waitForTask(const TaskPtr& task, const TaskPtr& dependentTask = {});

	/// \brief Process events from the event queue when the tasks manager has started
	///        a local event loop. Otherwise does nothing and lets the main event loop
	///        do the processing.
	void processEvents();

	/// \brief This should be called whenever a local event handling loop is entered.
	void startLocalEventHandling();

	/// \brief This should be called whenever a local event handling loop is left.
	void stopLocalEventHandling();

	/// \brief Returns whether printing of task status messages to the console is currently enabled.
	bool consoleLoggingEnabled() const { return _consoleLoggingEnabled; }

	/// \brief Enables or disables printing of task status messages to the console for this task manager.
	void setConsoleLoggingEnabled(bool enabled);

public Q_SLOTS:

	/// Cancels all running tasks.
	void cancelAll();

	/// Cancels all running tasks and waits for them to finish.
	void cancelAllAndWait();

	/// Waits for all running tasks to finish.
	void waitForAll();

Q_SIGNALS:

    /// \brief This signal is generated by the task manager whenever one of the registered tasks started to run.
    /// \param watcher The TaskWatcher can be used to track the operation's progress.
	void taskStarted(TaskWatcher* taskWatcher);

    /// \brief This signal is generated by the task manager whenever one of the registered tasks has finished or stopped running.
    /// \param watcher The TaskWatcher that was used by the task manager to track the running task.
	void taskFinished(TaskWatcher* taskWatcher);

private:

	/// \brief Registers a promise with the progress manager.
	Q_INVOKABLE TaskWatcher* addTaskInternal(const TaskPtr& sharedState);

    /// \brief Waits for a task to finish while running in the main UI thread.
    bool waitForTaskUIThread(const TaskPtr& task, const TaskPtr& dependentTask);

    /// \brief Waits for a task to finish while running in a thread other than the main UI thread.
    bool waitForTaskNonUIThread(const TaskPtr& task, const TaskPtr& dependentTask);

private Q_SLOTS:

	/// \brief Is called when a task has started to run.
	void taskStartedInternal();

	/// \brief Is called when a task has finished.
	void taskFinishedInternal();

	/// \brief Is called when a task has reported a new progress text (only if logging is enabled).
	void taskProgressTextChangedInternal(const QString& msg);

private:

	/// The list of watchers for the active tasks.
	std::vector<TaskWatcher*> _runningTaskStack;

	/// Indicates that waitForTask() has started a local event loop.
	int _inLocalEventLoop = 0;

	/// The dataset container owning this task manager (may be NULL).
	DataSetContainer* _datasetContainer;

	/// Enables printing of task status messages to the console.
	bool _consoleLoggingEnabled = false;

	friend class SynchronousOperation; // Needed by SynchronousOperation::create()
};

}	// End of namespace

Q_DECLARE_METATYPE(Ovito::TaskPtr);
