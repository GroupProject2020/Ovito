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

#include <ovito/core/Core.h>
#include "Task.h"
#include "TrackingTask.h"
#include "Future.h"
#include "TaskWatcher.h"

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
	OVITO_ASSERT(!_trackers);

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

void Task::cancelIfSingleFutureLeft() noexcept
{
	// Cancels this task if there is only a single future left that depends on it.
    // This is an internal method used by TaskManager::waitForTask() to cancel subtasks right away if their parent task got canceled.
	if(_shareCount.load() <= 1)
		cancel();
	OVITO_ASSERT(_shareCount.load() <= 1);
}

void Task::cancel() noexcept
{
	if(isCanceled() || isFinished()) return;

	_state = State(_state | Canceled);

	for(TaskWatcher* watcher = _watchers; watcher != nullptr; watcher = watcher->_nextInList)
		QMetaObject::invokeMethod(watcher, "promiseCanceled", Qt::QueuedConnection);
	for(TrackingTask* tracker = _trackers.get(); tracker != nullptr; tracker = tracker->_nextInList.get())
		tracker->cancel();
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
	for(TrackingTask* tracker = _trackers.get(); tracker != nullptr; tracker = tracker->_nextInList.get())
		tracker->setStarted();

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

	// Run the continuation functions.
	for(auto& cont : _continuations)
		std::move(cont)();
	_continuations.clear();

	// Inform promise watchers.
	for(TaskWatcher* watcher = _watchers; watcher != nullptr; watcher = watcher->_nextInList) {
		QMetaObject::invokeMethod(watcher, "promiseFinished", Qt::QueuedConnection);
	}

	// Inform promise trackers.
	while(_trackers) {
		_trackers->_resultsTuple = _resultsTuple;
#ifdef OVITO_DEBUG
		_trackers->_resultSet.store(this->_resultSet.load());
#endif
		_trackers->_exceptionStore = _exceptionStore;
		_trackers->setFinished();
		std::shared_ptr<TrackingTask> next = std::move(_trackers->_nextInList);
		_trackers.swap(next);
	}

	OVITO_ASSERT(isFinished());
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

void Task::registerTracker(TrackingTask* tracker)
{
	OVITO_ASSERT(!tracker->_nextInList);

	if(isStarted())
		tracker->setStarted();

	if(isCanceled())
		tracker->cancel();

	if(isFinished()) {
		OVITO_ASSERT(!_trackers);
		tracker->_resultsTuple = _resultsTuple;
#ifdef OVITO_DEBUG
		tracker->_resultSet.store(this->_resultSet.load());
#endif
		tracker->_exceptionStore = _exceptionStore;
		tracker->setFinished();
	}
	else {
		// Insert the tracker into the linked list of trackers.
		tracker->_nextInList = std::move(_trackers);
		_trackers = static_pointer_cast<TrackingTask>(tracker->shared_from_this());
	}
}

void Task::addContinuationImpl(std::function<void()>&& cont)
{
	if(!isFinished()) {
		_continuations.push_back(std::move(cont));
	}
	else {
		std::move(cont)();
	}
}

Promise<> Task::createSubTask()
{
	OVITO_ASSERT(false);
	return Promise<>::createFailed(Exception(QStringLiteral("Internal error: Calling createSubTask() on this type of Task is not allowed.")));
}

bool Task::waitForFuture(const FutureBase& future)
{
	OVITO_ASSERT(false);
	throw Exception(QStringLiteral("Internal error: Calling waitForFuture() on this type of Task is not allowed."));
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
