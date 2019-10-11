###############################################################################
#
#  Copyright (2019) Alexander Stukowski
#
#  This file is part of OVITO (Open Visualization Tool).
#
#  OVITO is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  OVITO is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
###############################################################################

###############################################################################
# The netcdf_integration CMake target provides a helper module that can be
# used by other modules in OVITO that make use of the NetCDF library. It
# pulls in the necessary header files and system libraries and provides the
# helper class NetCDFExclusiveAccess, which manages concurrent access to the
# NetCDF functions, which are not thread-safe.
###############################################################################

# Locate the HDF5 library.
FIND_PACKAGE(HDF5 REQUIRED COMPONENTS C HL)

# Locate the NetCDF library.
FIND_PACKAGE(NetCDF REQUIRED)

# Build library.
ADD_LIBRARY(NetCDFIntegration NetCDFIntegration.cpp)

# Give our library file a name to not confuse it with any system versions of the library.
SET_TARGET_PROPERTIES(NetCDFIntegration PROPERTIES OUTPUT_NAME "ovito_netcdf")

# Link to Qt5.
TARGET_LINK_LIBRARIES(NetCDFIntegration PUBLIC Qt5::Core)

# This library depends on the Core module of OVITO.
TARGET_LINK_LIBRARIES(NetCDFIntegration PUBLIC Core)

# Define macro for symbol export from the shared library.
IF(BUILD_SHARED_LIBS)
	TARGET_COMPILE_DEFINITIONS(NetCDFIntegration PRIVATE "OVITO_NETCDF_INTEGRATION_EXPORT=Q_DECL_EXPORT")
	TARGET_COMPILE_DEFINITIONS(NetCDFIntegration INTERFACE "OVITO_NETCDF_INTEGRATION_EXPORT=Q_DECL_IMPORT")
ELSE()
	TARGET_COMPILE_DEFINITIONS(NetCDFIntegration PUBLIC "OVITO_NETCDF_INTEGRATION_EXPORT=")
ENDIF()

# Set visibility of symbols in this shared library to hidden by default, except those exported in the source code.
SET_TARGET_PROPERTIES(NetCDFIntegration PROPERTIES CXX_VISIBILITY_PRESET "hidden")
SET_TARGET_PROPERTIES(NetCDFIntegration PROPERTIES VISIBILITY_INLINES_HIDDEN ON)

# Link in the NetCDF library
TARGET_LINK_LIBRARIES(NetCDFIntegration PUBLIC netcdf)
TARGET_INCLUDE_DIRECTORIES(NetCDFIntegration PUBLIC "${netcdf_INCLUDE_DIRS}")
IF(WIN32)
	TARGET_COMPILE_DEFINITIONS(NetCDFIntegration PUBLIC DLL_NETCDF)
ENDIF()

# Export this target.
INSTALL(TARGETS NetCDFIntegration EXPORT OVITO
	RUNTIME DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}"
	LIBRARY DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}"
	ARCHIVE DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}" COMPONENT "development")

IF(NOT APPLE)

	# Deploy NetCDF shared library.
	GET_TARGET_PROPERTY(NETCDF_LIBRARY netcdf LOCATION)
	OVITO_INSTALL_SHARED_LIB("${NETCDF_LIBRARY}" ".")

	# Deploy HDF5 shared library, which is required by NetCDF lib.
	IF(TARGET hdf5-shared)
		GET_TARGET_PROPERTY(HDF5_LIBRARY hdf5-shared IMPORTED_LOCATION_RELEASE)
	ENDIF()
	IF(NOT HDF5_LIBRARY AND TARGET hdf5::hdf5-shared)
		GET_TARGET_PROPERTY(HDF5_LIBRARY hdf5::hdf5-shared IMPORTED_LOCATION_RELEASE)
	ENDIF()
	IF(NOT HDF5_LIBRARY AND UNIX)
		LIST(GET HDF5_LIBRARIES 0 HDF5_LIBRARY)
	ENDIF()
	IF(NOT HDF5_LIBRARY)
		MESSAGE(FATAL_ERROR "CMake target for the HDF5 library does not exist. Please check you HDF5 installation.")
	ENDIF()
	OVITO_INSTALL_SHARED_LIB("${HDF5_LIBRARY}" ".")

	# Deploy HDF5_HL shared library, which is required by NetCDF lib.
	IF(TARGET hdf5_hl-shared)
		GET_TARGET_PROPERTY(HDF5_HL_LIBRARY hdf5_hl-shared IMPORTED_LOCATION_RELEASE)
	ENDIF()
	IF(NOT HDF5_HL_LIBRARY AND TARGET hdf5::hdf5_hl-shared)
		GET_TARGET_PROPERTY(HDF5_HL_LIBRARY hdf5::hdf5_hl-shared IMPORTED_LOCATION_RELEASE)
	ENDIF()
	IF(NOT HDF5_HL_LIBRARY AND UNIX)
                LIST(GET HDF5_HL_LIBRARIES 0 HDF5_HL_LIBRARY)
	ENDIF()
	IF(NOT HDF5_HL_LIBRARY)
		MESSAGE(FATAL_ERROR "CMake target for the HDF5_HL library does not exist. Please check you HDF5 installation.")
	ENDIF()
	OVITO_INSTALL_SHARED_LIB("${HDF5_HL_LIBRARY}" ".")

ENDIF()
