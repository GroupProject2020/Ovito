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
#include "ThreadSafePromiseState.h"

#include <QMutexLocker>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(Concurrency)

#ifdef OVITO_DEBUG
ThreadSafePromiseState::~ThreadSafePromiseState()
{
	// Check if the mutex is currently locked. 
	// This should never be the case while destroying the promise state.
	OVITO_ASSERT(_mutex.tryLock());
	_mutex.unlock();
}
#endif

void ThreadSafePromiseState::cancel() noexcept
{
	if(isCanceled() || isFinished()) return;
	QMutexLocker locker(&_mutex);
	PromiseStateWithProgress::cancel();
}

bool ThreadSafePromiseState::setStarted()
{
    QMutexLocker locker(&_mutex);
	return PromiseStateWithProgress::setStarted();
}

void ThreadSafePromiseState::setFinished()
{
	// Lock this promise while finishing up.
	PromiseStatePtr selfLock = shared_from_this();

	QMutexLocker locker(&_mutex);
	PromiseStateWithProgress::setFinished();
}

void ThreadSafePromiseState::setException(std::exception_ptr&& ex)
{
	QMutexLocker locker(&_mutex);
	PromiseStateWithProgress::setException(std::move(ex));
}

void ThreadSafePromiseState::registerWatcher(PromiseWatcher* watcher)
{
	QMutexLocker locker(&_mutex);
	PromiseStateWithProgress::registerWatcher(watcher);
}

void ThreadSafePromiseState::unregisterWatcher(PromiseWatcher* watcher)
{
	QMutexLocker locker(&_mutex);
	PromiseStateWithProgress::unregisterWatcher(watcher);
}

void ThreadSafePromiseState::registerTracker(TrackingPromiseState* tracker)
{
	QMutexLocker locker(&_mutex);
	PromiseStateWithProgress::registerTracker(tracker);
}

void ThreadSafePromiseState::addContinuationImpl(std::function<void()>&& cont)
{
	QMutexLocker locker(&_mutex);
	PromiseStateWithProgress::addContinuationImpl(std::move(cont));
}

void ThreadSafePromiseState::setProgressMaximum(int maximum)
{
    QMutexLocker locker(&_mutex);
	PromiseStateWithProgress::setProgressMaximum(maximum);
}

bool ThreadSafePromiseState::setProgressValue(int value)
{
    QMutexLocker locker(&_mutex);
    return PromiseStateWithProgress::setProgressValue(value);
}

bool ThreadSafePromiseState::incrementProgressValue(int increment)
{
    QMutexLocker locker(&_mutex);
	return PromiseStateWithProgress::incrementProgressValue(increment);
}

void ThreadSafePromiseState::beginProgressSubStepsWithWeights(std::vector<int> weights)
{
    QMutexLocker locker(&_mutex);
    PromiseStateWithProgress::beginProgressSubStepsWithWeights(std::move(weights));
}

void ThreadSafePromiseState::nextProgressSubStep()
{
	QMutexLocker locker(&_mutex);
	PromiseStateWithProgress::nextProgressSubStep();
}

void ThreadSafePromiseState::endProgressSubSteps()
{
	QMutexLocker locker(&_mutex);
	PromiseStateWithProgress::endProgressSubSteps();
}

void ThreadSafePromiseState::setProgressText(const QString& text)
{
    QMutexLocker locker(&_mutex);
	PromiseStateWithProgress::setProgressText(text);
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
