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

# Find Libav libraries for video encoding support.
FIND_PACKAGE(Libav)

# Find the zlib library. It is required for reading/writing compressed data files.
FIND_PACKAGE(ZLIB REQUIRED)

# Locate the libssh library. It is required for the built-in SSH client.
FIND_PACKAGE(Libssh REQUIRED)

# Locate the Boost library.
SET(_Boost_IMPORTED_TARGETS TRUE) # Workaround for bug in CMake 3.5, which doesn't define the Boost::boost imported target.
FIND_PACKAGE(Boost REQUIRED)
IF(NOT TARGET Boost::boost)
	MESSAGE(FATAL_ERROR "The Boost library was not found on your system. Please make sure it is available.")
ENDIF()

SET(SourceFiles
	utilities/Exception.cpp
	utilities/linalg/AffineDecomposition.cpp
	utilities/io/SaveStream.cpp
	utilities/io/LoadStream.cpp
	utilities/io/gzdevice/GzipIODevice.cpp
	app/Application.cpp
	app/ApplicationService.cpp
	app/PluginManager.cpp
	app/StandaloneApplication.cpp
	oo/CloneHelper.cpp
	oo/OvitoClass.cpp
	oo/OvitoObject.cpp
	oo/OvitoObjectExecutor.cpp
	oo/PropertyFieldDescriptor.cpp
	oo/PropertyField.cpp
	oo/RefMaker.cpp
	oo/RefMakerClass.cpp
	oo/RefTarget.cpp
	oo/RefTargetListener.cpp
	dataset/animation/AnimationSettings.cpp
	dataset/animation/controller/Controller.cpp
	dataset/animation/controller/ConstantControllers.cpp
	dataset/animation/controller/KeyframeController.cpp
	dataset/animation/controller/LinearInterpolationControllers.cpp
	dataset/animation/controller/SplineInterpolationControllers.cpp
	dataset/animation/controller/TCBInterpolationControllers.cpp
	dataset/animation/controller/AnimationKeys.cpp
	dataset/animation/controller/LookAtController.cpp
	dataset/animation/controller/PRSTransformationController.cpp
	dataset/DataSet.cpp
	dataset/DataSetContainer.cpp
	dataset/UndoStack.cpp
	dataset/io/FileImporter.cpp
	dataset/io/FileExporter.cpp
	dataset/io/FileSourceImporter.cpp
	dataset/io/FileSource.cpp
	dataset/io/AttributeFileExporter.cpp
	dataset/scene/SceneNode.cpp
	dataset/scene/PipelineSceneNode.cpp
	dataset/scene/RootSceneNode.cpp
	dataset/scene/SelectionSet.cpp
	dataset/data/DataVis.cpp
	dataset/data/DataObject.cpp
	dataset/data/DataCollection.cpp
	dataset/data/TransformingDataVis.cpp
	dataset/data/TransformedDataObject.cpp
	dataset/data/camera/AbstractCameraObject.cpp
	dataset/data/AttributeDataObject.cpp
	dataset/pipeline/PipelineFlowState.cpp
	dataset/pipeline/PipelineCache.cpp
	dataset/pipeline/PipelineObject.cpp
	dataset/pipeline/CachingPipelineObject.cpp
	dataset/pipeline/AsynchronousModifier.cpp
	dataset/pipeline/Modifier.cpp
	dataset/pipeline/ModifierApplication.cpp
	dataset/pipeline/ModifierTemplates.cpp
	dataset/pipeline/AsynchronousModifierApplication.cpp
	dataset/pipeline/AsynchronousDelegatingModifier.cpp
	dataset/pipeline/StaticSource.cpp
	dataset/pipeline/DelegatingModifier.cpp
	utilities/units/UnitsManager.cpp
	utilities/io/ObjectSaveStream.cpp
	utilities/io/ObjectLoadStream.cpp
	utilities/io/FileManager.cpp
	utilities/io/RemoteFileJob.cpp
	utilities/io/CompressedTextReader.cpp
	utilities/io/CompressedTextWriter.cpp
	utilities/io/ssh/SshConnection.cpp
	utilities/io/ssh/SshChannel.cpp
	utilities/io/ssh/ProcessChannel.cpp
	utilities/io/ssh/ScpChannel.cpp
	utilities/io/ssh/LsChannel.cpp
	utilities/io/ssh/CatChannel.cpp
	utilities/concurrent/TaskWatcher.cpp
	utilities/concurrent/Promise.cpp
	utilities/concurrent/Task.cpp
	utilities/concurrent/MainThreadTask.cpp
	utilities/concurrent/ProgressiveTask.cpp
	utilities/concurrent/ThreadSafeTask.cpp
	utilities/concurrent/TrackingTask.cpp
	utilities/concurrent/TaskManager.cpp
	utilities/concurrent/AsynchronousTask.cpp
	utilities/mesh/TriMesh.cpp
	rendering/SceneRenderer.cpp
	rendering/noninteractive/NonInteractiveSceneRenderer.cpp
	rendering/noninteractive/DefaultLinePrimitive.cpp
	rendering/noninteractive/DefaultParticlePrimitive.cpp
	rendering/noninteractive/DefaultTextPrimitive.cpp
	rendering/noninteractive/DefaultImagePrimitive.cpp
	rendering/noninteractive/DefaultArrowPrimitive.cpp
	rendering/noninteractive/DefaultMeshPrimitive.cpp
	rendering/noninteractive/DefaultMarkerPrimitive.cpp
	rendering/RenderSettings.cpp
	rendering/FrameBuffer.cpp
	viewport/Viewport.cpp
	viewport/ViewportConfiguration.cpp
	viewport/ViewportSettings.cpp
	viewport/overlays/ViewportOverlay.cpp
	viewport/overlays/CoordinateTripodOverlay.cpp
	viewport/overlays/TextLabelOverlay.cpp
)

