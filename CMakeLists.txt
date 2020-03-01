#######################################################################################
#
#  Copyright 2020 Alexander Stukowski
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

# Make sure we have a recent version of CMake.
CMAKE_MINIMUM_REQUIRED(VERSION 3.2.0 FATAL_ERROR)
CMAKE_POLICY(VERSION 3.2.0)

# Choose CMake policies.
CMAKE_POLICY(SET CMP0020 NEW) 	# Automatically link to qtmain.lib on Windows.
CMAKE_POLICY(SET CMP0042 NEW) 	# Set MACOSX_RPATH target property to TRUE by default.
CMAKE_POLICY(SET CMP0007 NEW)   # Let LIST command not ignore empty elements.
IF(NOT CMAKE_VERSION VERSION_LESS "3.3")
	CMAKE_POLICY(SET CMP0063 NEW)   # Honor visibility properties for all target types.
ENDIF()
IF(NOT CMAKE_VERSION VERSION_LESS "3.9")
	CMAKE_POLICY(SET CMP0068 OLD)   # Use the 'RPATH' settings for 'install_name' on macOS.
ENDIF()
IF(NOT CMAKE_VERSION VERSION_LESS "3.13")
	CMAKE_POLICY(SET CMP0079 NEW)   # Allows use of target_link_libraries() with targets defined in other directories.
ENDIF()
IF(NOT CMAKE_VERSION VERSION_LESS "3.12")
	CMAKE_POLICY(SET CMP0074 NEW)   # find_package() uses <PackageName>_ROOT variables.
ENDIF()
IF(POLICY CMP0025)
	CMAKE_POLICY(SET CMP0025 NEW)   # Compiler id for Apple Clang is now "AppleClang".
ENDIF()

PROJECT(Ovito)

# Check compiler requirements.
IF(CMAKE_CXX_COMPILER_ID STREQUAL MSVC)
	IF(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "19")
		MESSAGE(FATAL_ERROR "OVITO requires Visual C++ 2015 or newer.")
	ENDIF()
ENDIF()

# This is to enable target debugging within Visual Studio Code.
# It is ignored outside of Visual Studio Code/CMake tools.
INCLUDE(CMakeToolsHelpers OPTIONAL)

IF(EMSCRIPTEN)
	INCLUDE(cmake/wasm.cmake)
ENDIF()

SET(OVITO_SOURCE_BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
LIST(APPEND CMAKE_MODULE_PATH "${OVITO_SOURCE_BASE_DIR}/cmake")
INCLUDE(cmake/Version.cmake)
INCLUDE(cmake/Plugins.cmake)
INCLUDE(cmake/PrecompiledHeader.cmake)

# Define build options.
OPTION(OVITO_DOUBLE_PRECISION_FP "Use double-precision floating-point numbers." "ON")
OPTION(OVITO_BUILD_GUI "Build the graphical user interface for the desktop application." "ON")
OPTION(OVITO_BUILD_WEBGUI "Build the graphical user interface for the web brwoser." "OFF")
OPTION(OVITO_BUILD_MONOLITHIC "Build a monolithic executable, statically linking all plugins." "OFF")
OPTION(OVITO_REDISTRIBUTABLE_PACKAGE "Create a redistributable program package that includes third-party libraries." "OFF")
OPTION(OVITO_BUILD_APPSTORE_VERSION "Build binaries for the Apple App Store and Windows Store." "OFF")
OPTION(OVITO_RUN_CLANG_TIDY "Run the clang-tidy tool to check code." "OFF")
OPTION(OVITO_USE_PRECOMPILED_HEADERS "Use precompiled C++ headers to speed up build." "ON")
OPTION(OVITO_DISABLE_THREADING "Disable multi-threading code (meant only for development purposes)." "OFF")

# Define user options that control the building of OVITO's standard plugins.
OPTION(OVITO_BUILD_PLUGIN_STDOBJ "Build the standard objects plugin." "ON")
OPTION(OVITO_BUILD_PLUGIN_STDMOD "Build the standard modifiers plugin." "ON")
OPTION(OVITO_BUILD_PLUGIN_PARTICLES "Build the plugin for particle and bond data." "ON")
OPTION(OVITO_BUILD_PLUGIN_MESH "Build the Mesh plugin." "ON")
OPTION(OVITO_BUILD_PLUGIN_GRID "Build the Grid plugin." "ON")
OPTION(OVITO_BUILD_PLUGIN_CRYSTALANALYSIS "Build the CrystalAnalysis plugin." "ON")
OPTION(OVITO_BUILD_PLUGIN_NETCDFPLUGIN "Build the NetCDF plugin." "ON")
OPTION(OVITO_BUILD_PLUGIN_CORRELATION "Build the spatial correlation function modifier plugin." "ON")
OPTION(OVITO_BUILD_PLUGIN_VOROTOP "Build the VoroTop modifier plugin." "ON")
OPTION(OVITO_BUILD_PLUGIN_GALAMOST "Build the GALAMOST I/O plugin." "ON")
OPTION(OVITO_BUILD_PLUGIN_OXDNA "Build the oxDNA I/O plugin." "ON")

# Experimental features:
OPTION(OVITO_BUILD_PLUGIN_VR "Build the virtual reality plugin (experimental)." "OFF")

# This is a global list of plugin targets that will be built.
# It will get populated by the OVITO_PLUGIN macro.
SET(OVITO_PLUGINS_LIST "")

# Set the default component name for the installer.
SET(CMAKE_INSTALL_DEFAULT_COMPONENT_NAME "Ovito")

# Enable software testing framework.
ENABLE_TESTING()

# Activate C++14 language standard.
SET(CMAKE_CXX_STANDARD 14)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)

