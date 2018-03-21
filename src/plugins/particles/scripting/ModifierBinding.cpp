///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2017) Alexander Stukowski
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

#include <plugins/particles/Particles.h>
#include <plugins/particles/objects/ParticleProperty.h>
#include <plugins/particles/modifier/coloring/AmbientOcclusionModifier.h>
#include <plugins/particles/modifier/modify/WrapPeriodicImagesModifier.h>
#include <plugins/particles/modifier/modify/CreateBondsModifier.h>
#include <plugins/particles/modifier/modify/LoadTrajectoryModifier.h>
#include <plugins/particles/modifier/modify/CombineParticleSetsModifier.h>
#include <plugins/particles/modifier/modify/CoordinationPolyhedraModifier.h>
#include <plugins/particles/modifier/properties/ComputePropertyModifier.h>
#include <plugins/particles/modifier/properties/FreezePropertyModifier.h>
#include <plugins/particles/modifier/properties/ComputeBondLengthsModifier.h>
#include <plugins/particles/modifier/properties/InterpolateTrajectoryModifier.h>
#include <plugins/particles/modifier/properties/GenerateTrajectoryLinesModifier.h>
#include <plugins/particles/modifier/selection/ManualSelectionModifier.h>
#include <plugins/particles/modifier/selection/ExpandSelectionModifier.h>
#include <plugins/particles/modifier/analysis/StructureIdentificationModifier.h>
#include <plugins/particles/modifier/analysis/binandreduce/BinAndReduceModifier.h>
#include <plugins/particles/modifier/analysis/bondangle/BondAngleAnalysisModifier.h>
#include <plugins/particles/modifier/analysis/cna/CommonNeighborAnalysisModifier.h>
#include <plugins/particles/modifier/analysis/centrosymmetry/CentroSymmetryModifier.h>
#include <plugins/particles/modifier/analysis/cluster/ClusterAnalysisModifier.h>
#include <plugins/particles/modifier/analysis/coordination/CoordinationNumberModifier.h>
#include <plugins/particles/modifier/analysis/displacements/CalculateDisplacementsModifier.h>
#include <plugins/particles/modifier/analysis/strain/AtomicStrainModifier.h>
#include <plugins/particles/modifier/analysis/wignerseitz/WignerSeitzAnalysisModifier.h>
#include <plugins/particles/modifier/analysis/ptm/PolyhedralTemplateMatchingModifier.h>
#include <plugins/particles/modifier/analysis/voronoi/VoronoiAnalysisModifier.h>
#include <plugins/particles/modifier/analysis/diamond/IdentifyDiamondModifier.h>
#include <plugins/stdobj/properties/PropertyStorage.h>
#include <plugins/stdobj/scripting/PythonBinding.h>
#include <plugins/pyscript/binding/PythonBinding.h>
#include <core/dataset/pipeline/ModifierApplication.h>
#include <core/dataset/io/FileSource.h>
#include <core/dataset/animation/AnimationSettings.h>
#include <core/utilities/concurrent/TaskManager.h>
#include "PythonBinding.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Internal)

using namespace PyScript;

