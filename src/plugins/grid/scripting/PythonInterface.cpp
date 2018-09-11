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

#include <plugins/grid/Grid.h>
#include <plugins/grid/objects/VoxelGrid.h>
#include <plugins/grid/modifier/CreateIsosurfaceModifier.h>
#include <plugins/grid/modifier/SpatialBinningModifier.h>
#include <plugins/stdobj/scripting/PythonBinding.h>
#include <plugins/pyscript/binding/PythonBinding.h>
#include <core/app/PluginManager.h>

namespace Ovito { namespace Grid {

using namespace PyScript;

PYBIND11_MODULE(Grid, m)
{
	// Register the classes of this plugin with the global PluginManager.
	PluginManager::instance().registerLoadedPluginClasses();
	
	py::options options;
	options.disable_function_signatures();

	auto VoxelGrid_py = ovito_class<VoxelGrid, PropertyContainer>{m}
	;
	createDataPropertyAccessors(VoxelGrid_py, "title", &VoxelGrid::title, &VoxelGrid::setTitle,
		"The name of the voxel grid as shown in the user interface. ");
	createDataSubobjectAccessors(VoxelGrid_py, "domain", &VoxelGrid::domain, &VoxelGrid::setDomain, 
		"The :py:class:`~ovito.data.SimulationCell` describing the (possibly periodic) domain which this "
		"object is embedded in.");

	ovito_class<CreateIsosurfaceModifier, AsynchronousModifier>(m,
			":Base class: :py:class:`ovito.pipeline.Modifier`\n\n"
			"Generates an isosurface from a field defined on a structured data grid (voxel data). "
			"See also the corresponding `user manual page <../../particles.modifiers.create_isosurface.html>`__ for this modifier. "
			"\n\n"
			"**Modifier outputs:**"
			"\n\n"
			" * :py:class:`~ovito.data.SurfaceMesh`:\n"
			"   The isosurface mesh generted by the modifier.\n"
			)
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
			"See also the corresponding `user manual page <../../particles.modifiers.bin_and_reduce.html>`__ for this modifier. ")
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
				"   * ``SpatialBinningModifier.Direction.Vector_1``\n"
				"   * ``SpatialBinningModifier.Direction.Vector_2``\n"
				"   * ``SpatialBinningModifier.Direction.Vector_3``\n"
				"   * ``SpatialBinningModifier.Direction.Vectors_1_2``\n"
				"   * ``SpatialBinningModifier.Direction.Vectors_1_3``\n"
				"   * ``SpatialBinningModifier.Direction.Vectors_2_3``\n"
				"   * ``SpatialBinningModifier.Direction.Vectors_1_2_3``\n"
				"\n"
				"In the first three cases the modifier generates a one-dimensional grid with bins aligned perpendicular to the selected simulation cell vector. "
				"In the last three cases the modifier generates a two-dimensional grid with bins aligned perpendicular to both selected simulation cell vectors (i.e. parallel to the third vector). "
				"\n\n"
				":Default: ``SpatialBinningModifier.Direction.Vector_3``\n")
		.def_property("bin_count_x", &SpatialBinningModifier::numberOfBinsX, &SpatialBinningModifier::setNumberOfBinsX,
				"This attribute sets the number of bins to generate along the first binning axis."
				"\n\n"
				":Default: 200\n")
		.def_property("bin_count_y", &SpatialBinningModifier::numberOfBinsY, &SpatialBinningModifier::setNumberOfBinsY,
				"This attribute sets the number of bins to generate along the second binning axis (only used when working with a two-dimensional grid)."
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
		.value("Vector_1", SpatialBinningModifier::CELL_VECTOR_1)
		.value("Vector_2", SpatialBinningModifier::CELL_VECTOR_2)
		.value("Vector_3", SpatialBinningModifier::CELL_VECTOR_3)
		.value("Vectors_1_2", SpatialBinningModifier::CELL_VECTORS_1_2)
		.value("Vectors_1_3", SpatialBinningModifier::CELL_VECTORS_1_3)
		.value("Vectors_2_3", SpatialBinningModifier::CELL_VECTORS_2_3)
		.value("Vectors_1_2_3", SpatialBinningModifier::CELL_VECTORS_1_2_3)
	;
}

OVITO_REGISTER_PLUGIN_PYTHON_INTERFACE(Grid);

}	// End of namespace
}	// End of namespace
