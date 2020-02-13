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
#include "TaskWatcher.h"
#include "Task.h"

namespace Ovito {

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

}	// End of namespace
