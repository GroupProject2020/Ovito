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

void ThreadSafeTask::addContinuationImpl(fu2::unique_function<void(bool)>&& cont, bool defer)
{
	QMutexLocker locker(&_mutex);
	if(isFinished()) locker.unlock();
	ProgressiveTask::addContinuationImpl(std::move(cont), defer);
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