# Include video encoder interface code.
IF(LIBAV_FOUND)
	LIST(APPEND SourceFiles utilities/io/video/VideoEncoder.cpp)
ELSE()
	MESSAGE("Libav video encoding library not found. OVITO will be built without support for video output.")
ENDIF()

# Compile Qt resources, which will be linked into the executable.
QT5_ADD_RESOURCES(ResourceFiles resources/core.qrc)

# Define library target.
OVITO_STANDARD_PLUGIN(Core
	SOURCES Core.cpp ${SourceFiles} ${ResourceFiles}
	LIB_DEPENDENCIES Boost::boost ZLIB::ZLIB
	PRIVATE_LIB_DEPENDENCIES Qt5::Network Libssh::Libssh
)

# Link to video encoding lib.
IF(LIBAV_FOUND)
	TARGET_COMPILE_DEFINITIONS(Core PUBLIC OVITO_VIDEO_OUTPUT_SUPPORT)
	TARGET_LINK_LIBRARIES(Core PRIVATE "${Libav_LIBRARIES}")
	TARGET_INCLUDE_DIRECTORIES(Core BEFORE PRIVATE "${Libav_INCLUDE_DIRS}")
ENDIF()

# Define the OVITO_DEBUG macro in debug builds.
TARGET_COMPILE_DEFINITIONS(Core PUBLIC "$<$<CONFIG:Debug>:OVITO_DEBUG>")

