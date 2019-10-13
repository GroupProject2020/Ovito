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

# Tell CMake to run Qt moc whenever necessary.
SET(CMAKE_AUTOMOC ON)
# As moc files are generated in the binary dir, tell CMake to always look for includes there.
SET(CMAKE_INCLUDE_CURRENT_DIR ON)

# The set of required Qt modules:
LIST(APPEND OVITO_REQUIRED_QT_COMPONENTS Core Concurrent Network Gui Xml) # Note: Xml is a dependency of the Galamost plugin.
IF(OVITO_BUILD_GUI)
	LIST(APPEND OVITO_REQUIRED_QT_COMPONENTS Widgets PrintSupport Svg DBus) # Note: PrintSupport is an indirect dependency of the Qwt library. DBus is an indirect dependency of the Xcb platform plugin under Linux.
ENDIF()

# Find the required Qt5 modules.
FOREACH(component IN LISTS OVITO_REQUIRED_QT_COMPONENTS)
	FIND_PACKAGE(Qt5${component} REQUIRED)
ENDFOREACH()

# This macro installs a third-party shared library or DLL in the OVITO program directory
# so that it can be distributed together with the program.
FUNCTION(OVITO_INSTALL_SHARED_LIB shared_lib destination_dir)
	IF(WIN32 OR OVITO_REDISTRIBUTABLE_PACKAGE)
		# Make sure the destination directory exists.
		SET(_abs_dest_dir "${Ovito_BINARY_DIR}/${OVITO_RELATIVE_3RDPARTY_LIBRARY_DIRECTORY}/${destination_dir}")
		FILE(MAKE_DIRECTORY "${_abs_dest_dir}")
		# Strip version number from shared lib filename.
		GET_FILENAME_COMPONENT(shared_lib_ext "${shared_lib}" EXT)
		STRING(REPLACE ${shared_lib_ext} "" shared_lib_new "${shared_lib}")
		FILE(GLOB lib_versions LIST_DIRECTORIES FALSE "${shared_lib}" "${shared_lib_new}.*${CMAKE_SHARED_LIBRARY_SUFFIX}" "${shared_lib_new}${CMAKE_SHARED_LIBRARY_SUFFIX}.*")
		IF(NOT lib_versions)
			MESSAGE(FATAL_ERROR "Did not find any library files that match the file path ${shared_lib} (globbing patterns: ${shared_lib_new}.*${CMAKE_SHARED_LIBRARY_SUFFIX}; ${shared_lib_new}${CMAKE_SHARED_LIBRARY_SUFFIX}.*)")
		ENDIF()
		# Find all variants of the shared library name, including symbolic links.
		FOREACH(lib_version ${lib_versions})
			WHILE(IS_SYMLINK ${lib_version})
				GET_FILENAME_COMPONENT(symlink_target "${lib_version}" REALPATH)
				GET_FILENAME_COMPONENT(symlink_target_name "${symlink_target}" NAME)
				GET_FILENAME_COMPONENT(lib_version_name "${lib_version}" NAME)
				IF(NOT lib_version_name STREQUAL symlink_target_name)
					MESSAGE("Installing symlink ${lib_version_name} to ${symlink_target_name}")
					EXECUTE_PROCESS(COMMAND "${CMAKE_COMMAND}" -E create_symlink ${symlink_target_name} "${_abs_dest_dir}/${lib_version_name}")
					IF(NOT APPLE)
						INSTALL(FILES "${_abs_dest_dir}/${lib_version_name}" DESTINATION "${OVITO_RELATIVE_3RDPARTY_LIBRARY_DIRECTORY}/${destination_dir}/")
					ENDIF()
				ENDIF()
				SET(lib_version "${symlink_target}")
			ENDWHILE()
			IF(NOT IS_SYMLINK ${lib_version})
				LIST(APPEND lib_files "${lib_version}")
			ENDIF()
		ENDFOREACH()
		LIST(REMOVE_DUPLICATES lib_files)
		FOREACH(lib_file ${lib_files})
			IF(NOT APPLE)
				MESSAGE("Installing shared library ${lib_file}")
				EXECUTE_PROCESS(COMMAND "${CMAKE_COMMAND}" "-E" "copy_if_different" "${lib_file}" "${_abs_dest_dir}/" RESULT_VARIABLE _error_var)
				IF(_error_var)
					MESSAGE(FATAL_ERROR "Failed to copy DLL into build directory: ${lib_file}")
				ENDIF()
				INSTALL(FILES "${lib_file}" DESTINATION "${OVITO_RELATIVE_3RDPARTY_LIBRARY_DIRECTORY}/${destination_dir}/")
			ENDIF()
		ENDFOREACH()
		UNSET(lib_files)
	ENDIF()
