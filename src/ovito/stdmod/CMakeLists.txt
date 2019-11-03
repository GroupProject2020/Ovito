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
	PLUGIN_DEPENDENCIES
		StdObj
		Mesh
)

# Speed up compilation by using precompiled headers.
IF(OVITO_USE_PRECOMPILED_HEADERS)
	ADD_PRECOMPILED_HEADER(StdMod ovito/stdmod/StdMod.h)
ENDIF()

# Build corresponding GUI plugin.
IF(OVITO_BUILD_GUI)
	ADD_SUBDIRECTORY(gui)
ENDIF()

# Propagate list of plugins to parent scope.
SET(OVITO_PLUGIN_LIST ${OVITO_PLUGIN_LIST} PARENT_SCOPE)