IF(CYGWIN AND CMAKE_COMPILER_IS_GNUCXX)
	# Linking fails without -O3
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
ENDIF()

IF(APPLE)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++ -Wno-invalid-offsetof")
ENDIF()

IF(MSVC)
	# Suppress warning related to QVarLengthArray implementation.
	ADD_COMPILE_OPTIONS("/wd4996")
	# Suppress warning on conversion from size_t to int, possible loss of data.
	ADD_COMPILE_OPTIONS("/wd4267")
	# Suppress warning on missing definition for explicit template instantiation request.
	ADD_COMPILE_OPTIONS("/wd4661")
	# Compiling template code leads to large object files.
	ADD_COMPILE_OPTIONS("/bigobj")
ENDIF()

# Set the name of the macOS application bundle to generate.
SET(MACOSX_BUNDLE_NAME "Ovito")
SET(MACOSX_BUNDLE_BUNDLE_NAME "${MACOSX_BUNDLE_NAME}")

IF(NOT OVITO_BUILD_PYTHON_PACKAGE)
	IF(UNIX AND NOT APPLE)
		# The directory where the main executable goes to.
		SET(OVITO_RELATIVE_BINARY_DIRECTORY "bin")
		# The directory where the main libraries go to.
		SET(OVITO_RELATIVE_LIBRARY_DIRECTORY "lib/ovito")
		# The directory where the third-party libraries go to.
		SET(OVITO_RELATIVE_3RDPARTY_LIBRARY_DIRECTORY "lib/ovito")
		# The directory where the auxiliary files go to.
		SET(OVITO_RELATIVE_SHARE_DIRECTORY "share/ovito")
		# The directory where the compiled plugins go to.
		SET(OVITO_RELATIVE_PLUGINS_DIRECTORY "${OVITO_RELATIVE_LIBRARY_DIRECTORY}/plugins")
		# The directory where the Python source modules go to.
		IF(NOT OVITO_BUILD_CONDA)
			SET(OVITO_RELATIVE_PYTHON_DIRECTORY "${OVITO_RELATIVE_PLUGINS_DIRECTORY}/python")
		ELSE()
			# For Conda package builds:
			SET(OVITO_RELATIVE_PYTHON_DIRECTORY "$ENV{SP_DIR}")
		ENDIF()
	ELSEIF(APPLE)
		IF(NOT OVITO_BUILD_CONDA)
			# The directory where the main executable goes to.
			SET(OVITO_RELATIVE_BINARY_DIRECTORY "${MACOSX_BUNDLE_NAME}.app/Contents/MacOS")
			# The directory where the main libraries of Ovito go to.
			SET(OVITO_RELATIVE_LIBRARY_DIRECTORY "${MACOSX_BUNDLE_NAME}.app/Contents/MacOS")
			# The directory where the third-party libraries go to.
			SET(OVITO_RELATIVE_3RDPARTY_LIBRARY_DIRECTORY "${MACOSX_BUNDLE_NAME}.app/Contents/Frameworks")
			# The directory where the auxiliary files go to.
			SET(OVITO_RELATIVE_SHARE_DIRECTORY "${MACOSX_BUNDLE_NAME}.app/Contents/Resources")
			# The directory where the compiled plugins go to.
			SET(OVITO_RELATIVE_PLUGINS_DIRECTORY "${MACOSX_BUNDLE_NAME}.app/Contents/PlugIns")
			# The directory where the Python source modules go to.
			SET(OVITO_RELATIVE_PYTHON_DIRECTORY "${MACOSX_BUNDLE_NAME}.app/Contents/Resources/python")
		ELSE()
			# For Conda package builds:

			# The directory where the main executable goes to.
			SET(OVITO_RELATIVE_BINARY_DIRECTORY "bin")
			# The directory where the main libraries go to.
			SET(OVITO_RELATIVE_LIBRARY_DIRECTORY "lib/ovito")
			# The directory where the third-party libraries go to.
			SET(OVITO_RELATIVE_3RDPARTY_LIBRARY_DIRECTORY "${OVITO_RELATIVE_LIBRARY_DIRECTORY}")
			# The directory where the auxiliary files go to.
			SET(OVITO_RELATIVE_SHARE_DIRECTORY "share/ovito")
			# The directory where the compiled plugins go to.
			SET(OVITO_RELATIVE_PLUGINS_DIRECTORY "${OVITO_RELATIVE_LIBRARY_DIRECTORY}/plugins")
			# The directory where the Python source modules go to.
			SET(OVITO_RELATIVE_PYTHON_DIRECTORY "$ENV{SP_DIR}")
		ENDIF()	
	ELSEIF(WIN32)
		# The directory where the main executable goes to.
		SET(OVITO_RELATIVE_BINARY_DIRECTORY ".")
		# The directory where the main libraries go to.
		SET(OVITO_RELATIVE_LIBRARY_DIRECTORY ".")
		# The directory where the third-party libraries go to.
		SET(OVITO_RELATIVE_3RDPARTY_LIBRARY_DIRECTORY ".")
		# The directory where the auxiliary files go to.
		SET(OVITO_RELATIVE_SHARE_DIRECTORY ".")
		# The directory where the compiled plugins go to.
		SET(OVITO_RELATIVE_PLUGINS_DIRECTORY "${OVITO_RELATIVE_LIBRARY_DIRECTORY}/plugins")
		# The directory where the Python source modules go to.
		SET(OVITO_RELATIVE_PYTHON_DIRECTORY "${OVITO_RELATIVE_PLUGINS_DIRECTORY}/python")
	ENDIF()
