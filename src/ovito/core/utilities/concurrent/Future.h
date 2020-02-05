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
#include "Promise.h"
#include "FutureDetail.h"
#include "ContinuationTask.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(Concurrency)

/******************************************************************************
* Generic base class for futures, which implements the basic state management,
* progress reporting, and event processing.
******************************************************************************/
class OVITO_CORE_EXPORT FutureBase
{
public:

	/// Destructor.
	~FutureBase() { reset(); }

	/// Returns true if the shared state associated with this Future has been canceled.
	bool isCanceled() const { return task()->isCanceled(); }

	/// Returns true if the shared state associated with this Future has been fulfilled.
	bool isFinished() const { return task()->isFinished(); }

	/// Returns true if this Future is associated with a shared state.
	bool isValid() const { return (bool)_task.get(); }

	/// Dissociates this Future from its shared state.
	void reset() {
		_task.reset();
	}

	/// Returns the shared state associated with this Future.
	/// Make sure it has one before calling this function.
	const TaskPtr& task() const {
		OVITO_ASSERT(isValid());
		return _task.get();
	}

	/// Takes out the task dependency and invalidates this future.
	TaskDependency takeTaskDependency() {
		return std::move(_task);
	}

	/// Move constructor.
	FutureBase(FutureBase&& other) noexcept = default;

	/// Copy constructor.
	FutureBase(const FutureBase& other) noexcept = default;

	/// A future is moveable.
	FutureBase& operator=(FutureBase&& other) noexcept = default;

	/// Copy assignment.
	FutureBase& operator=(const FutureBase& other) noexcept = default;

	/// Runs the given function in any case once this future has reached the 'finished' state.
	/// The continuation function will always be executed, even if this future was canceled or set to an error state.
	template<typename Executor, typename F>
	void finally(Executor&& executor, bool defer, F&& cont) {
		// This future must be valid for finally() to work.
		OVITO_ASSERT_MSG(isValid(), "FutureBase::finally()", "Future must be valid.");
		task()->finally(std::forward<Executor>(executor), defer, std::forward<F>(cont));
	}

	/// Runs the given function in any case once this future has reached the 'finished' state.
	/// The continuation function will always be executed, even if this future was canceled or set to an error state.
	template<typename Executor, typename F>
	void finally(Executor&& executor, F&& cont) { finally(std::forward<Executor>(executor), false, std::forward<F>(cont)); }

	/// Version of the function above, which uses the default inline executor.
	template<typename F>
	void finally(F&& cont) { finally(Ovito::detail::InlineExecutor(), std::forward<F>(cont)); }

	/// Runs the given function in any case once this future has reached the 'exception' state.
	/// The function must accept a const reference to a std::exception_ptr as a parameter.
	template<typename Executor, typename F>
	void on_error(Executor&& executor, F&& f) {
		// This future must be valid for on_error() to work.
		OVITO_ASSERT_MSG(isValid(), "FutureBase::on_error()", "Future must be valid.");
		task()->finally(std::forward<Executor>(executor), false, [f = std::forward<F>(f), task = this->task()]() mutable {
			OVITO_STATIC_ASSERT((std::is_same<decltype(task), TaskPtr>::value));
			if(!task->isCanceled() && task->_exceptionStore)
				std::move(f)(task->_exceptionStore);
			task.reset();
		});
	}

protected:

	/// Default constructor creating a future without a shared state.
#ifndef Q_CC_MSVC
	FutureBase() noexcept = default;
#else
	FutureBase() noexcept {}
#endif

	/// Constructor that creates a Future associated with a share state.
	explicit FutureBase(TaskPtr&& p) noexcept : _task(std::move(p)) {}

	/// Constructor that creates a Future from an existing task dependency.
	explicit FutureBase(TaskDependency&& p) noexcept : _task(std::move(p)) {}

	/// The shared state associated with this Future.
	TaskDependency _task;
};

/******************************************************************************
* A future that provides access to the value computed by a Promise.
******************************************************************************/
template<typename... R>
class Future : public FutureBase
{
public:

	using this_type = Future<R...>;
	using tuple_type = std::tuple<R...>;
	using promise_type = Promise<R...>;

	/// Default constructor that constructs an invalid Future that is not associated with any shared state.
#ifndef Q_CC_MSVC
	Future() noexcept = default;
#else
	Future() noexcept {}
#endif

	/// A future is not copy-constructible.
	Future(const Future& other) = delete;

	/// A future is move-constructible.
	Future(Future&& other) noexcept = default;

	/// A future may directly be initialized from r-values.
	template<typename... R2, size_t C = sizeof...(R),
		typename = std::enable_if_t<C != 0
			&& !std::is_same<std::tuple<std::decay_t<R2>...>, std::tuple<Future<R...>>>::value
			&& !std::is_same<std::tuple<std::decay_t<R2>...>, std::tuple<TaskPtr>>::value>>
	Future(R2&&... val) noexcept : FutureBase(std::move(promise_type::createImmediate(std::forward<R2>(val)...)._task)) {}

