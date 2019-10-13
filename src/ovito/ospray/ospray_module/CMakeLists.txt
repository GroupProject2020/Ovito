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

# Load OSPRay macros.
INCLUDE(${OSPRAY_USE_FILE})

# Use the Embree library.
INCLUDE_DIRECTORIES("${EMBREE_INCLUDE_DIRS}")
INCLUDE_DIRECTORIES_ISPC("${EMBREE_INCLUDE_DIRS}")

# This builds the ospray extension module which implements raytracing
# functions for disc and cone geometry.

# Note the module name is important: In order for ospray to properly find and
# initialize a module referenced by a call to
# "ospLoadModule(<moduleName>) this module _has_ to
#
# a) be called libospray_module_<modulename>.so, and
# b) contain a (extern C linkage) initializatoin routine named
#    void ospray_init_module_<moduleName>()
#
OSPRAY_ADD_LIBRARY(ospray_module_ovito SHARED
	# the cpp files that contain all the plugin code - parsing
	# parameters in ospCommit(), creating and registering the object,
	# building accel structures, etc
	geometry/Discs.cpp
	geometry/Cones.cpp
	geometry/Quadrics.cpp

	# the ispc files that contain the plugins for all vector code - ie,
	# for ray-primitive intersection and 'postIntersect' (reporting info
	# on a previously computed ray-prim intersection)
	geometry/Discs.ispc
	geometry/Cones.ispc
	geometry/Quadrics.ispc

	# and finally, the module init code (not doing much, but must be there)
	moduleInit.cpp
)

TARGET_LINK_LIBRARIES(ospray_module_ovito PRIVATE "${EMBREE_LIBRARY}")
TARGET_LINK_LIBRARIES(ospray_module_ovito PRIVATE ospray::ospray ospray::ospray_module_ispc)
SET_TARGET_PROPERTIES(ospray_module_ovito PROPERTIES MACOSX_RPATH TRUE)
SET_TARGET_PROPERTIES(ospray_module_ovito PROPERTIES MACOSX_RPATH TRUE)
IF(WIN32)
	TARGET_COMPILE_DEFINITIONS(ospray_module_ovito PRIVATE "NOMINMAX=")
ENDIF()

SET_TARGET_PROPERTIES(ospray_module_ovito PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${Ovito_BINARY_DIR}/${OVITO_RELATIVE_3RDPARTY_LIBRARY_DIRECTORY}")
SET_TARGET_PROPERTIES(ospray_module_ovito PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${Ovito_BINARY_DIR}/${OVITO_RELATIVE_3RDPARTY_LIBRARY_DIRECTORY}")

# Install this target.
INSTALL(TARGETS ospray_module_ovito EXPORT OVITO
	RUNTIME DESTINATION "${OVITO_RELATIVE_3RDPARTY_LIBRARY_DIRECTORY}"
	LIBRARY DESTINATION "${OVITO_RELATIVE_3RDPARTY_LIBRARY_DIRECTORY}"
	ARCHIVE DESTINATION "${OVITO_RELATIVE_3RDPARTY_LIBRARY_DIRECTORY}" COMPONENT "development")
