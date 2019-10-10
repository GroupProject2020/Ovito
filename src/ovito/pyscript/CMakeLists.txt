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

# Locate the Python headers and library.
SET(Python_ADDITIONAL_VERSIONS 3.4 3.5 3.6 3.7 3.8)
FIND_PACKAGE(PythonInterp REQUIRED)
FIND_PACKAGE(PythonLibs REQUIRED)

IF(MSVC)
	# Precompiled headers require more compiler memory.
	ADD_COMPILE_OPTIONS(-Zm150)
ENDIF()

# Define the plugin module.
OVITO_STANDARD_PLUGIN(PyScript
	SOURCES
		PyScript.cpp
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
	LIB_DEPENDENCIES
		${PYTHON_LIBRARIES}
		OpenGLRenderer
	PYTHON_WRAPPERS
		"${CMAKE_CURRENT_SOURCE_DIR}/python/"
)

# Add Python include directory.
TARGET_INCLUDE_DIRECTORIES(PyScript PUBLIC ${PYTHON_INCLUDE_PATH})

# Speed up compilation by using precompiled headers.
IF(OVITO_USE_PRECOMPILED_HEADERS)
	ADD_PRECOMPILED_HEADER(PyScript ovito/pyscript/PyScript.h)
ENDIF()

# Install the Python source files that belong to the plugin and which provide the scripting interface.
INSTALL(DIRECTORY "${OVITO_PYTHON_DIRECTORY}/"
		DESTINATION "${OVITO_RELATIVE_PYTHON_DIRECTORY}/"
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
			PATTERN "*.pyc" EXCLUDE
			PATTERN "*.pyo" EXCLUDE
			PATTERN "__pycache__" EXCLUDE
			PATTERN "test" EXCLUDE
			PATTERN "*.exe" EXCLUDE
			PATTERN "sphinx" EXCLUDE)

		# Install Python header files.
		INSTALL(DIRECTORY "${PYTHON_INCLUDE_PATH}"
			DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}/../Frameworks/Python.framework/Versions/${PYTHON_MAJORMINOR_VERSION}/include/")
	ENDIF()
ELSEIF(UNIX)
	IF(OVITO_REDISTRIBUTABLE_PACKAGE)

		# Install standard Python packages.
		STRING(REGEX MATCH "[0-9]+\\.[0-9]+" PYTHON_MAJORMINOR_VERSION "${PYTHONLIBS_VERSION_STRING}")
		LIST(GET PYTHON_LIBRARIES 0 PYTHON_FIRST_LIBRARY)
		GET_FILENAME_COMPONENT(PYTHON_LIBRARY_PATH "${PYTHON_LIBRARY}" PATH)
		INSTALL(DIRECTORY "${PYTHON_LIBRARY_PATH}/python${PYTHON_MAJORMINOR_VERSION}"
			DESTINATION "lib/"
			PATTERN "*.pyc" EXCLUDE
			PATTERN "*.pyo" EXCLUDE
			PATTERN "__pycache__" EXCLUDE
			PATTERN "test" EXCLUDE
			PATTERN "sphinx" EXCLUDE
			PATTERN "Sphinx*" EXCLUDE)

		# Install Python header files.
		INSTALL(DIRECTORY "${PYTHON_INCLUDE_PATH}" DESTINATION "include/")

		# Install a copy of libpython.so shared library.
		CONFIGURE_FILE("${PYTHON_LIBRARIES}" "${OVITO_LIBRARY_DIRECTORY}" COPYONLY)
		INSTALL(FILES "${PYTHON_LIBRARIES}" DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}/")

		# Also install copies of shared libraries that are required by Python modules.
		SET(OVITO_PYTHON_DEPENDENCIES libssl.so libcrypto.so liblapack.so.3 libblas.so.3 libquadmath.so.0 libgfortran.so.3 libncurses.so libsqlite3.so.0 libffi.so.5)
		FOREACH(PYTHON_DEPENDENCY ${OVITO_PYTHON_DEPENDENCIES})
			FIND_LIBRARY(OVITO_PYTHON_DEP NAMES ${PYTHON_DEPENDENCY} PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu /usr/lib64 NO_DEFAULT_PATH)
			IF(NOT OVITO_PYTHON_DEP)
				MESSAGE(FATAL_ERROR "Could not find shared library ${PYTHON_DEPENDENCY}, which is required by Python interpreter, in system path.")
			ENDIF()
			OVITO_INSTALL_SHARED_LIB("${OVITO_PYTHON_DEP}" ".")
			UNSET(OVITO_PYTHON_DEP CACHE)
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
		PATTERN "*.pyc" EXCLUDE
		PATTERN "*.pyo" EXCLUDE
		PATTERN "__pycache__" EXCLUDE)

	# Also install the compiled modules that are shipped with Python.
	INSTALL(DIRECTORY "${PYTHON_LIBRARY_PATH}/../DLLs"
		DESTINATION "${OVITO_RELATIVE_BINARY_DIRECTORY}")

	# Install Python interpreter dlls.
	OVITO_INSTALL_SHARED_LIB("${PYTHON_LIBRARY_PATH}/../${PYTHON_LIBRARY_NAME}.dll" ".")
	OVITO_INSTALL_SHARED_LIB("${PYTHON_LIBRARY_PATH}/../python3.dll" ".")

	# Install Visual C++ runtime needed by Python interpreter.
	OVITO_INSTALL_SHARED_LIB("${PYTHON_LIBRARY_PATH}/../vcruntime140.dll" ".")
ENDIF()

# Propagate list of plugins to parent scope.
SET(OVITO_PLUGIN_LIST ${OVITO_PLUGIN_LIST} PARENT_SCOPE)
