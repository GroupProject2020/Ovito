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

#include <plugins/vorotop/VoroTopPlugin.h>
#include <plugins/vorotop/VoroTopModifier.h>
#include <plugins/pyscript/binding/PythonBinding.h>
#include <plugins/particles/scripting/PythonBinding.h>
#include <core/app/PluginManager.h>

namespace Ovito { namespace VoroTop {

using namespace PyScript;

PYBIND11_PLUGIN(VoroTop)
{
	// Register the classes of this plugin with the global PluginManager.
	PluginManager::instance().registerLoadedPluginClasses();
	
	py::options options;
	options.disable_function_signatures();

	py::module m("VoroTop");

	auto VoroTopModifier_py = ovito_class<VoroTopModifier, StructureIdentificationModifier>(m,
			":Base class: :py:class:`ovito.pipeline.Modifier`\n\n"
			"This modifier uses the Voronoi cell topology of particles to characterize their local environments "
			"[`Lazar, Han, Srolovitz, PNAS 112:43 (2015) <http://dx.doi.org/10.1073/pnas.1505788112>`_]. "
			"\n\n"
  			"The Voronoi cell of a particle is the region of space closer to it than to any other particle. "
  			"The topology of the Voronoi cell is the manner in which its faces are connected, and describes "
  			"the manner in which a particle's neighbors are arranged.  The topology of a Voronoi cell can be "
  			"completely described in a vector of integers called a *Weinberg vector* "
			"[`Weinberg, IEEE Trans. Circuit Theory 13:2 (1966) <http://dx.doi.org/10.1109/TCT.1966.1082573>`_]. "
			"\n\n"
			"This modifier requires loading a *filter*, which specifies structure types and associated "
    		"Weinberg vectors.  Filters for several common structures can be obtained from the "
			"`VoroTop <https://www.seas.upenn.edu/~mlazar/VoroTop/filters.html>`_ website. "
			"The modifier calculates the Voronoi cell topology of each particle, uses the provided "
    		"filter to determine the structure type, and stores the results in the ``Structure Type`` particle property. "
			"This allows the user to subsequently select particles  of a certain structural type, e.g. by using the "
			":py:class:`SelectParticleTypeModifier`. "
			"\n\n"
			"This method is well-suited for analyzing finite-temperature systems, including those heated to "
    		"their bulk melting temperatures.  This robust behavior relieves the need to quench a sample "
    		"(such as by energy minimization) prior to analysis. "
			"Further information about the Voronoi topology approach for local structure analysis, as well "
    		"as additional filters, can be found on the `VoroTop webpage <https://www.seas.upenn.edu/~mlazar/VoroTop/>`_. "
			"\n\n"
			"**Modifier outputs:**"
			"\n\n"
			" * ``Structure Type`` (:py:class:`~ovito.data.ParticleProperty`):\n"
			"   This output particle property contains the integer structure type computed by the modifier for each particle.\n"
			" * ``Color`` (:py:class:`~ovito.data.ParticleProperty`):\n"
			"   The modifier assigns a color to each particle to indicate its identified structure type. "
			"\n\n"
			)
		.def_property("only_selected", &VoroTopModifier::onlySelectedParticles, &VoroTopModifier::setOnlySelectedParticles,
				"Lets the modifier take into account only selected particles. Particles that are currently not selected will be treated as if they did not exist."
				"\n\n"
				":Default: ``False``\n")
		.def_property("use_radii", &VoroTopModifier::useRadii, &VoroTopModifier::setUseRadii,
				"If ``True``, the modifier computes the poly-disperse Voronoi tessellation, which takes into account the radii of particles. "
				"Otherwise a mono-disperse Voronoi tessellation is computed, which is independent of the particle sizes. "
				"\n\n"
				":Default: ``False``\n")
		.def_property("filter_file", &VoroTopModifier::filterFile, &VoroTopModifier::loadFilterDefinition,
				"Path to the filter definition file used by the modifier. "
				"Filters files are available from the `VoroTop <https://www.seas.upenn.edu/~mlazar/VoroTop/filters.html>`_ website. "
				"\n\n"
				":Default: ``''``\n")
	;
	expose_subobject_list(VoroTopModifier_py, std::mem_fn(&StructureIdentificationModifier::structureTypes), "structures", "VoroTopStructureTypeList",
		"A list of :py:class:`~ovito.data.ParticleType` instances managed by this modifier, one for each structural type loaded from the :py:attr:`.filter_file`. ");

	return m.ptr();
}

OVITO_REGISTER_PLUGIN_PYTHON_INTERFACE(VoroTop);

}	// End of namespace
}	// End of namespace
