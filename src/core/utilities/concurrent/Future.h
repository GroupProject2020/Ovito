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

#pragma once


#include <core/Core.h>
#include "Promise.h"
#include "FutureDetail.h"
#include "TrackingTask.h"
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

	/// Move constructor.
	FutureBase(FutureBase&& other) noexcept = default;

	/// Copy constructor.
	FutureBase(const FutureBase& other) noexcept = default;

	/// A future is moveable.
	FutureBase& operator=(FutureBase&& other) noexcept = default;

	/// Copy assignment.
	FutureBase& operator=(const FutureBase& other) noexcept = default;

	/// Runs the given function once this future has reached the 'finished' state.
	/// The function is run even if the future was canceled or set to an error state.
	template<typename FC, class Executor>
	void finally(Executor&& executor, FC&& cont);

	/// Version of the function above, which uses the default inline executor.
	template<typename FC>
	void finally(FC&& cont) { finally(detail::InlineExecutor(), std::forward<FC>(cont)); }

protected:

	/// Default constructor, which creates a Future without a shared state.
#ifndef Q_CC_MSVC
	FutureBase() noexcept = default;
#else
	FutureBase() noexcept {}
#endif

	/// Constructor that creates a Future associated with a share state.
	explicit FutureBase(TaskPtr&& p) noexcept : _task(std::move(p)) {}

	/// The shared state associated with this Future.
	TaskDependency _task;

	friend class TrackingTask;
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

	/// A future is not copy constructible.
	Future(const Future& other) = delete;

	/// A future is move constructible.
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

	/// Returns a new future that, upon the fulfillment of this future, will
    /// be fulfilled by the specified continuation function.
	template<typename FC, class Executor>
	typename detail::resulting_future_type<FC,std::add_rvalue_reference_t<tuple_type>>::type then(Executor&& executor, FC&& cont);

	/// Version of the function above, which uses the default inline executor.
	template<typename FC>
	typename detail::resulting_future_type<FC,std::add_rvalue_reference_t<tuple_type>>::type then(FC&& cont) {
		return then(detail::InlineExecutor(), std::forward<FC>(cont));
	}

	/// Returns a new future that, upon the fulfillment of this future, will
    /// be fulfilled by the specified continuation function.
	template<typename FC, class Executor>
	typename detail::resulting_future_type<FC,std::tuple<this_type>>::type then_future(Executor&& executor, FC&& cont);

	/// Runs the given function once this future has reached the 'finished' state.
	/// The function is always run, even if the future was canceled or set to an error state.
	template<typename FC, class Executor>
	void finally_future(Executor&& executor, FC&& cont);

	/// Version of the function above, which uses the default inline executor.
	template<typename FC>
	void finally_future(FC&& cont) { finally_future(detail::InlineExecutor(), std::forward<FC>(cont)); }

#ifndef Q_CC_GNU
protected:
#else
// This is a workaround for what is likely a bug in the GCC compiler, which doesn't respect the
// template fiend class declarations made below. The AsynchronousTask<> template specialization
// doesn't seem to get access to the Future constructor.
public:
#endif

	/// Constructor that constructs a Future that is associated with the given shared state.
	explicit Future(TaskPtr p) noexcept : FutureBase(std::move(p)) {}

	/// Move constructor taking the promise state pointer from a r-value Promise.
	Future(promise_type&& promise) : FutureBase(std::move(promise._task)) {}

	template<typename... R2> friend class Future;
	template<typename... R2> friend class Promise;
	template<typename... R2> friend class AsynchronousTask;
	template<typename... R2> friend class SharedFuture;
};

