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
	camera/CameraObjectEditor.cpp
	properties/PropertyInspectionApplet.cpp
	simcell/SimulationCellObjectEditor.cpp
	simcell/SimulationCellVisEditor.cpp
	widgets/PropertyClassParameterUI.cpp
	widgets/PropertyReferenceParameterUI.cpp
	widgets/PropertySelectionComboBox.cpp
)

# Compile resources.
QT5_ADD_RESOURCES(ResourceFiles resources/stdobj_gui.qrc)

OVITO_STANDARD_PLUGIN(StdObjGui
	SOURCES StdObjGui.cpp ${SourceFiles} ${ResourceFiles}
	PLUGIN_DEPENDENCIES StdObj
	GUI_PLUGIN
)

# Speed up compilation by using precompiled headers.
IF(OVITO_USE_PRECOMPILED_HEADERS)
	ADD_PRECOMPILED_HEADER(StdObjGui plugins/stdobj/gui/StdObjGui.h)
ENDIF()