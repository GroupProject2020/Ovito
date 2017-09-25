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

# Find Libav libraries for video encoding support.
FIND_PACKAGE(Libav)

# Find the zlib library. It is required for reading/writing compressed data files.
FIND_PACKAGE(ZLIB REQUIRED)

SET(SourceFiles 
	utilities/Exception.cpp
	utilities/linalg/AffineDecomposition.cpp
	utilities/io/SaveStream.cpp
	utilities/io/LoadStream.cpp
	utilities/io/gzdevice/qtiocompressor.cpp
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
	dataset/scene/ObjectNode.cpp
	dataset/scene/SceneRoot.cpp
	dataset/scene/SelectionSet.cpp
	dataset/data/DisplayObject.cpp
	dataset/data/DataObject.cpp
	dataset/data/camera/CameraObject.cpp
	dataset/data/helpers/TargetObject.cpp
	dataset/data/simcell/SimulationCellObject.cpp
	dataset/data/simcell/SimulationCellDisplay.cpp
	dataset/data/simcell/PeriodicDomainDataObject.cpp
	dataset/data/properties/PropertyStorage.cpp
	dataset/data/properties/PropertyObject.cpp
	dataset/data/properties/PropertyClass.cpp
	dataset/data/properties/PropertyReference.cpp
	dataset/data/properties/ElementType.cpp
	dataset/pipeline/PipelineFlowState.cpp
	dataset/pipeline/PipelineObject.cpp
	dataset/pipeline/CachingPipelineObject.cpp
	dataset/pipeline/AsynchronousModifier.cpp
	dataset/pipeline/Modifier.cpp
	dataset/pipeline/ModifierApplication.cpp
	dataset/pipeline/InputHelper.cpp
	dataset/pipeline/OutputHelper.cpp
	dataset/pipeline/AsynchronousModifierApplication.cpp
	dataset/pipeline/StaticSource.cpp
	dataset/pipeline/modifiers/DelegatingModifier.cpp
	dataset/pipeline/modifiers/GenericPropertyModifier.cpp
	dataset/pipeline/modifiers/SliceModifier.cpp
	dataset/pipeline/modifiers/AffineTransformationModifier.cpp
	dataset/pipeline/modifiers/InvertSelectionModifier.cpp
	dataset/pipeline/modifiers/ClearSelectionModifier.cpp
	dataset/pipeline/modifiers/ColorCodingModifier.cpp
	dataset/pipeline/modifiers/AssignColorModifier.cpp
	dataset/pipeline/modifiers/DeleteSelectedModifier.cpp
	dataset/pipeline/modifiers/SelectTypeModifier.cpp
	dataset/pipeline/modifiers/HistogramModifier.cpp
	dataset/pipeline/modifiers/ScatterPlotModifier.cpp
	dataset/pipeline/modifiers/ReplicateModifier.cpp
	utilities/units/UnitsManager.cpp
	utilities/io/ObjectSaveStream.cpp
	utilities/io/ObjectLoadStream.cpp
	utilities/io/FileManager.cpp
	utilities/io/SftpJob.cpp
	utilities/io/CompressedTextReader.cpp
	utilities/io/CompressedTextWriter.cpp
	utilities/concurrent/PromiseWatcher.cpp
	utilities/concurrent/Promise.cpp
	utilities/concurrent/PromiseState.cpp
	utilities/concurrent/SynchronousPromiseState.cpp
	utilities/concurrent/PromiseStateWithProgress.cpp
	utilities/concurrent/ThreadSafePromiseState.cpp
	utilities/concurrent/TrackingPromiseState.cpp
	utilities/concurrent/TaskManager.cpp
	utilities/concurrent/Task.cpp
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
	viewport/overlays/ColorLegendOverlay.cpp	
)

# Include video encoder interface code.
IF(LIBAV_FOUND)
	LIST(APPEND SourceFiles utilities/io/video/VideoEncoder.cpp)
ELSE()
	MESSAGE("LibAV video encoding library not found. Will build OVITO without video support.")
ENDIF()

# Compile resources.
QT5_ADD_RESOURCES(ResourceFiles resources/core.qrc)

# Build library
ADD_LIBRARY(Core ${OVITO_DEFAULT_LIBRARY_TYPE} Core.cpp ${SourceFiles} ${ResourceFiles})

# Pass name of core plugin to the code.
TARGET_COMPILE_DEFINITIONS(Core PRIVATE "OVITO_PLUGIN_NAME=\"Core\"")

# Set default include directory.
TARGET_INCLUDE_DIRECTORIES(Core PUBLIC "$<BUILD_INTERFACE:${OVITO_SOURCE_BASE_DIR}/src>")

# Required by built-in SSH client.
TARGET_LINK_LIBRARIES(Core PRIVATE QSsh)