	/// A future is moveable.
	Future& operator=(Future&& other) noexcept = default;

	/// A future is not copy assignable.
	Future& operator=(const Future& other) = delete;

	/// Creates a future that is in the 'canceled' state.
	static Future createCanceled() {
		return promise_type::createCanceled();
	}

	/// Create a future that is ready and provides an immediate result.
	template<typename... V>
	static Future createImmediate(V&&... result) {
		return promise_type::createImmediate(std::forward<V>(result)...);
	}

	/// Create a future that is ready and provides an immediate result.
	template<typename... Args>
	static Future createImmediateEmplace(Args&&... args) {
		return promise_type::createImmediateEmplace(std::forward<Args>(args)...);
	}

	/// Creates a future that is in the 'exception' state.
	static Future createFailed(const Exception& ex) {
		return promise_type::createFailed(ex);
	}

	/// Creates a future that is in the 'exception' state.
	static Future createFailed(std::exception_ptr ex_ptr) {
		return promise_type::createFailed(std::move(ex_ptr));
	}

	/// Cancels the shared state associated with this Future.
	/// The Future is no longer valid after calling this function.
	void cancelRequest() {
		reset();
	}

	/// Returns the results computed by the associated Promise.
	/// This function may only be called after the Promise was fulfilled (and not canceled).
	tuple_type results() {
		OVITO_ASSERT_MSG(isValid(), "Future::results()", "Future must be valid.");
		OVITO_ASSERT_MSG(isFinished(), "Future::results()", "Future must be in fulfilled state.");
		OVITO_ASSERT_MSG(!isCanceled(), "Future::results()", "Future must not be canceled.");
	    task()->throwPossibleException();
		tuple_type result = task()->template takeResults<tuple_type>();
		reset();
		return result;
	}

	/// Returns the results computed by the associated Promise.
	/// This function may only be called after the Promise was fulfilled (and not canceled).
	auto result() {
		return std::get<0>(results());
	}

	/// Returns a new future that, upon the fulfillment of this future, will be fulfilled by running the given continuation function.
	/// The provided continuation function must accept the results of this future as an input parameter.
	template<typename FC, class Executor>
	typename Ovito::detail::resulting_future_type<FC,std::add_rvalue_reference_t<tuple_type>>::type then(Executor&& executor, bool defer, FC&& cont);

	/// Overload of the function above allowing eager execution of the continuation function.
	template<typename FC, class Executor>
	typename Ovito::detail::resulting_future_type<FC,std::add_rvalue_reference_t<tuple_type>>::type then(Executor&& executor, FC&& cont) { return then(std::forward<Executor>(executor), false, std::forward<FC>(cont)); }

	/// Overload of the function above using the default inline executor.
	template<typename FC>
	typename Ovito::detail::resulting_future_type<FC,std::add_rvalue_reference_t<tuple_type>>::type then(FC&& cont) { return then(Ovito::detail::InlineExecutor(), std::forward<FC>(cont)); }

	/// Returns a new future that, upon the fulfillment of this future, will be fulfilled by running the specified continuation function.
	/// The provided continuation function must accept this future as an input parameter.
	template<typename FC, class Executor>
	typename Ovito::detail::resulting_future_type<FC,std::tuple<this_type>>::type then_future(Executor&& executor, bool defer, FC&& cont);

	/// Overload of the function above allowing eager execution of the continuation function.
	template<typename FC, class Executor>
	typename Ovito::detail::resulting_future_type<FC,std::tuple<this_type>>::type then_future(Executor&& executor, FC&& cont) { return then_future(std::forward<Executor>(executor), false, std::forward<FC>(cont)); }

#ifndef Q_CC_GNU
protected:
#else
// This is a workaround for what is likely a bug in the GCC compiler, which doesn't respect the
// template friend class declarations made below. The AsynchronousTask<> template specialization
// doesn't seem to get access to the Future constructor.
public:
#endif

	/// Constructor that constructs a Future that is associated with the given shared state.
	explicit Future(TaskPtr p) noexcept : FutureBase(std::move(p)) {}

	/// Constructor that constructs a Future from an existing task dependency.
	explicit Future(TaskDependency&& p) noexcept : FutureBase(std::move(p)) {}

	/// Move constructor taking the promise state pointer from a r-value Promise.
	Future(promise_type&& promise) : FutureBase(std::move(promise._task)) {}

	template<typename... R2> friend class Future;
	template<typename... R2> friend class Promise;
	template<typename... R2> friend class AsynchronousTask;
	template<typename... R2> friend class SharedFuture;
};