ELSE()
	# For Python-only (PyPI wheel) builds:

	# The directory where the main executable goes to.
	SET(OVITO_RELATIVE_BINARY_DIRECTORY "ovito")
	# The directory where the main libraries go to.
	SET(OVITO_RELATIVE_LIBRARY_DIRECTORY "ovito/plugins")
	# The directory where the third-party libraries go to.
	SET(OVITO_RELATIVE_3RDPARTY_LIBRARY_DIRECTORY "${OVITO_RELATIVE_LIBRARY_DIRECTORY}")
	# The directory where the auxiliary files go to.
	SET(OVITO_RELATIVE_SHARE_DIRECTORY "ovito")
	# The directory where the compiled plugins go to.
	SET(OVITO_RELATIVE_PLUGINS_DIRECTORY "${OVITO_RELATIVE_LIBRARY_DIRECTORY}")
	# The directory where the Python source modules go to.
	SET(OVITO_RELATIVE_PYTHON_DIRECTORY ".")
ENDIF()

# The directory where the main executable goes to.
IF(NOT IS_ABSOLUTE "${OVITO_RELATIVE_BINARY_DIRECTORY}")
	SET(OVITO_BINARY_DIRECTORY "${Ovito_BINARY_DIR}/${OVITO_RELATIVE_BINARY_DIRECTORY}")
ELSE()
	SET(OVITO_BINARY_DIRECTORY "${OVITO_RELATIVE_BINARY_DIRECTORY}")
