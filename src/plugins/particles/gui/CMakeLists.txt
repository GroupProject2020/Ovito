###############################################################################
# 
#  Copyright (2016) Alexander Stukowski
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
	objects/ParticlesVisEditor.cpp
	objects/ParticleTypeEditor.cpp
	objects/ParticlePropertyEditor.cpp
	objects/BondTypeEditor.cpp
	objects/BondPropertyEditor.cpp
	objects/VectorVisEditor.cpp
	objects/BondsVisEditor.cpp
	objects/TrajectoryVisEditor.cpp
	modifier/coloring/AmbientOcclusionModifierEditor.cpp
	modifier/modify/CreateBondsModifierEditor.cpp
	modifier/modify/LoadTrajectoryModifierEditor.cpp
	modifier/modify/CombineParticleSetsModifierEditor.cpp
	modifier/analysis/StructureListParameterUI.cpp
	modifier/analysis/binandreduce/BinAndReduceModifierEditor.cpp
	modifier/analysis/bondangle/BondAngleAnalysisModifierEditor.cpp
	modifier/analysis/cna/CommonNeighborAnalysisModifierEditor.cpp
	modifier/analysis/coordination/CoordinationNumberModifierEditor.cpp
	modifier/analysis/displacements/CalculateDisplacementsModifierEditor.cpp
	modifier/analysis/centrosymmetry/CentroSymmetryModifierEditor.cpp
	modifier/analysis/strain/AtomicStrainModifierEditor.cpp
	modifier/analysis/diamond/IdentifyDiamondModifierEditor.cpp	
	modifier/analysis/voronoi/VoronoiAnalysisModifierEditor.cpp
	modifier/analysis/cluster/ClusterAnalysisModifierEditor.cpp
	modifier/analysis/wignerseitz/WignerSeitzAnalysisModifierEditor.cpp
	modifier/analysis/ptm/PolyhedralTemplateMatchingModifierEditor.cpp
	modifier/selection/ExpandSelectionModifierEditor.cpp
	modifier/selection/ManualSelectionModifierEditor.cpp
	modifier/properties/ComputePropertyModifierEditor.cpp
	modifier/properties/InterpolateTrajectoryModifierEditor.cpp
	modifier/properties/GenerateTrajectoryLinesModifierEditor.cpp
	util/CutoffRadiusPresetsUI.cpp
	util/ParticlePickingHelper.cpp
	util/ParticleSettingsPage.cpp
	util/ParticleInspectionApplet.cpp
	util/BondInspectionApplet.cpp
	import/InputColumnMappingDialog.cpp
	import/lammps/LAMMPSTextDumpImporterEditor.cpp
	import/lammps/LAMMPSBinaryDumpImporterEditor.cpp
	import/lammps/LAMMPSDataImporterEditor.cpp
	import/xyz/XYZImporterEditor.cpp
	export/FileColumnParticleExporterEditor.cpp
	export/lammps/LAMMPSDataExporterEditor.cpp
	export/lammps/LAMMPSDumpExporterEditor.cpp
	export/xyz/XYZExporterEditor.cpp
)

# Compile resources.
QT5_ADD_RESOURCES(ResourceFiles resources/particles_gui.qrc)

OVITO_STANDARD_PLUGIN(ParticlesGui
	SOURCES ParticlesGui.cpp ${SourceFiles} ${ResourceFiles}
	PRIVATE_LIB_DEPENDENCIES Qwt
	PLUGIN_DEPENDENCIES Particles StdObjGui
	GUI_PLUGIN
)

# Speed up compilation by using precompiled headers.
ADD_PRECOMPILED_HEADER(ParticlesGui plugins/particles/gui/ParticlesGui.h)
