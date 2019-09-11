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

#include <ovito/core/Core.h>
#include "ThreadSafeTask.h"

#include <QMutexLocker>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(Concurrency)

#ifdef OVITO_DEBUG
ThreadSafeTask::~ThreadSafeTask()
{
	// Check if the mutex is currently locked.
	// This should never be the case while destroying the promise state.
	OVITO_ASSERT(_mutex.tryLock());
	_mutex.unlock();
}
#endif

void ThreadSafeTask::cancel() noexcept
{
	if(isCanceled() || isFinished()) return;
	QMutexLocker locker(&_mutex);
	ProgressiveTask::cancel();
}

bool ThreadSafeTask::setStarted()
{
    QMutexLocker locker(&_mutex);
	return ProgressiveTask::setStarted();
}

void ThreadSafeTask::setFinished()
{
	// Lock this promise while finishing up.
	TaskPtr selfLock = shared_from_this();

	QMutexLocker locker(&_mutex);
	ProgressiveTask::setFinished();
}

void ThreadSafeTask::setException(std::exception_ptr&& ex)
{
	QMutexLocker locker(&_mutex);
	ProgressiveTask::setException(std::move(ex));
}

void ThreadSafeTask::registerWatcher(TaskWatcher* watcher)
{
	QMutexLocker locker(&_mutex);
	ProgressiveTask::registerWatcher(watcher);
}

void ThreadSafeTask::unregisterWatcher(TaskWatcher* watcher)
{
	QMutexLocker locker(&_mutex);
	ProgressiveTask::unregisterWatcher(watcher);
}

void ThreadSafeTask::registerTracker(TrackingTask* tracker)
{
	QMutexLocker locker(&_mutex);
	ProgressiveTask::registerTracker(tracker);
}

void ThreadSafeTask::addContinuationImpl(std::function<void()>&& cont)
{
	QMutexLocker locker(&_mutex);
	ProgressiveTask::addContinuationImpl(std::move(cont));
}

void ThreadSafeTask::setProgressMaximum(qlonglong maximum)
{
    QMutexLocker locker(&_mutex);
	ProgressiveTask::setProgressMaximum(maximum);
}

bool ThreadSafeTask::setProgressValue(qlonglong value)
{
    QMutexLocker locker(&_mutex);
    return ProgressiveTask::setProgressValue(value);
}

bool ThreadSafeTask::incrementProgressValue(qlonglong increment)
{
    QMutexLocker locker(&_mutex);
	return ProgressiveTask::incrementProgressValue(increment);
}

void ThreadSafeTask::beginProgressSubStepsWithWeights(std::vector<int> weights)
{
    QMutexLocker locker(&_mutex);
    ProgressiveTask::beginProgressSubStepsWithWeights(std::move(weights));
}

void ThreadSafeTask::nextProgressSubStep()
{
	QMutexLocker locker(&_mutex);
	ProgressiveTask::nextProgressSubStep();
}

void ThreadSafeTask::endProgressSubSteps()
{
	QMutexLocker locker(&_mutex);
	ProgressiveTask::endProgressSubSteps();
}

void ThreadSafeTask::setProgressText(const QString& text)
{
    QMutexLocker locker(&_mutex);
	ProgressiveTask::setProgressText(text);
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