# Link to zlib library.
TARGET_LINK_LIBRARIES(Core PRIVATE ${ZLIB_LIBRARIES})
TARGET_INCLUDE_DIRECTORIES(Core PRIVATE "${ZLIB_INCLUDE_DIRS}")

# Add Boost include directories.
TARGET_INCLUDE_DIRECTORIES(Core PUBLIC ${Boost_INCLUDE_DIRS})

# Link to Qt5.
TARGET_LINK_LIBRARIES(Core PUBLIC Qt5::Core Qt5::Gui Qt5::Concurrent)
TARGET_LINK_LIBRARIES(Core PRIVATE Qt5::Network)

# Link to video encoding lib.
IF(LIBAV_FOUND)
	TARGET_COMPILE_DEFINITIONS(Core PUBLIC OVITO_VIDEO_OUTPUT_SUPPORT)
	TARGET_LINK_LIBRARIES(Core PRIVATE ${Libav_LIBRARIES})
	TARGET_INCLUDE_DIRECTORIES(Core BEFORE PRIVATE ${Libav_INCLUDE_DIRS})
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

# Define macro for symbol export from shared library.
IF(OVITO_BUILD_MONOLITHIC)
	TARGET_COMPILE_DEFINITIONS(Core PUBLIC "OVITO_CORE_EXPORT=")
ELSE()
	TARGET_COMPILE_DEFINITIONS(Core PRIVATE "OVITO_CORE_EXPORT=Q_DECL_EXPORT")
	TARGET_COMPILE_DEFINITIONS(Core INTERFACE "OVITO_CORE_EXPORT=Q_DECL_IMPORT")
ENDIF()

IF(OVITO_BUILD_APPSTORE_VERSION)
    TARGET_COMPILE_DEFINITIONS(Core PUBLIC "OVITO_BUILD_APPSTORE_VERSION")
ENDIF()
IF(OVITO_BUILD_MONOLITHIC)
	TARGET_COMPILE_DEFINITIONS(Core PUBLIC "OVITO_BUILD_MONOLITHIC")
ENDIF()

# Pass version information to source code.
SET_SOURCE_FILES_PROPERTIES(app/Application.cpp app/StandaloneApplication.cpp PROPERTIES COMPILE_DEFINITIONS 
    "OVITO_VERSION_MAJOR=${OVITO_VERSION_MAJOR};OVITO_VERSION_MINOR=${OVITO_VERSION_MINOR};OVITO_VERSION_REVISION=${OVITO_VERSION_REVISION};OVITO_VERSION_STRING=\"${OVITO_VERSION_STRING}\"")
SET_SOURCE_FILES_PROPERTIES(utilities/io/SaveStream.cpp utilities/io/LoadStream.cpp PROPERTIES COMPILE_DEFINITIONS 
    "OVITO_FILE_FORMAT_VERSION=${OVITO_FILE_FORMAT_VERSION}")

# Speed up compilation by using precompiled headers.
ADD_PRECOMPILED_HEADER(Core core/Core.h)

IF(APPLE)
    # Activate @rpath on MacOS.
    SET_TARGET_PROPERTIES(Core PROPERTIES MACOSX_RPATH TRUE)
	# This is required to avoid error by install_name_tool on MacOS.
	SET_TARGET_PROPERTIES(Core PROPERTIES LINK_FLAGS "-headerpad_max_install_names")
ELSEIF(UNIX)
    SET_TARGET_PROPERTIES(Core PROPERTIES INSTALL_RPATH "$ORIGIN")
ENDIF()

# This library is part of the installation package.
INSTALL(TARGETS Core EXPORT OVITO
	RUNTIME DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}" COMPONENT "runtime"
	LIBRARY DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}" COMPONENT "runtime"
	ARCHIVE DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}" COMPONENT "development")

# Export target to make it accessible for external plugins.
IF(CMAKE_VERSION VERSION_LESS "3")
	EXPORT(TARGETS Core NAMESPACE "Ovito::" APPEND FILE "${Ovito_BINARY_DIR}/OVITOTargets.cmake")
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
				OVITO_INSTALL_DLL("${dll}")
			ENDFOREACH()
		ELSEIF(UNIX AND NOT APPLE AND OVITO_REDISTRIBUTABLE_PACKAGE)
			FILE(GLOB lib_versions "${lib}*")
			FOREACH(lib_version ${lib_versions})
				INSTALL(FILES "${lib_version}" DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}/")
			ENDFOREACH()
		ENDIF()
	ENDFOREACH()
ENDIF()

# Install the zlib DLL.
IF(WIN32)
	LIST(GET ZLIB_LIBRARIES 0 ZLIB_FIRST_LIBRARY)
	GET_FILENAME_COMPONENT(ZLIB_LIBRARY_PATH "${ZLIB_FIRST_LIBRARY}" PATH)
	OVITO_INSTALL_DLL("${ZLIB_LIBRARY_PATH}/../bin/zlib.dll")
ENDIF()
