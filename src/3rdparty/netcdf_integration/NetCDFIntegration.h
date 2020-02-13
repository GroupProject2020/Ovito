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

#pragma once

#include <ovito/core/Core.h>
#include <QMutex>

#define NCERR(x)  Ovito::NetCDFError::ncerr((x), __FILE__, __LINE__)
#define NCERRI(x, info)  Ovito::NetCDFError::ncerr_with_info((x), __FILE__, __LINE__, info)

namespace Ovito {

/**
 * RAII helper class that is used by OVITO to coordinate concurrent
 * access to the functions from the NetCDF library, which are not thread-safe.
 */
class OVITO_NETCDF_INTEGRATION_EXPORT NetCDFExclusiveAccess
{
public:

    /// Constructor, which blocks until exclusive access to the NetCDF functions is available.
    NetCDFExclusiveAccess();

    /// Constructor, which blocks until exclusive access to the NetCDF functions is available
    /// or the given operation has been canceled, whichever happens first.
    NetCDFExclusiveAccess(Task* task);

    /// Destructor, which releases exclusive access to the NetCDF functions.
    ~NetCDFExclusiveAccess();

    /// Returns whether this object currently has obtained exclusive access to the NetCDF functions.
    bool isLocked() const { return _isLocked; }

private:

    /// Indicates that this object currently has exclusive access to the NetCDF functions.
    bool _isLocked = false;

	/// The global mutex used to serialize access to the NetCDF library functions.
	static QMutex _netcdfMutex;
};

/**
 * Namespace class, which provides error handling routines for NetCDF function calls.
 */
class OVITO_NETCDF_INTEGRATION_EXPORT NetCDFError
{
public:

    /// Checks for NetCDF errors and throws exception in case of an error.
    static void ncerr(int err, const char* file, int line);

    /// Checks for NetCDF errors and throws exception in case of an error (and attaches additional information to exception string).
    static void ncerr_with_info(int err, const char* file, int line, const QString& info);
};

}	// End of namespace
