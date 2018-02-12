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
#include <plugins/grid/objects/VoxelProperty.h>
#include <plugins/grid/modifier/CreateIsosurfaceModifier.h>
#include <plugins/stdobj/scripting/PythonBinding.h>
#include <plugins/pyscript/binding/PythonBinding.h>
#include <core/app/PluginManager.h>

namespace Ovito { namespace Grid {

using namespace PyScript;

PYBIND11_PLUGIN(Grid)
{
	// Register the classes of this plugin with the global PluginManager.
	PluginManager::instance().registerLoadedPluginClasses();
	
	py::options options;
	options.disable_function_signatures();

	py::module m("Grid");

	auto VoxelProperty_py = ovito_abstract_class<VoxelProperty, PropertyObject>{m}
	;

	ovito_class<CreateIsosurfaceModifier, AsynchronousModifier>(m,
			":Base class: :py:class:`ovito.pipeline.Modifier`\n\n"
			"Generates an isosurface from a field defined on a structured data grid (voxel data)."
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
		.def_property_readonly("vis", &CreateIsosurfaceModifier::surfaceMeshDisplay,
				"The :py:class:`~ovito.vis.SurfaceMeshVis` controlling the visual representation of the generated isosurface.\n")
	;	

	return m.ptr();
}

OVITO_REGISTER_PLUGIN_PYTHON_INTERFACE(Grid);

}	// End of namespace
}	// End of namespace
