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
#include "Future.h"
#include "TaskWatcher.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(Concurrency)

/**
 * An object representing an asynchronous program operation that is executed in the main thread.
 */
class OVITO_CORE_EXPORT AsyncOperation : public Promise<>
{
public:

	/// Default constructor creating an invalid operation object.
	AsyncOperation() = default;

	/// Constructor creating a new operation, registering it with the given task manager,
	/// and putting into the 'started' state.
	explicit AsyncOperation(TaskManager& taskManager);

	/// Creates a special async operation that can be used just for signaling the completion of 
	/// an operation and which is not registered with a task manager.
	static AsyncOperation createSignalOperation(bool startedState);

	/// Move constructor.
	AsyncOperation(AsyncOperation&& other) noexcept = default;

	/// Move assignment operator.
	AsyncOperation& operator=(AsyncOperation&& other) = default;

	/// Destructor, which automatically puts the operation into the 'finished' state.
	~AsyncOperation() {
		// Automatically put the promise into the finished state.
		if(isValid() && !isFinished()) {
			setStarted();
			setFinished();
		}
	}

	/// Returns the TaskWatcher automatically created by the TaskManager for this operation.
	TaskWatcher* watcher();

    /// Creates a child operation that executes within the context of this parent operation.
    /// In case the child task gets canceled, this parent task gets canceled too --and vice versa.
	AsyncOperation createSubOperation();

	/// Runs the given callback function as soon as the given future reaches the fulfilled state.
	/// The callback function must accept the future as a parameter.
	/// If this parent operation gets canceled, the callback function may not be run.
	template<typename future_type, typename Executor, typename FC>
	void waitForFutureAsync(future_type&& future, Executor&& executor, bool defer, FC&& callback) {
		OVITO_ASSERT(isValid());
		OVITO_ASSERT(future.isValid());

		// Run the callback function when the input future is finished.
		auto continuation = std::forward<future_type>(future).then_future(std::forward<Executor>(executor), defer, std::forward<FC>(callback));

		// Hold on to the strong dependency to keep the child operation alive until the parent task 
		// finishes or gets canceled.
		QObject::connect(watcher(), &TaskWatcher::canceled, [continuation = std::move(continuation)]() mutable {
			continuation.reset();
		});
	}

private:

	/// Constructor that takes an existing task object.
	AsyncOperation(TaskPtr&& p) noexcept : Promise<>(std::move(p)) {}
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