/// Returns a new future that, upon the fulfillment of this future, will
/// be fulfilled by the specified continuation function.
template<typename... R>
template<typename FC, class Executor>
typename detail::resulting_future_type<FC,std::add_rvalue_reference_t<typename Future<R...>::tuple_type>>::type Future<R...>::then(Executor&& executor, FC&& cont)
{
	// The future type returned by then():
	using ResultFutureType = typename detail::resulting_future_type<FC,tuple_type>::type;
	using ContinuationStateType = typename detail::continuation_state_type<FC,tuple_type>::type;

	// This future must be valid for then() to work.
	OVITO_ASSERT_MSG(isValid(), "Future::then()", "Future must be valid.");

	// Create an unfulfilled promise state for the result of the continuation.
	auto trackingState = std::make_shared<ContinuationStateType>(std::move(_task));

	trackingState->creatorState()->addContinuation(
		executor.createWork([cont = std::forward<FC>(cont), trackingState](bool workCanceled) mutable {

		// Don't need to run continuation function when our promise has been canceled in the meantime.
		if(trackingState->isCanceled()) {
			trackingState->setStarted();
			trackingState->setFinished();
			return;
		}
		if(workCanceled || trackingState->creatorState()->isCanceled()) {
			trackingState->setStarted();
			trackingState->cancel();
			trackingState->setFinished();
			return;
		}

		// Also skip continuation function in case of an exception state.
		if(trackingState->creatorState()->_exceptionStore) {
			trackingState->setStarted();
			trackingState->setException(std::move(trackingState->creatorState()->_exceptionStore));
			trackingState->setFinished();
			return;
		}

		trackingState->fulfillWith(std::forward<FC>(cont), trackingState->creatorState()->template takeResults<tuple_type>());
	}));

	// Calling then() on a Future invalidates it.
	OVITO_ASSERT(!isValid());

	return ResultFutureType(TaskPtr(std::move(trackingState)));
}

/// Returns a new future that, upon the fulfillment of this future, will
/// be fulfilled by the specified continuation function.
template<typename... R>
template<typename FC, class Executor>
typename detail::resulting_future_type<FC,std::tuple<Future<R...>>>::type Future<R...>::then_future(Executor&& executor, FC&& cont)
{
	// The future type returned by then_future():
	using ResultFutureType = typename detail::resulting_future_type<FC,std::tuple<this_type>>::type;
	using ContinuationStateType = typename detail::continuation_state_type<FC,std::tuple<this_type>>::type;

	// This future must be valid for then() to work.
	OVITO_ASSERT_MSG(isValid(), "Future::then_future()", "Future must be valid.");

	// Create an unfulfilled promise state for the result of the continuation.
	auto trackingState = std::make_shared<ContinuationStateType>(std::move(_task));

	trackingState->creatorState()->addContinuation(
		executor.createWork([cont = std::forward<FC>(cont), trackingState](bool workCanceled) mutable {

		// Don't need to run continuation function when our promise has been canceled in the meantime.
		if(trackingState->isCanceled()) {
			trackingState->setStarted();
			trackingState->setFinished();
			return;
		}
		if(workCanceled || trackingState->creatorState()->isCanceled()) {
			trackingState->setStarted();
			trackingState->cancel();
			trackingState->setFinished();
			return;
		}

		// Create a Future for the fulfilled state.
		Future<R...> future(trackingState->creatorState());

		trackingState->fulfillWith(std::forward<FC>(cont), std::forward_as_tuple(std::move(future)));
	}));

	// Calling then_future() on a Future invalidates it.
	OVITO_ASSERT(!isValid());

	return ResultFutureType(TaskPtr(std::move(trackingState)));
}

/// Runs the given function once this future has reached the 'finished' state.
/// The function is run even if the future was canceled or set to an error state.
template<typename... R>
template<typename FC, class Executor>
void Future<R...>::finally_future(Executor&& executor, FC&& cont)
{
	// This future must be valid for finally_future() to work.
	OVITO_ASSERT_MSG(isValid(), "Future::finally_future()", "Future must be valid.");

	task()->addContinuation(
		executor.createWork([cont = std::forward<FC>(cont), future = std::move(*this)](bool workCanceled) mutable {
			if(!workCanceled) {
				std::move(cont)(std::move(future));
			}
	}));

	// Calling finally_future() on a Future invalidates it.
	OVITO_ASSERT(!isValid());
}

/// Runs the given function once this future has reached the 'finished' state.
/// The function is run even if the future was canceled or set to an error state.
template<typename FC, class Executor>
void FutureBase::finally(Executor&& executor, FC&& cont)
{
	// This future must be valid for finally() to work.
	OVITO_ASSERT_MSG(isValid(), "Future::finally()", "Future must be valid.");

	task()->addContinuation(
		executor.createWork([cont = std::forward<FC>(cont)](bool workCanceled) mutable {
			if(!workCanceled)
				std::move(cont)();
	}));
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
