###############################################################################
# 
#  Copyright (2017) Alexander Stukowski
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

# Define the plugin module.
OVITO_STANDARD_PLUGIN(OSPRayRenderer SOURCES 
	renderer/OSPRayRenderer.cpp
	renderer/OSPRayBackend.cpp
)

# Locate the OSPRay library.
FIND_PACKAGE(ospray REQUIRED)

# Link to the OSPRay library and its dependencies.
TARGET_INCLUDE_DIRECTORIES(OSPRayRenderer PRIVATE ${OSPRAY_INCLUDE_DIRS} "${TBB_INCLUDE_DIR}")
TARGET_LINK_LIBRARIES(OSPRayRenderer PRIVATE ${OSPRAY_LIBRARIES})

# Build our extension module for the OSPRay system.
ADD_SUBDIRECTORY(ospray_module)
ADD_DEPENDENCIES(OSPRayRenderer ospray_module_ovito)

IF(UNIX AND NOT APPLE AND OVITO_REDISTRIBUTABLE_PACKAGE)

	# Deploy OSPRay and Embree shared libraries.
	FOREACH(lib ${OSPRAY_LIBRARIES})
		OVITO_INSTALL_SHARED_LIB("${lib}" "${OVITO_RELATIVE_LIBRARY_DIRECTORY}")
	ENDFOREACH()
	
	OVITO_INSTALL_SHARED_LIB("${TBB_LIBRARY}" "${OVITO_RELATIVE_LIBRARY_DIRECTORY}")
	OVITO_INSTALL_SHARED_LIB("${TBB_LIBRARY_MALLOC}" "${OVITO_RELATIVE_LIBRARY_DIRECTORY}")

ENDIF()

# Build corresponding GUI plugin.
IF(OVITO_BUILD_GUI)
	ADD_SUBDIRECTORY(gui)
ENDIF()

# Build corresponding Python binding plugin.
IF(OVITO_BUILD_PLUGIN_PYSCRIPT)
	ADD_SUBDIRECTORY(scripting)
ENDIF()

# Propagate list of plugins to parent scope.
SET(OVITO_PLUGIN_LIST ${OVITO_PLUGIN_LIST} PARENT_SCOPE)
