# This file defines the program version. 

SET(OVITO_VERSION_MAJOR 		"2")
SET(OVITO_VERSION_MINOR 		"8")
SET(OVITO_VERSION_REVISION		"1")
SET(OVITO_FILE_FORMAT_VERSION	"20502")

# Extract revision tag name from Git repository.
FIND_PACKAGE(Git)
IF(GIT_FOUND)
	EXECUTE_PROCESS(COMMAND ${GIT_EXECUTABLE} "describe" WORKING_DIRECTORY ${OVITO_SOURCE_BASE_DIR} 
		RESULT_VARIABLE GIT_RESULT_VAR 
		OUTPUT_VARIABLE OVITO_VERSION_STRING
		OUTPUT_STRIP_TRAILING_WHITESPACE)
	IF(NOT GIT_RESULT_VAR STREQUAL "0")
		MESSAGE(FATAL "Failed to run Git: ${GIT_RESULT_VAR}")
	ENDIF()
	STRING(REGEX REPLACE "-g[A-Fa-f0-9]*" "" OVITO_VERSION_STRING "${OVITO_VERSION_STRING}")
	STRING(REGEX REPLACE "-" "-dev" OVITO_VERSION_STRING "${OVITO_VERSION_STRING}")
	STRING(REGEX REPLACE "^v" "" OVITO_VERSION_STRING "${OVITO_VERSION_STRING}")
ELSE()
	SET(OVITO_VERSION_STRING "${OVITO_VERSION_MAJOR}.${OVITO_VERSION_MINOR}.${OVITO_VERSION_REVISION}")
ENDIF()
