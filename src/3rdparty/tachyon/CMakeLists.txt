#######################################################################################
#
#  Copyright 2019 Alexander Stukowski and Technische Universität Darmstadt
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

IF(WIN32)
	SET(BUILD_SHARED_LIBS OFF)
ENDIF()

# Build the library.
SET(TACHYON_SOURCE_FILES
	tachyon/api.c
	tachyon/apigeom.c
	tachyon/apitrigeom.c
	tachyon/box.c
	tachyon/camera.c
	tachyon/cone.c
	tachyon/coordsys.c
	tachyon/cylinder.c
	tachyon/extvol.c
	tachyon/global.c
	tachyon/grid.c
	tachyon/hash.c
	tachyon/imap.c
	tachyon/imageio.c
	tachyon/intersect.c
	tachyon/light.c
	tachyon/parallel.c
	tachyon/plane.c
	tachyon/quadric.c
	tachyon/render.c
	tachyon/ring.c
	tachyon/shade.c
	tachyon/sphere.c
	tachyon/texture.c
	tachyon/threads.c
	tachyon/tgafile.c
	tachyon/sgirgb.c
	tachyon/ppm.c
	tachyon/trace.c
	tachyon/triangle.c
	tachyon/ui.c
	tachyon/util.c
	tachyon/vector.c
	tachyon/vol.c
	tachyon/pngfile.c
	tachyon/jpeg.c
	tachyon/psd.c
	tachyon/winbmp.c
)

# Build library.
ADD_LIBRARY(TachyonLib ${TACHYON_SOURCE_FILES})

# Give our library file a new name to not confuse it with any system versions of the library.
SET_TARGET_PROPERTIES(TachyonLib PROPERTIES OUTPUT_NAME "ovito_tachyon")

TARGET_COMPILE_DEFINITIONS(TachyonLib PRIVATE TACHYON_NO_DEPRECATED THR _REENTRANT)
IF(APPLE)
	TARGET_COMPILE_DEFINITIONS(TachyonLib PRIVATE Bsd)
ELSEIF(UNIX)
	TARGET_COMPILE_DEFINITIONS(TachyonLib PRIVATE Linux)
ELSEIF(WIN32)
	TARGET_COMPILE_DEFINITIONS(TachyonLib PRIVATE WIN32)
ENDIF()

IF(APPLE OR UNIX)
	# Link to Pthreads multi-threading library on Unix/MacOS.
	TARGET_LINK_LIBRARIES(TachyonLib pthread)
ELSEIF(WIN32)
	# Link to ws2_32 library on Windows.
	TARGET_LINK_LIBRARIES(TachyonLib ws2_32)
ENDIF()

# Make header files of this library available to dependent targets.
TARGET_INCLUDE_DIRECTORIES(TachyonLib INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")

IF(NOT BUILD_SHARED_LIBS AND OVITO_BUILD_PYTHON_PACKAGE)
	# Since we will link this library into the dynamically loaded Python extension module, we need to use the fPIC flag.
	SET_PROPERTY(TARGET TachyonLib PROPERTY POSITION_INDEPENDENT_CODE ON)
ENDIF()

# Export this target.
INSTALL(TARGETS TachyonLib EXPORT OVITO
	RUNTIME DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}"
	LIBRARY DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}"
	ARCHIVE DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}" COMPONENT "development")
