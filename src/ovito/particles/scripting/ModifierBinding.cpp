///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2019) Alexander Stukowski
//
//  This file is part of OVITO (Open Visualization Tool).
//
//  OVITO is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  OVITO is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////

#include <ovito/particles/Particles.h>
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/particles/modifier/coloring/AmbientOcclusionModifier.h>
#include <ovito/particles/modifier/modify/WrapPeriodicImagesModifier.h>
#include <ovito/particles/modifier/modify/UnwrapTrajectoriesModifier.h>
#include <ovito/particles/modifier/modify/CreateBondsModifier.h>
#include <ovito/particles/modifier/modify/LoadTrajectoryModifier.h>
#include <ovito/particles/modifier/modify/CoordinationPolyhedraModifier.h>
#include <ovito/particles/modifier/properties/InterpolateTrajectoryModifier.h>
#include <ovito/particles/modifier/properties/GenerateTrajectoryLinesModifier.h>
#include <ovito/particles/modifier/properties/ParticlesComputePropertyModifierDelegate.h>
#include <ovito/particles/modifier/properties/BondsComputePropertyModifierDelegate.h>
#include <ovito/particles/modifier/selection/ExpandSelectionModifier.h>
#include <ovito/particles/modifier/analysis/StructureIdentificationModifier.h>
#include <ovito/particles/modifier/analysis/ackland_jones/AcklandJonesModifier.h>
#include <ovito/particles/modifier/analysis/cna/CommonNeighborAnalysisModifier.h>
#include <ovito/particles/modifier/analysis/centrosymmetry/CentroSymmetryModifier.h>
#include <ovito/particles/modifier/analysis/cluster/ClusterAnalysisModifier.h>
#include <ovito/particles/modifier/analysis/coordination/CoordinationAnalysisModifier.h>
#include <ovito/particles/modifier/analysis/displacements/CalculateDisplacementsModifier.h>
#include <ovito/particles/modifier/analysis/strain/AtomicStrainModifier.h>
#include <ovito/particles/modifier/analysis/wignerseitz/WignerSeitzAnalysisModifier.h>
#include <ovito/particles/modifier/analysis/ptm/PolyhedralTemplateMatchingModifier.h>
#include <ovito/particles/modifier/analysis/voronoi/VoronoiAnalysisModifier.h>
#include <ovito/particles/modifier/analysis/diamond/IdentifyDiamondModifier.h>
#include <ovito/particles/modifier/analysis/chill_plus/ChillPlusModifier.h>
#include <ovito/stdobj/properties/PropertyStorage.h>
#include <ovito/stdobj/scripting/PythonBinding.h>
#include <ovito/pyscript/binding/PythonBinding.h>
#include <ovito/pyscript/engine/ScriptEngine.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>
#include <ovito/core/dataset/io/FileSource.h>
#include <ovito/core/dataset/animation/AnimationSettings.h>
#include <ovito/core/dataset/DataSetContainer.h>
#include <ovito/core/utilities/concurrent/TaskManager.h>
#include "PythonBinding.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Internal)

using namespace PyScript;

