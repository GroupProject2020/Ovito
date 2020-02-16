////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2019 Alexander Stukowski
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
#include "AsynchronousTask.h"
#include "TaskManager.h"

namespace Ovito {

/******************************************************************************
* Destructor.
******************************************************************************/
AsynchronousTaskBase::~AsynchronousTaskBase()
{
	// If task was never started, cancel and finish it.
	if(Task::setStarted()) {
		Task::cancelNoSelfLock();
		Task::setFinishedNoSelfLock();
	}
	OVITO_ASSERT(isFinished());
}

/******************************************************************************
* Implementation of QRunnable.
******************************************************************************/
void AsynchronousTaskBase::run()
{
	OVITO_ASSERT(!isStarted() && !isFinished());
	if(!this->setStarted()) return;
	try {
		perform();
	}
	catch(...) {
		this->captureException();
	}
	this->setFinished();
}

}	// End of namespace
