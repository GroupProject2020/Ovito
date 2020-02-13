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
#include "Future.h"
#include "FutureDetail.h"
#include "ContinuationTask.h"

namespace Ovito {

/******************************************************************************
* A future that provides access to the value computed by a Promise.
******************************************************************************/
template<typename... R>
class SharedFuture : public FutureBase
{
public:

	using this_type = SharedFuture<R...>;
	using tuple_type = typename Future<R...>::tuple_type;
	using promise_type = typename Future<R...>::promise_type;

	/// Default constructor that constructs an invalid SharedFuture that is not associated with any shared state.
#ifndef Q_CC_MSVC
	SharedFuture() noexcept = default;
#else
	SharedFuture() noexcept {}
#endif

	/// Move constructor.
	SharedFuture(SharedFuture&& other) noexcept = default;

	/// Copy constructor.
	SharedFuture(const SharedFuture& other) noexcept = default;

	/// Constructor that constructs a shared future from a normal future.
	SharedFuture(Future<R...>&& other) noexcept : FutureBase(std::move(other)) {}

	/// A future may directly be initialized from r-values.
	template<typename... R2, size_t C = sizeof...(R),
		typename = std::enable_if_t<C != 0
			&& !std::is_same<std::tuple<std::decay_t<R2>...>, std::tuple<SharedFuture<R...>>>::value
			&& !std::is_same<std::tuple<std::decay_t<R2>...>, std::tuple<Future<R...>>>::value
			&& !std::is_same<std::tuple<std::decay_t<R2>...>, std::tuple<TaskPtr>>::value>>
	SharedFuture(R2&&... val) noexcept : FutureBase(std::move(promise_type::createImmediate(std::forward<R2>(val)...)._task)) {}

	/// Cancels the shared state associated with this Future.
	/// The SharedFuture is no longer valid after calling this function.
	void cancelRequest() {
		reset();
	}

	/// Move assignment operator.
	SharedFuture& operator=(SharedFuture&& other) noexcept = default;

	/// Copy assignment operator.
	SharedFuture& operator=(const SharedFuture& other) noexcept = default;

	/// Returns the results computed by the associated Promise.
	/// This function may only be called after the Promise was fulfilled (and not canceled).
	const std::tuple<R...>& results() const {
		OVITO_ASSERT_MSG(isValid(), "SharedFuture::results()", "Future must be valid.");
		OVITO_ASSERT_MSG(isFinished(), "SharedFuture::results()", "Future must be in fulfilled state.");
		OVITO_ASSERT_MSG(!isCanceled(), "SharedFuture::results()", "Future must not be canceled.");
		OVITO_ASSERT_MSG(std::tuple_size<tuple_type>::value != 0, "SharedFuture::results()", "Future must not be of type <void>.");
	    task()->throwPossibleException();
		return task()->template getResults<tuple_type>();
	}

	/// Returns the results computed by the associated Promise.
	/// This function may only be called after the Promise was fulfilled (and not canceled).
	decltype(auto) result() const {
		return std::get<0>(results());
	}

	/// Returns a new future that, upon the fulfillment of this future, will be fulfilled by running the given continuation function.
	/// The provided continuation function must accept the results of this future as an input parameter.
	template<typename FC, class Executor>
	typename Ovito::detail::resulting_future_type<FC,std::add_lvalue_reference_t<const tuple_type>>::type 
	then(Executor&& executor, bool defer, FC&& cont) noexcept;

	/// Overload of the function above allowing eager execution of the continuation function.
	template<typename FC, class Executor>
	typename Ovito::detail::resulting_future_type<FC,std::add_lvalue_reference_t<const tuple_type>>::type 
	then(Executor&& executor, FC&& cont) noexcept { return then(std::forward<Executor>(executor), false, std::forward<FC>(cont)); }

	/// Overload of the function above using the default inline executor.
	template<typename FC>
	typename Ovito::detail::resulting_future_type<FC,std::add_lvalue_reference_t<const tuple_type>>::type 
	then(FC&& cont) noexcept { return then(Ovito::detail::InlineExecutor(), std::forward<FC>(cont)); }

	/// Runs the given continuation function upon fulfillment of this future. The function creates a strong 
	/// reference to this future in order to keep it going even if this future object is destroyed.
	/// The provided continuation function must accept the results of this future as an input parameter.
	template<typename FC, class Executor>
	void force_then(Executor&& executor, bool defer, FC&& cont) noexcept;

	/// Overload of the function above allowing eager execution of the continuation function.
	template<typename FC, class Executor>
	void force_then(Executor&& executor, FC&& cont) noexcept { force_then(std::forward<Executor>(executor), false, std::forward<FC>(cont)); }

