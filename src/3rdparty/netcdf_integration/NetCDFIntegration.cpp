///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2018) Alexander Stukowski
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

#include <ovito/core/Core.h>
#include <ovito/core/utilities/concurrent/Task.h>
#include "NetCDFIntegration.h"

#include <netcdf.h>

namespace Ovito {

// The global mutex used to serialize access to the NetCDF library functions.
QMutex NetCDFExclusiveAccess::_netcdfMutex(QMutex::Recursive);

/******************************************************************************
* Constructor, which blocks until exclusive access to the NetCDF functions is
* available.
******************************************************************************/
NetCDFExclusiveAccess::NetCDFExclusiveAccess()
{
    _netcdfMutex.lock();
    _isLocked = true;
}

/******************************************************************************
* Constructor, which blocks until exclusive access to the NetCDF functions is
* available or the given operation has been canceled.
******************************************************************************/
NetCDFExclusiveAccess::NetCDFExclusiveAccess(Task* task)
{
    OVITO_ASSERT(task);
    while(!task->isCanceled()) {
        if(_netcdfMutex.tryLock(10)) {
            _isLocked = true;
            break;
        }
    }
}

/******************************************************************************
* Destructor, which releases exclusive access to the NetCDF functions.
******************************************************************************/
NetCDFExclusiveAccess::~NetCDFExclusiveAccess()
{
    if(_isLocked)
        _netcdfMutex.unlock();
}


/******************************************************************************
* Check for NetCDF error and throw exception
******************************************************************************/
void NetCDFError::ncerr(int err, const char* file, int line)
{
	if(err != NC_NOERR)
		throw Exception(QString("NetCDF I/O error: %1 (line %2 of %3)").arg(QString(nc_strerror(err))).arg(line).arg(file));
}

/******************************************************************************
* Check for NetCDF error and throw exception (and attach additional information
* to exception string)
******************************************************************************/
void NetCDFError::ncerr_with_info(int err, const char* file, int line, const QString& info)
{
	if(err != NC_NOERR)
		throw Exception(QString("NetCDF I/O error: %1 %2 (line %3 of %4)").arg(QString(nc_strerror(err))).arg(info).arg(line).arg(file));
}

}	// End of namespace