/// Returns a new future that, upon the fulfillment of this future, will be fulfilled by running the given continuation function.
/// The provided continuation function must accept the results of this future as an input parameter.
template<typename... R>
template<typename FC, class Executor>
typename Ovito::detail::resulting_future_type<FC,std::add_rvalue_reference_t<typename Future<R...>::tuple_type>>::type Future<R...>::then(Executor&& executor, bool defer, FC&& cont)
{
	// Infer the exact future/promise/task types to create.
	using result_future_type = typename Ovito::detail::resulting_future_type<FC,tuple_type>::type;
	using result_promise_type = typename result_future_type::promise_type;
	using continuation_task_type = ContinuationTask<result_promise_type>;

	// This future must be valid for then() to work.
	OVITO_ASSERT_MSG(isValid(), "Future::then()", "Future must be valid.");

	// Create an unfulfilled task and promise for the result of the continuation.
	// The dependency on the existing task is moved from this future into the continuation task.
	Task* continuedTask = task().get();
	result_promise_type promise(std::make_shared<continuation_task_type>(std::move(_task), executor.taskManager()));
	OVITO_ASSERT(!isValid());

	// Create the future, which will be returned to the caller.
	result_future_type future = promise.future();

	// Register continuation function with the current task.
	continuedTask->finally(std::forward<Executor>(executor), defer, [cont = std::forward<FC>(cont), promise = std::move(promise)]() mutable {

		// Get the task that is about to continue.
		continuation_task_type* continuationTask = static_cast<continuation_task_type*>(promise.task().get());
		OVITO_ASSERT(continuationTask != nullptr);

		// Get the task that has finished.
		TaskDependency finishedTask = continuationTask->takeContinuedTask();

		// Don't need to run continuation function when the promise has been canceled in the meantime.
		// Also don't run continuation function if the original task was canceled.
		if(promise.isCanceled() || !finishedTask || finishedTask->isCanceled())
			return;

		// Don't execute continuation function in case of an exception state in the original task.
		// In such a case, forward the exception state to the continuation promise.
		if(finishedTask->_exceptionStore) {
			continuationTask->setStarted();
			continuationTask->setException(std::move(finishedTask->_exceptionStore));
			continuationTask->setFinished();
			return;
		}

		// Now it's time to execute the continuation function.
		// Store its return value as result of the continuation promise. 
		OVITO_ASSERT(finishedTask->isFinished());
		continuationTask->fulfillWith(std::move(promise), std::forward<FC>(cont), finishedTask->template takeResults<tuple_type>());
	});

	return future;
}

/// Returns a new future that, upon the fulfillment of this future, will be fulfilled by running the specified continuation function.
/// The provided continuation function must accept this future as an input parameter.
template<typename... R>
template<typename FC, class Executor>
typename Ovito::detail::resulting_future_type<FC,std::tuple<Future<R...>>>::type Future<R...>::then_future(Executor&& executor, bool defer, FC&& cont)
{
	// Infer the exact future/promise/task types to create.
	using result_future_type = typename Ovito::detail::resulting_future_type<FC,tuple_type>::type;
	using result_promise_type = typename result_future_type::promise_type;
	using continuation_task_type = ContinuationTask<result_promise_type>;

	// This future must be valid for then_future() to work.
	OVITO_ASSERT_MSG(isValid(), "Future::then_future()", "Future must be valid.");

	// Create an unfulfilled task and promise for the result of the continuation.
	// The dependency on the existing task is moved from this future into the continuation task.
	Task* continuedTask = task().get();
	result_promise_type promise(std::make_shared<continuation_task_type>(std::move(_task), executor.taskManager()));
	OVITO_ASSERT(!isValid());

	// Create the future, which will be returned to the caller.
	result_future_type future = promise.future();

	// Register continuation function with the current task.
	continuedTask->finally(std::forward<Executor>(executor), defer, [cont = std::forward<FC>(cont), promise = std::move(promise)]() mutable {

		// Get the task that is about to continue.
		continuation_task_type* continuationTask = static_cast<continuation_task_type*>(promise.task().get());
		OVITO_ASSERT(continuationTask != nullptr);

		// Get the task that has finished.
		TaskDependency finishedTask = continuationTask->takeContinuedTask();

		// Don't need to run continuation function when the promise has been canceled in the meantime.
		// Also don't run continuation function if the original task was canceled.
		if(promise.isCanceled() || !finishedTask || finishedTask->isCanceled())
			return;
		
		// Now it's time to execute the continuation function.
		// Store its return value as result of the continuation promise. 
		OVITO_ASSERT(finishedTask->isFinished());
		continuationTask->fulfillWith(std::move(promise), std::forward<FC>(cont), std::forward_as_tuple(Future<R...>(std::move(finishedTask))));
	});

	return future;
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
