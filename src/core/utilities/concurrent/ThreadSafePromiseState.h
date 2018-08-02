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
#include "Promise.h"

#include <QMutex>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(Concurrency)

/******************************************************************************
* Generic base class for promises, which implements the basic state management,
* progress reporting, and event processing.
******************************************************************************/
class OVITO_CORE_EXPORT ThreadSafePromiseState : public PromiseStateWithProgress
{
public:

	/// Constructor.
	ThreadSafePromiseState() = default;

#ifdef OVITO_DEBUG
	/// Destructor.
	virtual ~ThreadSafePromiseState();
#endif	

	/// Sets the current maximum value for progress reporting.
    virtual void setProgressMaximum(qlonglong maximum) override;

	/// Sets the current progress value (must be in the range 0 to progressMaximum()).
	/// Returns false if the promise has been canceled.
    virtual bool setProgressValue(qlonglong value) override;

	/// Increments the progress value by 1.
	/// Returns false if the promise has been canceled.
    virtual bool incrementProgressValue(qlonglong increment = 1) override;

	/// Changes the status text of this promise.
    virtual void setProgressText(const QString& text) override;

	/// Begins a sequence of sub-steps in the progress range of this promise.
	/// This is used for long and complex computations, which consist of several logical sub-steps, each with
	/// a separate progress range.
    virtual void beginProgressSubStepsWithWeights(std::vector<int> weights) override;

	/// Changes to the next sub-step in the sequence started with beginProgressSubSteps().
    virtual void nextProgressSubStep() override;

	/// Must be called at the end of a sub-step sequence started with beginProgressSubSteps().
    virtual void endProgressSubSteps() override;

	/// This must be called after creating a promise to put it into the 'started' state.
	/// Returns false if the promise is or was already in the 'started' state.
    virtual bool setStarted() override;

	/// This must be called after the promise has been fulfilled (even if an exception occurred).
	virtual void setFinished() override;

	/// Cancels this promise.
	virtual void cancel() noexcept override;

	/// Sets the promise into the 'exception' state to signal that an exception has occurred 
	/// while trying to fulfill it. 
    virtual void setException(std::exception_ptr&& ex) override;

protected:

    virtual void registerWatcher(PromiseWatcher* watcher) override;
    virtual void unregisterWatcher(PromiseWatcher* watcher) override;
    virtual void registerTracker(TrackingPromiseState* tracker) override;
	virtual void addContinuationImpl(std::function<void()>&& cont) override;

	QMutex _mutex;
};


OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace


