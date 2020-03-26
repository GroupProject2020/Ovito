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
#include "MainThreadTask.h"
#include "TaskManager.h"
#include "FutureDetail.h"

namespace Ovito {

class OVITO_CORE_EXPORT PromiseBase
{
public:

	/// Default constructor.
#ifndef Q_CC_MSVC
	PromiseBase() noexcept = default;
#else
	PromiseBase() noexcept {}
#endif

	/// Destructor.
	~PromiseBase() { reset(); }

	/// A promise is not copy constructible.
	PromiseBase(const PromiseBase& other) = delete;

	/// Returns whether this promise object points to a valid shared state.
	bool isValid() const { return (bool)_task; }

	/// Detaches this promise from its shared state and makes sure that it reached the 'finished' state.
	/// If the promise wasn't already finished when this function is called, it is automatically canceled.
	void reset() {
		if(isValid()) {
			if(!isFinished()) {
				task()->cancel();
				if(isValid()) {
					task()->setStarted();
					task()->setFinished();
				}
			}
			_task.reset();
		}
	}

	/// Returns whether this promise has been canceled by a previous call to cancel().
	bool isCanceled() const { return task()->isCanceled(); }

	/// Returns true if the promise is in the 'started' state.
	bool isStarted() const { return task()->isStarted(); }

	/// Returns true if the promise is in the 'finished' state.
	bool isFinished() const { return task()->isFinished(); }

	/// Returns the maximum value for progress reporting.
    qlonglong progressMaximum() const { return task()->progressMaximum(); }

	/// Sets the current maximum value for progress reporting.
    void setProgressMaximum(qlonglong maximum) const { task()->setProgressMaximum(maximum); }

	/// Returns the current progress value (in the range 0 to progressMaximum()).
	qlonglong progressValue() const { return task()->progressValue(); }

	/// Sets the current progress value (must be in the range 0 to progressMaximum()).
	/// Returns false if the promise has been canceled.
    bool setProgressValue(qlonglong progressValue) const { return task()->setProgressValue(progressValue); }

	/// Increments the progress value by 1.
	/// Returns false if the promise has been canceled.
    bool incrementProgressValue(qlonglong increment = 1) const { return task()->incrementProgressValue(increment); }

	/// Sets the progress value of the promise but generates an update event only occasionally.
	/// Returns false if the promise has been canceled.
    bool setProgressValueIntermittent(qlonglong progressValue, int updateEvery = 2000) const { return task()->setProgressValueIntermittent(progressValue, updateEvery); }

	// Progress reporting for tasks with sub-steps:

	/// Begins a sequence of sub-steps in the progress range of this promise.
	/// This is used for long and complex computations, which consist of several logical sub-steps, each with
	/// a separate progress range.
    void beginProgressSubStepsWithWeights(std::vector<int> weights) const { task()->beginProgressSubStepsWithWeights(std::move(weights)); }

	/// Convenience version of the function above, which creates N substeps, all with the same weight.
	void beginProgressSubSteps(int nsteps) const { task()->beginProgressSubSteps(nsteps); }

	/// Changes to the next sub-step in the sequence started with beginProgressSubSteps().
	void nextProgressSubStep() const { task()->nextProgressSubStep(); }

	/// Must be called at the end of a sub-step sequence started with beginProgressSubSteps().
	void endProgressSubSteps() const { task()->endProgressSubSteps(); }

	/// Return the current status text set for this promise.
    QString progressText() const { return task()->progressText(); }

	/// Changes the status text of this promise.
	void setProgressText(const QString& progressText) const { task()->setProgressText(progressText); }

	/// Cancels this promise.
	void cancel() const { task()->cancel(); }

	/// This must be called after creating a promise to put it into the 'started' state.
	/// Returns false if the promise is or was already in the 'started' state.
	bool setStarted() const { return task()->setStarted(); }

	/// This must be called after the promise has been fulfilled (even if an exception occurred).
	void setFinished() const { task()->setFinished(); }

	/// Sets the promise into the 'exception' state to signal that an exception has occurred
	/// while trying to fulfill it. This method should be called from a catch(...) exception handler.
    void captureException() const { task()->captureException(); }

	/// Sets the promise into the 'exception' state to signal that an exception has occurred
	/// while trying to fulfill it.
    void setException(std::exception_ptr&& ex) const { task()->setException(std::move(ex)); }

	/// Blocks execution until the given future enters the completed state.
	bool waitForFuture(const FutureBase& future) const { return task()->waitForFuture(future); }

	/// Move assignment operator.
	PromiseBase& operator=(PromiseBase&& p) = default;

	/// A promise is not copy assignable.
	PromiseBase& operator=(const PromiseBase& other) = delete;

	/// Returns the shared state of this promise.
	const TaskPtr& task() const {
		OVITO_ASSERT(isValid());
		return _task;
	}

	/// Runs the given function in any case once this promise's task has reached the 'finished' or 'canceled' state.
	/// The continuation function will always be executed, even if this task was canceled or set to an error state.
    /// The continuation function must accept a TaskPtr (pointing to the finished task) as a parameter.
	template<typename Executor, typename F>
	void finally(Executor&& executor, bool defer, F&& cont) {
		// This future must be valid for finally() to work.
		OVITO_ASSERT_MSG(isValid(), "PromiseBase::finally()", "Promise must be valid.");
		task()->finally(std::forward<Executor>(executor), defer, std::forward<F>(cont));
	}

	/// Overload of the method above using the inline executor.
	template<typename F>
	void finally(F&& cont) { finally(Ovito::detail::InlineExecutor(), false, std::forward<F>(cont)); }	

protected:

	/// Move constructor.
	PromiseBase(PromiseBase&& p) noexcept = default;

	/// Constructor.
	PromiseBase(TaskPtr&& p) noexcept : _task(std::move(p)) {}

	/// Pointer to the state, which is shared with futures.
	TaskPtr _task;

	template<typename... R2> friend class Future;
	template<typename... R2> friend class SharedFuture;
};

template<typename... R>
class OVITO_CORE_EXPORT Promise : public PromiseBase
{
public:

	using tuple_type = std::tuple<R...>;
	using future_type = Future<R...>;
	using shared_future_type = SharedFuture<R...>;

	/// Default constructor.
#ifndef Q_CC_MSVC
	Promise() noexcept = default;
#else
	Promise() noexcept {}
#endif

	/// Create a promise that is ready and provides an immediate result.
	template<typename... R2>
	static Promise createImmediate(R2&&... result) {
		return Promise(std::make_shared<TaskWithResultStorage<Task, tuple_type>>(
			std::forward_as_tuple(std::forward<R2>(result)...),
			Task::State(Task::Started | Task::Finished)));
	}

	/// Create a promise that is ready and provides an immediate result.
	template<typename... Args>
	static Promise createImmediateEmplace(Args&&... args) {
		using first_type = typename std::tuple_element<0, tuple_type>::type;
		return Promise(std::make_shared<TaskWithResultStorage<Task, tuple_type>>(
			std::forward_as_tuple(first_type(std::forward<Args>(args)...)),
			Task::State(Task::Started | Task::Finished)));
	}

	/// Creates a promise that is in the 'exception' state.
	static Promise createFailed(const Exception& ex) {
		Promise promise(std::make_shared<Task>(Task::State(Task::Started | Task::Finished)));
		promise.task()->_exceptionStore = std::make_exception_ptr(ex);
		return promise;
	}

	/// Creates a promise that is in the 'exception' state.
	static Promise createFailed(std::exception_ptr ex_ptr) {
		Promise promise(std::make_shared<Task>(Task::State(Task::Started | Task::Finished)));
		promise.task()->_exceptionStore = std::move(ex_ptr);
		return promise;
	}

	/// Creates a promise without results that is in the canceled state.
	static Promise createCanceled() {
		return Promise(std::make_shared<Task>(Task::State(Task::Started | Task::Canceled | Task::Finished)));
	}

	/// Creates a promise that can be used just for signaling the completion of an asynchronous operation.
	static Promise createSignal() {
		static_assert(std::tuple_size<tuple_type>::value == 0, "Signal promise may not have a result value.");
		return Promise(std::make_shared<Task>(Task::Started));
	}

	/// Creates a new task that performs actions in an asynchronous fashion.
	static Promise createAsynchronousOperation(TaskManager& taskManager, bool startedState = true) {
		Promise promise(std::make_shared<TaskWithResultStorage<MainThreadTask, tuple_type>>(
			typename TaskWithResultStorage<MainThreadTask, tuple_type>::no_result_init_t(),
			startedState ? Task::State(Task::Started) : Task::NoState, &taskManager));
		return promise;
	}

	/// Returns a Future that is associated with the same shared state as this promise.
	future_type future() {
#ifdef OVITO_DEBUG
		OVITO_ASSERT_MSG(!_futureCreated, "Promise::future()", "Only a single Future may be created from a Promise.");
		_futureCreated = true;
#endif
		return future_type(TaskPtr(task()));
	}

	/// Returns a SharedFuture that is associated with the same shared state as this promise.
	shared_future_type sharedFuture() {
		return shared_future_type(TaskPtr(task()));
	}	

	/// Sets the result value of the promise.
	template<typename... R2>
	void setResults(R2&&... result) {
		task()->template setResults<tuple_type>(std::forward_as_tuple(std::forward<R2>(result)...));
	}

	/// Sets the result value of the promise to the return value of the given function.
	template<typename FC>
	std::enable_if_t<Ovito::detail::is_void_continuation_func<FC,std::tuple<>>::value> setResultsWith(FC&& func)
	{
		std::forward<FC>(func)();
	}

	/// Sets the result value of the promise to the return value of the given function.
	template<typename FC>
	std::enable_if_t<!Ovito::detail::is_void_continuation_func<FC,std::tuple<>>::value> setResultsWith(FC&& func)
	{
		setResultsDirect(std::forward<FC>(func)());
	}

