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
#include "ProgressiveTask.h"
#include "TaskWatcher.h"

namespace Ovito {

enum {
    MaxProgressEmitsPerSecond = 20
};

void ProgressiveTask::setProgressMaximum(qlonglong maximum)
{
	if(maximum == _progressMaximum || isCanceled() || isFinished())
		return;

    _progressMaximum = maximum;
    computeTotalProgress();

	for(TaskWatcher* watcher = _watchers; watcher != nullptr; watcher = watcher->_nextInList)
		QMetaObject::invokeMethod(watcher, "promiseProgressRangeChanged", Qt::QueuedConnection, Q_ARG(qlonglong, totalProgressMaximum()));
}

bool ProgressiveTask::setProgressValue(qlonglong value)
{
	_intermittentUpdateCounter = 0;

    if(value == _progressValue || isCanceled() || isFinished())
        return !isCanceled();

    _progressValue = value;
    computeTotalProgress();

    if(!_progressTime.isValid() || _progressValue == _progressMaximum || _progressTime.elapsed() >= (1000 / MaxProgressEmitsPerSecond)) {
		_progressTime.start();

		for(TaskWatcher* watcher = _watchers; watcher != nullptr; watcher = watcher->_nextInList)
			QMetaObject::invokeMethod(watcher, "promiseProgressValueChanged", Qt::QueuedConnection, Q_ARG(qlonglong, totalProgressValue()));
    }

    return !isCanceled();
}

bool ProgressiveTask::setProgressValueIntermittent(qlonglong progressValue, int updateEvery)
{
	if(_intermittentUpdateCounter == 0 || _intermittentUpdateCounter > updateEvery) {
		setProgressValue(progressValue);
	}
	_intermittentUpdateCounter++;
	return !isCanceled();
}

bool ProgressiveTask::incrementProgressValue(qlonglong increment)
{
    if(isCanceled() || isFinished())
        return !isCanceled();

    _progressValue += increment;
    computeTotalProgress();

    if(!_progressTime.isValid() || _progressValue == _progressMaximum || _progressTime.elapsed() >= (1000 / MaxProgressEmitsPerSecond)) {
		_progressTime.start();

		for(TaskWatcher* watcher = _watchers; watcher != nullptr; watcher = watcher->_nextInList)
			QMetaObject::invokeMethod(watcher, "promiseProgressValueChanged", Qt::QueuedConnection, Q_ARG(qlonglong, totalProgressValue()));
    }

    return !isCanceled();
}

void ProgressiveTask::computeTotalProgress()
{
	if(subStepsStack.empty()) {
		_totalProgressMaximum = _progressMaximum;
		_totalProgressValue = _progressValue;
	}
	else {
		double percentage;
		if(_progressMaximum > 0)
			percentage = (double)_progressValue / _progressMaximum;
		else
			percentage = 0;
		for(auto level = subStepsStack.crbegin(); level != subStepsStack.crend(); ++level) {
			OVITO_ASSERT(level->first >= 0 && level->first < level->second.size());
			int weightSum1 = std::accumulate(level->second.cbegin(), level->second.cbegin() + level->first, 0);
			int weightSum2 = std::accumulate(level->second.cbegin() + level->first, level->second.cend(), 0);
			percentage = ((double)weightSum1 + percentage * level->second[level->first]) / (weightSum1 + weightSum2);
		}
		_totalProgressMaximum = 1000;
		_totalProgressValue = qlonglong(percentage * 1000.0);
	}
}

void ProgressiveTask::beginProgressSubStepsWithWeights(std::vector<int> weights)
{
    OVITO_ASSERT(std::accumulate(weights.cbegin(), weights.cend(), 0) > 0);
    subStepsStack.emplace_back(0, std::move(weights));
    _progressMaximum = 0;
    _progressValue = 0;
    computeTotalProgress();
}

void ProgressiveTask::nextProgressSubStep()
{
    if(isCanceled() || isFinished())
        return;

	OVITO_ASSERT(!subStepsStack.empty());
	OVITO_ASSERT(subStepsStack.back().first < subStepsStack.back().second.size() - 1);
	subStepsStack.back().first++;
    _progressMaximum = 0;
    _progressValue = 0;
    computeTotalProgress();

	for(TaskWatcher* watcher = _watchers; watcher != nullptr; watcher = watcher->_nextInList)
		QMetaObject::invokeMethod(watcher, "promiseProgressValueChanged", Qt::QueuedConnection, Q_ARG(qlonglong, totalProgressValue()));
}

void ProgressiveTask::endProgressSubSteps()
{
	OVITO_ASSERT(!subStepsStack.empty());
	subStepsStack.pop_back();
    _progressMaximum = 0;
    _progressValue = 0;
    computeTotalProgress();
}

void ProgressiveTask::setProgressText(const QString& progressText)
{
    if(isCanceled() || isFinished())
        return;

    _progressText = progressText;

	for(TaskWatcher* watcher = _watchers; watcher != nullptr; watcher = watcher->_nextInList)
		QMetaObject::invokeMethod(watcher, "promiseProgressTextChanged", Qt::QueuedConnection, Q_ARG(QString, progressText));
}

}	// End of namespace
