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

SET(SourceFiles
	objects/VoxelGrid.cpp
	objects/VoxelGridVis.cpp
	modifier/CreateIsosurfaceModifier.cpp
	modifier/MarchingCubes.cpp
	modifier/SpatialBinningModifier.cpp
	io/VTKVoxelGridExporter.cpp
)

IF(OVITO_BUILD_PLUGIN_STDMOD)
	LIST(APPEND SourceFiles
		modifier/VoxelGridReplicateModifierDelegate.cpp
		modifier/VoxelGridAffineTransformationModifierDelegate.cpp
		modifier/VoxelGridColorCodingModifierDelegate.cpp
		modifier/VoxelGridComputePropertyModifierDelegate.cpp
	)
ENDIF()

# Define the plugin module.
OVITO_STANDARD_PLUGIN(Grid
	SOURCES
		Grid.cpp
		${SourceFiles}
	PLUGIN_DEPENDENCIES Mesh StdObj
	OPTIONAL_PLUGIN_DEPENDENCIES StdMod
)

# Speed up compilation by using precompiled headers.
IF(OVITO_USE_PRECOMPILED_HEADERS)
	ADD_PRECOMPILED_HEADER(Grid ovito/grid/Grid.h)
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
