#######################################################################################
#
#  Copyright 2020 Alexander Stukowski
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

# Compile Qt resources, which will be linked into the plugin.
QT5_ADD_RESOURCES(ResourceFiles resources/gui.qrc)

# Define plugin module.
OVITO_STANDARD_PLUGIN(GuiWeb
	SOURCES
		GUIWeb.cpp
		app/WasmApplication.cpp
		dataset/WasmDataSetContainer.cpp
		dataset/WasmFileManager.cpp
		mainwin/MainWindow.cpp
		mainwin/ViewportsPanel.cpp
		mainwin/ModifierListModel.cpp
		mainwin/PipelineListModel.cpp
		mainwin/PipelineListItem.cpp
		properties/ParameterUI.cpp
		viewport/ViewportWindow.cpp
		${ResourceFiles}
	PLUGIN_DEPENDENCIES
		OpenGLRenderer
		GuiBase
	LIB_DEPENDENCIES
		Qt5::Qml Qt5::Quick Qt5::QuickControls2 Qt5::QuickTemplates2
)

IF(NOT EMSCRIPTEN)
	TARGET_LINK_LIBRARIES(GuiWeb PRIVATE Qt5::Widgets)
ENDIF()

# Speed up compilation by using precompiled headers.
IF(OVITO_USE_PRECOMPILED_HEADERS)
	ADD_PRECOMPILED_HEADER(GuiWeb ovito/gui/web/GUIWeb.h)
ENDIF()
