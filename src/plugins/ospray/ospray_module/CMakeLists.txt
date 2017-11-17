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

# Locate the OSPRay library.
FIND_PACKAGE(ospray REQUIRED)
INCLUDE(${OSPRAY_USE_FILE})

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

OSPRAY_SET_LIBRARY_VERSION(ospray_module_ovito)
TARGET_INCLUDE_DIRECTORIES(ospray_module_ovito PRIVATE "${OSPRAY_INCLUDE_DIRS}")
TARGET_LINK_LIBRARIES(ospray_module_ovito PRIVATE ${OSPRAY_LIBRARIES})
SET_TARGET_PROPERTIES(ospray_module_ovito PROPERTIES MACOSX_RPATH TRUE)

# Export this target.
INSTALL(TARGETS ospray_module_ovito EXPORT OVITO 
	RUNTIME DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}" COMPONENT "runtime"
	LIBRARY DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}" COMPONENT "runtime")
