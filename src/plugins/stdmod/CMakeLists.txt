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

SET(SourceFiles 
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
	viewport/ColorLegendOverlay.cpp	
)

IF(OVITO_BUILD_PLUGIN_PYSCRIPT)
	LIST(APPEND SourceFiles scripting/PythonInterface.cpp)
ENDIF()

OVITO_STANDARD_PLUGIN(StdMod
	SOURCES StdMod.cpp ${SourceFiles}
	PLUGIN_DEPENDENCIES StdObj
	OPTIONAL_PLUGIN_DEPENDENCIES PyScript	
	PYTHON_WRAPPERS "${CMAKE_CURRENT_SOURCE_DIR}/resources/python/"
)

# Speed up compilation by using precompiled headers.
IF(OVITO_USE_PRECOMPILED_HEADERS)
	ADD_PRECOMPILED_HEADER(StdMod plugins/stdmod/StdMod.h)
ENDIF()

# Build corresponding GUI plugin.
IF(OVITO_BUILD_GUI)
	ADD_SUBDIRECTORY(gui)
ENDIF()

# Propagate list of plugins to parent scope.
SET(OVITO_PLUGIN_LIST ${OVITO_PLUGIN_LIST} PARENT_SCOPE)
