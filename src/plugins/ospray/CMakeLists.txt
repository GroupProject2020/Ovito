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

# Define the plugin module.
OVITO_STANDARD_PLUGIN(OSPRayRenderer SOURCES
	renderer/OSPRayRenderer.cpp
	renderer/OSPRayBackend.cpp
)

# Locate the OSPRay library.
FIND_PACKAGE(ospray 1.8.0 REQUIRED)

# Locate the Embree library.
FIND_PACKAGE(embree 3.3.0 REQUIRED)

# Locate Intel Thread Building Blocks (TBB) library
FIND_PACKAGE(TBB COMPONENTS tbb REQUIRED)

# Link to the OSPRay library and its dependencies.
TARGET_LINK_LIBRARIES(OSPRayRenderer PRIVATE ospray::ospray ospray::ospray_module_ispc)
TARGET_INCLUDE_DIRECTORIES(OSPRayRenderer PRIVATE "${EMBREE_INCLUDE_DIRS}")
TARGET_INCLUDE_DIRECTORIES(OSPRayRenderer PRIVATE "$<TARGET_PROPERTY:TBB::tbb,INTERFACE_INCLUDE_DIRECTORIES>")
SET_TARGET_PROPERTIES(OSPRayRenderer PROPERTIES MACOSX_RPATH TRUE)

# Build our extension module for the OSPRay system.
ADD_SUBDIRECTORY(ospray_module)
ADD_DEPENDENCIES(OSPRayRenderer ospray_module_ovito)

# Move the OSPRay shared libraries to the directory where the OSPRay extension module is being built.
# This is needed, because at runtime OSPRay will search for the extension module in directory containing libospray itself.
GET_TARGET_PROPERTY(OSPRAY_LIBRARY ospray::ospray LOCATION)
GET_TARGET_PROPERTY(OSPRAY_COMMON_LIBRARY ospray::ospray_common LOCATION)
GET_TARGET_PROPERTY(OSPRAY_MODULE_ISPC_LIBRARY ospray::ospray_module_ispc LOCATION)
OVITO_INSTALL_SHARED_LIB("${OSPRAY_LIBRARY}" "${OVITO_RELATIVE_LIBRARY_DIRECTORY}")
OVITO_INSTALL_SHARED_LIB("${OSPRAY_COMMON_LIBRARY}" "${OVITO_RELATIVE_LIBRARY_DIRECTORY}")
OVITO_INSTALL_SHARED_LIB("${OSPRAY_MODULE_ISPC_LIBRARY}" "${OVITO_RELATIVE_LIBRARY_DIRECTORY}")

# Linux:
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
