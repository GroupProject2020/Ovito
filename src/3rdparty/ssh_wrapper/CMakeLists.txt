###############################################################################
# 
#  Copyright (2018) Alexander Stukowski
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

# Locate the libssh library.
FIND_PACKAGE(Libssh REQUIRED)

# Build library
ADD_LIBRARY(sshwrapper SHARED
	sshconnection.cpp
	sftpchannel.cpp
)

# Link to libssh.
TARGET_INCLUDE_DIRECTORIES(sshwrapper PUBLIC ${LIBSSH_INCLUDE_DIRS})
TARGET_LINK_LIBRARIES(sshwrapper PUBLIC ${LIBSSH_LIBRARIES})

# Link to Qt5.
TARGET_LINK_LIBRARIES(sshwrapper PUBLIC Qt5::Core)

# Make this library part of the installation package.
INSTALL(TARGETS sshwrapper EXPORT OVITO 
	RUNTIME DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}" COMPONENT "runtime"
	LIBRARY DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}" COMPONENT "runtime")

# Export target to make it accessible for external plugins.
IF(CMAKE_VERSION VERSION_LESS "3")
	EXPORT(TARGETS sshwrapper NAMESPACE "Ovito::" FILE "${${PROJECT_NAME}_BINARY_DIR}/OVITOTargets.cmake")
ENDIF()
