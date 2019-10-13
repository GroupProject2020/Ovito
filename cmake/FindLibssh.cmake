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

# This module tries to find the libssh library.
#
# It creates an imported CMake target named "Libssh::Libssh".

FIND_PATH(LIBSSH_INCLUDE_DIR NAMES libssh/libssh.h)
FIND_LIBRARY(LIBSSH_LIBRARY NAMES ssh libssh)
SET(LIBSSH_INCLUDE_DIRS ${LIBSSH_INCLUDE_DIR})
SET(LIBSSH_LIBRARIES ${LIBSSH_LIBRARY})
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LIBSSH DEFAULT_MSG LIBSSH_LIBRARY LIBSSH_INCLUDE_DIR)
MARK_AS_ADVANCED(LIBSSH_INCLUDE_DIR LIBSSH_LIBRARY)

# Create imported target for the library.
IF(LIBSSH_FOUND AND NOT TARGET Libssh::Libssh)
	ADD_LIBRARY(Libssh::Libssh UNKNOWN IMPORTED)
	SET_TARGET_PROPERTIES(Libssh::Libssh PROPERTIES
		IMPORTED_LOCATION "${LIBSSH_LIBRARY}"
		INTERFACE_INCLUDE_DIRECTORIES "${LIBSSH_INCLUDE_DIR}"
	)
ENDIF()
