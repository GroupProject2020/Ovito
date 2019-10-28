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

# Locate the OSPRay library.
FIND_PACKAGE(ospray 1.8.0 REQUIRED)

# Locate the Embree library.
FIND_PACKAGE(embree 3.3.0 REQUIRED)

# Locate Intel Thread Building Blocks (TBB) library
FIND_PACKAGE(TBB COMPONENTS tbb tbbmalloc REQUIRED)

# Define the plugin module.
OVITO_STANDARD_PLUGIN(OSPRayRenderer
	SOURCES
		renderer/OSPRayRenderer.cpp
		renderer/OSPRayBackend.cpp
	PRIVATE_LIB_DEPENDENCIES
		ospray::ospray
		ospray::ospray_module_ispc
		"${EMBREE_LIBRARIES}"
)

# Link to the Embree library and its dependencies.
TARGET_INCLUDE_DIRECTORIES(OSPRayRenderer PRIVATE "${EMBREE_INCLUDE_DIRS}")
TARGET_INCLUDE_DIRECTORIES(OSPRayRenderer PRIVATE "$<TARGET_PROPERTY:TBB::tbb,INTERFACE_INCLUDE_DIRECTORIES>")
SET_TARGET_PROPERTIES(OSPRayRenderer PROPERTIES MACOSX_RPATH TRUE)

# Avoid conflict with Windows system header.
IF(WIN32)
	TARGET_COMPILE_DEFINITIONS(OSPRayRenderer PRIVATE "NOMINMAX=")
ENDIF()

# Build our extension module for the OSPRay system.
ADD_SUBDIRECTORY(ospray_module)
ADD_DEPENDENCIES(OSPRayRenderer ospray_module_ovito)

IF(NOT APPLE)
	# Move the OSPRay shared libraries to the directory where the OSPRay extension module is being built.
	# This is needed, because at runtime OSPRay will search for the extension module in directory containing libospray itself.
	GET_TARGET_PROPERTY(OSPRAY_LIBRARY ospray::ospray LOCATION)
	GET_TARGET_PROPERTY(OSPRAY_COMMON_LIBRARY ospray::ospray_common LOCATION)
	GET_TARGET_PROPERTY(OSPRAY_MODULE_ISPC_LIBRARY ospray::ospray_module_ispc LOCATION)
	OVITO_INSTALL_SHARED_LIB("${OSPRAY_LIBRARY}" ".")
	OVITO_INSTALL_SHARED_LIB("${OSPRAY_COMMON_LIBRARY}" ".")
	OVITO_INSTALL_SHARED_LIB("${OSPRAY_MODULE_ISPC_LIBRARY}" ".")
ELSE()
	INSTALL(CODE "
		EXECUTE_PROCESS(COMMAND \"\${CMAKE_COMMAND}\" -E create_symlink \"libospray_module_ispc.0.dylib\" \"\${CMAKE_INSTALL_PREFIX}/${OVITO_RELATIVE_3RDPARTY_LIBRARY_DIRECTORY}/libospray_module_ispc.dylib\")
	")
ENDIF()

# Linux:
IF(UNIX AND NOT APPLE AND (OVITO_REDISTRIBUTABLE_PACKAGE OR OVITO_BUILD_PYTHON_PACKAGE))
	# Deploy Embree shared libraries.
	FOREACH(lib ${EMBREE_LIBRARIES})
		OVITO_INSTALL_SHARED_LIB("${lib}" ".")
	ENDFOREACH()
	# Deploy Thread Building Blocks (TBB) shared libraries.
	OVITO_INSTALL_SHARED_LIB("${TBB_LIBRARY}" ".")
	OVITO_INSTALL_SHARED_LIB("${TBB_LIBRARY_MALLOC}" ".")
ENDIF()

# Windows:
IF(WIN32)
	# Deploy TBB DLLs.
	GET_TARGET_PROPERTY(TBB_DLL_LOCATION TBB::tbb IMPORTED_LOCATION_RELEASE)
	OVITO_INSTALL_SHARED_LIB("${TBB_DLL_LOCATION}" ".")
	GET_TARGET_PROPERTY(TBBMALLOC_DLL_LOCATION TBB::tbbmalloc IMPORTED_LOCATION_RELEASE)
	OVITO_INSTALL_SHARED_LIB("${TBBMALLOC_DLL_LOCATION}" ".")

	# Deploy Embree DLL.
	OVITO_INSTALL_SHARED_LIB("${EMBREE_ROOT_DIR}/bin/embree3.dll" ".")
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
