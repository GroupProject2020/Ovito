#######################################################################################
#
#  Copyright 2019 Alexander Stukowski
#
#  This file is part of OVITO (Open Visualization Tool).
#
#  OVITO is free software; you can redistribute it and/or modify it either under the
#  terms of the GNU General Public License version 3 as published by the Free Software
#  Foundation (the "GPL") or, at your option, under the terms of the MIT License.
#  If you do not alter this notice, a recipient may use your version of this
#  file under either the GPL or the MIT License.
#
#  You should have received a copy of the GPL along with this program in a
#  file LICENSE.GPL.txt.  You should have received a copy of the MIT License along
#  with this program in a file LICENSE.MIT.txt
#
#  This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND,
#  either express or implied. See the GPL or the MIT License for the specific language
#  governing rights and limitations.
#
#######################################################################################

# - Find NetCDF
# Find the native NetCDF includes and library.
# Once done this will define
#
#  netcdf_INCLUDE_DIRS   - where to find netcdf.h.
#  netcdf                - The imported library target.

# First look for netcdf-config.cmake
IF(NOT OVITO_BUILD_CONDA)
	FIND_PACKAGE(netcdf QUIET CONFIG)
ENDIF()

IF(NOT netcdf_FOUND)

	# Look for netCDF-config.cmake
	IF(NOT OVITO_BUILD_CONDA)
		FIND_PACKAGE(netCDF QUIET CONFIG)
	ENDIF()

	IF(NOT netCDF_FOUND)

		# If that doesn't work, use our method to find the library
		FIND_PATH(NETCDF_INCLUDE_DIR netcdf.h)

		FIND_LIBRARY(NETCDF_LIBRARY NAMES netcdf)
		MARK_AS_ADVANCED(NETCDF_LIBRARY NETCDF_INCLUDE_DIR)

		INCLUDE(FindPackageHandleStandardArgs)
		FIND_PACKAGE_HANDLE_STANDARD_ARGS(NetCDF DEFAULT_MSG NETCDF_LIBRARY NETCDF_INCLUDE_DIR)

		# Create imported target for the library.
		ADD_LIBRARY(netcdf UNKNOWN IMPORTED GLOBAL)

		IF(CYGWIN)
			SET_PROPERTY(TARGET netcdf PROPERTY IMPORTED_IMPLIB "${NETCDF_LIBRARY}")
		ELSE()
			SET_PROPERTY(TARGET netcdf PROPERTY IMPORTED_LOCATION "${NETCDF_LIBRARY}")
		ENDIF()

		SET_PROPERTY(TARGET netcdf APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${NETCDF_INCLUDE_DIR}")

	ENDIF()

ENDIF()

