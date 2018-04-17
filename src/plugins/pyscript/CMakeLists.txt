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

SET(Python_ADDITIONAL_VERSIONS 3.4 3.5 3.6 3.7 3.8)
FIND_PACKAGE(PythonInterp REQUIRED)
FIND_PACKAGE(PythonLibs REQUIRED)

IF(MSVC)
	# Precompiled headers require more compiler memory.
	ADD_COMPILE_OPTIONS(-Zm150)
ENDIF()

SET(SourceFiles
	engine/ScriptEngine.cpp
	engine/ScriptAutostarter.cpp
	engine/AdhocApplication.cpp
	binding/PythonInterface.cpp
	binding/ViewportBinding.cpp
	binding/AnimationBinding.cpp
	binding/SceneBinding.cpp
	binding/FileIOBinding.cpp
	binding/RenderingBinding.cpp
	binding/AppBinding.cpp
	binding/TypeCasting.cpp
	extensions/PythonViewportOverlay.cpp
	extensions/PythonScriptModifier.cpp
)

OVITO_STANDARD_PLUGIN(PyScript
	SOURCES PyScript.cpp ${SourceFiles}
	LIB_DEPENDENCIES ${PYTHON_LIBRARIES} OpenGLRenderer
	PYTHON_WRAPPERS "${CMAKE_CURRENT_SOURCE_DIR}/python/"
)

# Add Python include directory.
TARGET_INCLUDE_DIRECTORIES(PyScript PUBLIC ${PYTHON_INCLUDE_PATH})

# Speed up compilation by using precompiled headers.
IF(OVITO_USE_PRECOMPILED_HEADERS)
	ADD_PRECOMPILED_HEADER(PyScript plugins/pyscript/PyScript.h)
ENDIF()

# Install the Python source files that belong to the plugin and which provide the scripting interface.
INSTALL(DIRECTORY "${OVITO_PYTHON_DIRECTORY}/" 
		DESTINATION "${OVITO_RELATIVE_PYTHON_DIRECTORY}/" COMPONENT "runtime"
		REGEX "__pycache__" EXCLUDE)

# Build corresponding GUI plugin.
IF(OVITO_BUILD_GUI)
	ADD_SUBDIRECTORY(gui)
ENDIF()

# Install standard Python packages. 
IF(APPLE)
	IF(OVITO_REDISTRIBUTABLE_PACKAGE)
		STRING(REGEX MATCH "[0-9]+\\.[0-9]+" PYTHON_MAJORMINOR_VERSION ${PYTHONLIBS_VERSION_STRING})
		LIST(GET PYTHON_LIBRARIES 0 PYTHON_FIRST_LIBRARY)
		GET_FILENAME_COMPONENT(PYTHON_LIBRARY_PATH "${PYTHON_FIRST_LIBRARY}" PATH)
		INSTALL(DIRECTORY "${PYTHON_LIBRARY_PATH}/python${PYTHON_MAJORMINOR_VERSION}" 
			DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}/../Frameworks/Python.framework/Versions/${PYTHON_MAJORMINOR_VERSION}/lib/"
			COMPONENT "runtime"
			PATTERN "*.pyc" EXCLUDE 
			PATTERN "*.pyo" EXCLUDE 
			PATTERN "__pycache__" EXCLUDE
			PATTERN "test" EXCLUDE
			PATTERN "*.exe" EXCLUDE
			PATTERN "sphinx" EXCLUDE)
			
		# Install Python header files.
		INSTALL(DIRECTORY "${PYTHON_INCLUDE_PATH}" 
			DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}/../Frameworks/Python.framework/Versions/${PYTHON_MAJORMINOR_VERSION}/include/"
			COMPONENT "runtime")
	ENDIF()
