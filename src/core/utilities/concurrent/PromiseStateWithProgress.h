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

#pragma once


#include <core/Core.h>
#include "PromiseState.h"

#include <QElapsedTimer>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(Concurrency)

/******************************************************************************
* Class that provides the basic state management,
* progress reporting, and event processing in the promise/future framework.
******************************************************************************/
class OVITO_CORE_EXPORT PromiseStateWithProgress : public PromiseState
{
public:

	/// Returns the maximum value for progress reporting. 
    virtual int progressMaximum() const override { return _progressMaximum; }

	/// Sets the current maximum value for progress reporting.
    virtual void setProgressMaximum(int maximum) override;
    
	/// Returns the current progress value (in the range 0 to progressMaximum()).
	virtual int progressValue() const override { return _progressValue; }

	/// Sets the current progress value (must be in the range 0 to progressMaximum()).
	/// Returns false if the promise has been canceled.
    virtual bool setProgressValue(int value) override;

	/// Increments the progress value by 1.
	/// Returns false if the promise has been canceled.
    virtual bool incrementProgressValue(int increment = 1) override;

	/// Sets the progress value of the promise but generates an update event only occasionally.
	/// Returns false if the promise has been canceled.
    virtual bool setProgressValueIntermittent(int progressValue, int updateEvery = 2000) override;

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
    virtual int totalProgressMaximum() const override { return _totalProgressMaximum; }

	/// Returns the current progress value (taking into account sub-steps).
    virtual int totalProgressValue() const override { return _totalProgressValue; }

protected:

	/// Constructor.
	PromiseStateWithProgress(State initialState = NoState, const QString& progressText = QString()) : 
		PromiseState(initialState), _progressText(progressText) {}

    void computeTotalProgress();

	int _totalProgressValue = 0;
	int _totalProgressMaximum = 0;
    int _progressValue = 0;
    int _progressMaximum = 0;
    int _intermittentUpdateCounter = 0;
    QString _progressText;
    QElapsedTimer _progressTime;
    std::vector<std::pair<int, std::vector<int>>> subStepsStack;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace


