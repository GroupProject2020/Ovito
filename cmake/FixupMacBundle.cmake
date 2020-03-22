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

# Install needed Qt plugins by copying directories from the Qt installation
SET(_qtplugins_source_dir "${_qt5Core_install_prefix}/plugins")
SET(_qtplugins_dest_dir "${MACOSX_BUNDLE_NAME}.app/Contents/PlugIns")
INSTALL(DIRECTORY "${_qtplugins_source_dir}/imageformats" DESTINATION ${_qtplugins_dest_dir} PATTERN "*_debug.dylib" EXCLUDE PATTERN "*.dSYM" EXCLUDE)
INSTALL(DIRECTORY "${_qtplugins_source_dir}/platforms" DESTINATION ${_qtplugins_dest_dir} PATTERN "*_debug.dylib" EXCLUDE PATTERN "*.dSYM" EXCLUDE)
INSTALL(DIRECTORY "${_qtplugins_source_dir}/iconengines" DESTINATION ${_qtplugins_dest_dir} PATTERN "*_debug.dylib" EXCLUDE PATTERN "*.dSYM" EXCLUDE)
IF(NOT Qt5Core_VERSION VERSION_LESS "5.12")
	INSTALL(DIRECTORY "${_qtplugins_source_dir}/styles" DESTINATION ${_qtplugins_dest_dir} PATTERN "*_debug.dylib" EXCLUDE PATTERN "*.dSYM" EXCLUDE)
ENDIF()

