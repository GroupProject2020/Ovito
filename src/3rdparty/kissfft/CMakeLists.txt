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

# Build the KISS FFT library.
ADD_LIBRARY(kissfft
	kiss_fft.c
	kiss_fftnd.c
)

# Give our library file a new name to not confuse it with any system versions of the library.
SET_TARGET_PROPERTIES(kissfft PROPERTIES OUTPUT_NAME "ovito_kissfft")

# Use double precision for FFT calculation.
TARGET_COMPILE_DEFINITIONS(kissfft PUBLIC "kiss_fft_scalar=double")

# Make header files of this library available to dependent targets.
TARGET_INCLUDE_DIRECTORIES(kissfft INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/..")

# Set visibility of symbols in this shared library to hidden by default, except those exported in the source code.
SET_TARGET_PROPERTIES(kissfft PROPERTIES CXX_VISIBILITY_PRESET "hidden")
SET_TARGET_PROPERTIES(kissfft PROPERTIES VISIBILITY_INLINES_HIDDEN ON)

# Export this target.
INSTALL(TARGETS kissfft EXPORT OVITO
	RUNTIME DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}"
	LIBRARY DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}"
	ARCHIVE DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}" COMPONENT "development")