void defineModifiersSubmodule(py::module m)
{
	ovito_class<AmbientOcclusionModifier, AsynchronousModifier>(m,
			":Base class: :py:class:`ovito.pipeline.Modifier`\n\n"
			"Performs a quick lighting calculation to shade particles according to the degree of occlusion by other particles. ")
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
			"This modifier maps particles located outside the simulation cell back into the box by \"wrapping\" their coordinates "
			"around at the periodic boundaries of the simulation cell. This modifier has no parameters.")
	;

	ovito_class<ComputeBondLengthsModifier, Modifier>(m,
			":Base class: :py:class:`ovito.pipeline.Modifier`\n\n"
			"Computes the length of every bond in the system and outputs the values as "
			"a new bond property named ``Length``. "
			"\n\n"
			"**Modifier outputs:**"
			"\n\n"
			" * ``Length`` (:py:class:`~ovito.data.BondProperty`):\n"
			"   The output bond property containing the length of each bond.\n"
			"\n")
	;

	ovito_class<InterpolateTrajectoryModifier, Modifier>(m,
			":Base class: :py:class:`ovito.pipeline.Modifier`"
			"\n\n"
			"This modifier interpolates the particle positions in between successive snapshots of a simulation trajectory. "
    		"It can be used to create smoothly looking animations from relatively coarse sequences of simulation snapshots. ")
		.def_property("minimum_image_convention", &InterpolateTrajectoryModifier::useMinimumImageConvention, &InterpolateTrajectoryModifier::setUseMinimumImageConvention,
				"If this option is activated, the modifier will automatically detect if a particle has crossed a simulation box boundary between two "
    			"successive simulation frames and compute the unwrapped displacement vector correctly. "
				"You should leave this option activated unless the particle positions loaded from the input data file(s) were "
				"stored in unwrapped form by the molecular dynamics code. "
				"\n\n"
				":Default: ``True``\n")
	;

	ovito_class<ComputePropertyModifier, AsynchronousModifier>(m,
			":Base class: :py:class:`ovito.pipeline.Modifier`\n\n"
			"Evaluates a user-defined math expression for every particle and assigns the values to a particle property."
			"\n\n"
			"Usage example:"
			"\n\n"
			".. literalinclude:: ../example_snippets/compute_property_modifier.py\n"
			"   :lines: 6-\n"
			"\n"
			"Note that, in many cases, the :py:class:`PythonScriptModifier` is the better choice to perform computations on particle properties, "
			"unless you need the advanced capabaility of the :py:class:`!ComputePropertyModifier` to evaluate expressions over the neighbors "
			"of a particle. ")
		.def_property("expressions", &ComputePropertyModifier::expressions, &ComputePropertyModifier::setExpressions,
				"A list of strings containing the math expressions to compute, one for each vector component of the output property. "
				"If the output property is a scalar property, the list should comprise one string only. "
				"\n\n"
				":Default: ``[\"0\"]``\n")
		.def_property("neighbor_expressions", &ComputePropertyModifier::neighborExpressions, &ComputePropertyModifier::setNeighborExpressions,
				"A list of strings containing the math expressions for the per-neighbor terms, one for each vector component of the output property. "
				"If the output property is a scalar property, the list should comprise one string only. "
				"\n\n"
				"The neighbor expressions are only evaluated if :py:attr:`.neighbor_mode` is enabled."
				"\n\n"
				":Default: ``[\"0\"]``\n")
		.def_property("output_property", &ComputePropertyModifier::outputProperty, &ComputePropertyModifier::setOutputProperty,
				"The output particle property in which the modifier should store the computed values. "
				"This can be one of the :ref:`standard property names <particle-types-list>` defined by OVITO or a user-defined property name. "
				"Note that the modifier can only generate scalar custom properties, but standard properties may be vector properties. "
				"\n\n"
				":Default: ``\"Custom property\"``\n")
		.def_property("component_count", &ComputePropertyModifier::propertyComponentCount, &ComputePropertyModifier::setPropertyComponentCount)
		.def_property("only_selected", &ComputePropertyModifier::onlySelectedParticles, &ComputePropertyModifier::setOnlySelectedParticles,
				"If ``True``, the property is only computed for selected particles and existing property values "
				"are preserved for unselected particles."
				"\n\n"
				":Default: ``False``\n")
		.def_property("neighbor_mode", &ComputePropertyModifier::neighborModeEnabled, &ComputePropertyModifier::setNeighborModeEnabled,
				"Boolean flag that enabled the neighbor computation mode, where contributions from neighbor particles within the "
				"cutoff radius are taken into account. "
				"\n\n"
				":Default: ``False``\n")
		.def_property("cutoff_radius", &ComputePropertyModifier::cutoff, &ComputePropertyModifier::setCutoff,
				"The cutoff radius up to which neighboring particles are visited. This parameter is only used if :py:attr:`.neighbor_mode` is enabled. "
				"\n\n"
				":Default: 3.0\n")
	;
	ovito_class<ComputePropertyModifierApplication, AsynchronousModifierApplication>{m};

	ovito_class<FreezePropertyModifier, Modifier>(m,
			":Base class: :py:class:`ovito.pipeline.Modifier`\n\n"
			"This modifier obtains the value of a particle property by evaluating the data pipeline at a fixed animation time (frame 0 by default), "
			"and injects it back into the pipeline, optionally under a different name than the original property. "
			"Thus, the :py:class:`!FreezePropertyModifier` allows to *freeze* a dynamically changing property and overwrite its values with those from a fixed point in time. "
			"\n\n"
			"**Example:**"
			"\n\n"
			".. literalinclude:: ../example_snippets/freeze_property_modifier.py\n"
			"   :emphasize-lines: 12-14\n"
			"\n")
		.def_property("source_property", &FreezePropertyModifier::sourceProperty, &FreezePropertyModifier::setSourceProperty,
				"The name of the input particle property that should be evaluated by the modifier at the animation frame give by :py:attr:`.freeze_at`. "
				"It can be one of the :ref:`standard particle properties <particle-types-list>` or a custom particle property. ")
		.def_property("destination_property", &FreezePropertyModifier::destinationProperty, &FreezePropertyModifier::setDestinationProperty,
				"The name of the output particle property that should be created by the modifier. "
				"It can be one of the :ref:`standard particle properties <particle-types-list>` or a custom particle property. It may be the same as the :py:attr:`.source_property`. "
				"If the destination property already exists in the input, its values are overwritten. ")
		.def_property("freeze_at", 
				[](FreezePropertyModifier& mod) {
					return mod.dataset()->animationSettings()->timeToFrame(mod.freezeTime());
				},
				[](FreezePropertyModifier& mod, int frame) {
					mod.setFreezeTime(mod.dataset()->animationSettings()->frameToTime(frame));
				},
				"The animation frame number at which to freeze the input property's values. "
				"\n\n"
				":Default: 0\n")
	;
	ovito_class<FreezePropertyModifierApplication, ModifierApplication>{m};

	ovito_class<ManualSelectionModifier, Modifier>(m)
		.def("reset_selection", &ManualSelectionModifier::resetSelection)
		.def("select_all", &ManualSelectionModifier::selectAll)
		.def("clear_selection", &ManualSelectionModifier::clearSelection)
		.def("toggle_particle_selection", &ManualSelectionModifier::toggleParticleSelection)
	;
	ovito_class<ManualSelectionModifierApplication, ModifierApplication>{m};

	auto ExpandSelectionModifier_py = ovito_class<ExpandSelectionModifier, AsynchronousModifier>(m,
			":Base class: :py:class:`ovito.pipeline.Modifier`\n\n"
			"Expands the current particle selection by selecting particles that are neighbors of already selected particles.")
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

	py::enum_<ExpandSelectionModifier::ExpansionMode>(ExpandSelectionModifier_py, "ExpansionMode")
		.value("Cutoff", ExpandSelectionModifier::CutoffRange)
		.value("Nearest", ExpandSelectionModifier::NearestNeighbors)
		.value("Bonded", ExpandSelectionModifier::BondedNeighbors)
	;

	auto BinAndReduceModifier_py = ovito_class<BinAndReduceModifier, Modifier>(m,
			":Base class: :py:class:`ovito.pipeline.Modifier`\n\n"
			"This modifier applies a reduction operation to a property of the particles within a spatial bin. "
			"The output of the modifier is a one or two-dimensional grid of bin values. ")
		.def_property("property", &BinAndReduceModifier::sourceProperty, &BinAndReduceModifier::setSourceProperty,
				"The name of the input particle property to which the reduction operation should be applied. "
				"This can be one of the :ref:`standard particle properties <particle-types-list>` or a custom particle property. "
				"For vector properties the selected component must be appended to the name, e.g. ``\"Velocity.X\"``. ")
		.def_property("reduction_operation", &BinAndReduceModifier::reductionOperation, &BinAndReduceModifier::setReductionOperation,
				"Selects the reduction operation to be carried out. Possible values are:"
				"\n\n"
				"   * ``BinAndReduceModifier.Operation.Mean``\n"
				"   * ``BinAndReduceModifier.Operation.Sum``\n"
				"   * ``BinAndReduceModifier.Operation.SumVol``\n"
				"   * ``BinAndReduceModifier.Operation.Min``\n"
				"   * ``BinAndReduceModifier.Operation.Max``\n"
				"\n"
				"The operation ``SumVol`` first computes the sum and then divides the result by the volume of the respective bin. "
				"It is intended to compute pressure (or stress) within each bin from the per-atom virial."
				"\n\n"
				":Default: ``BinAndReduceModifier.Operation.Mean``\n")
		.def_property("first_derivative", &BinAndReduceModifier::firstDerivative, &BinAndReduceModifier::setFirstDerivative,
				"If true, the modifier numerically computes the first derivative of the binned data using a finite differences approximation. "
				"This works only for one-dimensional bin grids. "
				"\n\n"
				":Default: ``False``\n")
		.def_property("direction", &BinAndReduceModifier::binDirection, &BinAndReduceModifier::setBinDirection,
				"Selects the alignment of the bins. Possible values:"
				"\n\n"
				"   * ``BinAndReduceModifier.Direction.Vector_1``\n"
				"   * ``BinAndReduceModifier.Direction.Vector_2``\n"
				"   * ``BinAndReduceModifier.Direction.Vector_3``\n"
				"   * ``BinAndReduceModifier.Direction.Vectors_1_2``\n"
				"   * ``BinAndReduceModifier.Direction.Vectors_1_3``\n"
				"   * ``BinAndReduceModifier.Direction.Vectors_2_3``\n"
				"\n"
				"In the first three cases the modifier generates a one-dimensional grid with bins aligned perpendicular to the selected simulation cell vector. "
				"In the last three cases the modifier generates a two-dimensional grid with bins aligned perpendicular to both selected simulation cell vectors (i.e. parallel to the third vector). "
				"\n\n"
				":Default: ``BinAndReduceModifier.Direction.Vector_3``\n")
		.def_property("bin_count_x", &BinAndReduceModifier::numberOfBinsX, &BinAndReduceModifier::setNumberOfBinsX,
				"This attribute sets the number of bins to generate along the first binning axis."
				"\n\n"
				":Default: 200\n")
		.def_property("bin_count_y", &BinAndReduceModifier::numberOfBinsY, &BinAndReduceModifier::setNumberOfBinsY,
				"This attribute sets the number of bins to generate along the second binning axis (only used when working with a two-dimensional grid)."
				"\n\n"
				":Default: 200\n")
		.def_property("only_selected", &BinAndReduceModifier::onlySelected, &BinAndReduceModifier::setOnlySelected,
				"If ``True``, the computation takes into account only the currently selected particles. "
				"You can use this to restrict the calculation to a subset of particles. "
				"\n\n"
				":Default: ``False``\n")
		.def_property_readonly("bin_data", py::cpp_function([](BinAndReduceModifier& mod) {
					std::vector<size_t> shape;
					if(mod.is1D()) shape.push_back(mod.binData().size());
					else {
						shape.push_back(mod.numberOfBinsY());
						shape.push_back(mod.numberOfBinsX());
						OVITO_ASSERT(shape[0] * shape[1] == mod.binData().size());
					}
					py::array_t<double> array(shape, mod.binData().data(), py::cast(&mod));
					// Mark array as read-only.
					reinterpret_cast<py::detail::PyArray_Proxy*>(array.ptr())->flags &= ~py::detail::npy_api::NPY_ARRAY_WRITEABLE_;
					return array;
				}),
				"Returns a NumPy array containing the reduced bin values computed by the modifier. "
    			"Depending on the selected binning :py:attr:`.direction` the returned array is either "
    			"one or two-dimensional. In the two-dimensional case the outer index of the returned array "
    			"runs over the bins along the second binning axis. "
				"\n\n"    
    			"Note that accessing this array is only possible after the modifier has computed its results. " 
    			"Thus, you have to call :py:meth:`Pipeline.compute() <ovito.pipeline.Pipeline.compute>` first to ensure that the binning and reduction operation was performed.")
		.def_property_readonly("axis_range_x", [](BinAndReduceModifier& modifier) {
				return py::make_tuple(modifier.xAxisRangeStart(), modifier.xAxisRangeEnd());
			},
			"A 2-tuple containing the range of the generated bin grid along the first binning axis. "
			"Note that this is an output attribute which is only valid after the modifier has performed the bin and reduce operation. "
			"That means you have to call :py:meth:`Pipeline.compute() <ovito.pipeline.Pipeline.compute>` first to evaluate the data pipeline.")
		.def_property_readonly("axis_range_y", [](BinAndReduceModifier& modifier) {
				return py::make_tuple(modifier.yAxisRangeStart(), modifier.yAxisRangeEnd());
			},
			"A 2-tuple containing the range of the generated bin grid along the second binning axis. "
			"Note that this is an output attribute which is only valid after the modifier has performed the bin and reduce operation. "
			"That means you have to call :py:meth:`Pipeline.compute() <ovito.pipeline.Pipeline.compute>` first to evaluate the data pipeline.")
	;

	py::enum_<BinAndReduceModifier::ReductionOperationType>(BinAndReduceModifier_py, "Operation")
		.value("Mean", BinAndReduceModifier::RED_MEAN)
		.value("Sum", BinAndReduceModifier::RED_SUM)
		.value("SumVol", BinAndReduceModifier::RED_SUM_VOL)
		.value("Min", BinAndReduceModifier::RED_MIN)
		.value("Max", BinAndReduceModifier::RED_MAX)
	;

	py::enum_<BinAndReduceModifier::BinDirectionType>(BinAndReduceModifier_py, "Direction")
		.value("Vector_1", BinAndReduceModifier::CELL_VECTOR_1)
		.value("Vector_2", BinAndReduceModifier::CELL_VECTOR_2)
		.value("Vector_3", BinAndReduceModifier::CELL_VECTOR_3)
		.value("Vectors_1_2", BinAndReduceModifier::CELL_VECTORS_1_2)
		.value("Vectors_1_3", BinAndReduceModifier::CELL_VECTORS_1_3)
		.value("Vectors_2_3", BinAndReduceModifier::CELL_VECTORS_2_3)
	;

	ovito_abstract_class<StructureIdentificationModifier, AsynchronousModifier>{m};
	ovito_class<StructureIdentificationModifierApplication, AsynchronousModifierApplication>{m};

	auto BondAngleAnalysisModifier_py = ovito_class<BondAngleAnalysisModifier, StructureIdentificationModifier>(m,
			":Base class: :py:class:`ovito.pipeline.Modifier`\n\n"
			"Performs the bond-angle analysis described by Ackland & Jones to classify the local "
			"crystal structure around each particle. "
			"\n\n"
			"The modifier stores the results as integer values in the ``\"Structure Type\"`` particle property. "
			"The following structure type constants are defined: "
			"\n\n"
			"   * ``BondAngleAnalysisModifier.Type.OTHER`` (0)\n"
			"   * ``BondAngleAnalysisModifier.Type.FCC`` (1)\n"
			"   * ``BondAngleAnalysisModifier.Type.HCP`` (2)\n"
			"   * ``BondAngleAnalysisModifier.Type.BCC`` (3)\n"
			"   * ``BondAngleAnalysisModifier.Type.ICO`` (4)\n"
			"\n\n"
			"**Modifier outputs:**"
			"\n\n"
			" * ``BondAngleAnalysis.counts.OTHER`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
			"   The number of particles not matching any of the known structure types.\n"
			" * ``BondAngleAnalysis.counts.FCC`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
			"   The number of FCC particles found.\n"
			" * ``BondAngleAnalysis.counts.HCP`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
			"   The number of HCP particles found.\n"
			" * ``BondAngleAnalysis.counts.BCC`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
			"   The number of BCC particles found.\n"
			" * ``BondAngleAnalysis.counts.ICO`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
			"   The number of icosahedral found.\n"
			" * ``Structure Type`` (:py:class:`~ovito.data.ParticleProperty`):\n"
			"   This particle property will contain the per-particle structure type assigned by the modifier.\n"
			" * ``Color`` (:py:class:`~ovito.data.ParticleProperty`):\n"
			"   The modifier assigns a color to each particle according to its identified structure type. "
			"\n")
	;
	expose_subobject_list(BondAngleAnalysisModifier_py, std::mem_fn(&StructureIdentificationModifier::structureTypes), "structures", "BondAngleAnalysisStructureTypeList",
		"A list of :py:class:`~ovito.data.ParticleType` instances managed by this modifier, one for each supported structure type. "
		"The display color of a structure type can be changed as follows:: "
		"\n\n"
		"   modifier = BondAngleAnalysisModifier()\n"
		"   # Give all FCC atoms a blue color:\n"
		"   modifier.structures[BondAngleAnalysisModifier.Type.FCC].color = (0, 0, 1)\n"
		"\n\n.\n");

	py::enum_<BondAngleAnalysisModifier::StructureType>(BondAngleAnalysisModifier_py, "Type")
		.value("OTHER", BondAngleAnalysisModifier::OTHER)
		.value("FCC", BondAngleAnalysisModifier::FCC)
		.value("HCP", BondAngleAnalysisModifier::HCP)
		.value("BCC", BondAngleAnalysisModifier::BCC)
		.value("ICO", BondAngleAnalysisModifier::ICO)
	;

	auto CommonNeighborAnalysisModifier_py = ovito_class<CommonNeighborAnalysisModifier, StructureIdentificationModifier>(m,
			":Base class: :py:class:`ovito.pipeline.Modifier`\n\n"
			"Performs the common neighbor analysis (CNA) to classify the structure of the local neighborhood "
			"of each particle. "
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
	;
	expose_subobject_list(CommonNeighborAnalysisModifier_py, std::mem_fn(&StructureIdentificationModifier::structureTypes), "structures", "CommonNeighborAnalysisStructureTypeList",
		"A list of :py:class:`~ovito.data.ParticleType` instances managed by this modifier, one for each supported structure type. "
		"The display color of a structure type can be changed as follows:: "
		"\n\n"
		"   modifier = CommonNeighborAnalysisModifier()\n"
		"   # Give all FCC atoms a blue color:\n"
		"   modifier.structures[CommonNeighborAnalysisModifier.Type.FCC].color = (0, 0, 1)\n"
		"\n\n.\n");

	py::enum_<CommonNeighborAnalysisModifier::CNAMode>(CommonNeighborAnalysisModifier_py, "Mode")
		.value("FixedCutoff", CommonNeighborAnalysisModifier::FixedCutoffMode)
		.value("AdaptiveCutoff", CommonNeighborAnalysisModifier::AdaptiveCutoffMode)
		.value("BondBased", CommonNeighborAnalysisModifier::BondMode)
	;

	py::enum_<CommonNeighborAnalysisModifier::StructureType>(CommonNeighborAnalysisModifier_py, "Type")
		.value("OTHER", CommonNeighborAnalysisModifier::OTHER)
		.value("FCC", CommonNeighborAnalysisModifier::FCC)
		.value("HCP", CommonNeighborAnalysisModifier::HCP)
		.value("BCC", CommonNeighborAnalysisModifier::BCC)
		.value("ICO", CommonNeighborAnalysisModifier::ICO)
	;

	auto IdentifyDiamondModifier_py = ovito_class<IdentifyDiamondModifier, StructureIdentificationModifier>(m,
			":Base class: :py:class:`ovito.pipeline.Modifier`\n\n"
			"This analysis modifier finds atoms that are arranged in a cubic or hexagonal diamond lattice."
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
	;
	expose_subobject_list(IdentifyDiamondModifier_py, std::mem_fn(&StructureIdentificationModifier::structureTypes), "structures", "IdentifyDiamondStructureTypeList",
		"A list of :py:class:`~ovito.data.ParticleType` instances managed by this modifier, one for each supported structure type. "
		"The display color of a structure type can be changed as follows:: "
		"\n\n"
		"      modifier = BondAngleAnalysisModifier()\n"
		"      # Give all hexagonal diamond atoms a blue color:\n"
		"      modifier.structures[IdentifyDiamondModifier.Type.HEX_DIAMOND].color = (0, 0, 1)\n"
		"\n\n.\n");

	py::enum_<IdentifyDiamondModifier::StructureType>(IdentifyDiamondModifier_py, "Type")
		.value("OTHER", IdentifyDiamondModifier::OTHER)
		.value("CUBIC_DIAMOND", IdentifyDiamondModifier::CUBIC_DIAMOND)
		.value("CUBIC_DIAMOND_FIRST_NEIGHBOR", IdentifyDiamondModifier::CUBIC_DIAMOND_FIRST_NEIGH)
		.value("CUBIC_DIAMOND_SECOND_NEIGHBOR", IdentifyDiamondModifier::CUBIC_DIAMOND_SECOND_NEIGH)
		.value("HEX_DIAMOND", IdentifyDiamondModifier::HEX_DIAMOND)
		.value("HEX_DIAMOND_FIRST_NEIGHBOR", IdentifyDiamondModifier::HEX_DIAMOND_FIRST_NEIGH)
		.value("HEX_DIAMOND_SECOND_NEIGHBOR", IdentifyDiamondModifier::HEX_DIAMOND_SECOND_NEIGH)
	;

	auto CreateBondsModifier_py = ovito_class<CreateBondsModifier, AsynchronousModifier>(m,
			":Base class: :py:class:`ovito.pipeline.Modifier`\n\n"
			"Creates bonds between nearby particles. "
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
				"In ``Uniform`` mode one global :py:attr:`.cutoff` is used irrespective of the atom types. "
				"In ``Pairwise`` mode a separate cutoff distance must be specified for all pairs of atom types between which bonds are to be created. "
				"\n\n"
				":Default: ``CreateBondsModifier.Mode.Uniform``\n")
		.def_property("vis", &CreateBondsModifier::bondsVis, &CreateBondsModifier::setBondsVis,
				"The :py:class:`~ovito.vis.BondsVis` object controlling the visual appearance of the bonds created by this modifier.")
		.def_property("cutoff", &CreateBondsModifier::uniformCutoff, &CreateBondsModifier::setUniformCutoff,
				"The maximum cutoff distance for the creation of bonds between particles. This parameter is only used if :py:attr:`.mode` is ``Uniform``. "
				"\n\n"
				":Default: 3.2\n")
		.def_property("intra_molecule_only", &CreateBondsModifier::onlyIntraMoleculeBonds, &CreateBondsModifier::setOnlyIntraMoleculeBonds,
				"If this option is set to true, the modifier will create bonds only between atoms that belong to the same molecule (i.e. which have the same molecule ID assigned to them)."
				"\n\n"
				":Default: ``False``\n")
		.def_property("lower_cutoff", &CreateBondsModifier::minimumCutoff, &CreateBondsModifier::setMinimumCutoff,
				"The minimum bond length. No bonds will be created between atoms whose distance is below this threshold."
				"\n\n"
				":Default: 0.0\n")
		.def("set_pairwise_cutoff", &CreateBondsModifier::setPairCutoff,
				"set_pairwise_cutoff(type_a, type_b, cutoff)"
				"\n\n"
				"Sets the pair-wise cutoff distance for a pair of atom types. This information is only used if :py:attr:`.mode` is ``Pairwise``."
				"\n\n"
				":param str type_a: The :py:attr:`~ovito.data.ParticleType.name` of the first atom type\n"
				":param str type_b: The :py:attr:`~ovito.data.ParticleType.name` of the second atom type (order doesn't matter)\n"
				":param float cutoff: The cutoff distance to be set for the type pair\n"
				"\n\n"
				"If you do not want to create any bonds between a pair of types, set the corresponding cutoff radius to zero (which is the default).",
				py::arg("type_a"), py::arg("type_b"), py::arg("cutoff"))
		.def("get_pairwise_cutoff", &CreateBondsModifier::getPairCutoff,
				"get_pairwise_cutoff(type_a, type_b) -> float"
				"\n\n"
				"Returns the pair-wise cutoff distance set for a pair of atom types."
				"\n\n"
				":param str type_a: The :py:attr:`~ovito.data.ParticleType.name` of the first atom type\n"
				":param str type_b: The :py:attr:`~ovito.data.ParticleType.name` of the second atom type (order doesn't matter)\n"
				":return: The cutoff distance set for the type pair. Returns zero if no cutoff has been set for the pair.\n",
				py::arg("type_a"), py::arg("type_b"))
	;

	py::enum_<CreateBondsModifier::CutoffMode>(CreateBondsModifier_py, "Mode")
		.value("Uniform", CreateBondsModifier::UniformCutoff)
		.value("Pairwise", CreateBondsModifier::PairCutoff)
	;

	ovito_class<CentroSymmetryModifier, AsynchronousModifier>(m,
			":Base class: :py:class:`ovito.pipeline.Modifier`\n\n"
			"Computes the centro-symmetry parameter (CSP) of each particle."
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
			"The following script demonstrates how to apply the `numpy.bincount() <http://docs.scipy.org/doc/numpy/reference/generated/numpy.bincount.html>`__ "
			"function to the ``Cluster`` particle property generated by the :py:class:`!ClusterAnalysisModifier` to determine the size (=number of particles) of each cluster. "
			"\n\n"
			".. literalinclude:: ../example_snippets/cluster_analysis_modifier.py\n"
			"\n"
			)
		.def_property("neighbor_mode", &ClusterAnalysisModifier::neighborMode, &ClusterAnalysisModifier::setNeighborMode,
				"Selects the neighboring criterion for the clustering algorithm. Valid values are: "
				"\n\n"
				"  * ``ClusterAnalysisModifier.NeighborMode.CutoffRange``\n"
				"  * ``ClusterAnalysisModifier.NeighborMode.Bonded``\n"
				"\n\n"
				"In the first mode (``CutoffRange``), the clustering algorithm treats pairs of particles as neighbors which are within a certain range of "
				"each other given by the parameter :py:attr:`.cutoff`. "
				"\n\n"
				"In the second mode (``Bonded``), particles which are connected by bonds are combined into clusters. "
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

	py::enum_<ClusterAnalysisModifier::NeighborMode>(ClusterAnalysisModifier_py, "NeighborMode")
		.value("CutoffRange", ClusterAnalysisModifier::CutoffRange)
		.value("Bonding", ClusterAnalysisModifier::Bonding)
	;

	ovito_class<CoordinationNumberModifier, AsynchronousModifier>(m,
			":Base class: :py:class:`ovito.pipeline.Modifier`\n\n"
			"Computes coordination numbers of individual particles and the radial distribution function (RDF) for the entire system. "
			"\n\n"
			"The modifier stores the computed per-particle coordination numbers in the ``\"Coordination\"`` output particle property. "
			"The data points of the radial pair distribution histogram computed by the modifier can be accessed through "
			"its :py:attr:`.rdf` attribute after the pipeline evalution is complete. "
			"\n\n"
			"**Examples:**"
			"\n\n"
			"The following script (to be executed with :program:`ovitos`) demonstrates how to load a particle configuration, compute the RDF using the modifier and export the data to a text file:\n\n"
			".. literalinclude:: ../example_snippets/coordination_analysis_modifier.py\n"
			"\n\n"
			"The second script below demonstrates how to compute the RDF for every frame of a simulation sequence and build a time-averaged "
			"RDF histogram from the data:\n\n"
			".. literalinclude:: ../example_snippets/coordination_analysis_modifier_averaging.py\n"
			"\n\n")
		.def_property("cutoff", &CoordinationNumberModifier::cutoff, &CoordinationNumberModifier::setCutoff,
				"Specifies the cutoff distance for the coordination number calculation and also the range up to which the modifier calculates the RDF. "
				"\n\n"
				":Default: 3.2\n")
		.def_property("number_of_bins", &CoordinationNumberModifier::numberOfBins, &CoordinationNumberModifier::setNumberOfBins,
				"The number of histogram bins to use when computing the RDF."
				"\n\n"
				":Default: 200\n")
		// For backward compatibility with OVITO 2.9.0:
		.def_property_readonly("rdf_x", py::cpp_function([](CoordinationNumberModifier& mod) {
					CoordinationNumberModifierApplication* modApp = dynamic_object_cast<CoordinationNumberModifierApplication>(mod.someModifierApplication());
					if(!modApp) mod.throwException(CoordinationNumberModifier::tr("Modifier has not been evaluated yet. RDF data is not yet available."));
					py::array_t<double> array((size_t)modApp->rdfX().size(), modApp->rdfX().data(), py::cast(static_cast<ModifierApplication*>(modApp)));
					reinterpret_cast<py::detail::PyArray_Proxy*>(array.ptr())->flags &= ~py::detail::npy_api::NPY_ARRAY_WRITEABLE_;
					return array;
				}))
		.def_property_readonly("rdf_y", py::cpp_function([](CoordinationNumberModifier& mod) {
					CoordinationNumberModifierApplication* modApp = dynamic_object_cast<CoordinationNumberModifierApplication>(mod.someModifierApplication());
					if(!modApp) mod.throwException(CoordinationNumberModifier::tr("Modifier has not been evaluated yet. RDF data is not yet available."));
					py::array_t<double> array((size_t)modApp->rdfY().size(), modApp->rdfY().data(), py::cast(static_cast<ModifierApplication*>(modApp)));
					reinterpret_cast<py::detail::PyArray_Proxy*>(array.ptr())->flags &= ~py::detail::npy_api::NPY_ARRAY_WRITEABLE_;
					return array;
				}))
	;
	ovito_class<CoordinationNumberModifierApplication, AsynchronousModifierApplication>{m};

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
						fileSource->setAdjustAnimationIntervalEnabled(false);
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
	py::enum_<ReferenceConfigurationModifier::AffineMappingType>(ReferenceConfigurationModifier_py, "AffineMapping")
		.value("Off", ReferenceConfigurationModifier::NO_MAPPING)
		.value("ToReference", ReferenceConfigurationModifier::TO_REFERENCE_CELL)
		.value("ToCurrent", ReferenceConfigurationModifier::TO_CURRENT_CELL)
	;	
	ovito_class<ReferenceConfigurationModifierApplication, AsynchronousModifierApplication>{m};
	
	ovito_class<CalculateDisplacementsModifier, ReferenceConfigurationModifier>(m,
			":Base class: :py:class:`ovito.pipeline.ReferenceConfigurationModifier`"
			"\n\n"
			"Computes the displacement vectors of particles with respect to a reference configuration. "
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
				"Note that the computed displacement vectors are not shown by default. You can enable "
				"the display of arrows as follows: "
				"\n\n"
				".. literalinclude:: ../example_snippets/calculate_displacements.py\n"
				"   :lines: 3-\n")	
	;

	ovito_class<AtomicStrainModifier, ReferenceConfigurationModifier>(m,
			":Base class: :py:class:`ovito.pipeline.ReferenceConfigurationModifier`"
			"\n\n"
			"Computes the atomic-level deformation with respect to a reference configuration. "
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
			"   :py:class:`DeleteSelectedParticlesModifier`, for example. Selection of invalid particles is controlled by the :py:attr:`.select_invalid_particles` flag.\n"
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
		.def_property("keep_current_config", &WignerSeitzAnalysisModifier::keepCurrentConfig, &WignerSeitzAnalysisModifier::setKeepCurrentConfig,
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
			"Computes the atomic volumes and coordination numbers using a Voronoi tessellation of the particle system."
			"\n\n"
			"**Modifier outputs:**"
			"\n\n"
			" * ``Atomic Volume`` (:py:class:`~ovito.data.ParticleProperty`):\n"
			"   Stores the computed Voronoi cell volume of each particle.\n"
			" * ``Coordination`` (:py:class:`~ovito.data.ParticleProperty`):\n"
			"   Stores the number of faces of each particle's Voronoi cell.\n"
			" * ``Voronoi Index`` (:py:class:`~ovito.data.ParticleProperty`):\n"
			"   Stores the Voronoi indices computed from each particle's Voronoi cell. This property is only generated when :py:attr:`.compute_indices` is set.\n"
			" * ``Topology`` (:py:class:`~ovito.data.BondProperty`):\n"
			"   Contains the connectivity information of bonds. The modifier creates one bond for each Voronoi face (only if :py:attr:`.generate_bonds` is set)\n"
			" * ``Voronoi.max_face_order`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
			"   This output attribute reports the maximum number of edges of any face in the computed Voronoi tessellation "
			"   (ignoring edges and faces that are below the area and length thresholds)."
			"   Note that, if calculation of Voronoi indices is enabled (:py:attr:`.compute_indices` == true), and :py:attr:`.edge_count` < ``max_face_order``, then "
			"   the computed Voronoi index vectors will be truncated because there exists at least one Voronoi face having more edges than "
			"   the maximum Voronoi vector length specified by :py:attr:`.edge_count`. In such a case you should consider increasing "
			"   :py:attr:`.edge_count` (to at least ``max_face_order``) to not lose information because of truncated index vectors."
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
		.def_property("edge_count", &VoronoiAnalysisModifier::edgeCount, &VoronoiAnalysisModifier::setEdgeCount,
				"Integer parameter controlling the order up to which Voronoi indices are computed by the modifier. "
				"Any Voronoi face with more edges than this maximum value will not be counted! Computed Voronoi index vectors are truncated at the index specified by :py:attr:`.edge_count`. "
				"\n\n"
				"See the ``Voronoi.max_face_order`` output attributes described above on how to avoid truncated Voronoi index vectors."
				"\n\n"
				"This parameter is ignored if :py:attr:`.compute_indices` is false."
				"\n\n"
				":Minimum: 3\n"
				":Default: 6\n")
	;

	ovito_class<LoadTrajectoryModifier, Modifier>(m,
			":Base class: :py:class:`ovito.pipeline.Modifier`\n\n"
			"This modifier loads trajectories of particles from a separate simulation file. "
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

	ovito_class<CombineParticleSetsModifier, Modifier>(m,
			":Base class: :py:class:`ovito.pipeline.Modifier`\n\n"
			"This modifier loads a set of particles from a separate simulation file and merges them into the current dataset. "
			"\n\n"
			"Example:"
			"\n\n"
			".. literalinclude:: ../example_snippets/combine_particle_sets_modifier.py")
		.def_property("source", &CombineParticleSetsModifier::secondaryDataSource, &CombineParticleSetsModifier::setSecondaryDataSource,
				"A :py:class:`~ovito.pipeline.FileSource` that provides the set of particles to be merged. "
				"You can call its :py:meth:`~ovito.pipeline.FileSource.load` function to load a data file "
				"as shown in the code example above.")
	;

	auto PolyhedralTemplateMatchingModifier_py = ovito_class<PolyhedralTemplateMatchingModifier, StructureIdentificationModifier>(m,
			":Base class: :py:class:`ovito.pipeline.Modifier`\n\n"
			"Uses the Polyhedral Template Matching (PTM) method to classify the local structural neighborhood "
			"of each particle. "
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
			" * ``Alloy Type`` (:py:class:`~ovito.data.ParticleProperty`):\n"
			"   This output particle property contains the alloy type assigned to particles by the modifier.\n"
			"   (only if the :py:attr:`.output_alloy_types` flag is set).\n"
			"   The alloy types get stored as integer values in the ``\"Alloy Type\"`` particle property. "
			"   The following alloy type constants are defined: "
			"\n\n"
			"      * ``PolyhedralTemplateMatchingModifier.AlloyType.NONE`` (0)\n"
			"      * ``PolyhedralTemplateMatchingModifier.AlloyType.PURE`` (1)\n"
			"      * ``PolyhedralTemplateMatchingModifier.AlloyType.L10`` (2)\n"
			"      * ``PolyhedralTemplateMatchingModifier.AlloyType.L12_A`` (3)\n"
			"      * ``PolyhedralTemplateMatchingModifier.AlloyType.L12_B`` (4)\n"
			"      * ``PolyhedralTemplateMatchingModifier.AlloyType.B2`` (5)\n"
			"      * ``PolyhedralTemplateMatchingModifier.AlloyType.ZINCBLENDE_WURTZITE`` (6)\n"
			" * ``Color`` (:py:class:`~ovito.data.ParticleProperty`):\n"
			"   The modifier assigns a color to each particle based on its identified structure type. "
			"   You can change the color representing a structural type as follows::"
			"\n\n"
			"      modifier = PolyhedralTemplateMatchingModifier()\n"
			"      # Give all FCC atoms a blue color:\n"
			"      modifier.structures[PolyhedralTemplateMatchingModifier.Type.FCC].color = (0.0, 0.0, 1.0)\n"
			"\n")
		.def_property("rmsd_cutoff", &PolyhedralTemplateMatchingModifier::rmsdCutoff, &PolyhedralTemplateMatchingModifier::setRmsdCutoff,
				"The maximum allowed root mean square deviation for positive structure matches. "
				"If the cutoff is non-zero, template matches that yield a RMSD value above the cutoff are classified as \"Other\". "
				"This can be used to filter out spurious template matches (false positives). "
				"\n\n"
				"If this parameter is zero, no cutoff is applied."
				"\n\n"
				":Default: 0.0\n")
		.def_property("only_selected", &PolyhedralTemplateMatchingModifier::onlySelectedParticles, &PolyhedralTemplateMatchingModifier::setOnlySelectedParticles,
				"Lets the modifier perform the analysis only on the basis of currently selected particles. Unselected particles will be treated as if they did not exist."
				"\n\n"
				":Default: ``False``\n")
		.def_property("output_rmsd", &PolyhedralTemplateMatchingModifier::outputRmsd, &PolyhedralTemplateMatchingModifier::setOutputRmsd,
				"Boolean flag that controls whether the modifier outputs the computed per-particle RMSD values to the pipeline."
				"\n\n"
				":Default: ``False``\n")
		.def_property("output_interatomic_distance", &PolyhedralTemplateMatchingModifier::outputInteratomicDistance, &PolyhedralTemplateMatchingModifier::setOutputInteratomicDistance,
				"Boolean flag that controls whether the modifier outputs the computed per-particle interatomic distance to the pipeline."
				"\n\n"
				":Default: ``False``\n")
		.def_property("output_orientation", &PolyhedralTemplateMatchingModifier::outputOrientation, &PolyhedralTemplateMatchingModifier::setOutputOrientation,
				"Boolean flag that controls whether the modifier outputs the computed per-particle lattice orientation to the pipeline."
				"\n\n"
				":Default: ``False``\n")
		.def_property("output_deformation_gradient", &PolyhedralTemplateMatchingModifier::outputDeformationGradient, &PolyhedralTemplateMatchingModifier::setOutputDeformationGradient,
				"Boolean flag that controls whether the modifier outputs the computed per-particle elastic deformation gradients to the pipeline."
				"\n\n"
				":Default: ``False``\n")
		.def_property("output_alloy_types", &PolyhedralTemplateMatchingModifier::outputAlloyTypes, &PolyhedralTemplateMatchingModifier::setOutputAlloyTypes,
				"Boolean flag that controls whether the modifier identifies localalloy types and outputs them to the pipeline."
				"\n\n"
				":Default: ``False``\n")
	;
	expose_subobject_list(PolyhedralTemplateMatchingModifier_py, std::mem_fn(&PolyhedralTemplateMatchingModifier::structureTypes), "structures", "PolyhedralTemplateMatchingStructureTypeList",
		"A list of :py:class:`~ovito.data.ParticleType` instances managed by this modifier, one for each structural type. "
		"You can adjust the color of structural types as shown in the code example above.");
	
	ovito_class<PolyhedralTemplateMatchingModifierApplication, StructureIdentificationModifierApplication>{m};

	py::enum_<PolyhedralTemplateMatchingModifier::StructureType>(PolyhedralTemplateMatchingModifier_py, "Type")
		.value("OTHER", PolyhedralTemplateMatchingModifier::OTHER)
		.value("FCC", PolyhedralTemplateMatchingModifier::FCC)
		.value("HCP", PolyhedralTemplateMatchingModifier::HCP)
		.value("BCC", PolyhedralTemplateMatchingModifier::BCC)
		.value("ICO", PolyhedralTemplateMatchingModifier::ICO)
		.value("SC", PolyhedralTemplateMatchingModifier::SC)
		.value("CUBIC_DIAMOND", PolyhedralTemplateMatchingModifier::CUBIC_DIAMOND)
		.value("HEX_DIAMOND", PolyhedralTemplateMatchingModifier::HEX_DIAMOND)
	;

	py::enum_<PolyhedralTemplateMatchingModifier::AlloyType>(PolyhedralTemplateMatchingModifier_py, "AlloyType")
		.value("NONE", PolyhedralTemplateMatchingModifier::ALLOY_NONE)
		.value("PURE", PolyhedralTemplateMatchingModifier::ALLOY_PURE)
		.value("L10", PolyhedralTemplateMatchingModifier::ALLOY_L10)
		.value("L12_A", PolyhedralTemplateMatchingModifier::ALLOY_L12_A)
		.value("L12_B", PolyhedralTemplateMatchingModifier::ALLOY_L12_B)
		.value("B2", PolyhedralTemplateMatchingModifier::ALLOY_B2)
		.value("ZINCBLENDE_WURTZITE", PolyhedralTemplateMatchingModifier::ALLOY_ZINCBLENDE_WURTZITE)
	;

	ovito_class<CoordinationPolyhedraModifier, AsynchronousModifier>(m,
			":Base class: :py:class:`ovito.pipeline.Modifier`\n\n"
			"Constructs coordination polyhedra around currently selected particles. "
			"A coordination polyhedron is the convex hull spanned by the bonded neighbors of a particle. ")
		.def_property("vis", &CoordinationPolyhedraModifier::surfaceMeshVis, &CoordinationPolyhedraModifier::setSurfaceMeshVis,
				"A :py:class:`~ovito.vis.SurfaceMeshVis` element controlling the visual representation of the generated polyhedra.\n")
	;
	
	ovito_class<GenerateTrajectoryLinesModifier, Modifier>(m,
			":Base class: :py:class:`ovito.pipeline.Modifier`"
			"\n\n"
			"This modifier periodically samples the time-dependent positions of particles to produce a :py:class:`~ovito.data.TrajectoryLines` object. "
			"The modifier is typically used to visualize the trajectories of particles as static lines. "
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
				if(!modifier.generateTrajectories(ScriptEngine::activeTaskManager()))
					modifier.throwException(ScriptEngine::tr("Trajectory line generation has been canceled by the user."));
			},
			"Generates the trajectory lines by sampling the positions of the particles from the upstream pipeline in regular animation time intervals. "
			"Make sure you call this method *after* the modifier has been inserted into the pipeline. ")
		.def_property("vis", &GenerateTrajectoryLinesModifier::trajectoryVis, &GenerateTrajectoryLinesModifier::setTrajectoryVis,
			"The :py:class:`~ovito.vis.TrajectoryVis` element controlling the visual appearance of the trajectory lines created by this modifier.")
	;
	ovito_class<GenerateTrajectoryLinesModifierApplication, ModifierApplication>{m};
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
