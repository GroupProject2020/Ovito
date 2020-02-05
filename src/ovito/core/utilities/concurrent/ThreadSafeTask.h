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
#include "Promise.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(Concurrency)

/**
 * \brief Default shared state type used to connect Promise/Future pairs across thread boundaries.
 */
class OVITO_CORE_EXPORT ThreadSafeTask : public ProgressiveTask
{
public:

	/// Constructor.
	ThreadSafeTask() = default;

#ifdef OVITO_DEBUG
	/// Destructor.
	virtual ~ThreadSafeTask();
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

    virtual void registerWatcher(TaskWatcher* watcher) override;
    virtual void unregisterWatcher(TaskWatcher* watcher) override;
	virtual void addContinuationImpl(fu2::unique_function<void(bool)>&& cont, bool defer) override;

	QMutex _mutex;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