ENDFUNCTION()

# Ship the requires libraries with the program.
IF(UNIX AND NOT APPLE AND OVITO_REDISTRIBUTABLE_PACKAGE)

	# Install copies of the Qt libraries.
	FILE(MAKE_DIRECTORY "${OVITO_LIBRARY_DIRECTORY}/lib")
	FOREACH(component IN LISTS OVITO_REQUIRED_QT_COMPONENTS)
		GET_TARGET_PROPERTY(lib Qt5::${component} LOCATION)
		GET_TARGET_PROPERTY(lib_soname Qt5::${component} IMPORTED_SONAME_RELEASE)
		CONFIGURE_FILE("${lib}" "${OVITO_LIBRARY_DIRECTORY}" COPYONLY)
		MESSAGE("INSTALLED QT LIB: ${lib} to ${OVITO_LIBRARY_DIRECTORY}")
		GET_FILENAME_COMPONENT(lib_realname "${lib}" NAME)
		EXECUTE_PROCESS(COMMAND "${CMAKE_COMMAND}" -E create_symlink "${lib_realname}" "${OVITO_LIBRARY_DIRECTORY}/${lib_soname}")
		EXECUTE_PROCESS(COMMAND "${CMAKE_COMMAND}" -E create_symlink "../${lib_soname}" "${OVITO_LIBRARY_DIRECTORY}/lib/${lib_soname}")
		INSTALL(FILES "${lib}" DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}/")
		INSTALL(FILES "${OVITO_LIBRARY_DIRECTORY}/${lib_soname}" DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}/")
		INSTALL(FILES "${OVITO_LIBRARY_DIRECTORY}/lib/${lib_soname}" DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}/lib/")
		GET_FILENAME_COMPONENT(QtBinaryPath ${lib} PATH)
	ENDFOREACH()

	# Install Qt plugins.
	INSTALL(DIRECTORY "${QtBinaryPath}/../plugins/platforms" DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}/plugins_qt/")
	INSTALL(DIRECTORY "${QtBinaryPath}/../plugins/bearer" DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}/plugins_qt/")
	INSTALL(DIRECTORY "${QtBinaryPath}/../plugins/imageformats" DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}/plugins_qt/" PATTERN "libqsvg.so" EXCLUDE)
	INSTALL(DIRECTORY "${QtBinaryPath}/../plugins/iconengines" DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}/plugins_qt/")
	INSTALL(DIRECTORY "${QtBinaryPath}/../plugins/xcbglintegrations" DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}/plugins_qt/")
	# The XcbQpa library is required by the Qt5 Gui module.
	OVITO_INSTALL_SHARED_LIB("${QtBinaryPath}/libQt5XcbQpa.so" "./lib")

	# Distribute libxkbcommon.so with Ovito, which is a dependency of the Qt XCB plugin that might not be present on all systems.
	FIND_LIBRARY(OVITO_XKBCOMMON_DEP NAMES libxkbcommon.so.0 PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu /usr/lib64 NO_DEFAULT_PATH)
	IF(NOT OVITO_XKBCOMMON_DEP)
		MESSAGE(FATAL_ERROR "Could not find shared library libxkbcommon.so.0 in system path.")
	ENDIF()
	OVITO_INSTALL_SHARED_LIB("${OVITO_XKBCOMMON_DEP}" "./lib")
	UNSET(OVITO_XKBCOMMON_DEP CACHE)

	# Distribute libxkbcommon-x11.so with Ovito, which is a dependency of the Qt XCB plugin that might not be present on all systems.
	FIND_LIBRARY(OVITO_XKBCOMMONX11_DEP NAMES libxkbcommon-x11.so.0 PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu /usr/lib64 NO_DEFAULT_PATH)
	IF(NOT OVITO_XKBCOMMONX11_DEP)
		MESSAGE(FATAL_ERROR "Could not find shared library libxkbcommon-x11.so.0 in system path.")
	ENDIF()
	OVITO_INSTALL_SHARED_LIB("${OVITO_XKBCOMMONX11_DEP}" "./lib")
	UNSET(OVITO_XKBCOMMONX11_DEP CACHE)

