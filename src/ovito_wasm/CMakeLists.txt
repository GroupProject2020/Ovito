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

# Builds the main executable for the WebAssembly platform.
ADD_EXECUTABLE(Ovito 
	Main.cpp
)

# Copy in Qt HTML/JS launch files.
SET(APPNAME "ovito")
CONFIGURE_FILE("${_qt5Core_install_prefix}/plugins/platforms/wasm_shell.html" "${OVITO_BINARY_DIRECTORY}/Ovito.html")
CONFIGURE_FILE("${_qt5Core_install_prefix}/plugins/platforms/qtloader.js" "${OVITO_BINARY_DIRECTORY}/qtloader.js" COPYONLY)
CONFIGURE_FILE("${_qt5Core_install_prefix}/plugins/platforms/qtlogo.svg" "${OVITO_BINARY_DIRECTORY}/qtlogo.svg" COPYONLY)

# The executable depends on the core and the gui modules, of course.
TARGET_LINK_LIBRARIES(Ovito PRIVATE Core GuiWasm)

# Put the executable into the right directory.
SET_TARGET_PROPERTIES(Ovito PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${OVITO_BINARY_DIRECTORY}")

# Set name of executable.
SET_TARGET_PROPERTIES(Ovito PROPERTIES OUTPUT_NAME "${APPNAME}")

# Link Qt5.
TARGET_LINK_LIBRARIES(Ovito PRIVATE Qt5::Core Qt5::Gui Qt5::Qml Qt5::Quick)

# Link to missing Qt libraries (the order does matter).
TARGET_LINK_LIBRARIES(Ovito PRIVATE
	"${_qt5Core_install_prefix}/qml/QtQuick.2/libqtquick2plugin.a"      # QtQuick2Plugin
	"${_qt5Core_install_prefix}/qml/QtQuick/Window.2/libwindowplugin.a" # QtQuick2WindowPlugin
	"${_qt5Core_install_prefix}/plugins/platforms/libqwasm.a"       	# No longer needed in Qt 5.14
	"${_qt5Core_install_prefix}/lib/libQt5Quick.a"						# No longer needed in Qt 5.14
	"${_qt5Core_install_prefix}/lib/libQt5EventDispatcherSupport.a"		# No longer needed in Qt 5.14
    "${_qt5Core_install_prefix}/lib/libQt5FontDatabaseSupport.a"		# No longer needed in Qt 5.14
    "${_qt5Core_install_prefix}/lib/libqtfreetype.a"					# No longer needed in Qt 5.14
)

# This executable will be part of the installation package.
INSTALL(TARGETS Ovito DESTINATION "${OVITO_RELATIVE_BINARY_DIRECTORY}/")

# Build all plugins first before building the main executable.
FOREACH(plugin ${OVITO_PLUGIN_LIST})
	IF(BUILD_SHARED_LIBS)
		ADD_DEPENDENCIES(Ovito ${plugin})
	ELSE()
		TARGET_LINK_LIBRARIES(Ovito PRIVATE ${plugin})
	ENDIF()
ENDFOREACH()
