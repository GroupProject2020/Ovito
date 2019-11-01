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

# This macro creates an OVITO plugin module.
MACRO(OVITO_STANDARD_PLUGIN target_name)

    # Parse macro parameters
    SET(options GUI_PLUGIN)
    SET(oneValueArgs)
    SET(multiValueArgs SOURCES LIB_DEPENDENCIES PRIVATE_LIB_DEPENDENCIES PLUGIN_DEPENDENCIES OPTIONAL_PLUGIN_DEPENDENCIES PYTHON_WRAPPERS)
    CMAKE_PARSE_ARGUMENTS(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
	SET(plugin_sources ${ARG_SOURCES})
	SET(lib_dependencies ${ARG_LIB_DEPENDENCIES})
	SET(private_lib_dependencies ${ARG_PRIVATE_LIB_DEPENDENCIES})
	SET(plugin_dependencies ${ARG_PLUGIN_DEPENDENCIES})
	SET(optional_plugin_dependencies ${ARG_OPTIONAL_PLUGIN_DEPENDENCIES})
	SET(python_wrappers ${ARG_PYTHON_WRAPPERS})

	# Create the library target for the plugin.
    ADD_LIBRARY(${target_name} ${plugin_sources})

    # Set default include directory.
    TARGET_INCLUDE_DIRECTORIES(${target_name} PUBLIC
        "$<BUILD_INTERFACE:${OVITO_SOURCE_BASE_DIR}/src>")

	# Make the name of current plugin available to the source code.
	TARGET_COMPILE_DEFINITIONS(${target_name} PRIVATE "OVITO_PLUGIN_NAME=\"${target_name}\"")

	# Link to OVITO's core module (unless it's the core plugin itself we are defining).
	IF(NOT ${target_name} STREQUAL "Core")
		TARGET_LINK_LIBRARIES(${target_name} PUBLIC Core)
	ENDIF()

	# Link to OVITO's GUI module when the plugin provides a UI.
	IF(${ARG_GUI_PLUGIN})
		TARGET_LINK_LIBRARIES(${target_name} PUBLIC Gui)
    	TARGET_LINK_LIBRARIES(${target_name} PUBLIC Qt5::Widgets)
	ENDIF()

	# Link to Qt5.
	TARGET_LINK_LIBRARIES(${target_name} PUBLIC Qt5::Core Qt5::Gui)

	# Link to other required libraries needed by this specific plugin.
	TARGET_LINK_LIBRARIES(${target_name} PUBLIC ${lib_dependencies})

	# Link to other required libraries needed by this specific plugin, which are not visible to dependent plugins.
	TARGET_LINK_LIBRARIES(${target_name} PRIVATE ${private_lib_dependencies})

	# Link to other plugin modules that are dependencies of this plugin.
	FOREACH(plugin_name ${plugin_dependencies})
    	STRING(TOUPPER "${plugin_name}" uppercase_plugin_name)
    	IF(DEFINED OVITO_BUILD_PLUGIN_${uppercase_plugin_name})
	    	IF(NOT OVITO_BUILD_PLUGIN_${uppercase_plugin_name})
	    		MESSAGE(FATAL_ERROR "To build the ${target_name} plugin, the ${plugin_name} plugin has to be enabled too. Please set the OVITO_BUILD_PLUGIN_${uppercase_plugin_name} option to ON.")
	    	ENDIF()
	    ENDIF()
    	TARGET_LINK_LIBRARIES(${target_name} PUBLIC ${plugin_name})
	ENDFOREACH()

	# Link to other plugin modules that are optional dependencies of this plugin.
	FOREACH(plugin_name ${optional_plugin_dependencies})
		STRING(TOUPPER "${plugin_name}" uppercase_plugin_name)
		IF(OVITO_BUILD_PLUGIN_${uppercase_plugin_name})
        	TARGET_LINK_LIBRARIES(${target_name} PUBLIC ${plugin_name})
		ENDIF()
	ENDFOREACH()

	# Set prefix and suffix of library name.
	# This is needed so that the Python interpreter can load OVITO plugins as modules.
	SET_TARGET_PROPERTIES(${target_name} PROPERTIES PREFIX "" SUFFIX "${OVITO_PLUGIN_LIBRARY_SUFFIX}")

	# Define macro for symbol export from shared library.
	STRING(TOUPPER "${target_name}" _uppercase_plugin_name)
	IF(BUILD_SHARED_LIBS)
		TARGET_COMPILE_DEFINITIONS(${target_name} PRIVATE "OVITO_${_uppercase_plugin_name}_EXPORT=Q_DECL_EXPORT")
		TARGET_COMPILE_DEFINITIONS(${target_name} INTERFACE "OVITO_${_uppercase_plugin_name}_EXPORT=Q_DECL_IMPORT")
	ELSE()
		TARGET_COMPILE_DEFINITIONS(${target_name} PUBLIC "OVITO_${_uppercase_plugin_name}_EXPORT=")
	ENDIF()

	# Set visibility of symbols in this shared library to hidden by default, except those exported in the source code.
	SET_TARGET_PROPERTIES(${target_name} PROPERTIES CXX_VISIBILITY_PRESET "hidden")
	SET_TARGET_PROPERTIES(${target_name} PROPERTIES VISIBILITY_INLINES_HIDDEN ON)

	IF(APPLE)
		# This is required to avoid error by install_name_tool.
		SET_TARGET_PROPERTIES(${target_name} PROPERTIES LINK_FLAGS "-headerpad_max_install_names")
	ENDIF(APPLE)

	IF(NOT OVITO_BUILD_PYTHON_PACKAGE)
		IF(APPLE)
			# Enable the use of @rpath on macOS.
			SET_TARGET_PROPERTIES(${target_name} PROPERTIES MACOSX_RPATH TRUE)
			SET_TARGET_PROPERTIES(${target_name} PROPERTIES INSTALL_RPATH "@loader_path/;@executable_path/;@loader_path/../MacOS/;@executable_path/../Frameworks/")
			# The build tree target should have rpath of install tree target.
			SET_TARGET_PROPERTIES(${target_name} PROPERTIES BUILD_WITH_INSTALL_RPATH TRUE)
		ELSEIF(UNIX)
			# Look for other shared libraries in the parent directory ("lib/ovito/") and in the plugins directory ("lib/ovito/plugins/")
			SET_TARGET_PROPERTIES(${target_name} PROPERTIES INSTALL_RPATH "$ORIGIN:$ORIGIN/..")
		ENDIF()
	ELSE()
		IF(APPLE)
			# Use @loader_path on macOS when building the Python modules only.
			SET_TARGET_PROPERTIES(${target_name} PROPERTIES MACOSX_RPATH TRUE)
			SET_TARGET_PROPERTIES(${target_name} PROPERTIES INSTALL_RPATH "@loader_path/")
		ELSEIF(UNIX)
			# Look for other shared libraries in the same directory.
			SET_TARGET_PROPERTIES(${target_name} PROPERTIES INSTALL_RPATH "$ORIGIN")
		ENDIF()

		IF(NOT BUILD_SHARED_LIBS)
			# Since we will link this library into the dynamically loaded Python extension module, we need to use the fPIC flag.
			SET_PROPERTY(TARGET ${target_name} PROPERTY POSITION_INDEPENDENT_CODE ON)
		ENDIF()
	ENDIF()

	# Install Python wrapper files.
	IF(python_wrappers AND OVITO_BUILD_PLUGIN_PYSCRIPT)
		# Install the Python source files that belong to the plugin, which provide the scripting interface.
		ADD_CUSTOM_COMMAND(TARGET ${target_name} POST_BUILD
			COMMAND ${CMAKE_COMMAND} "-E" copy_directory "${python_wrappers}" "${OVITO_PYTHON_DIRECTORY}/"
			COMMENT "Copying Python files for plugin ${target_name}")

		# Also make them part of the installation package.
		INSTALL(DIRECTORY "${python_wrappers}"
			DESTINATION "${OVITO_RELATIVE_PYTHON_DIRECTORY}/"
			REGEX "__pycache__" EXCLUDE)
	ENDIF()

	# Make this module part of the installation package.
	IF(WIN32 AND (${target_name} STREQUAL "Core" OR ${target_name} STREQUAL "Gui"))
		# On Windows, the Core and Gui DLLs need to be placed in the same directory
		# as the Ovito executable, because Windows won't find them if they are in the
		# plugins subdirectory.
		SET_TARGET_PROPERTIES(${target_name} PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${OVITO_BINARY_DIRECTORY}")
		SET_TARGET_PROPERTIES(${target_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${OVITO_BINARY_DIRECTORY}")
		INSTALL(TARGETS ${target_name} EXPORT OVITO
			RUNTIME DESTINATION "${OVITO_RELATIVE_BINARY_DIRECTORY}"
			LIBRARY DESTINATION "${OVITO_RELATIVE_BINARY_DIRECTORY}"
			ARCHIVE DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}" COMPONENT "development")
	ELSE()
		# Install all plugins into the plugins directory.
		SET_TARGET_PROPERTIES(${target_name} PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${OVITO_PLUGINS_DIRECTORY}")
		SET_TARGET_PROPERTIES(${target_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${OVITO_PLUGINS_DIRECTORY}")
		INSTALL(TARGETS ${target_name} EXPORT OVITO
			RUNTIME DESTINATION "${OVITO_RELATIVE_PLUGINS_DIRECTORY}"
			LIBRARY DESTINATION "${OVITO_RELATIVE_PLUGINS_DIRECTORY}"
			ARCHIVE DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}" COMPONENT "development")
	ENDIF()

	# Maintain the list of all plugins.
	LIST(APPEND OVITO_PLUGIN_LIST ${target_name})
	SET(OVITO_PLUGIN_LIST ${OVITO_PLUGIN_LIST} PARENT_SCOPE)

ENDMACRO()
