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
)

OVITO_STANDARD_PLUGIN(StdModGui
	SOURCES StdModGui.cpp ${SourceFiles}
	PLUGIN_DEPENDENCIES StdMod StdObjGui
	PRIVATE_LIB_DEPENDENCIES Qwt
	GUI_PLUGIN
)

# Speed up compilation by using precompiled headers.
IF(OVITO_USE_PRECOMPILED_HEADERS)
	ADD_PRECOMPILED_HEADER(StdModGui plugins/stdmod/gui/StdModGui.h)
ENDIF()