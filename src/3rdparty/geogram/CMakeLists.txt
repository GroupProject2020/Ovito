###############################################################################
# 
#  Copyright (2018) Alexander Stukowski
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

# Compile the Geogram library (Delaunay "Pluggable Software Module")
ADD_LIBRARY(geogram STATIC Delaunay_psm.cpp)

# Set floating-point math compiler options.
IF(UNIX OR APPLE)
	TARGET_COMPILE_OPTIONS(geogram PRIVATE "-frounding-math" "-ffp-contract=off")
	IF(APPLE)
		TARGET_COMPILE_OPTIONS(geogram PRIVATE "-Wno-ignored-optimization-argument")
	ENDIF()
ENDIF()

# Enable Geogram's support for 64-bit indices.
TARGET_COMPILE_DEFINITIONS(geogram PUBLIC "GARGANTUA=1" )

# Build static version of Geogram lib.
TARGET_COMPILE_DEFINITIONS(geogram PUBLIC "GEO_STATIC_LIBS=1")

# Set visibility of symbols in this shared library to hidden by default, except those exported in the source code.
SET_TARGET_PROPERTIES(geogram PROPERTIES CXX_VISIBILITY_PRESET "hidden")
SET_TARGET_PROPERTIES(geogram PROPERTIES VISIBILITY_INLINES_HIDDEN ON)

# Make header files of this library available to dependent targets.
TARGET_INCLUDE_DIRECTORIES(geogram INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/..")
SET_PROPERTY(TARGET geogram PROPERTY POSITION_INDEPENDENT_CODE ON)

# Export this target.
INSTALL(TARGETS geogram EXPORT OVITO ARCHIVE DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}" CONFIGURATIONS "")