# Turn off warnings about undefined instantiation of static variable of class templates.
IF(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	TARGET_COMPILE_OPTIONS(Core PUBLIC "-Wno-undefined-var-template")
ENDIF()

# Floating point precision control.
IF(NOT OVITO_DOUBLE_PRECISION_FP)
    TARGET_COMPILE_DEFINITIONS(Core PUBLIC "FLOATTYPE_FLOAT")
ENDIF()

IF(OVITO_BUILD_APPSTORE_VERSION)
    TARGET_COMPILE_DEFINITIONS(Core PUBLIC "OVITO_BUILD_APPSTORE_VERSION")
ENDIF()
IF(OVITO_BUILD_MONOLITHIC)
	TARGET_COMPILE_DEFINITIONS(Core PUBLIC "OVITO_BUILD_MONOLITHIC")
ENDIF()

# Inject program release version into the source code.
SET_PROPERTY(SOURCE app/Application.cpp app/StandaloneApplication.cpp
	APPEND PROPERTY COMPILE_DEFINITIONS
	"OVITO_VERSION_MAJOR=${OVITO_VERSION_MAJOR}"
	"OVITO_VERSION_MINOR=${OVITO_VERSION_MINOR}"
	"OVITO_VERSION_REVISION=${OVITO_VERSION_REVISION}"
	"OVITO_VERSION_STRING=\"${OVITO_VERSION_STRING}\""
	"OVITO_APPLICATION_NAME=\"${OVITO_APPLICATION_NAME}\"")
SET_PROPERTY(SOURCE utilities/io/SaveStream.cpp utilities/io/LoadStream.cpp
	APPEND PROPERTY COMPILE_DEFINITIONS
    "OVITO_FILE_FORMAT_VERSION=${OVITO_FILE_FORMAT_VERSION}")

# Run clang-tidy to check C++ code and perform static analysis.
IF(OVITO_CLANG_TIDY_CMD)
	SET_TARGET_PROPERTIES(Core PROPERTIES CXX_CLANG_TIDY "${OVITO_CLANG_TIDY_CMD}")
ENDIF()

# Speed up compilation by using precompiled headers.
IF(OVITO_USE_PRECOMPILED_HEADERS)
	ADD_PRECOMPILED_HEADER(Core ovito/core/Core.h)
ENDIF()

# Install Libav video encoding libraries.
IF(LIBAV_FOUND)
	FOREACH(lib ${Libav_LIBRARIES})
		GET_FILENAME_COMPONENT(base_name "${lib}" NAME_WE)
		GET_FILENAME_COMPONENT(lib_path "${lib}" PATH)
		IF(WIN32)
			FILE(GLOB lib_dll "${lib_path}/${base_name}-*.dll")
			IF(NOT lib_dll)
				MESSAGE(FATAL_ERROR "Could not find corresponding DLL for Libav library '${lib}' in same directory.")
			ENDIF()
			FOREACH(dll ${lib_dll})
				OVITO_INSTALL_SHARED_LIB("${dll}" ".")
			ENDFOREACH()
		ELSEIF(UNIX AND NOT APPLE AND OVITO_REDISTRIBUTABLE_PACKAGE)
			FILE(GLOB lib_versions "${lib}*")
			FOREACH(lib_version ${lib_versions})
				INSTALL(FILES "${lib_version}" DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}/")
			ENDFOREACH()
		ENDIF()
	ENDFOREACH()
ENDIF()

IF(WIN32)
	# Deploy the zlib DLL.
	GET_TARGET_PROPERTY(ZLIB_LIBRARY_LOCATION ZLIB::ZLIB LOCATION)
	GET_FILENAME_COMPONENT(ZLIB_LIBRARY_PATH "${ZLIB_LIBRARY_LOCATION}" PATH)
	OVITO_INSTALL_SHARED_LIB("${ZLIB_LIBRARY_PATH}/../bin/zlib.dll" ".")
ENDIF()

# Deploy the libssh library and its dependencies.
IF(WIN32)
	LIST(GET LIBSSH_LIBRARIES 0 LIBSSH_FIRST_LIBRARY)
	GET_FILENAME_COMPONENT(LIBSSH_LIBRARY_DIR "${LIBSSH_FIRST_LIBRARY}" PATH)
	GET_FILENAME_COMPONENT(LIBSSH_LIBRARY_NAME "${LIBSSH_FIRST_LIBRARY}" NAME_WE)
	OVITO_INSTALL_SHARED_LIB("${LIBSSH_LIBRARY_DIR}/../bin/${LIBSSH_LIBRARY_NAME}.dll" ".")

	# Also install the OpenSSL DLLs needed by libssh.
	SET(OPENSSL_ROOT_DIR "" CACHE PATH "Location of the OpenSSL library installation")
	IF(NOT OPENSSL_ROOT_DIR)
		MESSAGE(FATAL_ERROR "Please specify the location of the OpenSSL library by setting the OPENSSL_ROOT_DIR variable.")
	ENDIF()
	FILE(GLOB openssl_dlls "${OPENSSL_ROOT_DIR}/bin/*.dll")
	IF(NOT openssl_dlls)
		MESSAGE(FATAL_ERROR "Couldn't not find any OpenSSL DLLs in \"${OPENSSL_ROOT_DIR}/bin/\". Please check value of OPENSSL_ROOT_DIR path variable.")
	ENDIF()
	FOREACH(openssl_dll ${openssl_dlls})
		OVITO_INSTALL_SHARED_LIB("${openssl_dll}" ".")
	ENDFOREACH()

ELSEIF(UNIX AND NOT APPLE AND OVITO_REDISTRIBUTABLE_PACKAGE)

	FOREACH(lib ${LIBSSH_LIBRARIES})
		OVITO_INSTALL_SHARED_LIB("${lib}" ".")
	ENDFOREACH()

	# Also install libcrypto needed by libssh.
	FIND_LIBRARY(LIB_CRYPTO_LOCATION NAMES libcrypto.so PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu NO_DEFAULT_PATH)
	IF(NOT LIB_CRYPTO_LOCATION)
		MESSAGE(FATAL_ERROR "Could not find shared library libcrypto.so, which is required by libssh, in system path.")
	ENDIF()
	OVITO_INSTALL_SHARED_LIB("${LIB_CRYPTO_LOCATION}" ".")

ENDIF()
