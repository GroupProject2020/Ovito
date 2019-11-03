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

# Propagate list of plugins to parent scope.
SET(OVITO_PLUGIN_LIST ${OVITO_PLUGIN_LIST} PARENT_SCOPE)
