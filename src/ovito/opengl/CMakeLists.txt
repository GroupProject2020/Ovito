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

# Find OpenGL library.
FIND_PACKAGE(OpenGL REQUIRED)

# Compile resources.
QT5_ADD_RESOURCES(ResourceFiles resources/resources.qrc)

# Define plugin module.
OVITO_STANDARD_PLUGIN(OpenGLRenderer
	SOURCES
		OpenGLLinePrimitive.cpp
		OpenGLParticlePrimitive.cpp
		OpenGLTextPrimitive.cpp
		OpenGLImagePrimitive.cpp
		OpenGLArrowPrimitive.cpp
		OpenGLMeshPrimitive.cpp
		OpenGLMarkerPrimitive.cpp
		OpenGLSharedResource.cpp
		OpenGLSceneRenderer.cpp
		StandardSceneRenderer.cpp
		${ResourceFiles}
	PRIVATE_LIB_DEPENDENCIES
		OpenGL::GL
)

IF(APPLE)
	# OpenGL functions have been marked as deprecated since macOS 10.14. Silence the compiler warnings.
	TARGET_COMPILE_DEFINITIONS(OpenGLRenderer PRIVATE "GL_SILENCE_DEPRECATION")
ENDIF()

# Build corresponding GUI plugin.
IF(OVITO_BUILD_GUI)
	ADD_SUBDIRECTORY(gui)
ENDIF()

# Propagate list of plugins to parent scope.
SET(OVITO_PLUGIN_LIST ${OVITO_PLUGIN_LIST} PARENT_SCOPE)
