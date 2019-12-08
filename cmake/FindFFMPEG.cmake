#######################################################################################
#
#  Copyright 2019 Alexander Stukowski
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

# - Finds ffmpeg libraries and headers
#
#  Will look for header files in FFMPEG_INCLUDE_DIR if defined.
#  Will look for library files in FFMPEG_LIBRARY_DIR if defined.
#
# Once done this will define
#
#  FFMPEG_FOUND			- system has ffmpeg
#  FFMPEG_INCLUDE_DIRS	- the include directories
#  FFMPEG_LIBRARY_DIR	- the directory containing the libraries
#  FFMPEG_LIBRARIES		- link these to use ffmpeg
#

# List of required headers.
SET(FFMPEG_HEADER_NAMES libavformat/avformat.h libavcodec/avcodec.h libavutil/avutil.h libavfilter/avfilter.h libswscale/swscale.h)
# List of required libraries.
SET(FFMPEG_LIBRARY_NAMES avformat avcodec avutil avfilter swscale swresample)

# Detect header path.
FIND_PATH(FFMPEG_INCLUDE_DIR NAMES libavcodec/avcodec.h PATH_SUFFIXES ffmpeg)
SET(FFMPEG_INCLUDE_DIRS "${FFMPEG_INCLUDE_DIR}")

# Detect library path.
IF(NOT FFMPEG_LIBRARY_DIR)
	FIND_LIBRARY(FFMPEG_AVCODEC_LIBRARY NAMES avcodec)
	IF(FFMPEG_AVCODEC_LIBRARY)
		GET_FILENAME_COMPONENT(FFMPEG_LIBRARY_DIR "${FFMPEG_AVCODEC_LIBRARY}" PATH)
	ENDIF()
ENDIF()
SET(FFMPEG_LIBRARY_DIR ${FFMPEG_LIBRARY_DIR} CACHE PATH "Location of ffmpeg libraries.")
MARK_AS_ADVANCED(FFMPEG_INCLUDE_DIR FFMPEG_LIBRARY_DIR)

# Check if all required headers exist.
FOREACH(header ${FFMPEG_HEADER_NAMES})
	UNSET(header_path CACHE)
	FIND_PATH(header_path ${header} PATHS ${FFMPEG_INCLUDE_DIRS} NO_DEFAULT_PATH)
	IF(NOT header_path)
	    IF(NOT FFMPEG_FIND_QUIETLY)
		    MESSAGE("Could not find ffmpeg header file '${header}' in search path(s) '${FFMPEG_INCLUDE_DIR}'.")
        ENDIF()
        UNSET(FFMPEG_INCLUDE_DIR)
	ENDIF()
	UNSET(header_path CACHE)
ENDFOREACH()

# Find the full paths of the libraries
UNSET(FFMPEG_LIBRARIES)
FOREACH(lib ${FFMPEG_LIBRARY_NAMES})
	UNSET(lib_path CACHE)
	FIND_LIBRARY(lib_path ${lib} PATHS "${FFMPEG_LIBRARY_DIR}" NO_DEFAULT_PATH)
	IF(lib_path)
		LIST(APPEND FFMPEG_LIBRARIES "${lib_path}")
	ELSE()
	    IF(NOT FFMPEG_FIND_QUIETLY)
		    MESSAGE("Could not find ffmpeg library '${lib}' in search path(s) '${FFMPEG_LIBRARY_DIR}'.")
		ENDIF()
		UNSET(FFMPEG_LIBRARY_DIR)
	ENDIF()
	UNSET(lib_path CACHE)
ENDFOREACH()

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(FFMPEG DEFAULT_MSG FFMPEG_LIBRARY_DIR FFMPEG_INCLUDE_DIR)

IF(APPLE)
	# libbz2 is an indirect dependency that needs to be linked in on MacOS.
	FIND_PACKAGE(BZip2 REQUIRED)

	# Apple's system libraries are required too.
	FIND_LIBRARY(COREFOUNDATION_LIBRARY CoreFoundation)
	FIND_LIBRARY(COREVIDEO_LIBRARY CoreVideo)
	FIND_LIBRARY(VIDEODECODEACCELERATION_LIBRARY VideoDecodeAcceleration)

	LIST(APPEND FFMPEG_LIBRARIES ${BZIP2_LIBRARIES} ${COREFOUNDATION_LIBRARY} ${COREVIDEO_LIBRARY} ${VIDEODECODEACCELERATION_LIBRARY})
	LIST(APPEND FFMPEG_INCLUDE_DIRS ${BZIP2_INCLUDE_DIR})
ENDIF()
