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

# Build library.
ADD_LIBRARY(muParser
	muParser.cpp
	muParserBytecode.cpp
	muParserInt.cpp
	muParserTokenReader.cpp
	muParserError.cpp
	muParserCallback.cpp
	muParserBase.cpp
)

# Give our library file a new name to not confuse it with any system versions of the library.
SET_TARGET_PROPERTIES(muParser PROPERTIES OUTPUT_NAME "ovito_muparser")

# Make header files of this library available to dependent targets.
TARGET_INCLUDE_DIRECTORIES(muParser INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/..")

# Export this target.
INSTALL(TARGETS muParser EXPORT OVITO
	RUNTIME DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}"
	LIBRARY DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}"
	ARCHIVE DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}" COMPONENT "development")
