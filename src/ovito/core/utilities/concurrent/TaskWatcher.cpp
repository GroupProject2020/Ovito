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
#include "TaskWatcher.h"
#include "Task.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(Concurrency)

void TaskWatcher::watch(const TaskPtr& task, bool pendingAssignment)
{
	OVITO_ASSERT_MSG(QCoreApplication::closingDown() || QThread::currentThread() == QCoreApplication::instance()->thread(), "TaskWatcher::watch", "Function may only be called from the main thread.");

	if(task == _task)
		return;

	if(_task) {
		_task->unregisterWatcher(this);
		if(pendingAssignment) {
	        _finished = false;
	        QCoreApplication::removePostedEvents(this);
		}
	}
	_task = task;
	if(_task)
		_task->registerWatcher(this);
}

/// Cancels the operation being watched by this watcher.
void TaskWatcher::cancel()
{
	if(isWatching())
		task()->cancel();
}

void TaskWatcher::promiseCanceled()
{
	if(isWatching())
		Q_EMIT canceled();
}

void TaskWatcher::promiseFinished()
{
	if(isWatching()) {
		_finished = true;
		Q_EMIT finished();
	}
}

void TaskWatcher::promiseStarted()
{
	if(isWatching())
    	Q_EMIT started();
}

void TaskWatcher::promiseProgressRangeChanged(qlonglong maximum)
{
	if(isWatching() && !task()->isCanceled())
		Q_EMIT progressRangeChanged(maximum);
}

void TaskWatcher::promiseProgressValueChanged(qlonglong progressValue)
{
	if(isWatching() && !task()->isCanceled())
		Q_EMIT progressValueChanged(progressValue);
}

void TaskWatcher::promiseProgressTextChanged(const QString& progressText)
{
	if(isWatching() && !task()->isCanceled())
		Q_EMIT progressTextChanged(progressText);
}

bool TaskWatcher::isCanceled() const
{
	return isWatching() ? task()->isCanceled() : false;
}

bool TaskWatcher::isFinished() const
{
	return _finished;
}

qlonglong TaskWatcher::progressMaximum() const
{
	return isWatching() ? task()->totalProgressMaximum() : 0;
}

qlonglong TaskWatcher::progressValue() const
{
	return isWatching() ? task()->totalProgressValue() : 0;
}

QString TaskWatcher::progressText() const
{
	return isWatching() ? task()->progressText() : QString();
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
