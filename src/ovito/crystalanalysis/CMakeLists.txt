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
	importer/CAImporter.cpp
	importer/DislocImporter.cpp
	importer/ParaDiSImporter.cpp
	exporter/CAExporter.cpp
	exporter/VTKDislocationsExporter.cpp
	objects/BurgersVectorFamily.cpp
	objects/ClusterGraphObject.cpp
	objects/DislocationNetworkObject.cpp
	objects/DislocationVis.cpp
	objects/RenderableDislocationLines.cpp
	objects/Microstructure.cpp
	objects/MicrostructurePhase.cpp
	objects/SlipSurfaceVis.cpp
	data/ClusterGraph.cpp
	data/DislocationNetwork.cpp
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
	modifier/microstructure/SimplifyMicrostructureModifier.cpp
	util/DelaunayTessellation.cpp
)

IF(OVITO_BUILD_PLUGIN_STDMOD)
	LIST(APPEND SourceFiles
		objects/DislocationSliceModifierDelegate.cpp
		objects/DislocationReplicateModifierDelegate.cpp
		)
ENDIF()

# Define the plugin module.
OVITO_STANDARD_PLUGIN(CrystalAnalysis
	SOURCES
		CrystalAnalysis.cpp
		${SourceFiles}
	PLUGIN_DEPENDENCIES Particles Mesh StdObj Grid
	LIB_DEPENDENCIES geogram
	PRIVATE_LIB_DEPENDENCIES NetCDFIntegration
	OPTIONAL_PLUGIN_DEPENDENCIES StdMod
)

# Speed up compilation by using precompiled headers.
IF(OVITO_USE_PRECOMPILED_HEADERS)
	ADD_PRECOMPILED_HEADER(CrystalAnalysis ovito/crystalanalysis/CrystalAnalysis.h)
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
