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

# This file defines release version information.

# This is the canonical program version number:
SET(OVITO_VERSION_MAJOR 		"3")
SET(OVITO_VERSION_MINOR 		"0")
SET(OVITO_VERSION_REVISION		"0")

# Increment the following version counter every time the .ovito file format
# changes in a backward-incompatible way.
SET(OVITO_FILE_FORMAT_VERSION	"30004")

# Extract revision number from Git repository in order to tag development builds of OVITO.
FIND_PACKAGE(Git)
IF(GIT_FOUND)
	# Count the number of commits since the most recent git tag.
	EXECUTE_PROCESS(COMMAND ${GIT_EXECUTABLE} "describe"
		WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
		RESULT_VARIABLE GIT_RESULT_VAR
		OUTPUT_VARIABLE OVITO_VERSION_STRING
		OUTPUT_STRIP_TRAILING_WHITESPACE
		ERROR_QUIET)
	IF(GIT_RESULT_VAR STREQUAL "0")
		STRING(REGEX REPLACE "v[0-9.]*" "${OVITO_VERSION_MAJOR}.${OVITO_VERSION_MINOR}.${OVITO_VERSION_REVISION}" OVITO_VERSION_STRING "${OVITO_VERSION_STRING}")
		STRING(REGEX REPLACE "-g[A-Fa-f0-9]*" "" OVITO_VERSION_STRING "${OVITO_VERSION_STRING}")
		STRING(REGEX REPLACE "-" "-dev" OVITO_VERSION_STRING "${OVITO_VERSION_STRING}")
	ELSE()
		# If there is no tag in this branch, count the number of commits since the very beginning.
		# This is the case for the ovito-pro repository, which has no tags yet.
		IF("${GIT_VERSION_STRING}" VERSION_LESS "1.7.3")
			# Workaround for old git versions which don't support the --count option.
			# We use "wc -l" instead to count the number of commits.
			EXECUTE_PROCESS(COMMAND ${GIT_EXECUTABLE} "rev-list" "HEAD"
				COMMAND  "wc" "-l"
				WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
				RESULT_VARIABLE GIT_RESULT_VAR
				OUTPUT_VARIABLE GIT_REVISION_NUMBER
				OUTPUT_STRIP_TRAILING_WHITESPACE)
		ELSE()
			EXECUTE_PROCESS(COMMAND ${GIT_EXECUTABLE} "rev-list" "--count" "HEAD"
				WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
				RESULT_VARIABLE GIT_RESULT_VAR
				OUTPUT_VARIABLE GIT_REVISION_NUMBER
				OUTPUT_STRIP_TRAILING_WHITESPACE)
		ENDIF()
		IF(NOT GIT_RESULT_VAR STREQUAL "0")
			MESSAGE(FATAL "Failed to run git rev-list: ${GIT_RESULT_VAR}")
		ENDIF()
		# Add offset of 530 to the commit number to maintain continuous numbering sequence.
		MATH(EXPR GIT_REVISION_NUMBER "${GIT_REVISION_NUMBER}+530")
		SET(OVITO_VERSION_STRING "${OVITO_VERSION_MAJOR}.${OVITO_VERSION_MINOR}.${OVITO_VERSION_REVISION}-dev${GIT_REVISION_NUMBER}")
	ENDIF()
ELSE()
	SET(OVITO_VERSION_STRING "${OVITO_VERSION_MAJOR}.${OVITO_VERSION_MINOR}.${OVITO_VERSION_REVISION}")
ENDIF()

# The application's name:
SET(OVITO_APPLICATION_NAME "Ovito" CACHE STRING "The name of the application being built.")
MARK_AS_ADVANCED(OVITO_APPLICATION_NAME)

# The copyright notice shown in the application's About dialog:
IF(NOT OVITO_COPYRIGHT_NOTICE)
	STRING(TIMESTAMP _CURRENT_YEAR "%Y")
	SET(OVITO_COPYRIGHT_NOTICE
		"<p>A scientific visualization and analysis software for atomistic simulation data.</p>\
		 <p>Copyright (C) ${_CURRENT_YEAR}, Alexander Stukowski</p>\
		 <p>\
		 This is free, open-source software, and you are welcome to redistribute\
		 it under certain conditions. See the user manual for copying conditions.</p>\
		 <p><a href=\\\"https://www.ovito.org/\\\">https://www.ovito.org/</a></p>")
ENDIF()

# Export variables to parent scope.
GET_DIRECTORY_PROPERTY(_hasParent PARENT_DIRECTORY)
IF(_hasParent)
	SET(OVITO_VERSION_MAJOR "${OVITO_VERSION_MAJOR}" PARENT_SCOPE)
	SET(OVITO_VERSION_MINOR "${OVITO_VERSION_MINOR}" PARENT_SCOPE)
	SET(OVITO_VERSION_REVISION "${OVITO_VERSION_REVISION}" PARENT_SCOPE)
	SET(OVITO_VERSION_STRING "${OVITO_VERSION_STRING}" PARENT_SCOPE)
	SET(OVITO_APPLICATION_NAME "${OVITO_APPLICATION_NAME}" PARENT_SCOPE)
	SET(OVITO_COPYRIGHT_NOTICE "${OVITO_COPYRIGHT_NOTICE}" PARENT_SCOPE)
ENDIF()