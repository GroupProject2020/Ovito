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
#include "PromiseStateWithProgress.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(Concurrency)

/******************************************************************************
* Generic base class for promises, which implements the basic state management,
* progress reporting, and event processing.
******************************************************************************/
class OVITO_CORE_EXPORT SynchronousPromiseState : public PromiseStateWithProgress
{
public:

	/// Constructor.
	SynchronousPromiseState(State initialState, TaskManager& taskManager) :
		PromiseStateWithProgress(initialState), _taskManager(taskManager) {}


	/// Sets the current progress value (must be in the range 0 to progressMaximum()).
	/// Returns false if the promise has been canceled.
    virtual bool setProgressValue(int progressValue) override;

	/// Increments the progress value by 1.
	/// Returns false if the promise has been canceled.
    virtual bool incrementProgressValue(int increment = 1) override;

	/// Changes the status text of this promise.
	virtual void setProgressText(const QString& progressText) override;

protected:

	TaskManager& _taskManager;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace


