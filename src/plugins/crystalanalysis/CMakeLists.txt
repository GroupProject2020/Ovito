###############################################################################
# 
#  Copyright (2017) Alexander Stukowski
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
	importer/CAImporter.cpp
	importer/DislocImporter.cpp
	exporter/CAExporter.cpp
	exporter/VTKDislocationsExporter.cpp
	objects/patterns/BurgersVectorFamily.cpp
	objects/patterns/StructurePattern.cpp
	objects/patterns/PatternCatalog.cpp
	objects/clusters/ClusterGraphObject.cpp
	objects/dislocations/DislocationNetworkObject.cpp
	objects/dislocations/DislocationDisplay.cpp
	objects/dislocations/RenderableDislocationLines.cpp
	objects/partition_mesh/PartitionMesh.cpp
	objects/partition_mesh/PartitionMeshDisplay.cpp
	objects/slip_surface/SlipSurface.cpp
	objects/slip_surface/SlipSurfaceDisplay.cpp
	data/ClusterGraph.cpp
	data/DislocationNetwork.cpp
	data/NodalDislocationNetwork.cpp
	data/PlanarDefects.cpp
	modifier/ConstructSurfaceModifier.cpp
	modifier/dxa/DislocationAnalysisModifier.cpp
	modifier/dxa/DislocationAnalysisEngine.cpp
	modifier/dxa/StructureAnalysis.cpp
	modifier/dxa/CrystalPathFinder.cpp
	modifier/dxa/ElasticMapping.cpp
	modifier/dxa/InterfaceMesh.cpp
	modifier/dxa/DislocationTracer.cpp
	modifier/elasticstrain/ElasticStrainModifier.cpp
	modifier/elasticstrain/ElasticStrainEngine.cpp
	util/DelaunayTessellation.cpp
)

IF(OVITO_BUILD_PLUGIN_STDMOD)
	LIST(APPEND SourceFiles 
		objects/dislocations/DislocationSliceModifierDelegate.cpp
		objects/dislocations/DislocationReplicateModifierDelegate.cpp
	)
ENDIF()

IF(OVITO_BUILD_PLUGIN_PYSCRIPT)
	LIST(APPEND SourceFiles scripting/PythonInterface.cpp)
ENDIF()

OVITO_STANDARD_PLUGIN(CrystalAnalysis
	SOURCES CrystalAnalysis.cpp ${SourceFiles}
	PLUGIN_DEPENDENCIES Particles Mesh
	LIB_DEPENDENCIES geogram
	OPTIONAL_PLUGIN_DEPENDENCIES PyScript StdObj StdMod
	PYTHON_WRAPPERS "${CMAKE_CURRENT_SOURCE_DIR}/resources/python/"
)

# Speed up compilation by using precompiled headers.
ADD_PRECOMPILED_HEADER(CrystalAnalysis plugins/crystalanalysis/CrystalAnalysis.h)

# Build corresponding GUI plugin.
IF(OVITO_BUILD_GUI)
	ADD_SUBDIRECTORY(gui)
ENDIF()

# Propagate list of plugins to parent scope.
SET(OVITO_PLUGIN_LIST ${OVITO_PLUGIN_LIST} PARENT_SCOPE)
