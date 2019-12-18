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

#include <ovito/core/Core.h>
#include <ovito/core/app/Application.h>
#include "OvitoObjectExecutor.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem)

/******************************************************************************
* Event class constructor.
******************************************************************************/
OvitoObjectExecutor::WorkEventBase::WorkEventBase(const OvitoObject* obj) :
    QEvent(workEventType()),
    _obj(const_cast<OvitoObject*>(obj)),
    _executionContext(static_cast<int>(Application::instance()->executionContext()))
{
}

/******************************************************************************
* Activates the original execution context under which the work was submitted.
******************************************************************************/
void OvitoObjectExecutor::WorkEventBase::activateExecutionContext()
{
    Application::ExecutionContext previousContext = Application::instance()->executionContext();
    Application::instance()->switchExecutionContext(static_cast<Application::ExecutionContext>(_executionContext));
    _executionContext = static_cast<int>(previousContext);
}

/******************************************************************************
* Restores the execution context as it was before the work was executed.
******************************************************************************/
void OvitoObjectExecutor::WorkEventBase::restoreExecutionContext()
{
    Application::ExecutionContext previousContext = Application::instance()->executionContext();
    Application::instance()->switchExecutionContext(static_cast<Application::ExecutionContext>(_executionContext));
    _executionContext = static_cast<int>(previousContext);
}

/******************************************************************************
* Submits the work for execution.
******************************************************************************/
void OvitoObjectExecutor::Work::operator()()
{
    OVITO_ASSERT(_event);

    if(!QCoreApplication::closingDown() && QThread::currentThread() != QCoreApplication::instance()->thread()) {
        // Schedule work for later execution in the main thread.
        std::move(*this).post();
    }
    else {
        // Execute work immediately by calling the event destructor.
        _event.reset();
    }
}

/******************************************************************************
* Posts the work for execution at a later time.
******************************************************************************/
void OvitoObjectExecutor::Work::post() &&
{
    OVITO_ASSERT(!QCoreApplication::closingDown());
    OVITO_ASSERT(_event);
    QCoreApplication::postEvent(Application::instance(), _event.release());
}

/******************************************************************************
* Determines whether work can be executed in the context of the OvitoObject or not.
******************************************************************************/
bool OvitoObjectExecutor::WorkEventBase::needToCancelWork() const
{
    // The OvitoObject must still be alive and the application may not be in
    // the process of shutting down for the work to be executable.
    return _obj.isNull() || QCoreApplication::closingDown();
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