	/// Requests this Promise to reset itself to the null state as soon as the task has reached the 'finished' state.
	template<typename Executor>
	void autoResetWhenFinished(Executor&& executor) {
		this->finally(std::forward<Executor>(executor), false, [this](const TaskPtr& task) {
			OVITO_ASSERT(this->task() == task);
			reset();
		});
	}

	/// Requests this Promise to reset itself to the null state as soon as the task is canceled.
	template<typename Executor>
	void autoResetWhenCanceled(Executor&& executor) {
		this->finally(std::forward<Executor>(executor), false, [this](const TaskPtr& task) {
			OVITO_ASSERT(this->task() == task);
			if(task->isCanceled())
				reset();
		});
	}

protected:

	Promise(TaskPtr p) noexcept : PromiseBase(std::move(p)) {}

	// Assigns a result to this promise.
	template<typename source_tuple_type>
	auto setResultsDirect(source_tuple_type&& results) -> typename std::enable_if<std::tuple_size<source_tuple_type>::value>::type {
		static_assert(std::tuple_size<tuple_type>::value != 0, "Must not be an empty tuple");
		static_assert(std::is_same<tuple_type, std::decay_t<source_tuple_type>>::value, "Must assign a compatible tuple");
		task()->template setResults<tuple_type>(std::forward<source_tuple_type>(results));
	}

	// Assigns a result to this promise.
	template<typename value_type>
	void setResultsDirect(value_type&& result) {
		static_assert(std::tuple_size<tuple_type>::value == 1, "Must be a tuple of size 1");
		task()->template setResults<tuple_type>(std::forward_as_tuple(std::forward<value_type>(result)));
	}

#ifdef OVITO_DEBUG
	bool _futureCreated = false;
#endif

	friend class TaskManager;
	template<typename... R2> friend class Future;
	template<typename... R2> friend class SharedFuture;
};

/**
 * A promise object that is used for long-running program actions that are executed in the main thread.
 * 
 * The task of a SynchronousOperation is automatically put into the 'finished' state by the destructor.
 */
class OVITO_CORE_EXPORT SynchronousOperation : public Promise<>
{
public:

	/// No default constructor.
	SynchronousOperation() = delete;

	/// Move constructor. 
	SynchronousOperation(SynchronousOperation&& p) noexcept = default;

	/// Destructor.
	~SynchronousOperation() { reset(); }

	/// Puts the task into the 'finished' state and detaches this SynchronousOperation object from the task.
	void reset() {
		if(isValid()) {
			// Set the task to the 'finished' if this is the master SynchronousOperation.
			// If this is not the master, keep the task unfinished, because there the operation is still going on.
			if(_isMaster && !isFinished()) {
				task()->setStarted();
				task()->setFinished();
			}
			_task.reset();
		}
	}

	/// Creates a new synchronous operation that should be used when performing some long-running work in in the main thread.
	/// The factory method registers the task with the given task manager and puts it into the 'started' state.
	static SynchronousOperation create(TaskManager& taskManager, bool startedState = true) {
		SynchronousOperation op(std::make_shared<TaskWithResultStorage<MainThreadTask, tuple_type>>(
			typename TaskWithResultStorage<MainThreadTask, tuple_type>::no_result_init_t(),
			startedState ? Task::State(Task::Started) : Task::NoState, &taskManager), true);
		taskManager.addTaskInternal(op.task());
		return op;
	}

	/// Creates a promise that can be used just for signaling the completion of an asynchronous operation.
	static SynchronousOperation createSignal(TaskManager& taskManager) {
		return SynchronousOperation(std::make_shared<Task>(Task::Started, &taskManager), true);
	}

	/// Creates a child operation that executes within the context of this parent operation.
    /// In case the child task gets canceled, this parent task gets canceled too --and vice versa.
	SynchronousOperation subOperation(bool registerAsNewTask = false) {
		OVITO_ASSERT(isValid());
		OVITO_ASSERT(isStarted());
		OVITO_ASSERT(!isFinished());

		if(registerAsNewTask) {

			// Create the task object for the child operation.
			SynchronousOperation subOperation = SynchronousOperation::create(*task()->taskManager(), true);

			// Ensure that the child operation gets canceled together with the parent operation.
			this->finally(Ovito::detail::InlineExecutor(), false, [subTask = subOperation.task()](const TaskPtr& task) {
				if(task->isCanceled())
					subTask->cancel();
			});

			// Ensure that the parent operation gets canceled if the child operation is canceled.
			subOperation.finally(Ovito::detail::InlineExecutor(), false, [thisTask = task()](const TaskPtr& task) {
				if(task->isCanceled())
					thisTask->cancel();
			});

			// Register child operation with task manager.
			task()->taskManager()->addTaskInternal(subOperation.task());

			return subOperation;
		}
		else {
			// Create a promise that is referring to the same task object.
			return SynchronousOperation(task(), false);
		}	
	}

private:

	/// Constructor.
	SynchronousOperation(TaskPtr p, bool isMaster) noexcept :
		Promise<>(std::move(p)), _isMaster(isMaster) {}

	/// Indicates whether this SynchronousOperation is managing the shared task object or not.
	bool _isMaster;
};

}	// End of namespace
