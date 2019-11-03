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

# Place all compiled libraries into OVITO's library directory by default.
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${OVITO_LIBRARY_DIRECTORY}")
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${OVITO_LIBRARY_DIRECTORY}")

# Build all third-party libraries.
ADD_SUBDIRECTORY(3rdparty)

# Build all plugin modules including the core module.
ADD_SUBDIRECTORY(ovito)

# This allows external CMake projects to inject their customization code into the OVITO project:
IF(OVITO_CMAKE_FILE_INJECTION_PLUGINS)
	INCLUDE("${OVITO_CMAKE_FILE_INJECTION_PLUGINS}")
ENDIF()

# Build the main application executable 'ovito'.
IF(OVITO_BUILD_GUI)
	ADD_SUBDIRECTORY(ovito_exe)
ENDIF()

# Propagate list of plugins to parent scope.
SET(OVITO_PLUGIN_LIST ${OVITO_PLUGIN_LIST} PARENT_SCOPE)
