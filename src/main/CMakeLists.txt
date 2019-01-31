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
    # Enable the use of @rpath on OSX.
    SET_TARGET_PROPERTIES(Ovito PROPERTIES MACOSX_RPATH TRUE)
    SET_TARGET_PROPERTIES(Ovito PROPERTIES INSTALL_RPATH "@executable_path/")
ELSEIF(UNIX)
    SET_TARGET_PROPERTIES(Ovito PROPERTIES INSTALL_RPATH "$ORIGIN/../lib/ovito/")
ENDIF()

# This executable will be part of the installation package.
INSTALL(TARGETS Ovito DESTINATION "${OVITO_RELATIVE_BINARY_DIRECTORY}/")

# Build all plugins first before building the main executable.
FOREACH(plugin ${OVITO_PLUGIN_LIST})
	IF(OVITO_BUILD_MONOLITHIC)
		IF(UNIX)
			TARGET_LINK_LIBRARIES(Ovito -Wl,--whole-archive ${plugin} -Wl,--no-whole-archive)
		ELSE()
			TARGET_LINK_LIBRARIES(Ovito ${plugin})
			IF(MSVC)
				SET_PROPERTY(TARGET Ovito APPEND_STRING PROPERTY LINK_FLAGS " /WHOLEARCHIVE:${plugin}")
			ENDIF()
		ENDIF()
	ELSE()
		ADD_DEPENDENCIES(Ovito ${plugin})
	ENDIF()
ENDFOREACH()

# Install fonts required by 'minimal' platform plugin on Linux
IF(UNIX AND NOT APPLE)
    INSTALL(DIRECTORY "resources/fonts" DESTINATION "${OVITO_RELATIVE_SHARE_DIRECTORY}/")
ENDIF()

