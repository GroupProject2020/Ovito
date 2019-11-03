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

# Create the plugins library and manifest subfolder in the build directory.
FILE(MAKE_DIRECTORY "${OVITO_PLUGINS_DIRECTORY}")

ADD_SUBDIRECTORY(core)

ADD_SUBDIRECTORY(opengl)

IF(OVITO_BUILD_GUI)
	ADD_SUBDIRECTORY(gui)
ENDIF()

IF(OVITO_BUILD_PLUGIN_STDOBJ)
	ADD_SUBDIRECTORY(stdobj)
ENDIF()

IF(OVITO_BUILD_PLUGIN_MESH)
	ADD_SUBDIRECTORY(mesh)
ENDIF()

IF(OVITO_BUILD_PLUGIN_STDMOD)
	ADD_SUBDIRECTORY(stdmod)
ENDIF()

IF(OVITO_BUILD_PLUGIN_MESH AND OVITO_BUILD_PLUGIN_STDMOD)
	ADD_SUBDIRECTORY(mesh/modifier)
ENDIF()

IF(OVITO_BUILD_PLUGIN_GRID)
	ADD_SUBDIRECTORY(grid)
ENDIF()

IF(OVITO_BUILD_PLUGIN_PARTICLES)
	ADD_SUBDIRECTORY(delaunay)
	ADD_SUBDIRECTORY(particles)
ENDIF()

IF(OVITO_BUILD_PLUGIN_CRYSTALANALYSIS)
	ADD_SUBDIRECTORY(crystalanalysis)
ENDIF()

IF(OVITO_BUILD_PLUGIN_NETCDFPLUGIN)
	ADD_SUBDIRECTORY(netcdf)
ENDIF()

IF(OVITO_BUILD_PLUGIN_GALAMOST)
	ADD_SUBDIRECTORY(galamost)
ENDIF()

IF(OVITO_BUILD_PLUGIN_VR)
	ADD_SUBDIRECTORY(vr)
ENDIF()

IF(OVITO_BUILD_PLUGIN_CORRELATION)
	ADD_SUBDIRECTORY(correlation)
ENDIF()

IF(OVITO_BUILD_PLUGIN_VOROTOP)
	ADD_SUBDIRECTORY(vorotop)
ENDIF()

# Propagate list of plugins to parent scope.
SET(OVITO_PLUGIN_LIST ${OVITO_PLUGIN_LIST} PARENT_SCOPE)
