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
#include <core/app/Application.h>
#include "OvitoObjectExecutor.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem)

/******************************************************************************
* Submits the work for execution.
******************************************************************************/
void OvitoObjectExecutor::Work::operator()() 
{
    OVITO_ASSERT(_event);
    if(!QCoreApplication::closingDown() && QThread::currentThread() != QCoreApplication::instance()->thread())
        std::move(*this).post();
    else
        _event.reset();
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
