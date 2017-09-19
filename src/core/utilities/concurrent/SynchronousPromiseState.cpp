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
#include "SynchronousPromiseState.h"
#include "Future.h"
#include "TaskManager.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(Concurrency)

bool SynchronousPromiseState::setProgressValue(int value)
{
	// Yield control to the event loop to process user interface events.
	// This is necessary so that the user can interrupt the running opertion.
	if(_taskManager) _taskManager->processEvents();

    return PromiseStateWithProgress::setProgressValue(value);
}

bool SynchronousPromiseState::incrementProgressValue(int increment)
{
	// Yield control to the event loop to process user interface events.
	// This is necessary so that the user can interrupt the running opertion.
	if(_taskManager) _taskManager->processEvents();

	return PromiseStateWithProgress::incrementProgressValue(increment);
}

void SynchronousPromiseState::setProgressText(const QString& progressText)
{
	PromiseStateWithProgress::setProgressText(progressText);

	// Yield control to the event loop to process user interface events.
	// This is necessary so that the user can interrupt the running opertion.
	if(_taskManager) _taskManager->processEvents();
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
