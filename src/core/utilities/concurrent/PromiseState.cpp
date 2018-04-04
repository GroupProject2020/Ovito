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

#include <core/Core.h>
#include "PromiseState.h"
#include "TrackingPromiseState.h"
#include "Future.h"
#include "PromiseWatcher.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(Concurrency)

#ifdef OVITO_DEBUG
// Global counter of PromiseState instance. Used to detect memory leaks.
std::atomic_size_t PromiseState::_instanceCounter{0};
#endif

PromiseState::~PromiseState()
{
	// No-op destructor.

	// Shared states must always end up in the finished state.
	OVITO_ASSERT(isFinished());
	OVITO_ASSERT(!_trackers);

#ifdef OVITO_DEBUG
	_instanceCounter.fetch_sub(1);
#endif
}

void PromiseState::decrementShareCount() noexcept 
{ 
	// Automatically cancel this shared state when there no more futures
	// referring to it.
	int oldCount = _shareCount.fetch_sub(1, std::memory_order_release);
	OVITO_ASSERT(oldCount >= 1);
	if(oldCount == 1) {
		cancel();
	}
}

void PromiseState::cancel() noexcept
{
	if(isCanceled() || isFinished()) return;
	
	_state = State(_state | Canceled);

	for(PromiseWatcher* watcher = _watchers; watcher != nullptr; watcher = watcher->_nextInList)
		QMetaObject::invokeMethod(watcher, "promiseCanceled", Qt::QueuedConnection);
	for(TrackingPromiseState* tracker = _trackers.get(); tracker != nullptr; tracker = tracker->_nextInList.get())
		tracker->cancel();
}

bool PromiseState::setStarted()
{
    if(isStarted()) {
        return false;	// It's already started. Don't run it again.
	}

    OVITO_ASSERT(!isFinished());
    _state = State(_state | Started);

	for(PromiseWatcher* watcher = _watchers; watcher != nullptr; watcher = watcher->_nextInList)
		QMetaObject::invokeMethod(watcher, "promiseStarted", Qt::QueuedConnection);
	for(TrackingPromiseState* tracker = _trackers.get(); tracker != nullptr; tracker = tracker->_nextInList.get())
		tracker->setStarted();

	return true;
}

void PromiseState::setFinished()
{
    OVITO_ASSERT(isStarted());
    if(!isFinished()) {
		
		// Lock this promise while finishing up.
		PromiseStatePtr selfLock = shared_from_this();

		setFinishedNoSelfLock();
    }
}

void PromiseState::setFinishedNoSelfLock()
{
	OVITO_ASSERT(!isFinished());
	
	// Change state.
	_state = State(_state | Finished);

	// Make sure that a result has been set (if not in canceled or error state).
	OVITO_ASSERT_MSG(_exceptionStore || isCanceled() || _resultSet.load() || !_resultsTuple, "PromiseState::setFinishedNoSelfLock()", "Result has not been set for this promise state.");

	// Run the continuation functions.
	for(auto& cont : _continuations)
		std::move(cont)();
	_continuations.clear();

	// Inform promise watchers.
	for(PromiseWatcher* watcher = _watchers; watcher != nullptr; watcher = watcher->_nextInList) {
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
		std::shared_ptr<TrackingPromiseState> next = std::move(_trackers->_nextInList);
		_trackers.swap(next);
	}
	
	OVITO_ASSERT(isFinished());
}

void PromiseState::setException(std::exception_ptr&& ex)
{
	if(isCanceled() || isFinished())
		return;
	
	OVITO_ASSERT(ex != std::exception_ptr());
	_exceptionStore = std::move(ex); // NOLINT
}

void PromiseState::registerWatcher(PromiseWatcher* watcher)
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

void PromiseState::unregisterWatcher(PromiseWatcher* watcher)
{
	if(_watchers == watcher) {
		_watchers = watcher->_nextInList;
	}
	else {
		for(PromiseWatcher* w2 = _watchers; w2 != nullptr; w2 = w2->_nextInList) {
			if(w2->_nextInList == watcher) {
				w2->_nextInList = watcher->_nextInList;
				return;
			}
		}
		OVITO_ASSERT(false);
	}
}

void PromiseState::registerTracker(TrackingPromiseState* tracker)
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
		_trackers = static_pointer_cast<TrackingPromiseState>(tracker->shared_from_this());
	}
}

void PromiseState::addContinuationImpl(std::function<void()>&& cont)
{
	if(!isFinished()) {
		_continuations.push_back(std::move(cont));
	}
	else {
		std::move(cont)();
	}
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
