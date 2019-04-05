###############################################################################
#
#  Copyright (2017) Alexander Stukowski
#
#  This file is part of OVITO (Open Visualization Tool).
#
#  OVITO is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 3 of the License, or
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

# This CMake script compiles the user manual for OVITO
# by transforming the docbook files to HTML.

# It also generates the scripting interface documentation using
# Sphinx, and the C++ API documentation using Doxygen.

# Controls the generation of the user manual.
OPTION(OVITO_BUILD_DOCUMENTATION "Build the user manual" "OFF")

# Find the XSLT processor program.
FIND_PROGRAM(XSLT_PROCESSOR "xsltproc" DOC "Path to the XSLT processor program used to build the documentation.")
IF(NOT XSLT_PROCESSOR)
	IF(OVITO_BUILD_DOCUMENTATION)
		MESSAGE(FATAL_ERROR "The XSLT processor program (xsltproc) was not found. Please install it and/or specify its location manually.")
	ENDIF()
ENDIF()
SET(XSLT_PROCESSOR_OPTIONS "--xinclude" CACHE STRING "Additional to pass to the XSLT processor program when building the documentation")
MARK_AS_ADVANCED(XSLT_PROCESSOR_OPTIONS)

# Create destination directories.
FILE(MAKE_DIRECTORY "${OVITO_SHARE_DIRECTORY}/doc/manual")
FILE(MAKE_DIRECTORY "${OVITO_SHARE_DIRECTORY}/doc/manual/html")

# XSL transform documentation files.
IF(XSLT_PROCESSOR)
	# This generates the user manual as a set of static HTML pages which get shipped with
	# the Ovito installation packages and which can be accessed from the Help menu of the application.
	ADD_CUSTOM_TARGET(documentation
					COMMAND ${CMAKE_COMMAND} "-E" copy_directory "images/" "${OVITO_SHARE_DIRECTORY}/doc/manual/html/images/"
					COMMAND ${CMAKE_COMMAND} "-E" copy "manual.css" "${OVITO_SHARE_DIRECTORY}/doc/manual/html/"
					COMMAND ${XSLT_PROCESSOR} "${XSLT_PROCESSOR_OPTIONS}" --nonet --stringparam base.dir "${OVITO_SHARE_DIRECTORY}/doc/manual/html/" html-customization-layer.xsl Manual.docbook
					WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/doc/manual/"
					COMMENT "Generating user documentation")

	# This generates the user manual which is put onto the www.ovito.org website.
	# It consists of a set of PHP files that adopt the look and feel of the WordPress site.
	ADD_CUSTOM_TARGET(wordpress_doc
					COMMAND ${CMAKE_COMMAND} "-E" copy_directory "images/" "${CMAKE_BINARY_DIR}/doc/wordpress/images/"
					COMMAND ${XSLT_PROCESSOR} "${XSLT_PROCESSOR_OPTIONS}" --nonet
						--stringparam base.dir "${CMAKE_BINARY_DIR}/doc/wordpress/"
						--stringparam ovito.version "${OVITO_VERSION_STRING}"
						wordpress-customization-layer.xsl Manual.docbook
					WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/doc/manual/"
					COMMENT "Generating user documentation (WordPress version)")

	INSTALL(DIRECTORY "${OVITO_SHARE_DIRECTORY}/doc/manual/html/" DESTINATION "${OVITO_RELATIVE_SHARE_DIRECTORY}/doc/manual/html/")
	IF(OVITO_BUILD_DOCUMENTATION)
		ADD_DEPENDENCIES(Ovito documentation)
	ENDIF()
ENDIf()

# Generate documentation for OVITO's scripting interface.
IF(OVITO_BUILD_PLUGIN_PYSCRIPT)

	# Use OVITO's built in Python interpreter to run the Sphinx doc program.
	ADD_CUSTOM_TARGET(scripting_documentation
				COMMAND "$<TARGET_FILE:ovitos>" "${CMAKE_SOURCE_DIR}/cmake/sphinx-build.py" "-b" "html" "-a" "-E"
				"-D" "version=${OVITO_VERSION_MAJOR}.${OVITO_VERSION_MINOR}"
				"-D" "release=${OVITO_VERSION_STRING}"
				"." "${OVITO_SHARE_DIRECTORY}/doc/manual/html/python/"
				WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/doc/python/"
				COMMENT "Generating scripting documentation")

	# Run Sphinx only after OVITO and all of its plugins have been built.
	ADD_DEPENDENCIES(scripting_documentation ovitos)

	IF(OVITO_BUILD_DOCUMENTATION)
		ADD_DEPENDENCIES(Ovito scripting_documentation)
	ENDIF()
ENDIF()

# Find the Doxygen program.
FIND_PACKAGE(Doxygen QUIET)

# Generate API documentation files.
IF(DOXYGEN_FOUND)
	ADD_CUSTOM_TARGET(apidocs
					COMMAND "env" "OVITO_VERSION_STRING=${OVITO_VERSION_MAJOR}.${OVITO_VERSION_MINOR}.${OVITO_VERSION_REVISION}"
					"OVITO_INCLUDE_PATH=${CMAKE_SOURCE_DIR}/src/" ${DOXYGEN_EXECUTABLE} Doxyfile
					WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/doc/develop/"
					COMMENT "Generating C++ API documentation")
ENDIF()