# Install a qt.conf file.
# This inserts some cmake code into the install script to write the file
INSTALL(CODE "
	file(WRITE \"\${CMAKE_INSTALL_PREFIX}/${MACOSX_BUNDLE_NAME}.app/Contents/Resources/qt.conf\" \"[Paths]\\nPlugins = PlugIns/\")
	")

# Purge any previous version of the nested bundle to avoid errors during bundle fixup.
IF(OVITO_BUILD_PLUGIN_PYSCRIPT)
	INSTALL(CODE "
		FILE(REMOVE_RECURSE \"\${CMAKE_INSTALL_PREFIX}/${MACOSX_BUNDLE_NAME}.app/Contents/MacOS/Ovito.app\")
		")
ENDIF()

# Use BundleUtilities to get all other dependencies for the application to work.
# It takes a bundle or executable along with possible plugins and inspects it
# for dependencies.  If they are not system dependencies, they are copied.

# Now the work of copying dependencies into the bundle/package
# The quotes are escaped and variables to use at install time have their $ escaped
# An alternative is the do a configure_file() on a script and use install(SCRIPT  ...).
# Note that the image plugins depend on QtSvg and QtXml, and it got those copied
# over.
INSTALL(CODE "
	CMAKE_POLICY(SET CMP0011 NEW)
	CMAKE_POLICY(SET CMP0009 NEW)

	# Use BundleUtilities to get all other dependencies for the application to work.
	# It takes a bundle or executable along with possible plugins and inspects it
	# for dependencies.  If they are not system dependencies, they are copied.
	SET(APPS \"\${CMAKE_INSTALL_PREFIX}/${MACOSX_BUNDLE_NAME}.app\")

	# Directories to look for dependencies:
	SET(DIRS
		${QT_LIBRARY_DIRS}
		\"\${CMAKE_INSTALL_PREFIX}/${OVITO_RELATIVE_PLUGINS_DIRECTORY}\"
		\"\${CMAKE_INSTALL_PREFIX}/${MACOSX_BUNDLE_NAME}.app/Contents/Frameworks\"
		\"\${CMAKE_INSTALL_PREFIX}/${_qtplugins_dest_dir}/imageformats\"
		\"\${CMAKE_INSTALL_PREFIX}/${_qtplugins_dest_dir}/platforms\"
		\"\${CMAKE_INSTALL_PREFIX}/${_qtplugins_dest_dir}/iconengines\"
		/opt/local/lib)

	# Returns the path that others should refer to the item by when the item is embedded inside a bundle.
	# This ensures that all plugin libraries go into the PlugIns/ directory of the bundle.
	FUNCTION(gp_item_default_embedded_path_override item default_embedded_path_var)
		# Embed plugin libraries (.so) in the PlugIns/ subdirectory:
		IF(item MATCHES \"\\\\${OVITO_PLUGIN_LIBRARY_SUFFIX}$\" AND (item MATCHES \"^@rpath\" OR item MATCHES \"PlugIns/\"))
			SET(path \"@executable_path/../PlugIns\")
			SET(\${default_embedded_path_var} \"\${path}\" PARENT_SCOPE)
			MESSAGE(\"     Embedding path override: \${item} -> \${path}\")
		ENDIF()
		# Leave helper libraries in the MacOS/ directory:
		IF(item MATCHES \"libovito\"  AND item MATCHES \"^@rpath\")
			SET(path \"@executable_path\")
			SET(\${default_embedded_path_var} \"\${path}\" PARENT_SCOPE)
			MESSAGE(\"     Embedding path override: \${item} -> \${path}\")
		ENDIF()
	ENDFUNCTION()

	FUNCTION(gp_resolved_file_type_override resolved_file type)

		# This is needed to correctly install Matplotlib's shared libraries in the .dylibs/ subdirectory:
		IF(resolved_file MATCHES \"@loader_path/\" AND resolved_file MATCHES \"/.dylibs/\")
			SET(\${type} \"system\" PARENT_SCOPE)
		ENDIF()

	ENDFUNCTION()

	FILE(GLOB_RECURSE QTPLUGINS
		\"\${CMAKE_INSTALL_PREFIX}/${_qtplugins_dest_dir}/*${CMAKE_SHARED_LIBRARY_SUFFIX}\")
	FILE(GLOB_RECURSE OVITO_PLUGINS
		\"\${CMAKE_INSTALL_PREFIX}/${OVITO_RELATIVE_PLUGINS_DIRECTORY}/*${OVITO_PLUGIN_LIBRARY_SUFFIX}\")
	FILE(GLOB_RECURSE PYTHON_DYNLIBS
		\"\${CMAKE_INSTALL_PREFIX}/${MACOSX_BUNDLE_NAME}.app/Contents/Frameworks/Python.framework/*.so\")
	FILE(GLOB OTHER_DYNLIBS
		\"\${CMAKE_INSTALL_PREFIX}/${MACOSX_BUNDLE_NAME}.app/Contents/MacOS/*.dylib\")
	FILE(GLOB OTHER_FRAMEWORK_DYNLIBS
		\"\${CMAKE_INSTALL_PREFIX}/${MACOSX_BUNDLE_NAME}.app/Contents/Frameworks/*.dylib\")
	FOREACH(lib \${QTPLUGINS} \${OVITO_PLUGINS} \${PYTHON_DYNLIBS} \${OTHER_DYNLIBS} \${OTHER_FRAMEWORK_DYNLIBS})
		IF(NOT IS_SYMLINK \${lib})
			LIST(APPEND BUNDLE_LIBS \${lib})
		ENDIF()
	ENDFOREACH()

	# Include the Python interpreter executable in the IGNORE_ITEM list.
	SET(IGNORE_ITEM_LIST \"Python\")

	# Collect the filenames of the Shiboken2 and PySide2 libraries.
	# These shared objects need to be preserved by adding them to the IGNORE_ITEM list of FIXUP_BUNDLE(). 
	FILE(GLOB PYSIDE_DYNLIBS \"\${CMAKE_INSTALL_PREFIX}/${MACOSX_BUNDLE_NAME}.app/Contents/Frameworks/Python.framework/Versions/*.*/lib/python*/site-packages/PySide2/*.dylib\")
	FILE(GLOB PYSIDE_SOLIBS \"\${CMAKE_INSTALL_PREFIX}/${MACOSX_BUNDLE_NAME}.app/Contents/Frameworks/Python.framework/Versions/*.*/lib/python*/site-packages/PySide2/*.so\")
	FILE(GLOB SHIBOKEN_DYNLIBS \"\${CMAKE_INSTALL_PREFIX}/${MACOSX_BUNDLE_NAME}.app/Contents/Frameworks/Python.framework/Versions/*.*/lib/python*/site-packages/shiboken2/*.dylib\")
	FILE(GLOB SHIBOKEN_SOLIBS \"\${CMAKE_INSTALL_PREFIX}/${MACOSX_BUNDLE_NAME}.app/Contents/Frameworks/Python.framework/Versions/*.*/lib/python*/site-packages/shiboken2/*.so\")
	FOREACH(lib \${PYSIDE_DYNLIBS} \${PYSIDE_SOLIBS} \${SHIBOKEN_DYNLIBS} \${SHIBOKEN_SOLIBS})
		GET_FILENAME_COMPONENT(lib_filename \"\${lib}\" NAME)
		LIST(APPEND IGNORE_ITEM_LIST \"\${lib_filename}\")
	ENDFOREACH()

	MESSAGE(\"Bundle libs: \${BUNDLE_LIBS}\")
	SET(BU_CHMOD_BUNDLE_ITEMS ON)	# Make copies of system libraries writable before install_name_tool tries to change them.
	INCLUDE(BundleUtilities)
	FIXUP_BUNDLE(\"\${APPS}\" \"\${BUNDLE_LIBS}\" \"\${DIRS}\" IGNORE_ITEM \${IGNORE_ITEM_LIST})

	# Fix the rpath information of the PySide2/Shiboken2 libraries.
	SET(QT_LIB_INSTALL_PATH \"${_qt5Core_install_prefix}/lib\")
	FOREACH(lib \${PYSIDE_DYNLIBS} \${PYSIDE_SOLIBS} \${SHIBOKEN_DYNLIBS} \${SHIBOKEN_SOLIBS})
		MESSAGE(\"Adding rpath to \${lib}\")
		EXECUTE_PROCESS(COMMAND install_name_tool -add_rpath \"@loader_path/\" -add_rpath \"@executable_path/../Frameworks/\" \"\${lib}\" RESULT_VARIABLE install_name_tool_result)
		IF(install_name_tool_result)
			MESSAGE(FATAL_ERROR \"install_name_tool returned error code \${install_name_tool_result}\")
		ENDIF()
		MESSAGE(\"Removing '\${QT_LIB_INSTALL_PATH}' from rpaths of \${lib}\")
		EXECUTE_PROCESS(COMMAND install_name_tool -delete_rpath \"\${QT_LIB_INSTALL_PATH}\" \"\${lib}\")
	ENDFOREACH()

")

IF(OVITO_BUILD_PLUGIN_OSPRAY AND NOT OVITO_BUILD_BASIC)
	# Extend the rpath information of the ospcommon library such that OSPRay extension modules loaded by dlopen()
	# are found in the Frameworks/ directory at runtime.
	INSTALL(CODE "
		SET(lib \"\${CMAKE_INSTALL_PREFIX}/${MACOSX_BUNDLE_NAME}.app/Contents/Frameworks/libospcommon.dylib\")
		MESSAGE(\"Adding rpath to \${lib}\")
		EXECUTE_PROCESS(COMMAND install_name_tool -add_rpath \"@loader_path/\" \"\${lib}\" RESULT_VARIABLE install_name_tool_result)
		IF(install_name_tool_result)
			MESSAGE(FATAL_ERROR \"install_name_tool returned error code \${install_name_tool_result}\")
		ENDIF()
	")
ENDIF()

IF(OVITO_BUILD_PLUGIN_PYSCRIPT AND NOT OVITO_BUILD_BASIC)

	# Create a nested bundle for 'ovitos'.
	# This is to prevent the program icon from showing up in the dock when 'ovitos' is run.
	INSTALL(CODE "
		SET(BundlePath \"\${CMAKE_INSTALL_PREFIX}/${MACOSX_BUNDLE_NAME}.app\")
		EXECUTE_PROCESS(COMMAND \"\${CMAKE_COMMAND}\" -E make_directory \"\${BundlePath}/Contents/MacOS/Ovito.app/Contents/MacOS\")
		FILE(RENAME \"\${BundlePath}/Contents/MacOS/ovitos.exe\" \"\${BundlePath}/Contents/MacOS/Ovito.app/Contents/MacOS/ovitos\")
		EXECUTE_PROCESS(COMMAND \"\${CMAKE_COMMAND}\" -E create_symlink \"../../../Resources\" \"\${BundlePath}/Contents/MacOS/Ovito.app/Contents/Resources\")
		EXECUTE_PROCESS(COMMAND \"\${CMAKE_COMMAND}\" -E create_symlink \"../../../Frameworks\" \"\${BundlePath}/Contents/MacOS/Ovito.app/Contents/Frameworks\")
		EXECUTE_PROCESS(COMMAND \"\${CMAKE_COMMAND}\" -E create_symlink \"../../../PlugIns\" \"\${BundlePath}/Contents/MacOS/Ovito.app/Contents/PlugIns\")
		CONFIGURE_FILE(\"${Ovito_SOURCE_DIR}/src/ovito_exe/resources/Info.plist\" \"\${BundlePath}/Contents/MacOS/Ovito.app/Contents/Info.plist\")
		EXECUTE_PROCESS(COMMAND defaults write \"\${BundlePath}/Contents/MacOS/Ovito.app/Contents/Info\" LSUIElement 1)
		FILE(GLOB DylibsToSymlink \"\${BundlePath}/Contents/MacOS/*.dylib\")
		FOREACH(FILE_ENTRY \${DylibsToSymlink})
			GET_FILENAME_COMPONENT(FILE_ENTRY_NAME \"\${FILE_ENTRY}\" NAME)
			EXECUTE_PROCESS(COMMAND \"\${CMAKE_COMMAND}\" -E create_symlink \"../../../\${FILE_ENTRY_NAME}\" \"\${BundlePath}/Contents/MacOS/Ovito.app/Contents/MacOS/\${FILE_ENTRY_NAME}\")
		ENDFOREACH()
	")

	# Uninstall PyPI packages from the embedded interpreter that should not be part of the official distribution.
	INSTALL(CODE "${OVITO_UNINSTALL_UNUSED_PYTHON_MODULES_CODE}")
ENDIF()