ELSEIF(UNIX)
	IF(OVITO_REDISTRIBUTABLE_PACKAGE)

		# Install standard Python packages. 
		STRING(REGEX MATCH "[0-9]+\\.[0-9]+" PYTHON_MAJORMINOR_VERSION ${PYTHONLIBS_VERSION_STRING})
		LIST(GET PYTHON_LIBRARIES 0 PYTHON_FIRST_LIBRARY)
		GET_FILENAME_COMPONENT(PYTHON_LIBRARY_PATH "${PYTHON_LIBRARY}" PATH)
		INSTALL(DIRECTORY "${PYTHON_LIBRARY_PATH}/python${PYTHON_MAJORMINOR_VERSION}" 
			DESTINATION "lib/"
			COMPONENT "runtime"
			PATTERN "*.pyc" EXCLUDE 
			PATTERN "*.pyo" EXCLUDE 
			PATTERN "__pycache__" EXCLUDE
			PATTERN "test" EXCLUDE
			PATTERN "sphinx" EXCLUDE
			PATTERN "Sphinx*" EXCLUDE
			PATTERN "Jinja*" EXCLUDE
			PATTERN "Pygments*" EXCLUDE
			PATTERN "Babel" EXCLUDE)

		# Install Python header files.
		INSTALL(DIRECTORY "${PYTHON_INCLUDE_PATH}" DESTINATION "include/" COMPONENT "runtime")

		# Install a copy of libpython.so shared library.
		CONFIGURE_FILE("${PYTHON_LIBRARIES}" "${OVITO_LIBRARY_DIRECTORY}" COPYONLY)
		INSTALL(FILES "${PYTHON_LIBRARIES}" DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}/" COMPONENT "runtime")

		# Also install copies of shared libraries that are required by Python modules.
		SET(OVITO_PYTHON_DEPENDENCIES libssl.so libcrypto.so liblapack.so.3gf libblas.so.3gf libquadmath.so.0 libgfortran.so.3 libncurses.so libsqlite3.so.0)
		FOREACH(PYTHON_DEPENDENCY ${OVITO_PYTHON_DEPENDENCIES})
			FIND_LIBRARY(OVITO_PYTHON_DEP NAMES ${PYTHON_DEPENDENCY} PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu NO_DEFAULT_PATH)
			IF(NOT OVITO_PYTHON_DEP)
				MESSAGE(FATAL_ERROR "Could not find shared library ${PYTHON_DEPENDENCY}, which is required by Python interpreter, in system path.")
			ENDIF()
#			MESSAGE("Installing shared library ${OVITO_PYTHON_DEP}, needed by Python interpreter")
			OVITO_INSTALL_SHARED_LIB("${OVITO_PYTHON_DEP}" "${OVITO_RELATIVE_LIBRARY_DIRECTORY}")
			UNSET(OVITO_PYTHON_DEP CACHE)
#			GET_FILENAME_COMPONENT(OVITO_PYTHON_DEP_REAL ${OVITO_PYTHON_DEP} REALPATH)
#			INSTALL(FILES "${OVITO_PYTHON_DEP_REAL}" DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}/" COMPONENT "runtime")
#			MESSAGE("Installing shared library ${PYTHON_DEPENDENCY}, needed by Python interpreter")
#			IF(NOT "${OVITO_PYTHON_DEP_REAL}" STREQUAL "${OVITO_PYTHON_DEP}")
#				GET_FILENAME_COMPONENT(OVITO_PYTHON_DEP_REAL_NAME ${OVITO_PYTHON_DEP_REAL} NAME)
#				EXECUTE_PROCESS(COMMAND "${CMAKE_COMMAND}" -E create_symlink ${OVITO_PYTHON_DEP_REAL_NAME} "${CMAKE_CURRENT_BINARY_DIR}/${PYTHON_DEPENDENCY}")
#				INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PYTHON_DEPENDENCY} DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}/" COMPONENT "runtime")
#			ENDIF()
		ENDFOREACH()
	ENDIF()
ELSEIF(WIN32)
	# Install standard Python packages. 
	LIST(GET PYTHON_LIBRARIES 0 PYTHON_FIRST_LIBRARY)
	IF(PYTHON_FIRST_LIBRARY STREQUAL "optimized")
		LIST(GET PYTHON_LIBRARIES 1 PYTHON_FIRST_LIBRARY)
	ENDIF()
	GET_FILENAME_COMPONENT(PYTHON_LIBRARY_PATH "${PYTHON_FIRST_LIBRARY}" PATH)
	GET_FILENAME_COMPONENT(PYTHON_LIBRARY_NAME "${PYTHON_FIRST_LIBRARY}" NAME_WE)
	INSTALL(DIRECTORY "${PYTHON_LIBRARY_PATH}/../Lib" 
		DESTINATION "${OVITO_RELATIVE_BINARY_DIRECTORY}"
		COMPONENT "runtime"
		PATTERN "*.pyc" EXCLUDE 
		PATTERN "*.pyo" EXCLUDE
		PATTERN "__pycache__" EXCLUDE)

	# Also install the compiled modules that are shipped with Python.
	INSTALL(DIRECTORY "${PYTHON_LIBRARY_PATH}/../DLLs"
		DESTINATION "${OVITO_RELATIVE_BINARY_DIRECTORY}" COMPONENT "runtime")

	# Install Python interpreter dlls.
	OVITO_INSTALL_DLL("${PYTHON_LIBRARY_PATH}/../${PYTHON_LIBRARY_NAME}.dll")
	OVITO_INSTALL_DLL("${PYTHON_LIBRARY_PATH}/../python3.dll")

	# Install Visual C++ runtime needed by Python interpreter.
	OVITO_INSTALL_DLL("${PYTHON_LIBRARY_PATH}/../vcruntime140.dll")
ENDIF()

# Propagate list of plugins to parent scope.
SET(OVITO_PLUGIN_LIST ${OVITO_PLUGIN_LIST} PARENT_SCOPE)
