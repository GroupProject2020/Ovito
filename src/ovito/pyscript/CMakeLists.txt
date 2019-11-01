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
		OpenGLRenderer
	PYTHON_WRAPPERS
		"${CMAKE_CURRENT_SOURCE_DIR}/python/"
)

IF(NOT OVITO_BUILD_PYTHON_PACKAGE)
	# Locate the Python headers and library.
	SET(Python_ADDITIONAL_VERSIONS 3.4 3.5 3.6 3.7 3.8)
	FIND_PACKAGE(PythonInterp REQUIRED)
	FIND_PACKAGE(PythonLibs REQUIRED)

	# Add Python library and include directory.
	TARGET_INCLUDE_DIRECTORIES(PyScript PUBLIC ${PYTHON_INCLUDE_PATH})
	TARGET_LINK_LIBRARIES(PyScript PUBLIC ${PYTHON_LIBRARIES})

	CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/python/ovito/plugins/__init__.embedded.py" "${OVITO_PYTHON_DIRECTORY}/ovito/plugins/__init__.py" COPYONLY)
ELSE()
	# Locate the Python headers and library.
	FIND_PACKAGE(Python3 REQUIRED COMPONENTS Interpreter Development)
	TARGET_LINK_LIBRARIES(PyScript PUBLIC Python3::Module)

	CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/python/ovito/plugins/__init__.external.py" "${OVITO_PYTHON_DIRECTORY}/ovito/plugins/__init__.py" COPYONLY)
ENDIF()
INSTALL(FILES "${OVITO_PYTHON_DIRECTORY}/ovito/plugins/__init__.py" DESTINATION "${OVITO_RELATIVE_PYTHON_DIRECTORY}/ovito/plugins/")

# Speed up compilation by using precompiled headers.
IF(OVITO_USE_PRECOMPILED_HEADERS)
	ADD_PRECOMPILED_HEADER(PyScript ovito/pyscript/PyScript.h)
ENDIF()

# Build the monolithic Python extension module if requested.
IF(OVITO_BUILD_MONOLITHIC AND OVITO_BUILD_PYTHON_PACKAGE)
	Python3_add_library(ovito_bindings MODULE binding/MonolithicExtension.cpp)

	# Set visibility of symbols in this shared library to hidden by default, except those exported in the source code.
	SET_TARGET_PROPERTIES(ovito_bindings PROPERTIES CXX_VISIBILITY_PRESET "hidden")
	SET_TARGET_PROPERTIES(ovito_bindings PROPERTIES VISIBILITY_INLINES_HIDDEN ON)

	INSTALL(TARGETS ovito_bindings EXPORT OVITO
		RUNTIME DESTINATION "${OVITO_RELATIVE_PLUGINS_DIRECTORY}"
		LIBRARY DESTINATION "${OVITO_RELATIVE_PLUGINS_DIRECTORY}"
		ARCHIVE DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}" COMPONENT "development")
ENDIF()

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
		SET(OVITO_PYTHON_DEPENDENCIES libssl.so libcrypto.so liblapack.so.3 libblas.so.3 libquadmath.so.0 libgfortran.so.3 libncurses.so libsqlite3.so.0 libffi.so)
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
	IF(NOT OVITO_BUILD_PYTHON_PACKAGE)
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

		# Also install the Tcl/Tk files shipping with the Python interpreter.
		INSTALL(DIRECTORY "${PYTHON_LIBRARY_PATH}/../tcl/" DESTINATION "${OVITO_RELATIVE_BINARY_DIRECTORY}/Lib/")
	ENDIF()
ENDIF()

# Propagate list of plugins to parent scope.
SET(OVITO_PLUGIN_LIST ${OVITO_PLUGIN_LIST} PARENT_SCOPE)
