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

# This third-party library is needed by the script editor.
FIND_PACKAGE(QScintilla REQUIRED)

# Compile resources.
QT5_ADD_RESOURCES(ResourceFiles resources/pyscript_gui.qrc)

# Define the GUI module, which provides the user interface for the parent module.
OVITO_STANDARD_PLUGIN(PyScriptGui
	SOURCES
		GUIBinding.cpp
		PythonViewportOverlayEditor.cpp
		PythonScriptModifierEditor.cpp
		RunScriptAction.cpp
		ObjectScriptEditor.cpp
		${ResourceFiles}
	PLUGIN_DEPENDENCIES PyScript
	PRIVATE_LIB_DEPENDENCIES QScintilla::QScintilla
	GUI_PLUGIN
)

# Install required libraries. 
IF(APPLE)
	# Nothing to do.
ELSEIF(UNIX)
	IF(OVITO_REDISTRIBUTABLE_PACKAGE)
		# Install a copy of the QScintilla2 library.
		CONFIGURE_FILE("${QScintilla_LIBRARIES}" "${OVITO_LIBRARY_DIRECTORY}" COPYONLY)		
		GET_FILENAME_COMPONENT(real_qscintilla_lib "${QScintilla_LIBRARIES}" REALPATH)
		INSTALL(FILES "${QScintilla_LIBRARIES}" "${real_qscintilla_lib}" DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}/")
	ENDIF()
ELSEIF(WIN32)
	# Install QScintilla DLL.
	GET_FILENAME_COMPONENT(QSCINTILLA_BASE_NAME "${QScintilla_LIBRARIES}" NAME_WE)
	GET_FILENAME_COMPONENT(QSCINTILLA_LIB_PATH "${QScintilla_LIBRARIES}" PATH)
	OVITO_INSTALL_DLL("${QSCINTILLA_LIB_PATH}/${QSCINTILLA_BASE_NAME}.dll")
ENDIF()
