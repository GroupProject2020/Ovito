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

#pragma once


#include <ovito/core/Core.h>
#include "Task.h"

#include <QElapsedTimer>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(Concurrency)

/******************************************************************************
* Class that provides the basic state management,
* progress reporting, and event processing in the promise/future framework.
******************************************************************************/
class OVITO_CORE_EXPORT ProgressiveTask : public Task
{
public:

	/// Returns the maximum value for progress reporting.
    virtual qlonglong progressMaximum() const override { return _progressMaximum; }

	/// Sets the current maximum value for progress reporting.
    virtual void setProgressMaximum(qlonglong maximum) override;

	/// Returns the current progress value (in the range 0 to progressMaximum()).
	virtual qlonglong progressValue() const override { return _progressValue; }

	/// Sets the current progress value (must be in the range 0 to progressMaximum()).
	/// Returns false if the promise has been canceled.
    virtual bool setProgressValue(qlonglong value) override;

	/// Increments the progress value by 1.
	/// Returns false if the promise has been canceled.
    virtual bool incrementProgressValue(qlonglong increment = 1) override;

	/// Sets the progress value of the promise but generates an update event only occasionally.
	/// Returns false if the promise has been canceled.
    virtual bool setProgressValueIntermittent(qlonglong progressValue, int updateEvery = 2000) override;

	/// Return the current status text set for this promise.
    virtual QString progressText() const override { return _progressText; }

	/// Changes the status text of this promise.
	virtual void setProgressText(const QString& progressText) override;

	// Progress reporting for tasks with sub-steps:

	/// Begins a sequence of sub-steps in the progress range of this promise.
	/// This is used for long and complex computations, which consist of several logical sub-steps, each with
	/// a separate progress range.
    virtual void beginProgressSubStepsWithWeights(std::vector<int> weights) override;

	/// Changes to the next sub-step in the sequence started with beginProgressSubSteps().
    virtual void nextProgressSubStep() override;

	/// Must be called at the end of a sub-step sequence started with beginProgressSubSteps().
    virtual void endProgressSubSteps() override;

	/// Returns the maximum progress value that can be reached (taking into account sub-steps).
    virtual qlonglong totalProgressMaximum() const override { return _totalProgressMaximum; }

	/// Returns the current progress value (taking into account sub-steps).
    virtual qlonglong totalProgressValue() const override { return _totalProgressValue; }

protected:

	/// Constructor.
	ProgressiveTask(State initialState = NoState, const QString& progressText = QString()) :
		Task(initialState), _progressText(progressText) {}

    void computeTotalProgress();

	qlonglong _totalProgressValue = 0;
	qlonglong _totalProgressMaximum = 0;
    qlonglong _progressValue = 0;
    qlonglong _progressMaximum = 0;
    int _intermittentUpdateCounter = 0;
    QString _progressText;
    QElapsedTimer _progressTime;
    std::vector<std::pair<int, std::vector<int>>> subStepsStack;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
