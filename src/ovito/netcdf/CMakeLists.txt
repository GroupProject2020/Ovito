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

IF(NOT TARGET NetCDFIntegration)
	MESSAGE(FATAL_ERROR 
		"The NetCDF file reader/writer plugin cannot be built without the HDF5 and NetCDF libraries. "
		"Please disable the plugin by setting OVITO_BUILD_PLUGIN_NETCDFPLUGIN to OFF, "
		"or make sure the required HDF5 and NetCDF libraries are found by the CMake script.")
ENDIF()

# Define the plugin module.
OVITO_STANDARD_PLUGIN(NetCDFPlugin
	SOURCES
		AMBERNetCDFImporter.cpp
		AMBERNetCDFExporter.cpp
	PLUGIN_DEPENDENCIES Particles
	PRIVATE_LIB_DEPENDENCIES NetCDFIntegration
)

# Build corresponding GUI plugin.
IF(OVITO_BUILD_GUI)
	ADD_SUBDIRECTORY(gui)
ENDIF()

# Propagate list of plugins to parent scope.
SET(OVITO_PLUGIN_LIST ${OVITO_PLUGIN_LIST} PARENT_SCOPE)
