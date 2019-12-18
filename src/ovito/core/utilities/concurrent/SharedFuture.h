////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2017 Alexander Stukowski
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

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(Concurrency)

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

	/// Returns a new future that, upon the fulfillment of this future, will
    /// be fulfilled by the specified continuation function.
	template<typename FC, class Executor>
	typename detail::resulting_future_type<FC,std::add_lvalue_reference_t<const tuple_type>>::type then(Executor&& executor, FC&& cont);

	/// Version of the function above, which uses the default inline executor.
	template<typename FC>
	typename detail::resulting_future_type<FC,std::add_lvalue_reference_t<const tuple_type>>::type then(FC&& cont) {
		return then(detail::InlineExecutor(), std::forward<FC>(cont));
	}

	/// Runs the given function once this future has reached the 'finished' state.
	/// The function is awlays run, even if the future was canceled or set to an error state.
	template<typename FC, class Executor>
	void finally_future(Executor&& executor, FC&& cont);

	/// Version of the function above, which uses the default inline executor.
	template<typename FC>
	void finally_future(FC&& cont) { finally_future(detail::InlineExecutor(), std::forward<FC>(cont)); }

protected:

	/// Constructor that constructs a SharedFuture that is associated with the given shared state.
	explicit SharedFuture(TaskPtr p) noexcept : FutureBase(std::move(p)) {}

	template<typename... R2> friend class WeakSharedFuture;
};

/// Returns a new future that, upon the fulfillment of this future, will
/// be fulfilled by the specified continuation function.
template<typename... R>
template<typename FC, class Executor>
typename detail::resulting_future_type<FC,std::add_lvalue_reference_t<const typename SharedFuture<R...>::tuple_type>>::type SharedFuture<R...>::then(Executor&& executor, FC&& cont)
{
	// The future type returned by then():
	using ResultFutureType = typename detail::resulting_future_type<FC, std::add_lvalue_reference_t<const tuple_type>>::type;
	using ContinuationStateType = typename detail::continuation_state_type<FC,std::add_lvalue_reference_t<const tuple_type>>::type;

	// This future must be valid for then() to work.
	OVITO_ASSERT_MSG(isValid(), "SharedFuture::then()", "Future must be valid.");

	// Create an unfulfilled promise state for the result of the continuation.
	auto trackingState = std::make_shared<ContinuationStateType>(task());

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
			trackingState->setException(std::exception_ptr(trackingState->creatorState()->_exceptionStore));
			trackingState->setFinished();
			return;
		}

		trackingState->fulfillWith(std::forward<FC>(cont), trackingState->creatorState()->template getResults<tuple_type>());
	}));

	// Calling then() on a SharedFuture doesn't invalidate it.
	OVITO_ASSERT(isValid());

	return ResultFutureType(TaskPtr(std::move(trackingState)));
}

/// Runs the given function once this future has reached the 'finished' state.
/// The function is run even if the future was canceled or set to an error state.
template<typename... R>
template<typename FC, class Executor>
void SharedFuture<R...>::finally_future(Executor&& executor, FC&& cont)
{
	// This future must be valid for finally_future() to work.
	OVITO_ASSERT_MSG(isValid(), "SharedFuture::finally_future()", "Future must be valid.");

	task()->addContinuation(
		executor.createWork([cont = std::forward<FC>(cont), future = *this](bool workCanceled) mutable {
			if(!workCanceled) {
				std::move(cont)(std::move(future));
			}
	}));
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

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace


