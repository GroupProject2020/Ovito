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

SET(SourceFiles 
	tri/TriMeshObject.cpp
	tri/TriMeshVis.cpp
	halfedge/HalfEdgeMesh.cpp		
	surface/SurfaceMesh.cpp
	surface/RenderableSurfaceMesh.cpp
	surface/SurfaceMeshVis.cpp
	surface/SurfaceMeshVertices.cpp
	surface/SurfaceMeshFaces.cpp
	surface/SurfaceMeshRegions.cpp
	io/TriMeshFrameData.cpp
	io/VTKFileImporter.cpp
	io/VTKTriangleMeshExporter.cpp
)

IF(OVITO_BUILD_PLUGIN_STDMOD)
	LIST(APPEND SourceFiles 
		surface/SurfaceMeshSliceModifierDelegate.cpp
		surface/SurfaceMeshAffineTransformationModifierDelegate.cpp
		surface/SurfaceMeshReplicateModifierDelegate.cpp)
ENDIF()

# List of source files of the GLU library used for polygon tessellation.
SET(GLUSourceFiles
	util/polytess/dict.c
	util/polytess/geom.c
	util/polytess/memalloc.c
	util/polytess/mesh.c
	util/polytess/normal.c
	util/polytess/priorityq.c
	util/polytess/render.c
	util/polytess/sweep.c
	util/polytess/tess.c
	util/polytess/tessmono.c
)

# Define the plugin module.
OVITO_STANDARD_PLUGIN(Mesh
	SOURCES 
		Mesh.cpp 
		${SourceFiles} 
		${GLUSourceFiles}
	OPTIONAL_PLUGIN_DEPENDENCIES StdObj StdMod
)

# Speed up compilation by using precompiled headers.
IF(OVITO_USE_PRECOMPILED_HEADERS)
	ADD_PRECOMPILED_HEADER(Mesh plugins/mesh/Mesh.h)
ENDIF()

# Build corresponding GUI plugin.
IF(OVITO_BUILD_GUI)
	ADD_SUBDIRECTORY(gui)
ENDIF()

# Build corresponding Python binding plugin.
IF(OVITO_BUILD_PLUGIN_PYSCRIPT)
	ADD_SUBDIRECTORY(scripting)
ENDIF()

# Propagate list of plugins to parent scope.
SET(OVITO_PLUGIN_LIST ${OVITO_PLUGIN_LIST} PARENT_SCOPE)