ENDIF()
# The directory where the main libraries go to.
IF(NOT IS_ABSOLUTE "${OVITO_RELATIVE_LIBRARY_DIRECTORY}")
	SET(OVITO_LIBRARY_DIRECTORY "${Ovito_BINARY_DIR}/${OVITO_RELATIVE_LIBRARY_DIRECTORY}")
ELSE()
	SET(OVITO_LIBRARY_DIRECTORY "${OVITO_RELATIVE_LIBRARY_DIRECTORY}")
ENDIF()
# The directory where the compiled plugins go to.
IF(NOT IS_ABSOLUTE "${OVITO_RELATIVE_PLUGINS_DIRECTORY}")
	SET(OVITO_PLUGINS_DIRECTORY "${Ovito_BINARY_DIR}/${OVITO_RELATIVE_PLUGINS_DIRECTORY}")
ELSE()
	SET(OVITO_PLUGINS_DIRECTORY "${OVITO_RELATIVE_PLUGINS_DIRECTORY}")
ENDIF()
# The directory where the Python source files go to.
IF(NOT IS_ABSOLUTE "${OVITO_RELATIVE_PYTHON_DIRECTORY}")
	SET(OVITO_PYTHON_DIRECTORY "${Ovito_BINARY_DIR}/${OVITO_RELATIVE_PYTHON_DIRECTORY}")
ELSE()
	SET(OVITO_PYTHON_DIRECTORY "${OVITO_RELATIVE_PYTHON_DIRECTORY}")
ENDIF()
# The directory where the auxiliary files go to.
IF(NOT IS_ABSOLUTE "${OVITO_RELATIVE_SHARE_DIRECTORY}")
	SET(OVITO_SHARE_DIRECTORY "${Ovito_BINARY_DIR}/${OVITO_RELATIVE_SHARE_DIRECTORY}")
ELSE()
	SET(OVITO_SHARE_DIRECTORY "${OVITO_RELATIVE_SHARE_DIRECTORY}")
ENDIF()

IF(APPLE OR WIN32 OR NOT OVITO_REDISTRIBUTABLE_PACKAGE)
	# Add the automatically determined parts of the RPATH,
	# which point to directories outside the build tree to the install RPATH.
	SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
ENDIF()

# Select linkage type (shared or static) for plugin libraries.
IF(OVITO_BUILD_MONOLITHIC)
	SET(BUILD_SHARED_LIBS OFF)
	SET(OVITO_PLUGIN_LIBRARY_SUFFIX "${CMAKE_STATIC_LIBRARY_SUFFIX}")
ELSE()
	SET(BUILD_SHARED_LIBS ON)
	# Define name suffix used for generating plugin libraries.
	IF(APPLE)
		# On macOS, we use the .so extension instead of the standard .dylib to be compatible
		# with the Python interpreter, which only finds modules having a .so suffix.
		SET(OVITO_PLUGIN_LIBRARY_SUFFIX ".so")
	ELSE()
		SET(OVITO_PLUGIN_LIBRARY_SUFFIX "${CMAKE_SHARED_LIBRARY_SUFFIX}")
	ENDIF()
ENDIF()

# Make prerequisites available.
INCLUDE(cmake/Prerequisites.cmake)

# Determine the command for running the clang-tidy tool.
IF(OVITO_RUN_CLANG_TIDY AND NOT CMAKE_VERSION VERSION_LESS "3.6")
	FIND_PROGRAM(CLANG_TIDY_EXE NAMES "clang-tidy" DOC "Path to clang-tidy executable")
	SET(OVITO_CLANG_TIDY_CMD "${CLANG_TIDY_EXE}") # List of checks will be read from the .clang-tidy file.
ENDIF()

# Generate all the build targets.
ADD_SUBDIRECTORY(src)

# Generate user documentation.
INCLUDE(cmake/Documentation.cmake)

# Export our targets so that external plugins can use them.
#CONFIGURE_FILE(cmake/OVITOConfig.cmake "${Ovito_BINARY_DIR}/" @ONLY)
#EXPORT(EXPORT OVITO NAMESPACE "Ovito::" FILE OVITOTargets.cmake)

# This allows external CMake projects to inject their customization code into the OVITO project:
IF(OVITO_CMAKE_FILE_INJECTION_PROJECT)
	INCLUDE("${OVITO_CMAKE_FILE_INJECTION_PROJECT}")
ENDIF()
