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

# Define the GUI module, which provides the user interface for the parent module.
OVITO_STANDARD_PLUGIN(StdModGui
	SOURCES
		StdModGui.cpp
		SliceModifierEditor.cpp
		AffineTransformationModifierEditor.cpp
		ClearSelectionModifierEditor.cpp
		InvertSelectionModifierEditor.cpp
		ColorCodingModifierEditor.cpp
		AssignColorModifierEditor.cpp
		DeleteSelectedModifierEditor.cpp
		SelectTypeModifierEditor.cpp
		HistogramModifierEditor.cpp
		ScatterPlotModifierEditor.cpp
		ReplicateModifierEditor.cpp
		ColorLegendOverlayEditor.cpp
		ExpressionSelectionModifierEditor.cpp
		FreezePropertyModifierEditor.cpp
		ManualSelectionModifierEditor.cpp
		ComputePropertyModifierEditor.cpp
		CombineDatasetsModifierEditor.cpp
	PLUGIN_DEPENDENCIES StdMod StdObjGui
	PRIVATE_LIB_DEPENDENCIES Qwt
	GUI_PLUGIN
)

# Speed up compilation by using precompiled headers.
IF(OVITO_USE_PRECOMPILED_HEADERS)
	ADD_PRECOMPILED_HEADER(StdModGui ovito/stdmod/gui/StdModGui.h)
ENDIF()