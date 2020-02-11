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
#include "Task.h"
#include "Future.h"
#include "TaskWatcher.h"
#include "TaskManager.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(Concurrency)

#ifdef OVITO_DEBUG
// Global counter of Task instance. Used to detect memory leaks.
std::atomic_size_t Task::_instanceCounter{0};
#endif

Task::~Task()
{
	// No-op destructor.

	// Shared states must always end up in the finished state.
	OVITO_ASSERT(isFinished());

#ifdef OVITO_DEBUG
	_instanceCounter.fetch_sub(1);
#endif
}

void Task::decrementShareCount() noexcept
{
	// Automatically cancel this shared state when there are no more futures left that depend on it.
	int oldCount = _shareCount.fetch_sub(1, std::memory_order_release);
	OVITO_ASSERT(oldCount >= 1);
	if(oldCount == 1) {
		cancel();
	}
}

void Task::cancel() noexcept
{
	if(isCanceled() || isFinished()) return;

	_state = State(_state | Canceled);

	for(TaskWatcher* watcher = _watchers; watcher != nullptr; watcher = watcher->_nextInList)
		QMetaObject::invokeMethod(watcher, "promiseCanceled", Qt::QueuedConnection);
}

bool Task::setStarted()
{
    if(isStarted()) {
        return false;	// It's already started. Don't run it again.
	}

    OVITO_ASSERT(!isFinished());
    _state = State(_state | Started);

	for(TaskWatcher* watcher = _watchers; watcher != nullptr; watcher = watcher->_nextInList)
		QMetaObject::invokeMethod(watcher, "promiseStarted", Qt::QueuedConnection);

	return true;
}

void Task::setFinished()
{
    OVITO_ASSERT(isStarted());
    if(!isFinished()) {

		// Lock this promise while finishing up.
		TaskPtr selfLock = shared_from_this();

		setFinishedNoSelfLock();
    }
}

void Task::setFinishedNoSelfLock()
{
	OVITO_ASSERT(!isFinished());

	// Change state.
	_state = State(_state | Finished);

	// Make sure that a result has been set (if not in canceled or error state).
	OVITO_ASSERT_MSG(_exceptionStore || isCanceled() || _resultSet.load() || !_resultsTuple,
		"Task::setFinishedNoSelfLock()",
		qPrintable(QStringLiteral("Result has not been set for the promise state. Please check program code setting the promise state. Progress text: %1").arg(progressText())));

	// Inform task watchers.
	for(TaskWatcher* watcher = _watchers; watcher != nullptr; watcher = watcher->_nextInList) {
		QMetaObject::invokeMethod(watcher, "promiseFinished", Qt::QueuedConnection);
	}

	// Run the continuation functions.
	for(auto& cont : _continuations)
		std::move(cont)(false);
	_continuations.clear();
}

void Task::setException(std::exception_ptr&& ex)
{
	if(isCanceled() || isFinished())
		return;

	OVITO_ASSERT(ex != std::exception_ptr());
	_exceptionStore = std::move(ex); // NOLINT
}

void Task::registerWatcher(TaskWatcher* watcher)
{
	if(isStarted())
		QMetaObject::invokeMethod(watcher, "promiseStarted", Qt::QueuedConnection);

	if(isCanceled())
		QMetaObject::invokeMethod(watcher, "promiseCanceled", Qt::QueuedConnection);

	if(isFinished())
		QMetaObject::invokeMethod(watcher, "promiseFinished", Qt::QueuedConnection);

	watcher->_nextInList = _watchers;
	_watchers = watcher;
}

void Task::unregisterWatcher(TaskWatcher* watcher)
{
	if(_watchers == watcher) {
		_watchers = watcher->_nextInList;
	}
	else {
		for(TaskWatcher* w2 = _watchers; w2 != nullptr; w2 = w2->_nextInList) {
			if(w2->_nextInList == watcher) {
				w2->_nextInList = watcher->_nextInList;
				return;
			}
		}
		OVITO_ASSERT(false);
	}
}

void Task::addContinuationImpl(fu2::unique_function<void(bool)>&& cont, bool defer)
{
	if(!isFinished()) {
		_continuations.push_back(std::move(cont));
	}
	else {
		std::move(cont)(defer);
	}
}

bool Task::waitForFuture(const FutureBase& future)
{
	OVITO_ASSERT_MSG(taskManager() != nullptr, "Task::waitForFuture()", "Calling waitForFuture() on this type of Task is not allowed.");

	if(!taskManager()->waitForTask(future.task(), shared_from_this())) {
		cancel();
		return false;
	}
	return true;
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
