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

# Include the resource in the Win32 executable.
IF(WIN32)
	SET(WINDOWS_RESOURCES resources/ovito.rc)
ENDIF(WIN32)

IF(CMAKE_BUILD_TYPE STREQUAL "Release")
	# This will create a GUI application on Windows platform.
	SET(OVITO_WIN32_EXECUTABLE "WIN32")
ELSE()
	# This will create a console application on Windows platform.
	SET(OVITO_WIN32_EXECUTABLE "")
ENDIF()

# Builds the main executable of the application
ADD_EXECUTABLE(Ovito ${OVITO_WIN32_EXECUTABLE} Main.cpp ${WINDOWS_RESOURCES})

# The executable depends on the core and the gui modules, of course.
TARGET_LINK_LIBRARIES(Ovito PRIVATE Core Gui)

# Put the executable into the right directory.
IF(APPLE AND NOT OVITO_BUILD_CONDA)
	SET_TARGET_PROPERTIES(Ovito PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${Ovito_BINARY_DIR}")
	INSTALL(TARGETS Ovito DESTINATION ".")
ELSEIF(UNIX)
	SET_TARGET_PROPERTIES(Ovito PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${Ovito_BINARY_DIR}/bin")
	INSTALL(TARGETS Ovito DESTINATION "bin/")
ELSEIF(WIN32)
	SET_TARGET_PROPERTIES(Ovito PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${Ovito_BINARY_DIR}")
	INSTALL(TARGETS Ovito DESTINATION ".")
ENDIF()

# Set name of executable.
SET_TARGET_PROPERTIES(Ovito PROPERTIES OUTPUT_NAME "ovito")

# Link Qt5.
TARGET_LINK_LIBRARIES(Ovito PRIVATE Qt5::Core Qt5::Gui)

# Under Linux, Qt's Xcb platform plugin will dynamically load the DBus Qt module.
# To avoid the dynamic linker pulling the wrong versions of the Qt module from the
# system path, we make the executable preload the module on application startup.
IF(UNIX AND NOT APPLE AND OVITO_REDISTRIBUTABLE_PACKAGE)
	TARGET_LINK_LIBRARIES(Ovito PRIVATE Qt5::DBus)
ENDIF()

SET_TARGET_PROPERTIES(Ovito PROPERTIES BUILD_WITH_INSTALL_RPATH TRUE)
IF(APPLE)
	IF(NOT OVITO_BUILD_CONDA)
	    SET_TARGET_PROPERTIES(Ovito PROPERTIES MACOSX_BUNDLE TRUE)
		SET_TARGET_PROPERTIES(Ovito PROPERTIES INSTALL_RPATH "@executable_path/;@executable_path/../Frameworks/;@executable_path/../PlugIns/")
	ELSE()
		SET_TARGET_PROPERTIES(Ovito PROPERTIES MACOSX_BUNDLE FALSE)
		SET_TARGET_PROPERTIES(Ovito PROPERTIES INSTALL_RPATH "@loader_path/;@loader_path/../${OVITO_RELATIVE_PLUGINS_DIRECTORY}/")
	ENDIF()
ELSEIF(UNIX)
	SET_TARGET_PROPERTIES(Ovito PROPERTIES INSTALL_RPATH "$ORIGIN/../${OVITO_RELATIVE_LIBRARY_DIRECTORY}/:$ORIGIN/../${OVITO_RELATIVE_PLUGINS_DIRECTORY}/")
ENDIF()

# Build all plugins first before building the main executable.
FOREACH(plugin ${OVITO_PLUGIN_LIST})
	IF(BUILD_SHARED_LIBS)
		ADD_DEPENDENCIES(Ovito ${plugin})
	ELSE()
		# When generating a static executable that contains all plugins, make sure
		# the linker doesn't remove apparently unreferenced symbols. They are needed to
		# have the plugin classes register themselves.
		IF(UNIX)
			TARGET_LINK_LIBRARIES(Ovito PRIVATE -Wl,--whole-archive ${plugin} -Wl,--no-whole-archive)
		ELSE()
			TARGET_LINK_LIBRARIES(Ovito PRIVATE ${plugin})
			IF(MSVC)
				SET_PROPERTY(TARGET Ovito APPEND_STRING PROPERTY LINK_FLAGS " /WHOLEARCHIVE:${plugin}")
			ENDIF()
		ENDIF()
	ENDIF()
ENDFOREACH()

# Install fonts required by 'minimal' platform plugin on Linux
IF(UNIX AND NOT APPLE AND NOT OVITO_BUILD_CONDA)
    INSTALL(DIRECTORY "resources/fonts" DESTINATION "${OVITO_RELATIVE_SHARE_DIRECTORY}/")
ENDIF()

IF(UNIX AND NOT APPLE AND OVITO_REDISTRIBUTABLE_PACKAGE)

	# Create a qt.conf file.
	# This is required to let Qt search for its plugin modules in the program directory.
	INSTALL(CODE "
	    FILE(WRITE \"\${CMAKE_INSTALL_PREFIX}/${OVITO_RELATIVE_BINARY_DIRECTORY}/qt.conf\" \"[Paths]\\nPlugins = ../${OVITO_RELATIVE_LIBRARY_DIRECTORY}/plugins_qt/\")
	    ")

ELSEIF(WIN32)

	# Create a qt.conf file.
	# This is required to let Qt search for its plugin modules in the program directory.
	INSTALL(CODE "
	    FILE(WRITE \"\${CMAKE_INSTALL_PREFIX}/${OVITO_RELATIVE_BINARY_DIRECTORY}/qt.conf\" \"[Paths]\\nPlugins = plugins/\")
		")

ELSEIF(APPLE AND NOT OVITO_BUILD_CONDA)

	# Install the Info.plist file in the app bundle.
	CONFIGURE_FILE("resources/Info.plist" "${Ovito_BINARY_DIR}/${MACOSX_BUNDLE_NAME}.app/Contents/Info.plist")
	SET_TARGET_PROPERTIES(Ovito PROPERTIES MACOSX_BUNDLE_INFO_PLIST "${Ovito_BINARY_DIR}/${MACOSX_BUNDLE_NAME}.app/Contents/Info.plist")

	# Copy the application icon into the resource directory.
	INSTALL(FILES "resources/ovito.icns" DESTINATION "${OVITO_RELATIVE_SHARE_DIRECTORY}")

	# Fix Mac app bundle.
	INCLUDE("${Ovito_SOURCE_DIR}/cmake/FixupMacBundle.cmake")

ENDIF()

# Special steps needed to build OVITO with Python scripting support.
IF(OVITO_BUILD_PLUGIN_PYSCRIPT)

	# Make sure the Python library gets loaded at program startup. We need to do it
	# this way, because the OVITO scripting plugins do not explicitly link the
	# Python interpreter library. They rely on it already being loaded into the 
	# process memory (as is the case if the OVITO Python modules are imported by an external interpreter).
	FIND_PACKAGE(Python3 REQUIRED COMPONENTS Interpreter Development)
	TARGET_LINK_LIBRARIES(Ovito PRIVATE Python3::Python)
	TARGET_COMPILE_DEFINITIONS(Ovito PRIVATE "OVITO_BUILD_PLUGIN_PYSCRIPT=1")

	IF(OVITO_REDISTRIBUTABLE_PACKAGE AND UNIX AND NOT APPLE)
		# Uninstall Python packages from the embedded interpreter that should not be part of the official distribution.
		INSTALL(CODE "${OVITO_UNINSTALL_UNUSED_PYTHON_MODULES_CODE}")
	ENDIF()
ENDIF()