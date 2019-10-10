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
ADD_EXECUTABLE(Ovito ${OVITO_WIN32_EXECUTABLE} MACOSX_BUNDLE Main.cpp ${WINDOWS_RESOURCES})

# The executable depends on the core and the gui modules, of course.
TARGET_LINK_LIBRARIES(Ovito Core Gui)

# Put the executable into the right directory.
SET_TARGET_PROPERTIES(Ovito PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${OVITO_BINARY_DIRECTORY}")

# Set name of executable.
SET_TARGET_PROPERTIES(Ovito PROPERTIES OUTPUT_NAME "ovito")

# Link Qt5.
TARGET_LINK_LIBRARIES(Ovito Qt5::Core Qt5::Gui)

# Under Linux, Qt's Xcb platform plugin will dynamically load the DBus Qt module.
# To avoid the dynamic linker pulling the wrong versions of the Qt module from the
# system path, we make the executable preload the module on application startup.
IF(UNIX AND NOT APPLE AND OVITO_REDISTRIBUTABLE_PACKAGE)
	TARGET_LINK_LIBRARIES(Ovito Qt5::DBus)
ENDIF()

IF(APPLE)
    # Enable the use of @rpath on macOS.
    SET_TARGET_PROPERTIES(Ovito PROPERTIES MACOSX_RPATH TRUE)
    SET_TARGET_PROPERTIES(Ovito PROPERTIES INSTALL_RPATH "@executable_path/;@executable_path/../Frameworks/;@executable_path/../PlugIns/")
	SET_TARGET_PROPERTIES(Ovito PROPERTIES BUILD_WITH_INSTALL_RPATH TRUE)
ELSEIF(UNIX)
    SET_TARGET_PROPERTIES(Ovito PROPERTIES INSTALL_RPATH "$ORIGIN/../${OVITO_RELATIVE_LIBRARY_DIRECTORY}/:$ORIGIN/../${OVITO_RELATIVE_PLUGINS_DIRECTORY}/")
	SET_TARGET_PROPERTIES(Ovito PROPERTIES BUILD_WITH_INSTALL_RPATH TRUE)
ENDIF()

# This executable will be part of the installation package.
INSTALL(TARGETS Ovito DESTINATION "${OVITO_RELATIVE_BINARY_DIRECTORY}/")

# Build all plugins first before building the main executable.
FOREACH(plugin ${OVITO_PLUGIN_LIST})
	IF(BUILD_SHARED_LIBS)
		ADD_DEPENDENCIES(Ovito ${plugin})
	ELSE()
		IF(UNIX)
			TARGET_LINK_LIBRARIES(Ovito -Wl,--whole-archive ${plugin} -Wl,--no-whole-archive)
		ELSE()
			TARGET_LINK_LIBRARIES(Ovito ${plugin})
			IF(MSVC)
				SET_PROPERTY(TARGET Ovito APPEND_STRING PROPERTY LINK_FLAGS " /WHOLEARCHIVE:${plugin}")
			ENDIF()
		ENDIF()
	ENDIF()
ENDFOREACH()

# Install fonts required by 'minimal' platform plugin on Linux
IF(UNIX AND NOT APPLE)
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

ELSEIF(APPLE)

	# Install the Info.plist file in the app bundle.
	CONFIGURE_FILE("resources/Info.plist" "${Ovito_BINARY_DIR}/${MACOSX_BUNDLE_NAME}.app/Contents/Info.plist")
	SET_TARGET_PROPERTIES(Ovito PROPERTIES MACOSX_BUNDLE_INFO_PLIST "${Ovito_BINARY_DIR}/${MACOSX_BUNDLE_NAME}.app/Contents/Info.plist")

	# Copy the application icon into the resource directory.
	INSTALL(FILES "resources/ovito.icns" DESTINATION "${OVITO_RELATIVE_SHARE_DIRECTORY}")

	# Fix Mac app bundle.
	INCLUDE("${Ovito_SOURCE_DIR}/cmake/FixupMacBundle.cmake")

ENDIF()

IF(OVITO_REDISTRIBUTABLE_PACKAGE AND OVITO_BUILD_PLUGIN_PYSCRIPT AND UNIX AND NOT APPLE)
	# Uninstall Python PIP packages from the embedded interpreter that should not be part of the official distribution.
	INSTALL(CODE "${OVITO_UNINSTALL_UNUSED_PYTHON_MODULES_CODE}")
ENDIF()
