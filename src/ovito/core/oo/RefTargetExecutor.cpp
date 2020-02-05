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
#include <ovito/core/oo/RefTarget.h>
#include <ovito/core/app/Application.h>
#include <ovito/core/dataset/DataSet.h>
#include "RefTargetExecutor.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem)

/******************************************************************************
* Returns the task manager that provides the context for tasks created by this executor.
******************************************************************************/
TaskManager* RefTargetExecutor::taskManager() const
{
    return &object()->dataset()->taskManager();
}

/******************************************************************************
* Event class constructor.
******************************************************************************/
RefTargetExecutor::WorkEventBase::WorkEventBase(const RefTarget* obj) :
    QEvent(workEventType()),
    _obj(const_cast<RefTarget*>(obj)),
    _executionContext(static_cast<int>(Application::instance()->executionContext()))
{    
//    OVITO_ASSERT(!_obj->dataset()->undoStack().isRecording());
}

/******************************************************************************
* Activates the original execution context under which the work was submitted.
******************************************************************************/
void RefTargetExecutor::WorkEventBase::activateExecutionContext()
{
    if(Application* app = Application::instance()) {
        Application::ExecutionContext previousContext = app->executionContext();
        app->switchExecutionContext(static_cast<Application::ExecutionContext>(_executionContext));
        _executionContext = static_cast<int>(previousContext);

        // In the current implementation, deferred work is always executed without undo recording.
        // Thus, we should suspend the undo stack while running the work function.    
        _obj->dataset()->undoStack().suspend();
    }
}

/******************************************************************************
* Restores the execution context as it was before the work was executed.
******************************************************************************/
void RefTargetExecutor::WorkEventBase::restoreExecutionContext()
{
    if(Application* app = Application::instance()) {
        Application::ExecutionContext previousContext = app->executionContext();
        app->switchExecutionContext(static_cast<Application::ExecutionContext>(_executionContext));
        _executionContext = static_cast<int>(previousContext);

        // Restore undo recording state.
        _obj->dataset()->undoStack().resume();
    }
}

/******************************************************************************
* Submits the work for execution.
******************************************************************************/
void RefTargetExecutor::Work::operator()(bool defer)
{
    OVITO_ASSERT(_event);

    if(defer || (!QCoreApplication::closingDown() && QThread::currentThread() != QCoreApplication::instance()->thread())) {
        // Schedule work for later execution in the main thread.
        QCoreApplication::postEvent(Application::instance(), _event.release());
    }
    else {
        // Execute work immediately by calling the event destructor.
        _event.reset();
    }
}

/******************************************************************************
* Determines whether work can be executed in the context of the RefTarget or not.
******************************************************************************/
bool RefTargetExecutor::WorkEventBase::needToCancelWork() const
{
    // The RefTarget must still be alive and the application may not be in
    // the process of shutting down for the work to be executable.
    return _obj.isNull() || QCoreApplication::closingDown();
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
