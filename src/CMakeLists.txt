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

# Place all compiled libraries into OVITO's library directory by default.
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${OVITO_LIBRARY_DIRECTORY}")
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${OVITO_LIBRARY_DIRECTORY}")

# Build all third-party libraries.
ADD_SUBDIRECTORY(3rdparty)

# Build all plugin modules including the core module.
ADD_SUBDIRECTORY(ovito)

# Build the Python script interpreter 'ovitos'.
IF(OVITO_BUILD_GUI AND OVITO_BUILD_PLUGIN_PYSCRIPT)
	ADD_SUBDIRECTORY(ovitos_exe)
ENDIF()

# Build the main application executable 'ovito'.
IF(OVITO_BUILD_GUI)
	ADD_SUBDIRECTORY(ovito_exe)
ENDIF()

# Propagate list of plugins to parent scope.
SET(OVITO_PLUGIN_LIST ${OVITO_PLUGIN_LIST} PARENT_SCOPE)
