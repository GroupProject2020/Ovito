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