ELSEIF(WIN32)

	# On Windows, the third-party library DLLs need to be installed in the OVITO directory.
	# Gather Qt dynamic link libraries.
	FOREACH(component IN LISTS OVITO_REQUIRED_QT_COMPONENTS)
		GET_TARGET_PROPERTY(dll Qt5::${component} LOCATION_${CMAKE_BUILD_TYPE})
		OVITO_INSTALL_SHARED_LIB("${dll}" ".")
		IF(${component} MATCHES "Core")
			GET_FILENAME_COMPONENT(QtBinaryPath ${dll} PATH)
			IF(Qt5Core_VERSION VERSION_LESS "5.7")
				OVITO_INSTALL_SHARED_LIB("${QtBinaryPath}/icudt54.dll" ".")
				OVITO_INSTALL_SHARED_LIB("${QtBinaryPath}/icuin54.dll" ".")
				OVITO_INSTALL_SHARED_LIB("${QtBinaryPath}/icuuc54.dll" ".")
			ENDIF()
		ENDIF()
	ENDFOREACH()

	# Install Qt plugins.
	IF(CMAKE_BUILD_TYPE STREQUAL "Debug")
		INSTALL(FILES "${QtBinaryPath}/../plugins/platforms/qwindowsd.dll" DESTINATION "${OVITO_RELATIVE_BINARY_DIRECTORY}/plugins/platforms/")
		INSTALL(FILES "${QtBinaryPath}/../plugins/imageformats/qjpegd.dll" DESTINATION "${OVITO_RELATIVE_BINARY_DIRECTORY}/plugins/imageformats/")
		INSTALL(FILES "${QtBinaryPath}/../plugins/imageformats/qgifd.dll" DESTINATION "${OVITO_RELATIVE_BINARY_DIRECTORY}/plugins/imageformats/")
		INSTALL(FILES "${QtBinaryPath}/../plugins/iconengines/qsvgicond.dll" DESTINATION "${OVITO_RELATIVE_BINARY_DIRECTORY}/plugins/iconengines/")
		INSTALL(FILES "${QtBinaryPath}/../plugins/styles/qwindowsvistastyled.dll" DESTINATION "${OVITO_RELATIVE_BINARY_DIRECTORY}/plugins/styles/")
	ELSE()
		INSTALL(FILES "${QtBinaryPath}/../plugins/platforms/qwindows.dll" DESTINATION "${OVITO_RELATIVE_BINARY_DIRECTORY}/plugins/platforms/")
		INSTALL(FILES "${QtBinaryPath}/../plugins/imageformats/qjpeg.dll" DESTINATION "${OVITO_RELATIVE_BINARY_DIRECTORY}/plugins/imageformats/")
		INSTALL(FILES "${QtBinaryPath}/../plugins/imageformats/qgif.dll" DESTINATION "${OVITO_RELATIVE_BINARY_DIRECTORY}/plugins/imageformats/")
		INSTALL(FILES "${QtBinaryPath}/../plugins/iconengines/qsvgicon.dll" DESTINATION "${OVITO_RELATIVE_BINARY_DIRECTORY}/plugins/iconengines/")
		INSTALL(FILES "${QtBinaryPath}/../plugins/styles/qwindowsvistastyle.dll" DESTINATION "${OVITO_RELATIVE_BINARY_DIRECTORY}/plugins/styles/")
	ENDIF()

ENDIF()
