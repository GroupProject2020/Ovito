###############################################################################
# 
#  Copyright (2018) Alexander Stukowski
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
	mainwin/MainWindow.cpp
	mainwin/ViewportsPanel.cpp
	mainwin/TaskDisplayWidget.cpp
	mainwin/cmdpanel/CommandPanel.cpp
	mainwin/cmdpanel/RenderCommandPage.cpp
	mainwin/cmdpanel/ModifyCommandPage.cpp
	mainwin/cmdpanel/PipelineListModel.cpp
	mainwin/cmdpanel/PipelineListItem.cpp
	mainwin/cmdpanel/ModifierListBox.cpp
	mainwin/cmdpanel/OverlayCommandPage.cpp
	mainwin/data_inspector/DataInspectorPanel.cpp
	mainwin/data_inspector/DataInspectionApplet.cpp
	mainwin/data_inspector/GlobalAttributesInspectionApplet.cpp
	dialogs/AdjustCameraDialog.cpp
	dialogs/AnimationKeyEditorDialog.cpp
	dialogs/AnimationSettingsDialog.cpp
	dialogs/ApplicationSettingsDialog.cpp
	dialogs/GeneralSettingsPage.cpp
	dialogs/ViewportSettingsPage.cpp
	dialogs/ModifierTemplatesPage.cpp
	dialogs/HistoryFileDialog.cpp
	dialogs/ImportFileDialog.cpp
	dialogs/ImportRemoteFileDialog.cpp
	dialogs/SaveImageFileDialog.cpp
	dialogs/LoadImageFileDialog.cpp
	dialogs/RemoteAuthenticationDialog.cpp
	dialogs/FileExporterSettingsDialog.cpp
	dialogs/ClonePipelineDialog.cpp
	dialogs/FontSelectionDialog.cpp
	actions/ActionManager.cpp
	actions/FileActions.cpp
	actions/ViewportActions.cpp
	actions/AnimationActions.cpp
	actions/RenderActions.cpp
	actions/ViewportModeAction.cpp
	dataset/GuiDataSetContainer.cpp
	dataset/io/FileSourceEditor.cpp
	dataset/io/FileImporterEditor.cpp
	dataset/io/AttributeFileExporterEditor.cpp
	dataset/animation/controller/TCBInterpolationControllerEditor.cpp
	widgets/general/RolloutContainer.cpp
	widgets/general/RolloutContainerLayout.cpp
	widgets/general/SpinnerWidget.cpp
	widgets/general/ColorPickerWidget.cpp
	widgets/general/ElidedTextLabel.cpp
	widgets/general/AutocompleteLineEdit.cpp
	widgets/general/AutocompleteTextEdit.cpp
	widgets/general/HtmlListWidget.cpp
	widgets/animation/AnimationTimeSpinner.cpp
	widgets/animation/AnimationFramesToolButton.cpp
	widgets/animation/AnimationTimeSlider.cpp
	widgets/animation/AnimationTrackBar.cpp
	widgets/rendering/FrameBufferWidget.cpp
	widgets/rendering/FrameBufferWindow.cpp
	widgets/selection/SceneNodesListModel.cpp
	widgets/selection/SceneNodeSelectionBox.cpp
	widgets/display/CoordinateDisplayWidget.cpp
	widgets/display/StatusWidget.cpp
	properties/PropertiesEditor.cpp
	properties/PropertiesPanel.cpp
	properties/ParameterUI.cpp
	properties/NumericalParameterUI.cpp
	properties/AffineTransformationParameterUI.cpp
	properties/BooleanActionParameterUI.cpp
	properties/BooleanGroupBoxParameterUI.cpp
	properties/BooleanParameterUI.cpp
	properties/BooleanRadioButtonParameterUI.cpp
	properties/ColorParameterUI.cpp
	properties/CustomParameterUI.cpp
	properties/DefaultPropertiesEditor.cpp
	properties/FilenameParameterUI.cpp
	properties/FloatParameterUI.cpp
	properties/FontParameterUI.cpp
	properties/IntegerParameterUI.cpp
	properties/IntegerRadioButtonParameterUI.cpp
	properties/ModifierPropertiesEditor.cpp
	properties/ModifierDelegateParameterUI.cpp
	properties/ModifierDelegateListParameterUI.cpp
	properties/RefTargetListParameterUI.cpp
	properties/StringParameterUI.cpp
	properties/SubObjectParameterUI.cpp
	properties/VariantComboBoxParameterUI.cpp
	properties/Vector3ParameterUI.cpp
	rendering/ViewportSceneRenderer.cpp
	rendering/RenderSettingsEditor.cpp
	viewport/ViewportWindow.cpp
	viewport/ViewportMenu.cpp
	viewport/input/ViewportInputManager.cpp
	viewport/input/ViewportInputMode.cpp
	viewport/input/NavigationModes.cpp
	viewport/input/XFormModes.cpp
	viewport/picking/PickingSceneRenderer.cpp
	viewport/overlays/CoordinateTripodOverlayEditor.cpp
	viewport/overlays/TextLabelOverlayEditor.cpp
	viewport/overlays/MoveOverlayInputMode.cpp
	app/GuiApplication.cpp
	app/GuiApplicationService.cpp
	utilities/io/GuiFileManager.cpp
	utilities/concurrent/ProgressDialog.cpp
)

# Compile resources.
QT5_ADD_RESOURCES(ResourceFiles resources/gui.qrc)

# Build library
ADD_LIBRARY(Gui ${OVITO_DEFAULT_LIBRARY_TYPE} GUI.cpp ${SourceFiles} ${ResourceFiles})

# Pass name of gui plugin to the code.
TARGET_COMPILE_DEFINITIONS(Gui PRIVATE "OVITO_PLUGIN_NAME=\"Gui\"")

# Set default include directory.
TARGET_INCLUDE_DIRECTORIES(Gui PUBLIC "$<BUILD_INTERFACE:${OVITO_SOURCE_BASE_DIR}/src>")

# GUI is based on Core and the OpenGL rendering library.
TARGET_LINK_LIBRARIES(Gui PUBLIC Core OpenGLRenderer)

# Link Qt5.
TARGET_LINK_LIBRARIES(Gui PUBLIC Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Network)

# Define macro for symbol export from shared library.
IF(OVITO_BUILD_MONOLITHIC)
	TARGET_COMPILE_DEFINITIONS(Gui PUBLIC "OVITO_GUI_EXPORT=")
ELSE()
	TARGET_COMPILE_DEFINITIONS(Gui PRIVATE "OVITO_GUI_EXPORT=Q_DECL_EXPORT")
	TARGET_COMPILE_DEFINITIONS(Gui INTERFACE "OVITO_GUI_EXPORT=Q_DECL_IMPORT")
ENDIF()

# Set visibility of symbols in this shared library to hidden by default, except those exported in the source code.
SET_TARGET_PROPERTIES(Gui PROPERTIES CXX_VISIBILITY_PRESET "hidden")
SET_TARGET_PROPERTIES(Gui PROPERTIES VISIBILITY_INLINES_HIDDEN ON)

# Speed up compilation by using precompiled headers.
IF(OVITO_USE_PRECOMPILED_HEADERS)
	ADD_PRECOMPILED_HEADER(Gui gui/GUI.h)
ENDIF()

IF(APPLE)
    # Activate @rpath on MacOS.
    SET_TARGET_PROPERTIES(Gui PROPERTIES MACOSX_RPATH TRUE)
	# This is required to avoid error by install_name_tool on MacOS.
	SET_TARGET_PROPERTIES(Gui PROPERTIES LINK_FLAGS "-headerpad_max_install_names")
ELSEIF(UNIX)
    SET_TARGET_PROPERTIES(Gui PROPERTIES INSTALL_RPATH "$ORIGIN")
ENDIF()
    
# This library is part of the installation package.
INSTALL(TARGETS Gui EXPORT OVITO
	RUNTIME DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}" COMPONENT "runtime"
	LIBRARY DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}" COMPONENT "runtime"
	ARCHIVE DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}" COMPONENT "development")
