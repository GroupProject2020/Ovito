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
	objects/ParticlesVis.cpp
	objects/ParticleType.cpp
	objects/ParticlesObject.cpp
	objects/BondType.cpp
	objects/ParticleBondMap.cpp
	objects/BondsObject.cpp
	objects/BondsVis.cpp
	objects/TrajectoryObject.cpp
	objects/TrajectoryVis.cpp
	objects/VectorVis.cpp
	modifier/coloring/AmbientOcclusionModifier.cpp
	modifier/coloring/AmbientOcclusionRenderer.cpp
	modifier/modify/WrapPeriodicImagesModifier.cpp
	modifier/modify/CreateBondsModifier.cpp
	modifier/modify/LoadTrajectoryModifier.cpp
	modifier/modify/CoordinationPolyhedraModifier.cpp
	modifier/analysis/StructureIdentificationModifier.cpp
	modifier/analysis/ReferenceConfigurationModifier.cpp
	modifier/analysis/ackland_jones/AcklandJonesModifier.cpp
	modifier/analysis/cna/CommonNeighborAnalysisModifier.cpp
	modifier/analysis/coordination/CoordinationAnalysisModifier.cpp
	modifier/analysis/displacements/CalculateDisplacementsModifier.cpp
	modifier/analysis/centrosymmetry/CentroSymmetryModifier.cpp
	modifier/analysis/strain/AtomicStrainModifier.cpp
	modifier/analysis/diamond/IdentifyDiamondModifier.cpp	
	modifier/analysis/voronoi/VoronoiAnalysisModifier.cpp
	modifier/analysis/cluster/ClusterAnalysisModifier.cpp
	modifier/analysis/wignerseitz/WignerSeitzAnalysisModifier.cpp
	modifier/analysis/ptm/PolyhedralTemplateMatchingModifier.cpp
	modifier/selection/ExpandSelectionModifier.cpp
	modifier/properties/InterpolateTrajectoryModifier.cpp
	modifier/properties/GenerateTrajectoryLinesModifier.cpp
	import/ParticleImporter.cpp
	import/ParticleFrameData.cpp
	import/InputColumnMapping.cpp
	import/lammps/LAMMPSTextDumpImporter.cpp
	import/lammps/LAMMPSBinaryDumpImporter.cpp
	import/lammps/LAMMPSDataImporter.cpp
	import/vasp/POSCARImporter.cpp
	import/xyz/XYZImporter.cpp
	import/imd/IMDImporter.cpp
	import/parcas/ParcasFileImporter.cpp
	import/cfg/CFGImporter.cpp
	import/pdb/PDBImporter.cpp
	import/fhi_aims/FHIAimsImporter.cpp
	import/fhi_aims/FHIAimsLogFileImporter.cpp
	import/gsd/GSDImporter.cpp
	import/gsd/gsd.c
	import/castep/CastepCellImporter.cpp
	import/castep/CastepMDImporter.cpp
	import/cube/GaussianCubeImporter.cpp
	import/xsf/XSFImporter.cpp
	import/dl_poly/DLPOLYImporter.cpp
	import/quantumespresso/QuantumEspressoImporter.cpp
	export/ParticleExporter.cpp
	export/FileColumnParticleExporter.cpp
	export/OutputColumnMapping.cpp
	export/vasp/POSCARExporter.cpp
	export/fhi_aims/FHIAimsExporter.cpp
	export/lammps/LAMMPSDataExporter.cpp
	export/lammps/LAMMPSDumpExporter.cpp
	export/xyz/XYZExporter.cpp
	export/imd/IMDExporter.cpp
	util/NearestNeighborFinder.cpp
	util/CutoffNeighborFinder.cpp
	util/ParticleExpressionEvaluator.cpp
)

IF(OVITO_BUILD_PLUGIN_STDMOD)
	LIST(APPEND SourceFiles 
		modifier/coloring/ParticlesColorCodingModifierDelegate.cpp
		modifier/coloring/ParticlesAssignColorModifierDelegate.cpp
		modifier/modify/ParticlesSliceModifierDelegate.cpp
		modifier/modify/ParticlesAffineTransformationModifierDelegate.cpp
		modifier/modify/ParticlesDeleteSelectedModifierDelegate.cpp
		modifier/modify/ParticlesReplicateModifierDelegate.cpp
		modifier/analysis/binning/ParticlesSpatialBinningModifierDelegate.cpp
		modifier/properties/ParticlesComputePropertyModifierDelegate.cpp
		modifier/properties/BondsComputePropertyModifierDelegate.cpp
		modifier/selection/ParticlesExpressionSelectionModifierDelegate.cpp
		modifier/modify/ParticlesCombineDatasetsModifierDelegate.cpp
	)
ENDIF()

IF(OVITO_BUILD_PLUGIN_PYSCRIPT)
	LIST(APPEND SourceFiles 
		scripting/PythonInterface.cpp
		scripting/ImporterBinding.cpp
		scripting/ExporterBinding.cpp
		scripting/ModifierBinding.cpp
	)
ENDIF()

OVITO_STANDARD_PLUGIN(Particles
	SOURCES Particles.cpp ${SourceFiles}
	PLUGIN_DEPENDENCIES Mesh Grid StdObj
	PRIVATE_LIB_DEPENDENCIES VoroPlusPlus OpenGLRenderer PolyhedralTemplateMatching CombinatoriallyOptimalPointRegistration
	OPTIONAL_PLUGIN_DEPENDENCIES PyScript StdMod
	PYTHON_WRAPPERS "${CMAKE_CURRENT_SOURCE_DIR}/resources/python/"
)

# Speed up compilation by using precompiled headers.
IF(OVITO_USE_PRECOMPILED_HEADERS)
	ADD_PRECOMPILED_HEADER(Particles plugins/particles/Particles.h)
ENDIF()

# Build corresponding GUI plugin.
IF(OVITO_BUILD_GUI)
	ADD_SUBDIRECTORY(gui)
ENDIF()

# Propagate list of plugins to parent scope.
SET(OVITO_PLUGIN_LIST ${OVITO_PLUGIN_LIST} PARENT_SCOPE)
