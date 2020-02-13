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
#include "FutureDetail.h"
#include "ThreadSafeTask.h"

namespace Ovito {

/******************************************************************************
* This shared state is returned by the Future::then() method.
******************************************************************************/
template<typename promise_type>
class ContinuationTask : public TaskWithResultStorage<ThreadSafeTask, typename promise_type::tuple_type>
{
public:
	using tuple_type = typename promise_type::tuple_type;
	using super_class = TaskWithResultStorage<ThreadSafeTask, tuple_type>;

	/// Constructor.
	ContinuationTask(TaskDependency continuedTask, TaskManager* taskManager) :
		super_class(typename super_class::no_result_init_t()),
		_continuedTask(std::move(continuedTask)) 
	{
		this->setTaskManager(taskManager);
	}

	/// Moves the dependencies on the continued task out of this continuation task.
	TaskDependency takeContinuedTask() { 
		return std::move(_continuedTask); 
	}

	/// Cancels this task.
	virtual void cancel() noexcept override {
		super_class::cancel();

		// Clear the dependency on the parent task.
		_continuedTask.reset();
	}

#ifdef OVITO_DEBUG
    /// Switches the task into the 'finished' state.
    virtual void setFinished() override {
		super_class::setFinished();
		OVITO_ASSERT(!_continuedTask);
	}
#endif

	/// This overload is used for continuation functions returning void.
	template<typename FC, typename Args>
	auto fulfillWith(promise_type promise, FC&& cont, Args&& params) noexcept
		-> std::enable_if_t<Ovito::detail::is_void_continuation_func<FC,Args>::value>
	{
		OVITO_ASSERT(!_continuedTask);
		try {
			// Call the continuation function with the results of the finished task.
			this->setStarted();
			Ovito::detail::apply(std::forward<FC>(cont), std::forward<Args>(params));
		}
		catch(...) {
			this->captureException();
		}
		this->setFinished();
	}

	/// This overload is used for continuation functions returning a result value.
	template<typename FC, typename Args>
	auto fulfillWith(promise_type promise, FC&& cont, Args&& params) noexcept
		-> std::enable_if_t<!Ovito::detail::is_void_continuation_func<FC,Args>::value
			&& !Ovito::detail::is_future<typename Ovito::detail::continuation_func_return_type<FC,Args>::type>::value>
	{
		OVITO_ASSERT(!_continuedTask);
		try {
			// Call the continuation function with the results of the finished task.
			this->setStarted();
			setResultsDirect(Ovito::detail::apply(std::forward<FC>(cont), std::forward<Args>(params)));
		}
		catch(...) {
			this->captureException();
		}
		this->setFinished();
	}

	/// This overload is used for continuation functions returning a future.
	template<typename FC, typename Args>
	auto fulfillWith(promise_type promise, FC&& cont, Args&& params) noexcept
		-> std::enable_if_t<!Ovito::detail::is_void_continuation_func<FC,Args>::value
			&& Ovito::detail::is_future<typename Ovito::detail::continuation_func_return_type<FC,Args>::type>::value>
	{
		OVITO_ASSERT(!_continuedTask);
		try {
			// Call the continuation function with the results of the finished task.
			// The continuation function returns another future, which will provide the final results.
			this->setStarted();
			auto future = Ovito::detail::apply(std::forward<FC>(cont), std::forward<Args>(params));
			OVITO_ASSERT(future.isValid());
			// Make this task dependent on the future's task.
			_continuedTask = future.takeTaskDependency();
			// Get results from the future's task once it completes and use it as the results of this continuation task.
			_continuedTask->finally(Ovito::detail::InlineExecutor(), false, [this,promise = std::move(promise)]() {
				if(TaskDependency finishedTask = this->takeContinuedTask()) {
					if(!finishedTask->isCanceled()) {
						if(finishedTask->_exceptionStore)
							this->setException(std::exception_ptr(finishedTask->_exceptionStore));
						else
							this->setResultsDirect(finishedTask->template getResults<tuple_type>());
						this->setFinished();
					}
				}
				// Note: Promise destructor will put this task into the finished state if it isn't already.
			});
		}
		catch(...) {
			this->captureException();
			this->setFinished();
		}
	}	

private:

	/// Assigns a result to this shared state.
	template<typename... R>
	auto setResultsDirect(std::tuple<R...>&& results) {
		using source_tuple_type = std::tuple<R...>;
		static_assert(std::tuple_size<tuple_type>::value != 0, "Must not be an empty tuple");
		static_assert(std::is_same<tuple_type, std::decay_t<source_tuple_type>>::value, "Must assign a compatible tuple");
		this->template setResults<tuple_type>(std::move(results));
	}

	/// Assigns a result to this shared state.
	template<typename... R>
	auto setResultsDirect(const std::tuple<R...>& results) {
		using source_tuple_type = std::tuple<R...>;
		static_assert(std::tuple_size<tuple_type>::value != 0, "Must not be an empty tuple");
		static_assert(std::is_same<tuple_type, std::decay_t<source_tuple_type>>::value, "Must assign a compatible tuple");
		this->template setResults<tuple_type>(results);
	}	

	/// Assigns a result to this shared state.
	template<typename value_type>
	void setResultsDirect(value_type&& result) {
		static_assert(std::tuple_size<tuple_type>::value == 1, "Must be a tuple of size 1");
		this->template setResults<tuple_type>(std::forward_as_tuple(std::forward<value_type>(result)));
	}

	/// The task that spawned this task as a continuation.
	TaskDependency _continuedTask;
};

}	// End of namespace
