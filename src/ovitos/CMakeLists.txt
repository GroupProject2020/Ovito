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
IF(APPLE)
	SET_TARGET_PROPERTIES(ovitos PROPERTIES OUTPUT_NAME "ovitos.exe")
	SET_TARGET_PROPERTIES(ovitos PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${OVITO_LIBRARY_DIRECTORY}")
	INSTALL(TARGETS ovitos DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}")
	SET(OVITOS_EXECUTABLE "${OVITO_LIBRARY_DIRECTORY}/ovitos.exe")
	
	# Install the launch helper script.
	INSTALL(PROGRAMS "resources/ovitos" DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}")
ELSE()
	SET_TARGET_PROPERTIES(ovitos PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${OVITO_BINARY_DIRECTORY}")
	INSTALL(TARGETS ovitos DESTINATION "${OVITO_RELATIVE_BINARY_DIRECTORY}")
	GET_PROPERTY(OVITOS_EXECUTABLE TARGET ovitos PROPERTY LOCATION)
ENDIF()

IF(APPLE)
	SET_TARGET_PROPERTIES(ovitos PROPERTIES MACOSX_RPATH TRUE)
	SET_TARGET_PROPERTIES(ovitos PROPERTIES INSTALL_RPATH "@executable_path/")
ELSEIF(UNIX)
	SET_TARGET_PROPERTIES(ovitos PROPERTIES INSTALL_RPATH "$ORIGIN/../lib/ovito/")
ENDIF()

# Build all plugins first before building the interpreter executable.
FOREACH(plugin ${OVITO_PLUGIN_LIST})
	IF(OVITO_BUILD_MONOLITHIC)
		TARGET_LINK_LIBRARIES(ovitos -Wl,--whole-archive ${plugin} -Wl,--no-whole-archive)
	ELSE()
		ADD_DEPENDENCIES(ovitos ${plugin})
	ENDIF()
ENDFOREACH()

# Define tests for the scripting interface.
SET(PYTHON_TEST_SCRIPTS_DIR "${CMAKE_SOURCE_DIR}/tests/scripts/test_suite")
FILE(GLOB_RECURSE PYTHON_TEST_SCRIPTS RELATIVE "${PYTHON_TEST_SCRIPTS_DIR}" "${PYTHON_TEST_SCRIPTS_DIR}/*.py")
FOREACH(test_script ${PYTHON_TEST_SCRIPTS})
	# Run each test case both with 'ovitos' and a second time with the system Python interpreter.
	ADD_TEST(NAME "${test_script}" WORKING_DIRECTORY "${PYTHON_TEST_SCRIPTS_DIR}" COMMAND "${OVITOS_EXECUTABLE}" "${test_script}")
	IF(NOT OVITO_BUILD_MONOLITHIC)
		ADD_TEST(NAME "${test_script}_extern" WORKING_DIRECTORY "${PYTHON_TEST_SCRIPTS_DIR}" COMMAND "${PYTHON_EXECUTABLE}" "${test_script}")
		SET_PROPERTY(TEST "${test_script}_extern" PROPERTY ENVIRONMENT "PYTHONPATH=${OVITO_PYTHON_DIRECTORY}")
	ENDIF()
ENDFOREACH()

# Also test all code example snippets that are part of scripting documentation.
SET(PYTHON_EXAMPLE_SNIPPETS_DIR "${CMAKE_SOURCE_DIR}/doc/python/example_snippets")
FILE(GLOB_RECURSE PYTHON_EXAMPLE_SNIPPETS RELATIVE "${PYTHON_EXAMPLE_SNIPPETS_DIR}" "${PYTHON_EXAMPLE_SNIPPETS_DIR}/*.py")
FOREACH(test_script ${PYTHON_EXAMPLE_SNIPPETS})
	ADD_TEST(NAME "snippet_${test_script}" WORKING_DIRECTORY "${PYTHON_EXAMPLE_SNIPPETS_DIR}" COMMAND "${OVITOS_EXECUTABLE}" "${test_script}")
	IF(NOT OVITO_BUILD_MONOLITHIC)
		ADD_TEST(NAME "snippet_${test_script}_extern" WORKING_DIRECTORY "${PYTHON_EXAMPLE_SNIPPETS_DIR}" COMMAND "${PYTHON_EXECUTABLE}" "${test_script}")
		SET_PROPERTY(TEST "snippet_${test_script}_extern" PROPERTY ENVIRONMENT "PYTHONPATH=${OVITO_PYTHON_DIRECTORY}")
	ENDIF()
ENDFOREACH()