IF(UNIX AND NOT APPLE AND OVITO_REDISTRIBUTABLE_PACKAGE)
	# Install copies of the Qt libraries.
	FOREACH(component IN LISTS OVITO_REQUIRED_QT_COMPONENTS)
		GET_TARGET_PROPERTY(lib Qt5::${component} LOCATION)
		GET_TARGET_PROPERTY(lib_soname Qt5::${component} IMPORTED_SONAME_RELEASE)
		CONFIGURE_FILE("${lib}" "${OVITO_LIBRARY_DIRECTORY}" COPYONLY)		
		GET_FILENAME_COMPONENT(lib_realname "${lib}" NAME)
		EXECUTE_PROCESS(COMMAND "${CMAKE_COMMAND}" -E create_symlink "${lib_realname}" "${OVITO_RELATIVE_LIBRARY_DIRECTORY}/${lib_soname}")
		EXECUTE_PROCESS(COMMAND "${CMAKE_COMMAND}" -E create_symlink "../${lib_soname}" "${OVITO_RELATIVE_LIBRARY_DIRECTORY}/lib/${lib_soname}")
		INSTALL(FILES "${lib}" DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}/")
		INSTALL(FILES "${OVITO_LIBRARY_DIRECTORY}/${lib_soname}" DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}/")
		INSTALL(FILES "${OVITO_LIBRARY_DIRECTORY}/lib/${lib_soname}" DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}/lib/")
		GET_FILENAME_COMPONENT(QtBinaryPath ${lib} PATH)
	ENDFOREACH()

	# Create a qt.conf file.
	# This is required to let Qt search for its plugin modules in the program directory.
	INSTALL(CODE "
	    file(WRITE \"\${CMAKE_INSTALL_PREFIX}/${OVITO_RELATIVE_BINARY_DIRECTORY}/qt.conf\" \"[Paths]\\nPlugins = ../${OVITO_RELATIVE_LIBRARY_DIRECTORY}/plugins_qt/\")
	    ")

	# Install Qt plugins.
	INSTALL(DIRECTORY "${QtBinaryPath}/../plugins/platforms" DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}/plugins_qt/")
	INSTALL(DIRECTORY "${QtBinaryPath}/../plugins/bearer" DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}/plugins_qt/")
	INSTALL(DIRECTORY "${QtBinaryPath}/../plugins/imageformats" DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}/plugins_qt/" PATTERN "libqsvg.so" EXCLUDE)
	INSTALL(DIRECTORY "${QtBinaryPath}/../plugins/iconengines" DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}/plugins_qt/")
	INSTALL(DIRECTORY "${QtBinaryPath}/../plugins/xcbglintegrations" DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}/plugins_qt/")
	# The XcbQpa library is required by the Qt5 Gui module.
	OVITO_INSTALL_SHARED_LIB("${QtBinaryPath}/libQt5XcbQpa.so" "${OVITO_RELATIVE_LIBRARY_DIRECTORY}/lib")

	# Make the executable look in lib/ovito/ directory for shared libraries.
	SET_TARGET_PROPERTIES(Ovito PROPERTIES INSTALL_RPATH "\$ORIGIN/../lib/ovito")
	SET_TARGET_PROPERTIES(Ovito PROPERTIES BUILD_WITH_INSTALL_RPATH "1")

	# Update: No longer bundle libstdc++.so.6 with OVITO, because it causes conflicts with the OpenGL driver on
	# some systems.
	#FIND_LIBRARY(OVITO_LIBSTDCXX NAMES libstdc++.so.6 PATHS /usr/lib /usr/lib/x86_64-linux-gnu NO_DEFAULT_PATH)
	#IF(NOT OVITO_LIBSTDCXX)
	#	MESSAGE(FATAL_ERROR "Please specify the location of the C++ standard library with the OVITO_LIBSTDCXX option.") 
	#ENDIF()
	#GET_FILENAME_COMPONENT(OVITO_LIBSTDCXX_REAL ${OVITO_LIBSTDCXX} REALPATH)
	#INSTALL(FILES "${OVITO_LIBSTDCXX}" "${OVITO_LIBSTDCXX_REAL}" DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}/")

ENDIF()

# On Windows, the third-party library DLLs need to be installed in the OVITO directory.
IF(WIN32)
	# Gather Qt dynamic link libraries.
	FOREACH(component IN LISTS OVITO_REQUIRED_QT_COMPONENTS)
		GET_TARGET_PROPERTY(dll Qt5::${component} LOCATION_${CMAKE_BUILD_TYPE})
		OVITO_INSTALL_DLL("${dll}")
		IF(${component} MATCHES "Core")
			GET_FILENAME_COMPONENT(QtBinaryPath ${dll} PATH)
			IF(Qt5Core_VERSION VERSION_LESS "5.7")
				OVITO_INSTALL_DLL("${QtBinaryPath}/icudt54.dll")
				OVITO_INSTALL_DLL("${QtBinaryPath}/icuin54.dll")
				OVITO_INSTALL_DLL("${QtBinaryPath}/icuuc54.dll")
			ENDIF()
		ENDIF()
	ENDFOREACH()
	
	# Install Qt plugins.
	IF(CMAKE_BUILD_TYPE STREQUAL "Debug")
		INSTALL(FILES "${QtBinaryPath}/../plugins/platforms/qwindowsd.dll" DESTINATION "${OVITO_RELATIVE_BINARY_DIRECTORY}/plugins/platforms/")
		INSTALL(FILES "${QtBinaryPath}/../plugins/imageformats/qjpegd.dll" DESTINATION "${OVITO_RELATIVE_BINARY_DIRECTORY}/plugins/imageformats/")
		INSTALL(FILES "${QtBinaryPath}/../plugins/imageformats/qgifd.dll" DESTINATION "${OVITO_RELATIVE_BINARY_DIRECTORY}/plugins/imageformats/")
		INSTALL(FILES "${QtBinaryPath}/../plugins/iconengines/qsvgicond.dll" DESTINATION "${OVITO_RELATIVE_BINARY_DIRECTORY}/plugins/iconengines/")
	ELSE()
		INSTALL(FILES "${QtBinaryPath}/../plugins/platforms/qwindows.dll" DESTINATION "${OVITO_RELATIVE_BINARY_DIRECTORY}/plugins/platforms/")
		INSTALL(FILES "${QtBinaryPath}/../plugins/imageformats/qjpeg.dll" DESTINATION "${OVITO_RELATIVE_BINARY_DIRECTORY}/plugins/imageformats/")
		INSTALL(FILES "${QtBinaryPath}/../plugins/imageformats/qgif.dll" DESTINATION "${OVITO_RELATIVE_BINARY_DIRECTORY}/plugins/imageformats/")
		INSTALL(FILES "${QtBinaryPath}/../plugins/iconengines/qsvgicon.dll" DESTINATION "${OVITO_RELATIVE_BINARY_DIRECTORY}/plugins/iconengines/")
	ENDIF()
	
	# Create a qt.conf file.
	# This is required to let Qt search for its plugin modules in the program directory.
	INSTALL(CODE "
	    file(WRITE \"\${CMAKE_INSTALL_PREFIX}/${OVITO_RELATIVE_BINARY_DIRECTORY}/qt.conf\" \"[Paths]\\nPlugins = plugins/\")
	    ")

ENDIF()

# Fix Mac app bundle.
INCLUDE(${OVITO_SOURCE_BASE_DIR}/cmake/FixupMacBundle.cmake)

IF(OVITO_BUILD_PLUGIN_PYSCRIPT AND UNIX AND NOT APPLE)
	# Uninstall Python PIP packages from the embedded interpreter that should not be part of the official distribution.
	INSTALL(CODE "${OVITO_UNINSTALL_UNUSED_PYTHON_MODULES_CODE}")
ENDIF()