void defineModifiersSubmodule(py::module m)
{
	ovito_class<AmbientOcclusionModifier, AsynchronousModifier>(m,
			":Base class: :py:class:`ovito.pipeline.Modifier`\n\n"
			"Performs a quick lighting calculation to shade particles according to the degree of occlusion by other particles. "
			"See also the corresponding :ovitoman:`user manual page <../../particles.modifiers.ambient_occlusion>` for this modifier. ")
		.def_property("intensity", &AmbientOcclusionModifier::intensity, &AmbientOcclusionModifier::setIntensity,
				"Controls the strength of the shading effect. "
				"\n\n"
				":Valid range: [0.0, 1.0]\n"
				":Default: 0.7")
		.def_property("sample_count", &AmbientOcclusionModifier::samplingCount, &AmbientOcclusionModifier::setSamplingCount,
				"The number of light exposure samples to compute. More samples give a more even light distribution "
				"but take longer to compute."
				"\n\n"
				":Default: 40\n")
		.def_property("buffer_resolution", &AmbientOcclusionModifier::bufferResolution, &AmbientOcclusionModifier::setBufferResolution,
				"A positive integer controlling the resolution of the internal render buffer, which is used to compute how much "
				"light each particle receives. For large datasets, where the size of a particle is small compared to the "
				"simulation dimensions, a highezr buffer resolution should be used."
				"\n\n"
				":Valid range: [1, 4]\n"
				":Default: 3\n")
	;

	ovito_class<WrapPeriodicImagesModifier, Modifier>(m,
			":Base class: :py:class:`ovito.pipeline.Modifier`\n\n"
			"This modifier maps particles located outside of the simulation cell back into the cell by \"wrapping\" their coordinates "
			"around at the periodic boundaries of the :py:class:`~ovito.data.SimulationCell`. This modifier has no parameters. "
			"\n\n"
			"See also the corresponding :ovitoman:`user manual page <../../particles.modifiers.wrap_at_periodic_boundaries>` for this modifier. ")
	;

	ovito_class<InterpolateTrajectoryModifier, Modifier>(m,
			":Base class: :py:class:`ovito.pipeline.Modifier`"
			"\n\n"
			"This modifier interpolates the particle positions in between successive snapshots of a simulation trajectory. "
    		"It can be used to create smoothly looking animations from relatively coarse sequences of simulation snapshots. "
			"See also the corresponding :ovitoman:`user manual page <../../particles.modifiers.interpolate_trajectory>` for this modifier. "
			)
		.def_property("minimum_image_convention", &InterpolateTrajectoryModifier::useMinimumImageConvention, &InterpolateTrajectoryModifier::setUseMinimumImageConvention,
				"If this option is activated, the modifier will automatically detect if a particle has crossed a simulation box boundary between two "
    			"successive simulation frames and compute the unwrapped displacement vector correctly. "
				"You should leave this option activated unless the particle positions loaded from the input data file(s) were "
				"stored in unwrapped form by the molecular dynamics code. "
				"\n\n"
				":Default: ``True``\n")
	;

	auto ExpandSelectionModifier_py = ovito_class<ExpandSelectionModifier, AsynchronousModifier>(m,
			":Base class: :py:class:`ovito.pipeline.Modifier`\n\n"
			"Expands the current particle selection by selecting particles that are neighbors of already selected particles. "
			"See also the corresponding :ovitoman:`user manual page <../../particles.modifiers.expand_selection>` for this modifier. "
			)
		.def_property("mode", &ExpandSelectionModifier::mode, &ExpandSelectionModifier::setMode,
				"Selects the mode of operation, i.e., how the modifier extends the selection around already selected particles. "
				"Valid values are:"
				"\n\n"
				"  * ``ExpandSelectionModifier.ExpansionMode.Cutoff``\n"
				"  * ``ExpandSelectionModifier.ExpansionMode.Nearest``\n"
				"  * ``ExpandSelectionModifier.ExpansionMode.Bonded``\n"
				"\n\n"
				":Default: ``ExpandSelectionModifier.ExpansionMode.Cutoff``\n")
		.def_property("cutoff", &ExpandSelectionModifier::cutoffRange, &ExpandSelectionModifier::setCutoffRange,
				"The maximum distance up to which particles are selected around already selected particles. "
				"This parameter is only used if :py:attr:`.mode` is set to ``ExpansionMode.Cutoff``."
				"\n\n"
				":Default: 3.2\n")
		.def_property("num_neighbors", &ExpandSelectionModifier::numNearestNeighbors, &ExpandSelectionModifier::setNumNearestNeighbors,
				"The number of nearest neighbors to select around each already selected particle. "
				"This parameter is only used if :py:attr:`.mode` is set to ``ExpansionMode.Nearest``."
				"\n\n"
				":Default: 1\n")
		.def_property("iterations", &ExpandSelectionModifier::numberOfIterations, &ExpandSelectionModifier::setNumberOfIterations,
				"Controls how many iterations of the modifier are executed. This can be used to select "
				"neighbors of neighbors up to a certain recursive depth."
				"\n\n"
				":Default: 1\n")
	;

	ovito_enum<ExpandSelectionModifier::ExpansionMode>(ExpandSelectionModifier_py, "ExpansionMode")
		.value("Cutoff", ExpandSelectionModifier::CutoffRange)
		.value("Nearest", ExpandSelectionModifier::NearestNeighbors)
		.value("Bonded", ExpandSelectionModifier::BondedNeighbors)
	;

	ovito_abstract_class<StructureIdentificationModifier, AsynchronousModifier>{m};

	auto AcklandJonesModifier_py = ovito_class<AcklandJonesModifier, StructureIdentificationModifier>(m,
			":Base class: :py:class:`ovito.pipeline.Modifier`\n\n"
			"Performs the bond-angle analysis described by Ackland & Jones to identify the local "
			"crystal structure around each particle. "
			"See also the corresponding :ovitoman:`user manual page <../../particles.modifiers.bond_angle_analysis>` for this modifier. "
			"\n\n"
			"The modifier stores the results as integer values in the ``\"Structure Type\"`` particle property. "
			"The following structure type constants are defined: "
			"\n\n"
			"   * ``AcklandJonesModifier.Type.OTHER`` (0)\n"
			"   * ``AcklandJonesModifier.Type.FCC`` (1)\n"
			"   * ``AcklandJonesModifier.Type.HCP`` (2)\n"
			"   * ``AcklandJonesModifier.Type.BCC`` (3)\n"
			"   * ``AcklandJonesModifier.Type.ICO`` (4)\n"
			"\n\n"
			"**Modifier outputs:**"
			"\n\n"
			" * ``AcklandJones.counts.OTHER`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
			"   The number of particles not matching any of the known structure types.\n"
			" * ``AcklandJones.counts.FCC`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
			"   The number of FCC particles found.\n"
			" * ``AcklandJones.counts.HCP`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
			"   The number of HCP particles found.\n"
			" * ``AcklandJones.counts.BCC`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
			"   The number of BCC particles found.\n"
			" * ``AcklandJones.counts.ICO`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
			"   The number of icosahedral found.\n"
			" * ``Structure Type`` (:py:class:`~ovito.data.ParticleProperty`):\n"
			"   This particle property will contain the per-particle structure type assigned by the modifier.\n"
			" * ``Color`` (:py:class:`~ovito.data.ParticleProperty`):\n"
			"   The modifier assigns a color to each particle according to its identified structure type. "
			"\n")
		.def_property("color_by_type", &AcklandJonesModifier::colorByType, &AcklandJonesModifier::setColorByType,
				"Controls whether the modifier assigns a color to each particle based on the identified structure type. "
				"\n\n"
				":Default: ``True``\n")
	;
	expose_subobject_list(AcklandJonesModifier_py, std::mem_fn(&StructureIdentificationModifier::structureTypes), "structures", "AcklandJonesStructureTypeList",
		"A list of :py:class:`~ovito.data.ParticleType` instances managed by this modifier, one for each supported structure type. "
		"The display color of a structure type can be changed as follows:: "
		"\n\n"
		"   modifier = AcklandJonesModifier()\n"
		"   # Give all FCC atoms a blue color:\n"
		"   modifier.structures[AcklandJonesModifier.Type.FCC].color = (0, 0, 1)\n"
		"\n\n.\n");

	ovito_enum<AcklandJonesModifier::StructureType>(AcklandJonesModifier_py, "Type")
		.value("OTHER", AcklandJonesModifier::OTHER)
		.value("FCC", AcklandJonesModifier::FCC)
		.value("HCP", AcklandJonesModifier::HCP)
		.value("BCC", AcklandJonesModifier::BCC)
		.value("ICO", AcklandJonesModifier::ICO)
	;

	auto CommonNeighborAnalysisModifier_py = ovito_class<CommonNeighborAnalysisModifier, StructureIdentificationModifier>(m,
			":Base class: :py:class:`ovito.pipeline.Modifier`\n\n"
			"Performs the common neighbor analysis (CNA) to classify the structure of the local neighborhood "
			"of each particle. "
			"See also the corresponding :ovitoman:`user manual page <../../particles.modifiers.common_neighbor_analysis>` for this modifier. "
			"\n\n"
			"The modifier stores its results as integer values in the ``\"Structure Type\"`` particle property. "
			"The following constants are defined: "
			"\n\n"
			"   * ``CommonNeighborAnalysisModifier.Type.OTHER`` (0)\n"
			"   * ``CommonNeighborAnalysisModifier.Type.FCC`` (1)\n"
			"   * ``CommonNeighborAnalysisModifier.Type.HCP`` (2)\n"
			"   * ``CommonNeighborAnalysisModifier.Type.BCC`` (3)\n"
			"   * ``CommonNeighborAnalysisModifier.Type.ICO`` (4)\n"
			"\n\n"
			"**Modifier outputs:**"
			"\n\n"
			" * ``CommonNeighborAnalysis.counts.OTHER`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
			"   The number of particles not matching any of the known structure types.\n"
			" * ``CommonNeighborAnalysis.counts.FCC`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
			"   The number of FCC particles found.\n"
			" * ``CommonNeighborAnalysis.counts.HCP`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
			"   The number of HCP particles found.\n"
			" * ``CommonNeighborAnalysis.counts.BCC`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
			"   The number of BCC particles found.\n"
			" * ``CommonNeighborAnalysis.counts.ICO`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
			"   The number of icosahedral particles found.\n"
			" * ``Structure Type`` (:py:class:`~ovito.data.ParticleProperty`):\n"
			"   This output particle property contains the per-particle structure types assigned by the modifier.\n"
			" * ``Color`` (:py:class:`~ovito.data.ParticleProperty`):\n"
			"   The modifier assigns a color to each particle according to its identified structure type. "
			"\n")
		.def_property("cutoff", &CommonNeighborAnalysisModifier::cutoff, &CommonNeighborAnalysisModifier::setCutoff,
				"The cutoff radius used for the conventional common neighbor analysis. "
				"This parameter is only used if :py:attr:`.mode` == ``CommonNeighborAnalysisModifier.Mode.FixedCutoff``."
				"\n\n"
				":Default: 3.2\n")
		.def_property("mode", &CommonNeighborAnalysisModifier::mode, &CommonNeighborAnalysisModifier::setMode,
				"Selects the mode of operation. "
				"Valid values are:"
				"\n\n"
				"  * ``CommonNeighborAnalysisModifier.Mode.FixedCutoff``\n"
				"  * ``CommonNeighborAnalysisModifier.Mode.AdaptiveCutoff``\n"
				"  * ``CommonNeighborAnalysisModifier.Mode.BondBased``\n"
				"\n\n"
				":Default: ``CommonNeighborAnalysisModifier.Mode.AdaptiveCutoff``\n")
		.def_property("only_selected", &CommonNeighborAnalysisModifier::onlySelectedParticles, &CommonNeighborAnalysisModifier::setOnlySelectedParticles,
				"Lets the modifier perform the analysis only for selected particles. Particles that are not selected will be treated as if they did not exist."
				"\n\n"
				":Default: ``False``\n")
		.def_property("color_by_type", &CommonNeighborAnalysisModifier::colorByType, &CommonNeighborAnalysisModifier::setColorByType,
				"Controls whether the modifier assigns a color to each particle based on the identified structure type. "
				"\n\n"
				":Default: ``True``\n")
	;
	expose_subobject_list(CommonNeighborAnalysisModifier_py, std::mem_fn(&StructureIdentificationModifier::structureTypes), "structures", "CommonNeighborAnalysisStructureTypeList",
		"A list of :py:class:`~ovito.data.ParticleType` instances managed by this modifier, one for each supported structure type. "
		"The display color of a structure type can be changed as follows:: "
		"\n\n"
		"   modifier = CommonNeighborAnalysisModifier()\n"
		"   # Give all FCC atoms a blue color:\n"
		"   modifier.structures[CommonNeighborAnalysisModifier.Type.FCC].color = (0, 0, 1)\n"
		"\n\n.\n");

	ovito_enum<CommonNeighborAnalysisModifier::CNAMode>(CommonNeighborAnalysisModifier_py, "Mode")
		.value("FixedCutoff", CommonNeighborAnalysisModifier::FixedCutoffMode)
		.value("AdaptiveCutoff", CommonNeighborAnalysisModifier::AdaptiveCutoffMode)
		.value("BondBased", CommonNeighborAnalysisModifier::BondMode)
	;

	ovito_enum<CommonNeighborAnalysisModifier::StructureType>(CommonNeighborAnalysisModifier_py, "Type")
		.value("OTHER", CommonNeighborAnalysisModifier::OTHER)
		.value("FCC", CommonNeighborAnalysisModifier::FCC)
		.value("HCP", CommonNeighborAnalysisModifier::HCP)
		.value("BCC", CommonNeighborAnalysisModifier::BCC)
		.value("ICO", CommonNeighborAnalysisModifier::ICO)
	;

	auto IdentifyDiamondModifier_py = ovito_class<IdentifyDiamondModifier, StructureIdentificationModifier>(m,
			":Base class: :py:class:`ovito.pipeline.Modifier`\n\n"
			"This analysis modifier finds atoms that are arranged in a cubic or hexagonal diamond lattice. "
			"See also the corresponding :ovitoman:`user manual page <../../particles.modifiers.identify_diamond_structure>` for this modifier. "
			"\n\n"
			"The modifier stores its results as integer values in the ``\"Structure Type\"`` particle property. "
			"The following structure type constants are defined: "
			"\n\n"
			"   * ``IdentifyDiamondModifier.Type.OTHER`` (0)\n"
			"   * ``IdentifyDiamondModifier.Type.CUBIC_DIAMOND`` (1)\n"
			"   * ``IdentifyDiamondModifier.Type.CUBIC_DIAMOND_FIRST_NEIGHBOR`` (2)\n"
			"   * ``IdentifyDiamondModifier.Type.CUBIC_DIAMOND_SECOND_NEIGHBOR`` (3)\n"
			"   * ``IdentifyDiamondModifier.Type.HEX_DIAMOND`` (4)\n"
			"   * ``IdentifyDiamondModifier.Type.HEX_DIAMOND_FIRST_NEIGHBOR`` (5)\n"
			"   * ``IdentifyDiamondModifier.Type.HEX_DIAMOND_SECOND_NEIGHBOR`` (6)\n"
			"\n\n"
			"**Modifier outputs:**"
			"\n\n"
			" * ``IdentifyDiamond.counts.OTHER`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
			"   The number of atoms not matching any of the known structure types.\n"
			" * ``IdentifyDiamond.counts.CUBIC_DIAMOND`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
			"   The number of cubic diamond atoms found.\n"
			" * ``IdentifyDiamond.counts.CUBIC_DIAMOND_FIRST_NEIGHBOR`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
			"   The number of atoms found that are first neighbors of a cubic diamond atom.\n"
			" * ``IdentifyDiamond.counts.CUBIC_DIAMOND_SECOND_NEIGHBOR`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
			"   The number of atoms found that are second neighbors of a cubic diamond atom.\n"
			" * ``IdentifyDiamond.counts.HEX_DIAMOND`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
			"   The number of hexagonal diamond atoms found.\n"
			" * ``IdentifyDiamond.counts.HEX_DIAMOND_FIRST_NEIGHBOR`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
			"   The number of atoms found that are first neighbors of a hexagonal diamond atom.\n"
			" * ``IdentifyDiamond.counts.HEX_DIAMOND_SECOND_NEIGHBOR`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
			"   The number of atoms found that are second neighbors of a hexagonal diamond atom.\n"
			" * ``Structure Type`` (:py:class:`~ovito.data.ParticleProperty`):\n"
			"   This particle property will contain the per-particle structure type assigned by the modifier.\n"
			" * ``Color`` (:py:class:`~ovito.data.ParticleProperty`):\n"
			"   The modifier assigns a color to each atom according to its identified structure type. "
			"\n")
		.def_property("only_selected", &IdentifyDiamondModifier::onlySelectedParticles, &IdentifyDiamondModifier::setOnlySelectedParticles,
				"Lets the modifier perform the analysis only for selected particles. Particles that are not selected will be treated as if they did not exist."
				"\n\n"
				":Default: ``False``\n")
		.def_property("color_by_type", &IdentifyDiamondModifier::colorByType, &IdentifyDiamondModifier::setColorByType,
				"Controls whether the modifier assigns a color to each particle based on the identified structure type. "
				"\n\n"
				":Default: ``True``\n")
	;
	expose_subobject_list(IdentifyDiamondModifier_py, std::mem_fn(&StructureIdentificationModifier::structureTypes), "structures", "IdentifyDiamondStructureTypeList",
		"A list of :py:class:`~ovito.data.ParticleType` instances managed by this modifier, one for each supported structure type. "
		"The display color of a structure type can be changed as follows:: "
		"\n\n"
		"      modifier = AcklandJonesModifier()\n"
		"      # Give all hexagonal diamond atoms a blue color:\n"
		"      modifier.structures[IdentifyDiamondModifier.Type.HEX_DIAMOND].color = (0, 0, 1)\n"
		"\n\n.\n");

	ovito_enum<IdentifyDiamondModifier::StructureType>(IdentifyDiamondModifier_py, "Type")
		.value("OTHER", IdentifyDiamondModifier::OTHER)
		.value("CUBIC_DIAMOND", IdentifyDiamondModifier::CUBIC_DIAMOND)
		.value("CUBIC_DIAMOND_FIRST_NEIGHBOR", IdentifyDiamondModifier::CUBIC_DIAMOND_FIRST_NEIGH)
		.value("CUBIC_DIAMOND_SECOND_NEIGHBOR", IdentifyDiamondModifier::CUBIC_DIAMOND_SECOND_NEIGH)
		.value("HEX_DIAMOND", IdentifyDiamondModifier::HEX_DIAMOND)
		.value("HEX_DIAMOND_FIRST_NEIGHBOR", IdentifyDiamondModifier::HEX_DIAMOND_FIRST_NEIGH)
		.value("HEX_DIAMOND_SECOND_NEIGHBOR", IdentifyDiamondModifier::HEX_DIAMOND_SECOND_NEIGH)
	;

	auto ChillPlusModifier_py = ovito_class<ChillPlusModifier, StructureIdentificationModifier>(m,
			":Base class: :py:class:`ovito.pipeline.Modifier`\n\n"
			"Runs the CHILL+ algorithm to identify various structural arrangements of water molecules. "
			"See also the corresponding :ovitoman:`user manual page <../../particles.modifiers.chill_plus>` for this modifier. "
			"\n\n"
			"The modifier stores its results as integer values in the ``\"Structure Type\"`` particle property. "
			"The following constants are defined: "
			"\n\n"
			"   * ``ChillPlusModifier.Type.OTHER`` (0)\n"
			"   * ``ChillPlusModifier.Type.HEXAGONAL_ICE`` (1)\n"
			"   * ``ChillPlusModifier.Type.CUBIC_ICE`` (2)\n"
			"   * ``ChillPlusModifier.Type.INTERFACIAL_ICE`` (3)\n"
			"   * ``ChillPlusModifier.Type.HYDRATE`` (4)\n"
			"   * ``ChillPlusModifier.Type.INTERFACIAL_HYDRATE`` (5)\n"
			"\n")
		.def_property("cutoff", &ChillPlusModifier::cutoff, &ChillPlusModifier::setCutoff,
				"The cutoff range for bonds between water molecules. "
				"\n\n"
				":Default: 3.5\n")
		.def_property("only_selected", &ChillPlusModifier::onlySelectedParticles, &ChillPlusModifier::setOnlySelectedParticles,
				"Lets the modifier perform the analysis only on currently selected particles. Particles that are not selected will be treated as if they did not exist."
				"\n\n"
				":Default: ``False``\n")
		.def_property("color_by_type", &ChillPlusModifier::colorByType, &ChillPlusModifier::setColorByType,
				"Controls whether the modifier assigns a color to each particle based on the identified structure type. "
				"\n\n"
				":Default: ``True``\n")
	;
	expose_subobject_list(ChillPlusModifier_py, std::mem_fn(&StructureIdentificationModifier::structureTypes), "structures", "ChillPlusModifierStructureTypeList",
		"A list of :py:class:`~ovito.data.ParticleType` instances managed by this modifier, one for each supported structure type. "
		"The display color of a structure type can be changed as follows:: "
		"\n\n"
		"   modifier = ChillPlusModifier()\n"
		"   # Give all hexagonal ice particles a blue color:\n"
		"   modifier.structures[ChillPlusModifier.Type.HEXAGONAL_ICE].color = (0.0, 0.0, 1.0)\n"
		"\n\n.\n");

	ovito_enum<ChillPlusModifier::StructureType>(ChillPlusModifier_py, "Type")
		.value("OTHER", ChillPlusModifier::OTHER)
		.value("HEXAGONAL_ICE", ChillPlusModifier::HEXAGONAL_ICE)
		.value("CUBIC_ICE", ChillPlusModifier::CUBIC_ICE)
		.value("INTERFACIAL_ICE", ChillPlusModifier::INTERFACIAL_ICE)
		.value("HYDRATE", ChillPlusModifier::HYDRATE)
		.value("INTERFACIAL_HYDRATE", ChillPlusModifier::INTERFACIAL_HYDRATE)
	;

	auto CreateBondsModifier_py = ovito_class<CreateBondsModifier, AsynchronousModifier>(m,
			":Base class: :py:class:`ovito.pipeline.Modifier`\n\n"
			"Creates bonds between particles. "
			"See also the corresponding :ovitoman:`user manual page <../../particles.modifiers.create_bonds>` for this modifier. "
			"\n\n"
			"**Modifier outputs:**"
			"\n\n"
			" * ``CreateBonds.num_bonds`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
			"   The number of full bonds created by the modifier.\n"
			" * ``Topology`` (:py:class:`~ovito.data.BondProperty`):\n"
			"   The connectivity information of each created bond.\n"
			" * ``Periodic Image`` (:py:class:`~ovito.data.BondProperty`):\n"
			"   The shift vector at periodic boundaries of each created bond.\n")
		.def_property("mode", &CreateBondsModifier::cutoffMode, &CreateBondsModifier::setCutoffMode,
				"Selects the mode of operation. Valid modes are:"
				"\n\n"
				"  * ``CreateBondsModifier.Mode.Uniform``\n"
				"  * ``CreateBondsModifier.Mode.Pairwise``\n"
				"\n\n"
				"In ``Uniform`` mode one global :py:attr:`.cutoff` is used irrespective of the particle types. "
				"In ``Pairwise`` mode a separate cutoff distance must be specified for all pairs of particle types between which bonds are to be created. "
				"\n\n"
				":Default: ``CreateBondsModifier.Mode.Uniform``\n")
		.def_property("vis", &CreateBondsModifier::bondsVis, &CreateBondsModifier::setBondsVis,
				"The :py:class:`~ovito.vis.BondsVis` object controlling the visual appearance of the bonds created by this modifier.")
		.def_property("bond_type", &CreateBondsModifier::bondType, &CreateBondsModifier::setBondType,
				"The :py:class:`~ovito.data.BondType` that will be assigned to the newly created bonds. This lets you control the display color of the new bonds. ")
		.def_property("cutoff", &CreateBondsModifier::uniformCutoff, &CreateBondsModifier::setUniformCutoff,
				"The upper cutoff distance for the creation of bonds between particles. This parameter is only used if :py:attr:`.mode` is set to ``Uniform``. "
				"\n\n"
				":Default: 3.2\n")
		.def_property("intra_molecule_only", &CreateBondsModifier::onlyIntraMoleculeBonds, &CreateBondsModifier::setOnlyIntraMoleculeBonds,
				"If this option is set to true, the modifier will create bonds only between atoms that belong to the same molecule (i.e. which have the same molecule ID assigned to them)."
				"\n\n"
				":Default: ``False``\n")
		.def_property("lower_cutoff", &CreateBondsModifier::minimumCutoff, &CreateBondsModifier::setMinimumCutoff,
				"The minimum bond length. No bonds will be created between particles whose distance is below this threshold. "
				"\n\n"
				":Default: 0.0\n")
		.def("set_pairwise_cutoff", &CreateBondsModifier::setPairwiseCutoff,
				"set_pairwise_cutoff(type_a, type_b, cutoff)"
				"\n\n"
				"Sets the cutoff range for creating bonds between a specific pair of particle types. This information is only used if :py:attr:`.mode` is set to ``Pairwise``."
				"\n\n"
				":param str,int type_a: The :py:attr:`~ovito.data.ElementType.name` or numeric :py:attr:`~ovito.data.ElementType.id` of the first particle type\n"
				":param str,int type_b: The :py:attr:`~ovito.data.ElementType.name` or numeric :py:attr:`~ovito.data.ElementType.id` of the second particle type\n"
				":param float cutoff: The cutoff distance to be used by the modifier for the type pair\n"
				"\n\n"
				"If you want no bonds to be created between a pair of types, set the corresponding cutoff radius to zero (which is the default).",
				py::arg("type_a"), py::arg("type_b"), py::arg("cutoff"))
		.def("get_pairwise_cutoff", &CreateBondsModifier::getPairwiseCutoff,
				"get_pairwise_cutoff(type_a, type_b) -> float"
				"\n\n"
				"Returns the pair-wise cutoff distance that was previously set for a specific pair of particle types using the :py:meth:`set_pairwise_cutoff` method. "
				"\n\n"
				":param str,int type_a: The :py:attr:`~ovito.data.ElementType.name` or numeric :py:attr:`~ovito.data.ElementType.id` of the first particle type\n"
				":param str,int type_b: The :py:attr:`~ovito.data.ElementType.name` or numeric :py:attr:`~ovito.data.ElementType.id` of the second particle type\n"
				":return: The cutoff distance set for the type pair. Returns zero if no cutoff has been set for the pair.\n",
				py::arg("type_a"), py::arg("type_b"))
	;

	ovito_enum<CreateBondsModifier::CutoffMode>(CreateBondsModifier_py, "Mode")
		.value("Uniform", CreateBondsModifier::UniformCutoff)
		.value("Pairwise", CreateBondsModifier::PairCutoff)
	;

	ovito_class<CentroSymmetryModifier, AsynchronousModifier>(m,
			":Base class: :py:class:`ovito.pipeline.Modifier`\n\n"
			"Computes the centro-symmetry parameter (CSP) of each particle. "
			"See also the corresponding :ovitoman:`user manual page <../../particles.modifiers.centrosymmetry>` for this modifier. "
			"\n\n"
			"The modifier outputs the computed values in the ``\"Centrosymmetry\"`` particle property.")
		.def_property("num_neighbors", &CentroSymmetryModifier::numNeighbors, &CentroSymmetryModifier::setNumNeighbors,
				"The number of neighbors to take into account (12 for FCC crystals, 8 for BCC crystals)."
				"\n\n"
				":Default: 12\n")
	;

	auto ClusterAnalysisModifier_py = ovito_class<ClusterAnalysisModifier, AsynchronousModifier>(m,
			":Base class: :py:class:`ovito.pipeline.Modifier`\n\n"
			"This modifier groups particles into clusters on the basis of a neighboring criterion. "
			"See also the corresponding :ovitoman:`user manual page <../../particles.modifiers.cluster_analysis>` for this modifier. "
			"\n\n"
			"**Modifier outputs:**"
			"\n\n"
			" * ``Cluster`` (:py:class:`~ovito.data.ParticleProperty`):\n"
			"   This output particle property stores the IDs of the clusters the particles have been assigned to.\n"
			" * ``ClusterAnalysis.cluster_count`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
			"   The total number of clusters produced by the modifier. Cluster IDs range from 1 to this number.\n"
			" * ``ClusterAnalysis.largest_size`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
			"   The number of particles belonging to the largest cluster (cluster ID 1). This attribute is only computed by the modifier when :py:attr:`.sort_by_size` is set.\n"
			"\n"
			"**Example:**"
			"\n\n"
			"The following script demonstrates the usage of the `numpy.bincount() <https://docs.scipy.org/doc/numpy/reference/generated/numpy.bincount.html>`__ "
			"function for the determining the size (=number of particles) of each cluster. "
			"This function counts how often each cluster ID occurs in the ``Cluster`` particle property generated by the :py:class:`!ClusterAnalysisModifier`. "
			"\n\n"
			".. literalinclude:: ../example_snippets/cluster_analysis_modifier.py\n"
			"\n"
			)
		.def_property("neighbor_mode", &ClusterAnalysisModifier::neighborMode, &ClusterAnalysisModifier::setNeighborMode,
				"Selects the neighboring criterion for the clustering algorithm. Valid values are: "
				"\n\n"
				"  * ``ClusterAnalysisModifier.NeighborMode.CutoffRange``\n"
				"  * ``ClusterAnalysisModifier.NeighborMode.Bonding``\n"
				"\n\n"
				"In the first mode (``CutoffRange``), the clustering algorithm treats pairs of particles as neighbors which are within a certain range of "
				"each other given by the parameter :py:attr:`.cutoff`. "
				"\n\n"
				"In the second mode (``Bonding``), particles which are connected by bonds are combined into clusters. "
				"Bonds between particles can either be loaded from the input simulation file or dynamically created using for example the "
				":py:class:`CreateBondsModifier` or the :py:class:`VoronoiAnalysisModifier`. "
				"\n\n"
				":Default: ``ClusterAnalysisModifier.NeighborMode.CutoffRange``\n")
		.def_property("cutoff", &ClusterAnalysisModifier::cutoff, &ClusterAnalysisModifier::setCutoff,
				"The cutoff distance used by the algorithm to form clusters of connected particles. "
				"This parameter is only used when :py:attr:`.neighbor_mode` is set to ``CutoffRange``; otherwise it is ignored. "
				"\n\n"
				":Default: 3.2\n")
		.def_property("only_selected", &ClusterAnalysisModifier::onlySelectedParticles, &ClusterAnalysisModifier::setOnlySelectedParticles,
				"Lets the modifier perform the analysis only for selected particles. "
				"In this case, particles which are not selected are treated as if they did not exist and will be assigned cluster ID 0. "
				"\n\n"
				":Default: ``False``\n")
		.def_property("sort_by_size", &ClusterAnalysisModifier::sortBySize, &ClusterAnalysisModifier::setSortBySize,
				"Enables the sorting of clusters by size (in descending order). Cluster 1 will be the largest cluster, cluster 2 the second largest, and so on."
				"\n\n"
				":Default: ``False``\n")
	;

	ovito_enum<ClusterAnalysisModifier::NeighborMode>(ClusterAnalysisModifier_py, "NeighborMode")
		.value("CutoffRange", ClusterAnalysisModifier::CutoffRange)
		.value("Bonding", ClusterAnalysisModifier::Bonding)
	;

	ovito_class<CoordinationAnalysisModifier, AsynchronousModifier>(m,
			":Base class: :py:class:`ovito.pipeline.Modifier`\n\n"
			"Computes coordination numbers of individual particles and the radial distribution function (RDF) for the entire system. "
			"See also the corresponding :ovitoman:`user manual page <../../particles.modifiers.coordination_analysis>` for this modifier. "
			"\n\n"
			"The modifier stores the computed per-particle coordination numbers in the ``\"Coordination\"`` output particle property. "
			"The data points of the radial pair distribution histogram computed by the modifier can be accessed through "
			"its :py:attr:`.rdf` attribute after the pipeline evalution is complete. "
			"\n\n"
			"**Examples:**"
			"\n\n"
			"The following batch script demonstrates how to load a particle configuration, compute the RDF using the modifier and export the data to a text file:\n\n"
			".. literalinclude:: ../example_snippets/coordination_analysis_modifier.py\n"
			"\n\n"
			"The next script demonstrates how to compute the RDF for every frame of a simulation sequence and build a time-averaged "
			"RDF histogram from the data:\n\n"
			".. literalinclude:: ../example_snippets/coordination_analysis_modifier_averaging.py\n"
			"\n\n")
		.def_property("cutoff", &CoordinationAnalysisModifier::cutoff, &CoordinationAnalysisModifier::setCutoff,
				"Specifies the cutoff distance for the coordination number calculation and also the range up to which the modifier calculates the RDF. "
				"\n\n"
				":Default: 3.2\n")
		.def_property("number_of_bins", &CoordinationAnalysisModifier::numberOfBins, &CoordinationAnalysisModifier::setNumberOfBins,
				"The number of histogram bins to use when computing the RDF."
				"\n\n"
				":Default: 200\n")
		.def_property("partial", &CoordinationAnalysisModifier::computePartialRDF, &CoordinationAnalysisModifier::setComputePartialRDF,
				"Setting this flag to true requests calculation of element-specific (partial) RDFs. "
				"\n\n"
				":Default: ``False``\n")
	;

	auto ReferenceConfigurationModifier_py = ovito_abstract_class<ReferenceConfigurationModifier, AsynchronousModifier>(m,
			":Base class: :py:class:`ovito.pipeline.Modifier`\n\n"
			"This is the common base class of analyis modifiers that perform some kind of comparison "
			"of the current particle configuration with a reference configuration. For example, "
			"the :py:class:`~ovito.modifiers.CalculateDisplacementsModifier`, the :py:class:`~ovito.modifiers.AtomicStrainModifier` "
			"and the :py:class:`~ovito.modifiers.WignerSeitzAnalysisModifier` are modifier types that require "
			"a reference configuration as additional input. "
			"\n\n"
			"**Constant and sliding reference configurations**"
			"\n\n"
			"The :py:class:`!ReferenceConfigurationModifier` base class provides various fields that "
			"allow you to specify the reference particle configuration. By default, frame 0 of the currently loaded "
			"simulation sequence is used as reference. You can select any other frame with the :py:attr:`.reference_frame` field. "
			"Sometimes an incremental analysis is desired, instead of a fixed reference configuration. That means the sliding reference configuration and the current configuration "
			"are separated along the time axis by a constant period (*delta t*). The incremental analysis mode is activated by "
			"setting the :py:attr:`.use_frame_offset` flag and specifying the desired :py:attr:`.frame_offset`. "
			"\n\n"
			"**External reference configuration file**"
			"\n\n"
			"By default, the reference particle positions are obtained by evaluating the same data pipeline that also "
			"provides the current particle positions, i.e. which the modifier is part of. That means any modifiers preceding this modifier in the pipeline "
			"will also act upon the reference particle configuration, but not modifiers that follow in the pipeline. "
			"\n\n"
			"Instead of taking it from the same data pipeline, you can explicitly provide a reference configuration by loading it from a separate data file. "
			"To this end the :py:attr:`.reference` field contains a :py:class:`~ovito.pipeline.FileSource` object and you can "
			"use its :py:meth:`~ovito.pipeline.FileSource.load` method to load the reference particle positions from a separate file. "
			"\n\n"
			"**Handling of periodic boundary conditions and cell deformations**"
			"\n\n"
			"Certain analysis modifiers such as the :py:class:`~ovito.modifiers.CalculateDisplacementsModifier` and the :py:class:`~ovito.modifiers.AtomicStrainModifier` "
			"calculate the displacements particles experienced between the reference and the current configuration. "
			"Since particle coordinates in periodic simulation cells are often stored in a *wrapped* form, "
			"caculating the displacement vectors is non-trivial when particles have crossed the periodic boundaries. "
			"By default, the *minimum image convention* is used in these cases, but you can turn if off by "
			"setting :py:attr:`.minimum_image_convention` to ``False``, for example if the input particle coordinates "
			"are given in unwrapped form. "
			"\n\n"
			"Furthermore, if the simulation cell of the reference and the current configuration are different, it makes "
			"a slight difference whether displacements are calculated in the reference or in the current frame. "
			"The :py:attr:`.affine_mapping` property controls the type of coordinate mapping that is used. ")
		.def_property("reference", [](ReferenceConfigurationModifier& mod) {
					// This is for backward compatibility with OVITO 2.9.0:
					// A first access to the .reference attribute automatically creates a new FileSource if the field is still empty.
					if(!mod.referenceConfiguration()) {
						PyErr_WarnEx(PyExc_DeprecationWarning, "Access the .reference attribute without creating a FileSource first is deprecated. Automatically creating a FileSource now for backward compatibility.", 2);
						OORef<FileSource> fileSource = new FileSource(mod.dataset());
						mod.setReferenceConfiguration(fileSource);
					}
					return mod.referenceConfiguration();
				},
				&ReferenceConfigurationModifier::setReferenceConfiguration,
				"A pipeline :py:attr:`~ovito.pipeline.Pipeline.source` object that provides the reference particle positions. "
				"By default this field is ``None``, in which case the modifier obtains the reference particle positions from data pipeline it is part of. "
				"You can explicitly assign a data source object such as a :py:class:`~ovito.pipeline.FileSource` or a :py:class:`~ovito.pipeline.StaticSource` to this field "
				"to specify an explicit reference configuration. "
				"\n\n"
				"For backward compatibility reasons with older OVITO versions, a :py:class:`~ovito.pipeline.FileSource` "
				"instance is automatically created for you on the first *read* access to this field. You can call its :py:meth:`~ovito.pipeline.FileSource.load` method "
				"to load the reference particle positions from a data file. "
				"\n\n"
				".. literalinclude:: ../example_snippets/reference_config_modifier_source.py\n"
				"   :lines: 4-\n"
				"\n\n"
				":Default: ``None``\n")
		.def_property("reference_frame", &ReferenceConfigurationModifier::referenceFrameNumber, &ReferenceConfigurationModifier::setReferenceFrameNumber,
				"The frame number to use as reference configuration. Ignored if :py:attr:`.use_frame_offset` is set."
				"\n\n"
				":Default: 0\n")
		.def_property("use_frame_offset", &ReferenceConfigurationModifier::useReferenceFrameOffset, &ReferenceConfigurationModifier::setUseReferenceFrameOffset,
				"Determines whether a sliding reference configuration is taken at a constant time offset (specified by :py:attr:`.frame_offset`) "
				"relative to the current frame. If ``False``, a constant reference configuration is used (set by the :py:attr:`.reference_frame` parameter) "
				"irrespective of the current frame."
				"\n\n"
				":Default: ``False``\n")
		.def_property("frame_offset", &ReferenceConfigurationModifier::referenceFrameOffset, &ReferenceConfigurationModifier::setReferenceFrameOffset,
				"The relative frame offset when using a sliding reference configuration (if :py:attr:`.use_frame_offset` == ``True``). "
				"Negative frame offsets correspond to reference configurations that precede the current configuration in time. "
				"\n\n"
				":Default: -1\n")
		.def_property("minimum_image_convention", &ReferenceConfigurationModifier::useMinimumImageConvention, &ReferenceConfigurationModifier::setUseMinimumImageConvention,
				"If ``False``, then displacements are calculated from the particle coordinates in the reference and the current configuration as is. "
				"Note that in this case the calculated displacements of particles that have crossed a periodic simulation cell boundary will be wrong if their coordinates are stored in a wrapped form. "
				"If ``True``, then the minimum image convention is applied when calculating the displacements of particles that have crossed a periodic boundary. "
				"\n\n"
				":Default: ``True``\n")
		.def_property("affine_mapping", &ReferenceConfigurationModifier::affineMapping, &ReferenceConfigurationModifier::setAffineMapping,
				"Selects the type of affine deformation applied to the particle coordinates of either the reference or the current configuration prior to the actual analysis computation. "
				"Must be one of the following modes:\n"
				" * ``ReferenceConfigurationModifier.AffineMapping.Off``\n"
				" * ``ReferenceConfigurationModifier.AffineMapping.ToReference``\n"
				" * ``ReferenceConfigurationModifier.AffineMapping.ToCurrent``\n"
				"\n\n"
				"When affine mapping is disabled (``AffineMapping.Off``), particle displacement vectors are simply calculated from the difference of current and reference "
				"positions, irrespective of the cell shape the reference and current configuration. Note that this can introduce a small geometric error if the shape of the periodic simulation cell changes considerably. "
           		"The mode ``AffineMapping.ToReference`` applies an affine transformation to the current configuration such that "
           		"all particle positions are first mapped to the reference cell before calculating the displacement vectors. "
           		"The last option, ``AffineMapping.ToCurrent``, does the reverse: it maps the reference particle positions to the deformed cell before calculating the displacements. "
				"\n\n"
				":Default: ``ReferenceConfigurationModifier.AffineMapping.Off``\n")
		// For backward compatibility with OVITO 2.8.2:
		.def_property("eliminate_cell_deformation",
				[](ReferenceConfigurationModifier& mod) { return mod.affineMapping() != ReferenceConfigurationModifier::NO_MAPPING; },
				[](ReferenceConfigurationModifier& mod, bool b) { mod.setAffineMapping(b ? ReferenceConfigurationModifier::TO_REFERENCE_CELL : ReferenceConfigurationModifier::NO_MAPPING); })
		// For backward compatibility with OVITO 2.9.0:
		.def_property("assume_unwrapped_coordinates",
				[](ReferenceConfigurationModifier& mod) { return !mod.useMinimumImageConvention(); },
				[](ReferenceConfigurationModifier& mod, bool b) { mod.setUseMinimumImageConvention(!b); })
	;
	ovito_enum<ReferenceConfigurationModifier::AffineMappingType>(ReferenceConfigurationModifier_py, "AffineMapping")
		.value("Off", ReferenceConfigurationModifier::NO_MAPPING)
		.value("ToReference", ReferenceConfigurationModifier::TO_REFERENCE_CELL)
		.value("ToCurrent", ReferenceConfigurationModifier::TO_CURRENT_CELL)
	;
	ovito_class<ReferenceConfigurationModifierApplication, AsynchronousModifierApplication>{m};

	ovito_class<CalculateDisplacementsModifier, ReferenceConfigurationModifier>(m,
			":Base class: :py:class:`ovito.pipeline.ReferenceConfigurationModifier`"
			"\n\n"
			"Computes the displacement vectors of particles with respect to a reference configuration. "
			"See also the corresponding :ovitoman:`user manual page <../../particles.modifiers.displacement_vectors>` for this modifier. "
			"\n\n"
			"This modifier class inherits from :py:class:`~ovito.pipeline.ReferenceConfigurationModifier`, which provides "
			"various properties that control how the reference configuration is specified and also how displacement "
			"vectors are calculated. "
			"By default, frame 0 of the current simulation sequence is used as reference configuration. "
			"\n\n"
			"**Modifier outputs:**"
			"\n\n"
			" * ``Displacement`` (:py:class:`~ovito.data.ParticleProperty`):\n"
			"   The computed displacement vectors\n"
			" * ``Displacement Magnitude`` (:py:class:`~ovito.data.ParticleProperty`):\n"
			"   The length of the computed displacement vectors\n"
			"\n\n")
		.def_property("vis", &CalculateDisplacementsModifier::vectorVis, &CalculateDisplacementsModifier::setVectorVis,
				"A :py:class:`~ovito.vis.VectorVis` element controlling the visual representation of the computed "
				"displacement vectors. "
				"Note that the computed displacement vectors are hidden by default. You can enable "
				"the visualization of arrows as follows: "
				"\n\n"
				".. literalinclude:: ../example_snippets/calculate_displacements.py\n"
				"   :lines: 4-\n")
	;

	ovito_class<AtomicStrainModifier, ReferenceConfigurationModifier>(m,
			":Base class: :py:class:`ovito.pipeline.ReferenceConfigurationModifier`"
			"\n\n"
			"Computes the atomic-level deformation with respect to a reference configuration. "
			"See also the corresponding :ovitoman:`user manual page <../../particles.modifiers.atomic_strain>` for this modifier. "
			"\n\n"
			"This modifier class inherits from :py:class:`~ovito.pipeline.ReferenceConfigurationModifier`, which provides "
			"various properties that control how the reference configuration is specified and also how particle displacements "
			"are calculated. "
			"By default, frame 0 of the current simulation sequence is used as reference configuration. "
			"\n\n"
			"**Modifier outputs:**"
			"\n\n"
			" * ``Shear Strain`` (:py:class:`~ovito.data.ParticleProperty`):\n"
			"   The *von Mises* shear strain invariant of the atomic Green-Lagrangian strain tensor.\n"
			" * ``Volumetric Strain`` (:py:class:`~ovito.data.ParticleProperty`):\n"
			"   One third of the trace of the atomic Green-Lagrangian strain tensor.\n"
			" * ``Strain Tensor`` (:py:class:`~ovito.data.ParticleProperty`):\n"
			"   The six components of the symmetric Green-Lagrangian strain tensor.\n"
			"   Output of this property must be explicitly enabled with the :py:attr:`.output_strain_tensors` flag.\n"
			" * ``Deformation Gradient`` (:py:class:`~ovito.data.ParticleProperty`):\n"
			"   The nine components of the atomic deformation gradient tensor.\n"
			"   Output of this property must be explicitly enabled with the :py:attr:`.output_deformation_gradients` flag.\n"
			" * ``Stretch Tensor`` (:py:class:`~ovito.data.ParticleProperty`):\n"
			"   The six components of the symmetric right stretch tensor U in the polar decomposition F=RU.\n"
			"   Output of this property must be explicitly enabled with the :py:attr:`.output_stretch_tensors` flag.\n"
			" * ``Rotation`` (:py:class:`~ovito.data.ParticleProperty`):\n"
			"   The atomic microrotation obtained from the polar decomposition F=RU as a quaternion.\n"
			"   Output of this property must be explicitly enabled with the :py:attr:`.output_rotations` flag.\n"
			" * ``Nonaffine Squared Displacement`` (:py:class:`~ovito.data.ParticleProperty`):\n"
			"   The D\\ :sup:`2`\\ :sub:`min` measure of Falk & Langer, which describes the non-affine part of the local deformation.\n"
			"   Output of this property must be explicitly enabled with the :py:attr:`.output_nonaffine_squared_displacements` flag.\n"
			" * ``Selection`` (:py:class:`~ovito.data.ParticleProperty`):\n"
			"   The modifier can select those particles for which a local deformation could not be computed because there were not\n"
			"   enough neighbors within the :py:attr:`.cutoff` range. Those particles with invalid deformation values can subsequently be removed using the\n"
			"   :py:class:`DeleteSelectedModifier`, for example. Selection of invalid particles is controlled by the :py:attr:`.select_invalid_particles` flag.\n"
			" * ``AtomicStrain.invalid_particle_count`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
			"   The number of particles for which the local strain calculation failed because they had not enough neighbors within the :py:attr:`.cutoff` range.\n"
			)
		.def_property("cutoff", &AtomicStrainModifier::cutoff, &AtomicStrainModifier::setCutoff,
				"Sets the distance up to which neighbor atoms are taken into account in the local strain calculation."
				"\n\n"
				":Default: 3.0\n")
		.def_property("output_deformation_gradients", &AtomicStrainModifier::calculateDeformationGradients, &AtomicStrainModifier::setCalculateDeformationGradients,
				"Controls the output of the per-particle deformation gradient tensors. If ``False``, the computed tensors are not output as a particle property to save memory."
				"\n\n"
				":Default: ``False``\n")
		.def_property("output_strain_tensors", &AtomicStrainModifier::calculateStrainTensors, &AtomicStrainModifier::setCalculateStrainTensors,
				"Controls the output of the per-particle strain tensors. If ``False``, the computed strain tensors are not output as a particle property to save memory."
				"\n\n"
				":Default: ``False``\n")
		.def_property("output_stretch_tensors", &AtomicStrainModifier::calculateStretchTensors, &AtomicStrainModifier::setCalculateStretchTensors,
				"Flag that controls the calculation of the per-particle stretch tensors."
				"\n\n"
				":Default: ``False``\n")
		.def_property("output_rotations", &AtomicStrainModifier::calculateRotations, &AtomicStrainModifier::setCalculateRotations,
				"Flag that controls the calculation of the per-particle rotations."
				"\n\n"
				":Default: ``False``\n")
		.def_property("output_nonaffine_squared_displacements", &AtomicStrainModifier::calculateNonaffineSquaredDisplacements, &AtomicStrainModifier::setCalculateNonaffineSquaredDisplacements,
				"Enables the computation of the squared magnitude of the non-affine part of the atomic displacements. The computed values are output in the ``\"Nonaffine Squared Displacement\"`` particle property."
				"\n\n"
				":Default: ``False``\n")
		.def_property("select_invalid_particles", &AtomicStrainModifier::selectInvalidParticles, &AtomicStrainModifier::setSelectInvalidParticles,
				"If ``True``, the modifier selects the particle for which the local strain tensor could not be computed (because of an insufficient number of neighbors within the cutoff)."
				"\n\n"
				":Default: ``True``\n")
	;

	ovito_class<WignerSeitzAnalysisModifier, ReferenceConfigurationModifier>(m,
			":Base class: :py:class:`ovito.pipeline.ReferenceConfigurationModifier`"
			"\n\n"
			"Performs the Wigner-Seitz cell analysis to identify point defects in crystals. "
			"See also the corresponding :ovitoman:`user manual page <../../particles.modifiers.wigner_seitz_analysis>` for this modifier. "
			"\n\n"
			"Defects are identified with respect to a perfect reference crystal configuration. "
			"By default, frame 0 of the current simulation sequence is used as reference configuration. "
			"The modifier inherits from the :py:class:`~ovito.pipeline.ReferenceConfigurationModifier` class, which provides "
			"further settings that control the definition of the reference configuration. "
			"\n\n"
			"**Modifier outputs:**"
			"\n\n"
			" * ``Occupancy`` (:py:class:`~ovito.data.ParticleProperty`):\n"
			"   The computed site occupation numbers, one for each particle in the reference configuration.\n"
			" * ``WignerSeitz.vacancy_count`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
			"   The total number of vacant sites (having ``Occupancy`` == 0). \n"
			" * ``WignerSeitz.interstitial_count`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
			"   The total number of of interstitial atoms. This is equal to the sum of occupancy numbers of all non-empty sites minus the number of non-empty sites.\n"
			"\n\n"
			"**Usage example:**"
			"\n\n"
			"The ``Occupancy`` particle property generated by the Wigner-Seitz algorithm allows to select specific types of point defects, e.g. "
			"antisites, using OVITO's selection tools. One option is to use the :py:class:`ExpressionSelectionModifier` to pick "
			"sites having a certain occupancy. The following script exemplarily demonstrates the use of a custom :py:class:`PythonScriptModifier` to "
			"select and count A-sites occupied by B-atoms in a binary system with two atom types (A=1 and B=2). "
			"\n\n"
			".. literalinclude:: ../example_snippets/wigner_seitz_example.py\n"
			)
		.def_property("per_type_occupancies", &WignerSeitzAnalysisModifier::perTypeOccupancy, &WignerSeitzAnalysisModifier::setPerTypeOccupancy,
				"A flag controlling whether the modifier should compute occupancy numbers on a per-particle-type basis. "
				"\n\n"
				"If false, only the total occupancy number is computed for each reference site, which counts the number "
				"of particles that occupy the site irrespective of their types. If true, then the ``Occupancy`` property "
				"computed by the modifier becomes a vector property with *N* components, where *N* is the number of particle types defined in the system. "
				"Each component of the ``Occupancy`` property counts the number of particles of the corresponding type that occupy the site. For example, "
				"the property component ``Occupancy.1`` contains the number of particles of type 1 that occupy a site. "
				"\n\n"
				":Default: ``False``\n")
		.def_property("output_displaced", &WignerSeitzAnalysisModifier::outputCurrentConfig, &WignerSeitzAnalysisModifier::setOutputCurrentConfig,
				"Specifies whether the modifier should output the atoms of the current configuration or replace them with the sites from the reference configuration. "
				"\n\n"
				"By default, the modifier throws away all atoms of the current configuration and outputs the atomic sites from the reference configuration instead. "
				"Thus, in this default mode, you will obtain information about how many atoms occupy each site from the reference configuration. "
				"If, however, you are more insterested in visualizing the physical atoms that are currently occupying the sites (instead of the sites being occupied), then you should activate this "
				"modifier option. If set to true, the modifier will maintain the input atoms from the current configuration. "
				"The ``Occupancy`` property generated by the modifier will now pertain to the atoms instead of the sites, with the following new meaning: "
				"The occupancy number now counts how many atoms in total are occupying the same site as the atom the property refers to does. "
				"Furthermore, the modifier will in this mode output another property named ``Site Type``, which reports for each atom the type of the reference site "
				"it was assigned to by the W-S algorithm. "
				"\n\n"
				":Default: ``False``\n")
	;

	ovito_class<VoronoiAnalysisModifier, AsynchronousModifier>(m,
			":Base class: :py:class:`ovito.pipeline.Modifier`\n\n"
			"Computes the atomic volumes and coordination numbers using a Voronoi tessellation of the particle system. "
			"See also the corresponding :ovitoman:`user manual page <../../particles.modifiers.voronoi_analysis>` for this modifier. "
			"\n\n"
			"**Modifier outputs:**"
			"\n\n"
			" * ``Atomic Volume`` (:py:class:`~ovito.data.ParticleProperty`):\n"
			"   Stores the computed Voronoi cell volume of each particle.\n"
			" * ``Coordination`` (:py:class:`~ovito.data.ParticleProperty`):\n"
			"   Stores the number of faces of each particle's Voronoi cell.\n"
			" * ``Voronoi Index`` (:py:class:`~ovito.data.ParticleProperty`):\n"
			"   Stores the Voronoi indices computed from each particle's Voronoi cell. This property is only generated when :py:attr:`.compute_indices` is set.\n"
			" * ``Max Face Order`` (:py:class:`~ovito.data.ParticleProperty`):\n"
			"   Particle property with the maximum number of edges in any face of a particle's Voronoi cell. Only when :py:attr:`.compute_indices` is set.\n"
			" * ``Topology`` (:py:class:`~ovito.data.BondProperty`):\n"
			"   Contains the connectivity information of bonds. The modifier creates one bond for each Voronoi face (only if :py:attr:`.generate_bonds` is set)\n"
			" * ``Voronoi.max_face_order`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
			"   This output attribute reports the maximum number of edges of any face in the computed Voronoi tessellation "
			"   (ignoring edges and faces that are below the area and length thresholds)."
			"\n")
		.def_property("only_selected", &VoronoiAnalysisModifier::onlySelected, &VoronoiAnalysisModifier::setOnlySelected,
				"Lets the modifier perform the analysis only for selected particles. Particles that are currently not selected will be treated as if they did not exist."
				"\n\n"
				":Default: ``False``\n")
		.def_property("use_radii", &VoronoiAnalysisModifier::useRadii, &VoronoiAnalysisModifier::setUseRadii,
				"If ``True``, the modifier computes the poly-disperse Voronoi tessellation, which takes into account the radii of particles. "
				"Otherwise a mono-disperse Voronoi tessellation is computed, which is independent of the particle sizes. "
				"\n\n"
				":Default: ``False``\n")
		.def_property("face_threshold", &VoronoiAnalysisModifier::faceThreshold, &VoronoiAnalysisModifier::setFaceThreshold,
				"Specifies a minimum area for individual Voronoi faces in terms of an absolute area. The algorithm will ignore any face of a Voronoi polyhedron with an area smaller than this "
				"threshold when computing the coordination number and the Voronoi index of a particle. "
				"The threshold parameter is an absolute area given in units of length squared (in whatever units your input data is given). "
				"\n\n"
				"Note that this absolute area threshold and the :py:attr:`.relative_face_threshold` are applied simultaneously. "
				"\n\n"
				":Default: 0.0\n")
		.def_property("relative_face_threshold", &VoronoiAnalysisModifier::relativeFaceThreshold, &VoronoiAnalysisModifier::setRelativeFaceThreshold,
				"Specifies a minimum area for Voronoi faces in terms of a fraction of total area of the Voronoi polyhedron surface. The algorithm will ignore any face of a Voronoi polyhedron with an area smaller than this "
				"threshold when computing the coordination number and the Voronoi index of particles. "
				"The threshold parameter is specified as a fraction of the total surface area of the Voronoi polyhedron the faces belong to. "
				"For example, a threshold value of 0.01 would remove those faces from the analysis with an area less than 1% of the total area "
				"of the polyhedron surface. "
				"\n\n"
				"Note that this relative threshold and the absolute :py:attr:`.face_threshold` are applied simultaneously. "
				"\n\n"
				":Default: 0.0\n")
		.def_property("edge_threshold", &VoronoiAnalysisModifier::edgeThreshold, &VoronoiAnalysisModifier::setEdgeThreshold,
				"Specifies the minimum length an edge must have to be considered in the Voronoi index calculation. Edges that are shorter "
				"than this threshold will be ignored when counting the number of edges of a Voronoi face. "
				"The threshold parameter is an absolute value in units of length of your input data. "
				"\n\n"
				":Default: 0.0\n")
		.def_property("compute_indices", &VoronoiAnalysisModifier::computeIndices, &VoronoiAnalysisModifier::setComputeIndices,
				"If ``True``, the modifier calculates the Voronoi indices of particles. The modifier stores the computed indices in a vector particle property "
				"named ``Voronoi Index``. The *i*-th component of this property will contain the number of faces of the "
				"Voronoi cell that have *i* edges. Thus, the first two components of the per-particle vector will always be zero, because the minimum "
				"number of edges a polygon can have is three. "
				"\n\n"
				":Default: ``False``\n")
		.def_property("generate_bonds", &VoronoiAnalysisModifier::computeBonds, &VoronoiAnalysisModifier::setComputeBonds,
				"Controls whether the modifier outputs the nearest neighbor bonds. The modifier will generate a bond "
				"for every pair of adjacent atoms that share a face of the Voronoi tessellation. "
				"No bond will be created if the face's area is below the :py:attr:`.face_threshold` or if "
				"the face has less than three edges that are longer than the :py:attr:`.edge_threshold`."
				"\n\n"
				":Default: ``False``\n")
	;

	ovito_class<LoadTrajectoryModifier, Modifier>(m,
			":Base class: :py:class:`ovito.pipeline.Modifier`\n\n"
			"This modifier loads trajectories of particles from a separate simulation file. "
			"See also the corresponding :ovitoman:`user manual page <../../particles.modifiers.load_trajectory>` for this modifier. "
			"\n\n"
			"A typical usage scenario for this modifier is when the topology of a molecular system (i.e. the definition of atom types, bonds, etc.) is "
			"stored separately from the trajectories of atoms. In this case you should load the topology file first using :py:func:`~ovito.io.import_file`. "
			"Then create and apply the :py:class:`!LoadTrajectoryModifier` to the topology dataset, which loads the trajectory file. "
			"The modifier will replace the static atom positions from the topology dataset with the time-dependent positions from the trajectory file. "
			"\n\n"
			"Example:"
			"\n\n"
			".. literalinclude:: ../example_snippets/load_trajectory_modifier.py")
		.def_property("source", &LoadTrajectoryModifier::trajectorySource, &LoadTrajectoryModifier::setTrajectorySource,
				"A :py:class:`~ovito.pipeline.FileSource` that provides the trajectories of particles. "
				"You can call its :py:meth:`~ovito.pipeline.FileSource.load` function to load a simulation trajectory file "
				"as shown in the code example above.")
	;

	auto PolyhedralTemplateMatchingModifier_py = ovito_class<PolyhedralTemplateMatchingModifier, StructureIdentificationModifier>(m,
			":Base class: :py:class:`ovito.pipeline.Modifier`"
			"\n\n"
			"Uses the Polyhedral Template Matching (PTM) method to classify the local structural neighborhood "
			"of each particle. Additionally, the modifier can compute local orientations, elastic lattice strains and identify "
			"local chemical ordering. "
			"See also the corresponding :ovitoman:`user manual page <../../particles.modifiers.polyhedral_template_matching>` for this modifier. "
			"\n\n"
			"The modifier stores its results as integer values in the ``\"Structure Type\"`` particle property. "
			"The following constants are defined: "
			"\n\n"
			"   * ``PolyhedralTemplateMatchingModifier.Type.OTHER`` (0)\n"
			"   * ``PolyhedralTemplateMatchingModifier.Type.FCC`` (1)\n"
			"   * ``PolyhedralTemplateMatchingModifier.Type.HCP`` (2)\n"
			"   * ``PolyhedralTemplateMatchingModifier.Type.BCC`` (3)\n"
			"   * ``PolyhedralTemplateMatchingModifier.Type.ICO`` (4)\n"
			"   * ``PolyhedralTemplateMatchingModifier.Type.SC`` (5)\n"
			"   * ``PolyhedralTemplateMatchingModifier.Type.CUBIC_DIAMOND`` (6)\n"
			"   * ``PolyhedralTemplateMatchingModifier.Type.HEX_DIAMOND`` (7)\n"
			"\n"
			"**Modifier outputs:**"
			"\n\n"
			" * ``PolyhedralTemplateMatching.counts.OTHER`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
			"   The number of particles not matching any of the known structure types.\n"
			" * ``PolyhedralTemplateMatching.counts.FCC`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
			"   The number of FCC particles found.\n"
			" * ``PolyhedralTemplateMatching.counts.HCP`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
			"   The number of HCP particles found.\n"
			" * ``PolyhedralTemplateMatching.counts.BCC`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
			"   The number of BCC particles found.\n"
			" * ``PolyhedralTemplateMatching.counts.ICO`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
			"   The number of icosahedral particles found.\n"
			" * ``PolyhedralTemplateMatching.counts.SC`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
			"   The number of simple cubic particles found.\n"
			" * ``PolyhedralTemplateMatching.counts.CUBIC_DIAMOND`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
			"   The number of cubic diamond particles found.\n"
			" * ``PolyhedralTemplateMatching.counts.HEX_DIAMOND`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
			"   The number of hexagonal diamond particles found.\n"
			" * ``Structure Type`` (:py:class:`~ovito.data.ParticleProperty`):\n"
			"   This output particle property will contain the per-particle structure types assigned by the modifier.\n"
			" * ``RMSD`` (:py:class:`~ovito.data.ParticleProperty`):\n"
			"   This particle property will contain the per-particle RMSD values computed by the PTM method.\n"
			"   The modifier will output this property only if the :py:attr:`.output_rmsd` flag is set.\n"
			" * ``Interatomic Distance`` (:py:class:`~ovito.data.ParticleProperty`):\n"
			"   This particle property will contain the local interatomic distances computed by the PTM method.\n"
			"   The modifier will output this property only if the :py:attr:`.output_interatomic_distance` flag is set.\n"
			" * ``Orientation`` (:py:class:`~ovito.data.ParticleProperty`):\n"
			"   This particle property will contain the local lattice orientations computed by the PTM method\n"
			"   encoded as quaternions.\n"
			"   The modifier will generate this property only if the :py:attr:`.output_orientation` flag is set.\n"
			" * ``Elastic Deformation Gradient`` (:py:class:`~ovito.data.ParticleProperty`):\n"
			"   This particle property will contain the local elastic deformation gradient tensors computed by the PTM method.\n"
			"   The modifier will output this property only if the :py:attr:`.output_deformation_gradient` flag is set.\n"
			" * ``Ordering Type`` (:py:class:`~ovito.data.ParticleProperty`):\n"
			"   This output particle property contains the compound ordering types assigned to particles by the modifier \n"
			"   (only if the :py:attr:`.output_ordering` flag is set).\n"
			"   The ordering types are encoded as integer values in the ``\"Ordering Type\"`` particle property. "
			"   The following type constants are defined: "
			"\n\n"
			"      * ``PolyhedralTemplateMatchingModifier.OrderingType.NONE`` (0)\n"
			"      * ``PolyhedralTemplateMatchingModifier.OrderingType.PURE`` (1)\n"
			"      * ``PolyhedralTemplateMatchingModifier.OrderingType.L10`` (2)\n"
			"      * ``PolyhedralTemplateMatchingModifier.OrderingType.L12_A`` (3)\n"
			"      * ``PolyhedralTemplateMatchingModifier.OrderingType.L12_B`` (4)\n"
			"      * ``PolyhedralTemplateMatchingModifier.OrderingType.B2`` (5)\n"
			"      * ``PolyhedralTemplateMatchingModifier.OrderingType.ZINCBLENDE_WURTZITE`` (6)\n"
			"      * ``PolyhedralTemplateMatchingModifier.OrderingType.BORON_NITRIDE`` (7)\n"
			" * ``Color`` (:py:class:`~ovito.data.ParticleProperty`):\n"
			"   The modifier assigns a color to each particle based on its identified structure type. "
			"   See the :py:attr:`structures` list for how to change the display color of a structural type. ")
		.def_property("rmsd_cutoff", &PolyhedralTemplateMatchingModifier::rmsdCutoff, &PolyhedralTemplateMatchingModifier::setRmsdCutoff,
				"The maximum allowed root mean square deviation for positive structure matches. "
				"If the this threshold value is non-zero, template matches that yield a RMSD value above the cutoff are classified as \"Other\". "
				"This can be used to filter out spurious template matches (false positives). "
				"\n\n"
				":Default: 0.1\n")
		.def_property("only_selected", &PolyhedralTemplateMatchingModifier::onlySelectedParticles, &PolyhedralTemplateMatchingModifier::setOnlySelectedParticles,
				"Lets the modifier perform the analysis only on the basis of currently selected particles. Unselected particles will be treated as if they did not exist "
				"and will all be assigned to the \"Other\" structure category. "
				"\n\n"
				":Default: ``False``\n")
		.def_property("color_by_type", &PolyhedralTemplateMatchingModifier::colorByType, &PolyhedralTemplateMatchingModifier::setColorByType,
				"Controls whether the modifier assigns a color to each particle based on the identified structure type. "
				"\n\n"
				":Default: ``True``\n")
		.def_property("output_rmsd", &PolyhedralTemplateMatchingModifier::outputRmsd, &PolyhedralTemplateMatchingModifier::setOutputRmsd,
				"Boolean flag that controls whether the modifier outputs the computed per-particle RMSD values as a new particle property named ``RMSD``."
				"\n\n"
				":Default: ``False``\n")
		.def_property("output_interatomic_distance", &PolyhedralTemplateMatchingModifier::outputInteratomicDistance, &PolyhedralTemplateMatchingModifier::setOutputInteratomicDistance,
				"Boolean flag that controls whether the modifier outputs the computed per-particle interatomic distance as a new particle property named ``Interatomic Distance``."
				"\n\n"
				":Default: ``False``\n")
		.def_property("output_orientation", &PolyhedralTemplateMatchingModifier::outputOrientation, &PolyhedralTemplateMatchingModifier::setOutputOrientation,
				"Boolean flag that controls whether the modifier outputs the computed per-particle lattice orientations as a new particle property named ``Orientation``. "
				"The lattice orientation is specified in terms of a quaternion that describes the rotation of the crystal with repect to a reference lattice orientation. "
				"See the OVITO user manual for details. "
				"\n\n"
				":Default: ``False``\n")
		.def_property("output_deformation_gradient", &PolyhedralTemplateMatchingModifier::outputDeformationGradient, &PolyhedralTemplateMatchingModifier::setOutputDeformationGradient,
				"Boolean flag that controls whether the modifier outputs the computed per-particle elastic deformation gradients as a new particle property named ``Elastic Deformation Gradient``."
				"The elastic deformation gradient describes the local deformation and rigid-body rotation of the crystal with repect to an ideal reference lattice configuration. "
				"See the OVITO user manual for details. "
				"\n\n"
				":Default: ``False``\n")
		.def_property("output_ordering", &PolyhedralTemplateMatchingModifier::outputOrderingTypes, &PolyhedralTemplateMatchingModifier::setOutputOrderingTypes,
				"Boolean flag that controls whether the modifier should identify local ordering types and output them as a new particle property named ``Ordering Type``. "
				"\n\n"
				":Default: ``False``\n")
	;
	expose_subobject_list(PolyhedralTemplateMatchingModifier_py, std::mem_fn(&PolyhedralTemplateMatchingModifier::structureTypes), "structures", "PolyhedralTemplateMatchingStructureTypeList",
		"This list contains one :py:class:`~ovito.data.ParticleType` instance each for structural type the modifier can identify. "
		"You can adjust the display color of a structural type as shown in the code example below and turn the identification "
		"of individual types on or off: "
		"\n\n"
		".. literalinclude:: ../example_snippets/polyhedral_template_matching.py\n"
		"   :lines: 5-\n");

	ovito_enum<PTMAlgorithm::StructureType>(PolyhedralTemplateMatchingModifier_py, "Type")
		.value("OTHER", PTMAlgorithm::OTHER)
		.value("FCC", PTMAlgorithm::FCC)
		.value("HCP", PTMAlgorithm::HCP)
		.value("BCC", PTMAlgorithm::BCC)
		.value("ICO", PTMAlgorithm::ICO)
		.value("SC", PTMAlgorithm::SC)
		.value("CUBIC_DIAMOND", PTMAlgorithm::CUBIC_DIAMOND)
		.value("HEX_DIAMOND", PTMAlgorithm::HEX_DIAMOND)
		.value("GRAPHENE", PTMAlgorithm::GRAPHENE)
	;

	ovito_enum<PTMAlgorithm::OrderingType>(PolyhedralTemplateMatchingModifier_py, "OrderingType")
		.value("NONE", PTMAlgorithm::ORDERING_NONE)
		.value("PURE", PTMAlgorithm::ORDERING_PURE)
		.value("L10", PTMAlgorithm::ORDERING_L10)
		.value("L12_A", PTMAlgorithm::ORDERING_L12_A)
		.value("L12_B", PTMAlgorithm::ORDERING_L12_B)
		.value("B2", PTMAlgorithm::ORDERING_B2)
		.value("ZINCBLENDE_WURTZITE", PTMAlgorithm::ORDERING_ZINCBLENDE_WURTZITE)
		.value("BORON_NITRIDE", PTMAlgorithm::ORDERING_BORON_NITRIDE)
	;

#if 0
	// A helper class that combines both the PTMAlgorithm class and an associated PTMAlgorithm::Kernel instance.
	// Only a single kernel is created, because Python scripts are always single-threaded.
	// The constructor function takes a DataCollection as input and will extract the needed information to
	// initialize the PTMAlgorithm with.
	class PTMAlgorithmWrapper : public PTMAlgorithm, public PTMAlgorithm::Kernel {
	public:
		PTMAlgorithmWrapper(const DataCollection& input, const std::set<PTMAlgorithm::StructureType>& structs) : PTMAlgorithm::Kernel(static_cast<PTMAlgorithm&>(*this)) {
			const ParticlesObject* particles = input.expectObject<ParticlesObject>();
			const PropertyObject* posProperty = particles->expectProperty(ParticlesObject::PositionProperty);
			const SimulationCellObject* simCell = input.expectObject<SimulationCellObject>();
			setIdentifyOrdering(particles->getPropertyStorage(ParticlesObject::TypeProperty));
			for(PTMAlgorithm::StructureType stype : structs)
				setStructureTypeIdentification(stype, true);
			if(!isAnyStructureTypeEnabled())
				throw Exception("No PTM structure template has been enabled for identification.");
			prepare(*posProperty->storage(), simCell->data());
			setCalculateDefGradient(true);
		}
	};
	py::class_<PTMAlgorithmWrapper>(PolyhedralTemplateMatchingModifier_py, "Algorithm")
		.def(py::init<const DataCollection&, const std::set<PTMAlgorithm::StructureType>&>())
		.def("identify", &PTMAlgorithmWrapper::identifyStructure)
		.def_property_readonly("structure", &PTMAlgorithmWrapper::structureType)
		.def_property_readonly("rmsd", &PTMAlgorithmWrapper::rmsd)
		.def_property_readonly("defgrad", &PTMAlgorithmWrapper::deformationGradient)
		.def_property_readonly("interatomic_distance", &PTMAlgorithmWrapper::interatomicDistance)
		.def_property_readonly("ordering", &PTMAlgorithmWrapper::orderingType)
		.def_property_readonly("orientation", &PTMAlgorithmWrapper::orientation)
		.def_property_readonly("orientation_mat", [](const PTMAlgorithmWrapper& wrapper) {
				return Matrix3::rotation(wrapper.orientation());
			})
		.def_property_readonly("neighbor_count", &PTMAlgorithmWrapper::numStructureNeighbors)
		.def("get_neighbor", &PTMAlgorithmWrapper::getNeighborInfo, py::return_value_policy::reference_internal)
		.def("get_ideal_vector", &PTMAlgorithmWrapper::getIdealNeighborVector)
	;
#endif

	ovito_class<CoordinationPolyhedraModifier, AsynchronousModifier>(m,
			":Base class: :py:class:`ovito.pipeline.Modifier`\n\n"
			"Constructs coordination polyhedra around currently selected particles. "
			"A coordination polyhedron is the convex hull spanned by the bonded neighbors of a particle. "
			"See also the corresponding :ovitoman:`user manual page <../../particles.modifiers.coordination_polyhedra>` for this modifier. ")
		.def_property("vis", &CoordinationPolyhedraModifier::surfaceMeshVis, &CoordinationPolyhedraModifier::setSurfaceMeshVis,
				"A :py:class:`~ovito.vis.SurfaceMeshVis` element controlling the visual representation of the generated polyhedra.\n")
	;

	ovito_class<GenerateTrajectoryLinesModifier, Modifier>(m,
			":Base class: :py:class:`ovito.pipeline.Modifier`"
			"\n\n"
			"This modifier periodically samples the time-dependent positions of particles to produce a :py:class:`~ovito.data.TrajectoryLines` object. "
			"The modifier is typically used to visualize the trajectories of particles as static lines. "
			"See also the corresponding :ovitoman:`user manual page <../../particles.modifiers.generate_trajectory_lines>` for this modifier. "
			"\n\n"
			"The trajectory line generation must be explicitly triggered by a call to :py:meth:`.generate` as shown in the following example. "
			"\n\n"
			".. literalinclude:: ../example_snippets/trajectory_lines.py")
		.def_property("only_selected", &GenerateTrajectoryLinesModifier::onlySelectedParticles, &GenerateTrajectoryLinesModifier::setOnlySelectedParticles,
				"Controls whether trajectory lines should only by generated for currently selected particles."
				"\n\n"
				":Default: ``True``\n")
		.def_property("unwrap_trajectories", &GenerateTrajectoryLinesModifier::unwrapTrajectories, &GenerateTrajectoryLinesModifier::setUnwrapTrajectories,
				"Controls whether trajectory lines should be automatically unwrapped at the box boundaries when the particles cross a periodic boundary."
				"\n\n"
				":Default: ``True``\n")
		.def_property("sampling_frequency", &GenerateTrajectoryLinesModifier::everyNthFrame, &GenerateTrajectoryLinesModifier::setEveryNthFrame,
				"Length of the animation frame intervals at which the particle positions should be sampled."
				"\n\n"
				":Default: 1\n")
		.def_property("frame_interval", [](GenerateTrajectoryLinesModifier& tgo) -> py::object {
					if(tgo.useCustomInterval()) return py::make_tuple(
						tgo.dataset()->animationSettings()->timeToFrame(tgo.customIntervalStart()),
						tgo.dataset()->animationSettings()->timeToFrame(tgo.customIntervalEnd()));
					else
						return py::none();
				},
				[](GenerateTrajectoryLinesModifier& tgo, py::object arg) {
					if(py::isinstance<py::none>(arg)) {
						tgo.setUseCustomInterval(false);
						return;
					}
					else if(py::isinstance<py::tuple>(arg)) {
						py::tuple tup = py::reinterpret_borrow<py::tuple>(arg);
						if(tup.size() == 2) {
							int a  = tup[0].cast<int>();
							int b  = tup[1].cast<int>();
							tgo.setCustomIntervalStart(tgo.dataset()->animationSettings()->frameToTime(a));
							tgo.setCustomIntervalEnd(tgo.dataset()->animationSettings()->frameToTime(b));
							tgo.setUseCustomInterval(true);
							return;
						}
					}
					throw py::value_error("Tuple of two integers or None expected.");
				},
				"The animation frame interval over which the particle positions are sampled to generate the trajectory lines. "
				"Set this to a tuple of two integers to specify the first and the last animation frame; or use ``None`` to generate trajectory lines "
				"over the entire animation sequence."
				"\n\n"
				":Default: ``None``\n")
		.def("generate", [](GenerateTrajectoryLinesModifier& modifier) {
				if(!modifier.generateTrajectories(ScriptEngine::currentTask()->createSubTask()))
					modifier.throwException(GenerateTrajectoryLinesModifier::tr("Trajectory line generation has been canceled by the user."));
			},
			"Generates the trajectory lines by sampling the positions of the particles from the upstream pipeline in regular animation time intervals. "
			"Make sure you call this method *after* the modifier has been inserted into the pipeline. ")
		.def_property("vis", &GenerateTrajectoryLinesModifier::trajectoryVis, &GenerateTrajectoryLinesModifier::setTrajectoryVis,
			"The :py:class:`~ovito.vis.TrajectoryVis` element controlling the visual appearance of the trajectory lines created by this modifier.")
	;
	ovito_class<GenerateTrajectoryLinesModifierApplication, ModifierApplication>{m};

	ovito_class<UnwrapTrajectoriesModifier, Modifier>(m,
			":Base class: :py:class:`ovito.pipeline.Modifier`"
			"\n\n"
			"This modifier determines when particles cross through the periodic boundaries of the simulation cell "
			"and unwraps the particle coordinates in order to make the trajectories continuous. As a result of this "
			"operation, particle trajectories will no longer fold back into the simulation cell and instead lead outside the "
			"cell. "
			"\n\n"
			"For unwrapping the particle coordinates, the modifier must load all frames of the input simulation trajectory "
			"to detect crossings of the periodic cell boundaries. In the current version of OVITO, this initialization step must be "
			"explicitly triggered by calling the :py:meth:`.update` method as shown in the following example. "
			"\n\n"
			".. literalinclude:: ../example_snippets/unwrap_trajectories.py")
		.def("update", [](UnwrapTrajectoriesModifier& modifier) {
				if(!modifier.detectPeriodicCrossings(ScriptEngine::currentTask()->createSubTask()))
					modifier.throwException(UnwrapTrajectoriesModifier::tr("Unwrapping of particle trajectories has been canceled by the user."));
			},
			"This method detects crossings of the particles through of the periodic cell boundaries. The list of crossing events will subsequently be used by the modifier to unwrap "
			"the particle coordinates and produce continuous particle trajectories. The method loads and steps through all animation frames of the input trajectory, which can take some time. "
			"Make sure you call this method right *after* the modifier has been inserted into the pipeline and *before* the pipeline is evaluated for the first time. ")
	;
	ovito_class<UnwrapTrajectoriesModifierApplication, ModifierApplication>{m};

	ovito_class<ParticlesComputePropertyModifierDelegate, ComputePropertyModifierDelegate>{m}
		.def_property("neighbor_expressions", &ParticlesComputePropertyModifierDelegate::neighborExpressions, &ParticlesComputePropertyModifierDelegate::setNeighborExpressions)
		.def_property("cutoff_radius", &ParticlesComputePropertyModifierDelegate::cutoff, &ParticlesComputePropertyModifierDelegate::setCutoff)
	;

	ovito_class<BondsComputePropertyModifierDelegate, ComputePropertyModifierDelegate>{m};
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
