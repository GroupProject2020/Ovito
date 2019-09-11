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


#include <ovito/core/Core.h>
#include "ProgressiveTask.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(Concurrency)

/**
 * \brief Type of Task to be used within a single-thread context.
 *
 * The methods of this class are not thread-safe and may only be from the main thread.
 * A new main-thread task is typically created using the TaskManager::createMainThreadOperation() method.
 *
 * \sa ThreadSafeTask
 */
class OVITO_CORE_EXPORT MainThreadTask : public ProgressiveTask
{
public:

	/// Constructor.
	MainThreadTask(State initialState, TaskManager& taskManager) :
		ProgressiveTask(initialState), _taskManager(taskManager) {}

	/// Sets the current progress value (must be in the range 0 to progressMaximum()).
	/// Returns false if the promise has been canceled.
    virtual bool setProgressValue(qlonglong value) override;

	/// Increments the progress value by 1.
	/// Returns false if the promise has been canceled.
    virtual bool incrementProgressValue(qlonglong increment = 1) override;

	/// Changes the status text of this promise.
	virtual void setProgressText(const QString& progressText) override;

	/// Creates a child operation.
	/// If the child operation is canceled, this parent operation gets canceled too -and vice versa.
	virtual Promise<> createSubTask() override;

	/// Blocks execution until the given future enters the completed state.
	virtual bool waitForFuture(const FutureBase& future) override;

protected:

	TaskManager& _taskManager;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace


