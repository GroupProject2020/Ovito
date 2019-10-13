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
	OVITO_INSTALL_SHARED_LIB("${QSCINTILLA_LIB_PATH}/${QSCINTILLA_BASE_NAME}.dll" ".")
ENDIF()
