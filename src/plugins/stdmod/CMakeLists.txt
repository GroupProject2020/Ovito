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
OVITO_STANDARD_PLUGIN(StdMod
	SOURCES
		StdMod.cpp
		modifiers/SliceModifier.cpp
		modifiers/AffineTransformationModifier.cpp
		modifiers/InvertSelectionModifier.cpp
		modifiers/ClearSelectionModifier.cpp
		modifiers/ColorCodingModifier.cpp
		modifiers/AssignColorModifier.cpp
		modifiers/DeleteSelectedModifier.cpp
		modifiers/SelectTypeModifier.cpp
		modifiers/HistogramModifier.cpp
		modifiers/ScatterPlotModifier.cpp
		modifiers/ReplicateModifier.cpp
		modifiers/ExpressionSelectionModifier.cpp
		modifiers/FreezePropertyModifier.cpp
		modifiers/ManualSelectionModifier.cpp
		modifiers/ComputePropertyModifier.cpp
		modifiers/CombineDatasetsModifier.cpp
		viewport/ColorLegendOverlay.cpp	
	PLUGIN_DEPENDENCIES StdObj
)

# Speed up compilation by using precompiled headers.
IF(OVITO_USE_PRECOMPILED_HEADERS)
	ADD_PRECOMPILED_HEADER(StdMod plugins/stdmod/StdMod.h)
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
