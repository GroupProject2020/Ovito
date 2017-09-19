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
#include "PromiseWatcher.h"
#include "PromiseState.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(Concurrency)

void PromiseWatcher::watch(const PromiseStatePtr& sharedState, bool pendingAssignment)
{
	OVITO_ASSERT_MSG(QCoreApplication::closingDown() || QThread::currentThread() == QCoreApplication::instance()->thread(), "PromiseWatcher::watch", "Function may only be called from the main thread.");

	if(sharedState == _sharedState)
		return;

	if(_sharedState) {
		_sharedState->unregisterWatcher(this);
		if(pendingAssignment) {
	        _finished = false;
	        QCoreApplication::removePostedEvents(this);
		}
	}
	_sharedState = sharedState;
	if(_sharedState)
		_sharedState->registerWatcher(this);
}

void PromiseWatcher::promiseCanceled()
{
	if(isWatching())
		Q_EMIT canceled();
}

void PromiseWatcher::promiseFinished()
{
	if(isWatching()) {
		_finished = true;
		Q_EMIT finished();
	}
}

void PromiseWatcher::promiseStarted()
{
	if(isWatching())
    	Q_EMIT started();
}

void PromiseWatcher::promiseProgressRangeChanged(int maximum)
{
	if(isWatching() && !sharedState()->isCanceled())
		Q_EMIT progressRangeChanged(maximum);
}

void PromiseWatcher::promiseProgressValueChanged(int progressValue)
{
	if(isWatching() && !sharedState()->isCanceled())
		Q_EMIT progressValueChanged(progressValue);
}

void PromiseWatcher::promiseProgressTextChanged(QString progressText)
{
	if(isWatching() && !sharedState()->isCanceled())
		Q_EMIT progressTextChanged(progressText);
}

bool PromiseWatcher::isCanceled() const
{
	return isWatching() ? sharedState()->isCanceled() : false;
}

bool PromiseWatcher::isFinished() const
{
	return _finished;
}

int PromiseWatcher::progressMaximum() const
{
	return isWatching() ? sharedState()->totalProgressMaximum() : 0;
}

int PromiseWatcher::progressValue() const
{
	return isWatching() ? sharedState()->totalProgressValue() : 0;
}

QString PromiseWatcher::progressText() const
{
	return isWatching() ? sharedState()->progressText() : QString();
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
