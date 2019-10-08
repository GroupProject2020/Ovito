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

ADD_EXECUTABLE(ovitos Main.cpp)
TARGET_LINK_LIBRARIES(ovitos Core Gui)
TARGET_LINK_LIBRARIES(ovitos Qt5::Core Qt5::Gui)

# Under Linux, Qt's Xcb platform plugin will dynamically load the DBus Qt module.
# To avoid the dynamic linker pulling the wrong versions of the Qt module from the
# system path, we make the executable preload the module on application startup.
IF(UNIX AND NOT APPLE AND OVITO_REDISTRIBUTABLE_PACKAGE)
	TARGET_LINK_LIBRARIES(ovitos Qt5::DBus)
ENDIF()

IF(APPLE)
	SET_TARGET_PROPERTIES(ovitos PROPERTIES OUTPUT_NAME "ovitos.exe")
	SET_TARGET_PROPERTIES(ovitos PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${OVITO_LIBRARY_DIRECTORY}")
	INSTALL(TARGETS ovitos DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}")

	# Build the launch helper, which is needed because the 'ovitos' interpreter is located in a nested app bundle under macOS.
	ADD_EXECUTABLE(MacOSWrapper MacOSWrapper.cpp)
	SET_TARGET_PROPERTIES(MacOSWrapper PROPERTIES OUTPUT_NAME "ovitos")
	SET_TARGET_PROPERTIES(MacOSWrapper PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${OVITO_LIBRARY_DIRECTORY}")
	INSTALL(TARGETS MacOSWrapper DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}")
ELSE()
	SET_TARGET_PROPERTIES(ovitos PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${OVITO_BINARY_DIRECTORY}")
	INSTALL(TARGETS ovitos DESTINATION "${OVITO_RELATIVE_BINARY_DIRECTORY}")
ENDIF()

IF(APPLE)
	SET_TARGET_PROPERTIES(ovitos PROPERTIES MACOSX_RPATH TRUE)
    SET_TARGET_PROPERTIES(ovitos PROPERTIES INSTALL_RPATH "@executable_path/;@executable_path/../Frameworks/;@executable_path/../PlugIns/")
	SET_TARGET_PROPERTIES(ovitos PROPERTIES BUILD_WITH_INSTALL_RPATH TRUE)
ELSEIF(UNIX)
    SET_TARGET_PROPERTIES(ovitos PROPERTIES INSTALL_RPATH "$ORIGIN/../${OVITO_RELATIVE_LIBRARY_DIRECTORY}/:$ORIGIN/../${OVITO_RELATIVE_PLUGINS_DIRECTORY}/")
	SET_TARGET_PROPERTIES(ovitos PROPERTIES BUILD_WITH_INSTALL_RPATH TRUE)
ENDIF()

# Build all plugins first before building the interpreter executable.
FOREACH(plugin ${OVITO_PLUGIN_LIST})
	IF(OVITO_BUILD_MONOLITHIC)
		IF(UNIX)
			TARGET_LINK_LIBRARIES(ovitos -Wl,--whole-archive ${plugin} -Wl,--no-whole-archive)
		ELSE()
			TARGET_LINK_LIBRARIES(ovitos ${plugin})
			IF(MSVC)
				SET_PROPERTY(TARGET ovitos APPEND_STRING PROPERTY LINK_FLAGS " /WHOLEARCHIVE:${plugin}")
			ENDIF()
		ENDIF()
	ELSE()
		ADD_DEPENDENCIES(ovitos ${plugin})
	ENDIF()
ENDFOREACH()

# Define tests for the scripting interface.
SET(PYTHON_TEST_SCRIPTS_DIR "${Ovito_SOURCE_DIR}/tests/scripts")
FILE(GLOB_RECURSE PYTHON_TEST_SCRIPTS RELATIVE "${PYTHON_TEST_SCRIPTS_DIR}" "${PYTHON_TEST_SCRIPTS_DIR}/*.py")
FOREACH(test_script ${PYTHON_TEST_SCRIPTS})
	# Run each test case both with 'ovitos' and a second time with the system Python interpreter.
	ADD_TEST(NAME "${test_script}" WORKING_DIRECTORY "${PYTHON_TEST_SCRIPTS_DIR}" COMMAND "$<TARGET_FILE:ovitos>" "${test_script}")
	IF(APPLE AND CMAKE_BUILD_TYPE STREQUAL "Debug")
		# Workaround for an issue on the macOS platform, where the Qt framework ships with release and debug
		# builds. Need to make sure the debug version of Qt gets loaded at runtime when running 'ovitos' below.
		SET_PROPERTY(TEST "${test_script}" APPEND PROPERTY ENVIRONMENT "DYLD_IMAGE_SUFFIX=_debug")
	ENDIF()
	IF(NOT OVITO_BUILD_MONOLITHIC)
		ADD_TEST(NAME "${test_script}_extern" WORKING_DIRECTORY "${PYTHON_TEST_SCRIPTS_DIR}" COMMAND "${PYTHON_EXECUTABLE}" "${test_script}")
		SET_PROPERTY(TEST "${test_script}_extern" PROPERTY ENVIRONMENT "PYTHONPATH=${OVITO_PYTHON_DIRECTORY}")
		IF(APPLE AND CMAKE_BUILD_TYPE STREQUAL "Debug")
			# Workaround for an issue on the macOS platform, where the Qt framework ships with release and debug
			# builds. Need to make sure the debug version of Qt gets loaded at runtime when running 'ovitos' below.
			SET_PROPERTY(TEST "${test_script}_extern" APPEND PROPERTY ENVIRONMENT "DYLD_IMAGE_SUFFIX=_debug")
		ENDIF()
	ENDIF()
ENDFOREACH()

# Also test all code example snippets that are part of scripting documentation.
SET(PYTHON_EXAMPLE_SNIPPETS_DIR "${Ovito_SOURCE_DIR}/doc/python/example_snippets")
FILE(GLOB_RECURSE PYTHON_EXAMPLE_SNIPPETS RELATIVE "${PYTHON_EXAMPLE_SNIPPETS_DIR}" "${PYTHON_EXAMPLE_SNIPPETS_DIR}/*.py")
FOREACH(test_script ${PYTHON_EXAMPLE_SNIPPETS})
	ADD_TEST(NAME "snippet_${test_script}" WORKING_DIRECTORY "${PYTHON_EXAMPLE_SNIPPETS_DIR}" COMMAND "$<TARGET_FILE:ovitos>" "${test_script}")
	IF(APPLE AND CMAKE_BUILD_TYPE STREQUAL "Debug")
		# Workaround for an issue on the macOS platform, where the Qt framework ships with release and debug
		# builds. Need to make sure the debug version of Qt gets loaded at runtime when running 'ovitos' below.
		SET_PROPERTY(TEST "snippet_${test_script}" APPEND PROPERTY ENVIRONMENT "DYLD_IMAGE_SUFFIX=_debug")
	ENDIF()
	IF(NOT OVITO_BUILD_MONOLITHIC)
		ADD_TEST(NAME "snippet_${test_script}_extern" WORKING_DIRECTORY "${PYTHON_EXAMPLE_SNIPPETS_DIR}" COMMAND "${PYTHON_EXECUTABLE}" "${test_script}")
		SET_PROPERTY(TEST "snippet_${test_script}_extern" PROPERTY ENVIRONMENT "PYTHONPATH=${OVITO_PYTHON_DIRECTORY}")
		IF(APPLE AND CMAKE_BUILD_TYPE STREQUAL "Debug")
			# Workaround for an issue on the macOS platform, where the Qt framework ships with release and debug
			# builds. Need to make sure the debug version of Qt gets loaded at runtime when running 'ovitos' below.
			SET_PROPERTY(TEST "snippet_${test_script}_extern" APPEND PROPERTY ENVIRONMENT "DYLD_IMAGE_SUFFIX=_debug")
		ENDIF()
	ENDIF()
ENDFOREACH()

# Uninstall Python PIP packages (e.g. Sphinx) from the embedded interpreter that should not be shipped with
# the official OVITO distribution.
SET(OVITO_UNINSTALL_UNUSED_PYTHON_MODULES_CODE "
	MESSAGE(\"Uninstalling unnecessary Python packages.\")
	# Get path to installed ovitos executable.
	IF(APPLE)
		SET(OVITOS_COMMAND \"\${CMAKE_INSTALL_PREFIX}/${OVITO_RELATIVE_LIBRARY_DIRECTORY}/ovitos\")
	ELSE()
		SET(OVITOS_COMMAND \"\${CMAKE_INSTALL_PREFIX}/${OVITO_RELATIVE_BINARY_DIRECTORY}/ovitos\")
	ENDIF()
	# Make sure PIP will operate on the right Python installation.
	IF(WIN32)
		SET(ENV{PYTHONHOME} \"\${CMAKE_INSTALL_PREFIX}\")
	ENDIF()
	# Execute PIP uninstall via ovitos interpreter.
	EXECUTE_PROCESS(COMMAND \"\${OVITOS_COMMAND}\" -m pip --disable-pip-version-check uninstall --yes Sphinx alabaster babel
		Jinja2 imagesize docutils requests snowballstemmer packaging idna chardet urllib3 certifi
		MarkupSafe sphinxcontrib-websupport)
	# Remove __pycache__ files from installation bundle.
	IF(NOT WIN32)
		MESSAGE(\"Removing .pyc files.\")
		EXECUTE_PROCESS(COMMAND find \"\${CMAKE_INSTALL_PREFIX}\" -name *.pyc -delete)
		MESSAGE(\"Removing __pycache__ directories.\")
		EXECUTE_PROCESS(COMMAND find \"\${CMAKE_INSTALL_PREFIX}\" -name __pycache__ -delete)
	ENDIF()
")
IF(WIN32)
	# Uninstall Python modules now.
	INSTALL(CODE "${OVITO_UNINSTALL_UNUSED_PYTHON_MODULES_CODE}")
ELSE()
	# Propagate variable to parent scope.
	SET(OVITO_UNINSTALL_UNUSED_PYTHON_MODULES_CODE "${OVITO_UNINSTALL_UNUSED_PYTHON_MODULES_CODE}" PARENT_SCOPE)
	# Python module uninstall via PIP will be performed at a later point in time
	# in FixupMacBundle.cmake (on macOS), or after building the main executable (on Linux),
	# because the installation must be brought into a fully-working state prior to invoking 'ovitos'.
ENDIF()
