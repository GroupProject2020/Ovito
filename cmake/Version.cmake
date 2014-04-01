
# This helper file provides information on the program version of OVITO. 

SET(OVITO_VERSION_MAJOR 		"2")
SET(OVITO_VERSION_MINOR 		"3")
SET(OVITO_VERSION_REVISION		"1")
SET(OVITO_FILE_FORMAT_VERSION	"20003")

# Make the version information available to the source code.

ADD_DEFINITIONS("-DOVITO_VERSION_MAJOR=${OVITO_VERSION_MAJOR}")
ADD_DEFINITIONS("-DOVITO_VERSION_MINOR=${OVITO_VERSION_MINOR}")
ADD_DEFINITIONS("-DOVITO_VERSION_REVISION=${OVITO_VERSION_REVISION}")
ADD_DEFINITIONS("-DOVITO_FILE_FORMAT_VERSION=${OVITO_FILE_FORMAT_VERSION}")

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
	ADD_DEFINITIONS("-DOVITO_VERSION_STRING=\"${OVITO_VERSION_STRING}\"")
ELSE()
	ADD_DEFINITIONS("-DOVITO_VERSION_STRING=\"${OVITO_VERSION_MAJOR}.${OVITO_VERSION_MINOR}.${OVITO_VERSION_REVISION}\"")
ENDIF()
