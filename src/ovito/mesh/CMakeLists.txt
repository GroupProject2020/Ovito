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

SET(SourceFiles
	tri/TriMeshObject.cpp
	tri/TriMeshVis.cpp
	surface/HalfEdgeMesh.cpp
	surface/SurfaceMesh.cpp
	surface/SurfaceMeshData.cpp
	surface/RenderableSurfaceMesh.cpp
	surface/SurfaceMeshVis.cpp
	surface/SurfaceMeshVertices.cpp
	surface/SurfaceMeshFaces.cpp
	surface/SurfaceMeshRegions.cpp
	io/TriMeshFrameData.cpp
	io/VTKFileImporter.cpp
	io/VTKTriangleMeshExporter.cpp
	io/WavefrontOBJImporter.cpp
	io/STLImporter.cpp
)

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
	PLUGIN_DEPENDENCIES StdObj
)

# Speed up compilation by using precompiled headers.
IF(OVITO_USE_PRECOMPILED_HEADERS)
	ADD_PRECOMPILED_HEADER(Mesh ovito/mesh/Mesh.h)
ENDIF()

# Build corresponding GUI plugin.
IF(OVITO_BUILD_GUI)
	ADD_SUBDIRECTORY(gui)
ENDIF()

# Propagate list of plugins to parent scope.
SET(OVITO_PLUGIN_LIST ${OVITO_PLUGIN_LIST} PARENT_SCOPE)
