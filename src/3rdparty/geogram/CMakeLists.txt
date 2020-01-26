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

# Compile the Geogram library (Delaunay "Pluggable Software Module")
ADD_LIBRARY(geogram Delaunay_psm.cpp)

# Set floating-point math compiler options.
IF((UNIX OR APPLE) AND NOT EMSCRIPTEN)
	TARGET_COMPILE_OPTIONS(geogram PRIVATE "-frounding-math" "-ffp-contract=off")
	IF(APPLE)
		TARGET_COMPILE_OPTIONS(geogram PRIVATE "-Wno-ignored-optimization-argument")
	ENDIF()
ENDIF()

# Enable Geogram's support for 64-bit indices.
TARGET_COMPILE_DEFINITIONS(geogram PUBLIC "GARGANTUA=1" )

IF(BUILD_SHARED_LIBS)
	# Build shared library version of Geogram lib.
	TARGET_COMPILE_DEFINITIONS(geogram PRIVATE "geogram_EXPORTS")
ELSE()
	# Build static version of Geogram lib.
	TARGET_COMPILE_DEFINITIONS(geogram PUBLIC "GEO_STATIC_LIBS=1")
	SET_PROPERTY(TARGET geogram PROPERTY POSITION_INDEPENDENT_CODE ON)
ENDIF()

# Give our library file a new name to not confuse it with any system versions of the library.
SET_TARGET_PROPERTIES(geogram PROPERTIES OUTPUT_NAME "ovito_geogram")

# Set visibility of symbols in this shared library to hidden by default, except those exported in the source code.
SET_TARGET_PROPERTIES(geogram PROPERTIES CXX_VISIBILITY_PRESET "hidden")
SET_TARGET_PROPERTIES(geogram PROPERTIES VISIBILITY_INLINES_HIDDEN ON)

# Make header files of this library available to dependent targets.
TARGET_INCLUDE_DIRECTORIES(geogram INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/..")

# Export this target.
INSTALL(TARGETS geogram EXPORT OVITO
	RUNTIME DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}"
	LIBRARY DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}"
	ARCHIVE DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}" COMPONENT "development")
