///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2018) Alexander Stukowski
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

#include <ovito/grid/Grid.h>
#include <ovito/grid/objects/VoxelGrid.h>
#include <ovito/grid/modifier/CreateIsosurfaceModifier.h>
#include <ovito/grid/modifier/SpatialBinningModifier.h>
#include <ovito/grid/io/VTKVoxelGridExporter.h>
#include <ovito/stdobj/scripting/PythonBinding.h>
#include <ovito/pyscript/binding/PythonBinding.h>
#include <ovito/core/app/PluginManager.h>

namespace Ovito { namespace Grid {

using namespace PyScript;

PYBIND11_MODULE(GridPython, m)
{
	// Register the classes of this plugin with the global PluginManager.
	PluginManager::instance().registerLoadedPluginClasses();

	py::options options;
	options.disable_function_signatures();

	auto VoxelGrid_py = ovito_class<VoxelGrid, PropertyContainer>(m,
		":Base class: :py:class:`ovito.data.PropertyContainer`"
		"\n\n"
		"A two- or three-dimensional structured grid. Each cell or voxel of the grid is of the same size and shape. "
		"The geometry of the entire grid, its :py:attr:`.domain`, is defined by an attached "
		":py:class:`SimulationCell` object, which specific a three-dimensional parallelpiped "
		"or a two-dimensional parallelogram. "
		"\n\n"
		"The :py:attr:`.shape` property of the grid specifies the number of voxels along each "
		"domain cell vector. The size of an individual voxel is given by domain cell size "
		"divided by the number of voxels in each spatial direction. "
		"\n\n"
		"Every voxel of the grid may be associated with one or more field values. The data for these *voxel properties* "
		"is stored in standard :py:class:`Property` objects, similar to particle or bond properties. Voxel properties can be accessed by name through "
		"the dictionary interface that the :py:class:`!VoxelGrid` class inherits from its :py:class:`PropertyContainer` "
		"base class. "
		"\n\n"
		"Voxel grids can be loaded from input data files, e.g. a CHGCAR file containing the electron density computed by the VASP code, "
		"or they can be dynamically generated within OVITO. The :py:class:`~ovito.modifiers.SpatialBinningModifier` lets you "
		"project the information associated with the unstructured particle set to a structured voxel grid. "
		"\n\n"
		"Given a voxel grid, the :py:class:`~ovito.modifiers.CreateIsosurfaceModifier` can then generate a :py:class:`~ovito.data.SurfaceMesh` "
		"representing an isosurface for a field quantity defined on the voxel grid. ")
			.def_property_readonly("shape", [](const VoxelGrid& grid) {
				return py::make_tuple(grid.shape()[0], grid.shape()[1], grid.shape()[2]);
			},
			"A tuple with the numbers of grid cells along each of the three cell vectors of the :py:attr:`.domain`. "
			"\n\n"
			"For two-dimensional grids, for which the :py:attr:`~ovito.data.SimulationCell.is2D` property of the :py:attr:`.domain` "
			"is set to true, the third entry of the :py:attr:`!shape` tuple is always equal to 1. ")
	;
	createDataPropertyAccessors(VoxelGrid_py, "title", &VoxelGrid::title, &VoxelGrid::setTitle,
		"The name of the voxel grid as shown in the user interface. ");
	createDataSubobjectAccessors(VoxelGrid_py, "domain", &VoxelGrid::domain, &VoxelGrid::setDomain,
		"The :py:class:`~ovito.data.SimulationCell` describing the (possibly periodic) domain which this "
		"grid is embedded in. Note that this cell generally is indepenent of and may be different from the :py:attr:`~ovito.data.DataCollection.cell` "
		"found in the :py:class:`~ovito.data.DataCollection`. ");

	ovito_class<CreateIsosurfaceModifier, AsynchronousModifier>(m,
			":Base class: :py:class:`ovito.pipeline.Modifier`\n\n"
			"Generates an isosurface from a field defined on a structured data grid (voxel data). "
			"See also the corresponding :ovitoman:`user manual page <../../particles.modifiers.create_isosurface>` for this modifier. "
			"\n\n"
			"**Modifier outputs:**"
			"\n\n"
			" * :py:class:`~ovito.data.SurfaceMesh`:\n"
			"   The isosurface mesh generted by the modifier.\n"
			)
		.def_property("operate_on", modifierPropertyContainerGetter(PROPERTY_FIELD(CreateIsosurfaceModifier::subject)), modifierPropertyContainerSetter(PROPERTY_FIELD(CreateIsosurfaceModifier::subject)),
				"Specifies the voxel grid this modifier should operate on. "
				"\n\n"
				":Default: ``'voxels:'``\n")
		.def_property("isolevel", &CreateIsosurfaceModifier::isolevel, &CreateIsosurfaceModifier::setIsolevel,
				"The value at which to create the isosurface."
				"\n\n"
				":Default: 0.0\n")
		.def_property("property", &CreateIsosurfaceModifier::sourceProperty, &CreateIsosurfaceModifier::setSourceProperty,
				"The name of the voxel property from which the isosurface should be constructed.")
		.def_property("vis", &CreateIsosurfaceModifier::surfaceMeshVis, &CreateIsosurfaceModifier::setSurfaceMeshVis,
				"The :py:class:`~ovito.vis.SurfaceMeshVis` controlling the visual representation of the generated isosurface.\n")
	;

	ovito_abstract_class<SpatialBinningModifierDelegate, AsynchronousModifierDelegate>{m}
	;
	auto BinningModifier_py = ovito_class<SpatialBinningModifier, AsynchronousDelegatingModifier>(m,
			":Base class: :py:class:`ovito.pipeline.Modifier`"
			"\n\n"
			"This modifier applies a reduction operation to a property of all the particles located within a spatial bin. "
			"The output of the modifier is a one-, two- or three-dimensional grid of bin values. "
			"See also the corresponding :ovitoman:`user manual page <../../particles.modifiers.bin_and_reduce>` for this modifier. ")
		.def_property("property", &SpatialBinningModifier::sourceProperty, [](SpatialBinningModifier& mod, py::object val) {
					mod.setSourceProperty(convertPythonPropertyReference(val, mod.delegate() ? &mod.delegate()->containerClass() : nullptr));
				},
				"The name of the input particle property to which the reduction operation should be applied. "
				"This can be one of the :ref:`standard particle properties <particle-types-list>` or a custom particle property. "
				"For vector properties the selected component must be appended to the name, e.g. ``\"Velocity.X\"``. ")
		.def_property("reduction_operation", &SpatialBinningModifier::reductionOperation, &SpatialBinningModifier::setReductionOperation,
				"Selects the reduction operation to be carried out. Possible values are:"
				"\n\n"
				"   * ``SpatialBinningModifier.Operation.Mean``\n"
				"   * ``SpatialBinningModifier.Operation.Sum``\n"
				"   * ``SpatialBinningModifier.Operation.SumVol``\n"
				"   * ``SpatialBinningModifier.Operation.Min``\n"
				"   * ``SpatialBinningModifier.Operation.Max``\n"
				"\n"
				"The operation ``SumVol`` first computes the sum and then divides the result by the volume of the respective bin. "
				"It is intended to compute pressure (or stress) within each bin from the per-atom virial."
				"\n\n"
				":Default: ``SpatialBinningModifier.Operation.Mean``\n")
		.def_property("first_derivative", &SpatialBinningModifier::firstDerivative, &SpatialBinningModifier::setFirstDerivative,
				"If true, the modifier numerically computes the first derivative of the binned data using a finite differences approximation. "
				"This works only for one-dimensional bin grids. "
				"\n\n"
				":Default: ``False``\n")
		.def_property("direction", &SpatialBinningModifier::binDirection, &SpatialBinningModifier::setBinDirection,
				"Selects the alignment of the bins. Possible values:"
				"\n\n"
				"   * ``SpatialBinningModifier.Direction.X``\n"
				"   * ``SpatialBinningModifier.Direction.Y``\n"
				"   * ``SpatialBinningModifier.Direction.Z``\n"
				"   * ``SpatialBinningModifier.Direction.XY``\n"
				"   * ``SpatialBinningModifier.Direction.XZ``\n"
				"   * ``SpatialBinningModifier.Direction.YZ``\n"
				"   * ``SpatialBinningModifier.Direction.XYZ``\n"
				"\n"
				"In the first three cases the modifier generates a one-dimensional grid with bins aligned perpendicular to the selected simulation cell vector. "
				"In the last three cases the modifier generates a two-dimensional grid with bins aligned perpendicular to both selected simulation cell vectors (i.e. parallel to the third vector). "
				"\n\n"
				":Default: ``SpatialBinningModifier.Direction.X``\n")
		.def_property("bin_count_x", &SpatialBinningModifier::numberOfBinsX, &SpatialBinningModifier::setNumberOfBinsX,
				"This attribute sets the number of bins to generate along the first binning axis."
				"\n\n"
				":Default: 200\n")
		.def_property("bin_count_y", &SpatialBinningModifier::numberOfBinsY, &SpatialBinningModifier::setNumberOfBinsY,
				"This attribute sets the number of bins to generate along the second binning axis (only used when generating a two- or three-dimensional grid)."
				"\n\n"
				":Default: 200\n")
		.def_property("bin_count_z", &SpatialBinningModifier::numberOfBinsZ, &SpatialBinningModifier::setNumberOfBinsZ,
				"This attribute sets the number of bins to generate along the third binning axis (only used when generting a three-dimensional grid)."
				"\n\n"
				":Default: 200\n")
		.def_property("only_selected", &SpatialBinningModifier::onlySelectedElements, &SpatialBinningModifier::setOnlySelectedElements,
				"If ``True``, the computation takes into account only the currently selected particles. "
				"You can use this to restrict the calculation to a subset of particles. "
				"\n\n"
				":Default: ``False``\n")
	;

	py::enum_<SpatialBinningModifier::ReductionOperationType>(BinningModifier_py, "Operation")
		.value("Mean", SpatialBinningModifier::RED_MEAN)
		.value("Sum", SpatialBinningModifier::RED_SUM)
		.value("SumVol", SpatialBinningModifier::RED_SUM_VOL)
		.value("Min", SpatialBinningModifier::RED_MIN)
		.value("Max", SpatialBinningModifier::RED_MAX)
	;

	py::enum_<SpatialBinningModifier::BinDirectionType>(BinningModifier_py, "Direction")
		.value("X", SpatialBinningModifier::CELL_VECTOR_1)
		.value("Y", SpatialBinningModifier::CELL_VECTOR_2)
		.value("Z", SpatialBinningModifier::CELL_VECTOR_3)
		.value("XY", SpatialBinningModifier::CELL_VECTORS_1_2)
		.value("XZ", SpatialBinningModifier::CELL_VECTORS_1_3)
		.value("YZ", SpatialBinningModifier::CELL_VECTORS_2_3)
		.value("XYZ", SpatialBinningModifier::CELL_VECTORS_1_2_3)
	;

	ovito_class<VTKVoxelGridExporter, FileExporter>{m}
	;
}

OVITO_REGISTER_PLUGIN_PYTHON_INTERFACE(GridPython);

}	// End of namespace
}	// End of namespace