	/// Returns a new future that, upon the fulfillment of this future, will be fulfilled by running the specified continuation function.
	/// The provided continuation function must accept this shared future as an input parameter.
	template<typename FC, class Executor>
	typename Ovito::detail::resulting_future_type<FC,std::tuple<this_type>>::type 
	then_future(Executor&& executor, bool defer, FC&& cont) noexcept;

	/// Overload of the function above allowing eager execution of the continuation function.
	template<typename FC, class Executor>
	typename Ovito::detail::resulting_future_type<FC,std::tuple<this_type>>::type 
	then_future(Executor&& executor, FC&& cont) noexcept { return then_future(std::forward<Executor>(executor), false, std::forward<FC>(cont)); }

	/// Returns a new future that, upon the fulfillment of this future, will be fulfilled by running the given continuation function.
	/// The provided continuation function must accept a Task object and the results of this future as two input parameters.
	template<typename FC, class Executor>
	typename Ovito::detail::resulting_future_type<FC,std::tuple<Task&, std::add_lvalue_reference_t<const R>...>>::type 
	then_task(Executor&& executor, bool defer, FC&& cont) noexcept;

	/// Overload of the function above allowing eager execution of the continuation function.
	template<typename FC, class Executor>
	typename Ovito::detail::resulting_future_type<FC,std::tuple<Task&, std::add_lvalue_reference_t<const R>...>>::type 
	then_task(Executor&& executor, FC&& cont) noexcept { return then_task(std::forward<Executor>(executor), false, std::forward<FC>(cont)); }

	/// Overload of the function above using the default inline executor.
	template<typename FC>
	typename Ovito::detail::resulting_future_type<FC,std::tuple<Task&, std::add_lvalue_reference_t<const R>...>>::type 
	then_task(FC&& cont) noexcept { return then_task(Ovito::detail::InlineExecutor(), std::forward<FC>(cont)); }

protected:

	/// Constructor that constructs a SharedFuture that is associated with the given shared state.
	explicit SharedFuture(TaskPtr p) noexcept : FutureBase(std::move(p)) {}

	/// Constructor that constructs a Future from an existing task dependency.
	explicit SharedFuture(TaskDependency&& p) noexcept : FutureBase(std::move(p)) {}

	template<typename... R2> friend class Promise;
	template<typename... R2> friend class WeakSharedFuture;
};

/// Returns a new future that, upon the fulfillment of this future, will be fulfilled by running the given continuation function.
/// The provided continuation function must accept the results of this future as an input parameter.
template<typename... R>
template<typename FC, class Executor>
typename Ovito::detail::resulting_future_type<FC,std::add_lvalue_reference_t<const typename SharedFuture<R...>::tuple_type>>::type SharedFuture<R...>::then(Executor&& executor, bool defer, FC&& cont) noexcept
{
	// Infer the exact future/promise/task types to create.
	using result_future_type = typename Ovito::detail::resulting_future_type<FC,tuple_type>::type;
	using result_promise_type = typename result_future_type::promise_type;
	using continuation_task_type = ContinuationTask<result_promise_type>;

	// This future must be valid for then() to work.
	OVITO_ASSERT_MSG(isValid(), "SharedFuture::then()", "Future must be valid.");

	// Create an unfulfilled task and promise for the result of the continuation.
	result_promise_type promise(std::make_shared<continuation_task_type>(task(), executor.taskManager()));

	// Create the future, which will be returned to the caller.
	result_future_type future = promise.future();

	// Register continuation function with the current task.
	task()->finally(std::forward<Executor>(executor), defer, [cont = std::forward<FC>(cont), promise = std::move(promise)]() mutable noexcept {

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
			continuationTask->setException(std::exception_ptr(finishedTask->_exceptionStore));
			continuationTask->setFinished();
			return;
		}

		// Now it's time to execute the continuation function.
		// Store its return value as result of the continuation promise. 
		OVITO_ASSERT(finishedTask->isFinished());
		continuationTask->fulfillWith(std::move(promise), std::forward<FC>(cont), finishedTask->template getResults<tuple_type>());
	});

	return future;
}

/// Runs the given continuation function upon fulfillment of this future. The function creates a strong 
/// reference to this future in order to keep it going even if this future object is destroyed.
/// The provided continuation function must accept the results of this future as an input parameter.
template<typename... R>
template<typename FC, class Executor>
void SharedFuture<R...>::force_then(Executor&& executor, bool defer, FC&& cont) noexcept
{
	// This future must be valid for force_then() to work.
	OVITO_ASSERT_MSG(isValid(), "SharedFuture::force_then()", "Future must be valid.");

	// Register continuation function with the current task.
	task()->finally(std::forward<Executor>(executor), defer, [cont = std::forward<FC>(cont), task = TaskDependency(task())]() mutable noexcept {
		OVITO_ASSERT(task->isFinished());

		// Don't need to run continuation function when the task has been canceled in the meantime.
		if(task->isCanceled())
			return;

		// Don't run continuation function either in case of an exception state.
		if(task->_exceptionStore)
			return;

		// Now it's time to execute the continuation function.
		Ovito::detail::apply(std::forward<FC>(cont), task->template getResults<tuple_type>());
	});
}

/// Returns a new future that, upon the fulfillment of this future, will be fulfilled by running the specified continuation function.
/// The provided continuation function must accept this shared future as an input parameter.
template<typename... R>
template<typename FC, class Executor>
typename Ovito::detail::resulting_future_type<FC,std::tuple<SharedFuture<R...>>>::type SharedFuture<R...>::then_future(Executor&& executor, bool defer, FC&& cont) noexcept
{
	// Infer the exact future/promise/task types to create.
	using result_future_type = typename Ovito::detail::resulting_future_type<FC,tuple_type>::type;
	using result_promise_type = typename result_future_type::promise_type;
	using continuation_task_type = ContinuationTask<result_promise_type>;

	// This future must be valid for then() to work.
	OVITO_ASSERT_MSG(isValid(), "SharedFuture::then()", "Future must be valid.");

	// Create an unfulfilled task and promise for the result of the continuation.
	result_promise_type promise(std::make_shared<continuation_task_type>(task(), executor.taskManager()));

	// Create the future, which will be returned to the caller.
	result_future_type future = promise.future();

	// Register continuation function with the current task.
	task()->finally(std::forward<Executor>(executor), defer, [cont = std::forward<FC>(cont), promise = std::move(promise)]() mutable noexcept {

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
		continuationTask->fulfillWith(std::move(promise), std::forward<FC>(cont), std::forward_as_tuple(SharedFuture<R...>(std::move(finishedTask))));
	});

	return future;
}

/// Returns a new future that, upon the fulfillment of this future, will be fulfilled by running the given continuation function.
/// The provided continuation function must accept a Task object and the results of this future as two input parameters.
template<typename... R>
template<typename FC, class Executor>
typename Ovito::detail::resulting_future_type<FC,std::tuple<Task&, std::add_lvalue_reference_t<const R>...>>::type SharedFuture<R...>::then_task(Executor&& executor, bool defer, FC&& cont) noexcept
{
	// Infer the exact future/promise/task types to create.
	using result_future_type = typename Ovito::detail::resulting_future_type<FC,tuple_type>::type;
	using result_promise_type = typename result_future_type::promise_type;
	using continuation_task_type = ContinuationTask<result_promise_type>;

	// This future must be valid for then_task() to work.
	OVITO_ASSERT_MSG(isValid(), "SharedFuture::then_task()", "Future must be valid.");

	// Create an unfulfilled task and promise for the result of the continuation.
	result_promise_type promise(std::make_shared<continuation_task_type>(task(), executor.taskManager()));

	// Create the future, which will be returned to the caller.
	result_future_type future = promise.future();

	// Register continuation function with the current task.
	task()->finally(std::forward<Executor>(executor), defer, [cont = std::forward<FC>(cont), promise = std::move(promise)]() mutable noexcept {

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
			continuationTask->setException(std::exception_ptr(finishedTask->_exceptionStore));
			continuationTask->setFinished();
			return;
		}

		// Now it's time to execute the continuation function.
		// Store its return value as result of the continuation promise. 
		OVITO_ASSERT(finishedTask->isFinished());
		continuationTask->fulfillWith(std::move(promise), std::forward<FC>(cont), std::tuple_cat(std::tuple<Task&>(*continuationTask), finishedTask->template getResults<tuple_type>()));
	});
}

/******************************************************************************
* A weak reference to a SharedFuture
******************************************************************************/
template<typename... R>
class WeakSharedFuture : private std::weak_ptr<Task>
{
public:

#ifndef Q_CC_MSVC
	constexpr WeakSharedFuture() noexcept = default;
#else
	constexpr WeakSharedFuture() noexcept : std::weak_ptr<Task>() {}
#endif

	WeakSharedFuture(const SharedFuture<R...>& future) noexcept : std::weak_ptr<Task>(future.task()) {}

	WeakSharedFuture& operator=(const Future<R...>& f) noexcept {
		std::weak_ptr<Task>::operator=(f.task());
		return *this;
	}

	WeakSharedFuture& operator=(const SharedFuture<R...>& f) noexcept {
		std::weak_ptr<Task>::operator=(f.task());
		return *this;
	}

	void reset() noexcept {
		std::weak_ptr<Task>::reset();
	}

	SharedFuture<R...> lock() const noexcept {
		return SharedFuture<R...>(std::weak_ptr<Task>::lock());
	}

	bool expired() const noexcept {
		return std::weak_ptr<Task>::expired();
	}
};

}	// End of namespace
