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

# Try to find the OpenVR library
#  OPENVR_FOUND - system has OpenVR lib
#  OpenVR_INCLUDE_DIRS - the include directories needed
#  OpenVR_LIBRARIES - libraries needed

FIND_PATH(OPENVR_INCLUDE_DIR NAMES openvr.h)
FIND_LIBRARY(OPENVR_LIBRARY
    NAMES openvr openvr_api
    PATH_SUFFIXES osx64 linux64 win64
    NO_DEFAULT_PATH
    NO_CMAKE_FIND_ROOT_PATH)

SET(OpenVR_INCLUDE_DIRS ${OPENVR_INCLUDE_DIR})
SET(OpenVR_LIBRARIES ${OPENVR_LIBRARY})

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(OpenVR DEFAULT_MSG OPENVR_LIBRARY OPENVR_INCLUDE_DIR)

MARK_AS_ADVANCED(OPENVR_INCLUDE_DIR OPENVR_LIBRARY)

# Create an CMake import target for the OpenVR framework.
IF(OPENVR_FOUND)
    ADD_LIBRARY(OpenVR::OpenVR UNKNOWN IMPORTED)
    SET_TARGET_PROPERTIES(OpenVR::OpenVR PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${OPENVR_INCLUDE_DIR}")
    IF(APPLE)
        IF(OPENVR_LIBRARY MATCHES "/([^/]+)\\.framework$")
            SET_TARGET_PROPERTIES(OpenVR::OpenVR PROPERTIES IMPORTED_LOCATION "${OPENVR_LIBRARY}/${CMAKE_MATCH_1}")
        ELSE()
            SET_TARGET_PROPERTIES(OpenVR::OpenVR PROPERTIES IMPORTED_LOCATION "${OPENVR_LIBRARY}")
        ENDIF()
    ENDIF()
ENDIF()