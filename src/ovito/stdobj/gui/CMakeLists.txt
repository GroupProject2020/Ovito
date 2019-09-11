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

# Compile Qt resources to be embedded into the executable.
QT5_ADD_RESOURCES(ResourceFiles resources/stdobj_gui.qrc)

# Define the GUI module, which provides the user interface for the parent module.
OVITO_STANDARD_PLUGIN(StdObjGui
	SOURCES
		StdObjGui.cpp
		camera/CameraObjectEditor.cpp
		io/DataSeriesPlotExporter.cpp
		io/DataSeriesPlotExporterEditor.cpp
		properties/PropertyInspectionApplet.cpp
		properties/PropertyObjectEditor.cpp
		simcell/SimulationCellObjectEditor.cpp
		simcell/SimulationCellVisEditor.cpp
		series/SeriesInspectionApplet.cpp
		widgets/PropertyContainerParameterUI.cpp
		widgets/PropertyReferenceParameterUI.cpp
		widgets/PropertySelectionComboBox.cpp
		widgets/DataSeriesPlotWidget.cpp
		${ResourceFiles}
	PLUGIN_DEPENDENCIES StdObj
	LIB_DEPENDENCIES Qwt
	GUI_PLUGIN
)

# Speed up compilation by using precompiled headers.
IF(OVITO_USE_PRECOMPILED_HEADERS)
	ADD_PRECOMPILED_HEADER(StdObjGui ovito/stdobj/gui/StdObjGui.h)
ENDIF()

# Build corresponding Python binding plugin.
IF(OVITO_BUILD_PLUGIN_PYSCRIPT)
	ADD_SUBDIRECTORY(scripting)
ENDIF()

# Propagate list of plugins to parent scope.
SET(OVITO_PLUGIN_LIST ${OVITO_PLUGIN_LIST} PARENT_SCOPE)
