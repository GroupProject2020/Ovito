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
#include <ovito/pyscript/binding/PythonBinding.h>
#include <ovito/stdobj/scripting/PythonBinding.h>
#include <ovito/particles/scripting/PythonBinding.h>
#include <ovito/correlation/SpatialCorrelationFunctionModifier.h>
#include <ovito/core/app/PluginManager.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

using namespace PyScript;

PYBIND11_MODULE(CorrelationFunctionPluginPython, m)
{
	// Register the classes of this plugin with the global PluginManager.
	PluginManager::instance().registerLoadedPluginClasses();

	py::options options;
	options.disable_function_signatures();

	auto SpatialCorrelationFunctionModifier_py = ovito_class<SpatialCorrelationFunctionModifier, AsynchronousModifier>(m,
			":Base class: :py:class:`ovito.pipeline.Modifier`"
			"\n\n"
			"This modifier calculates the spatial correlation function between two particle properties. "
			"See also the corresponding :ovitoman:`user manual page <../../particles.modifiers.correlation_function>` for this modifier. "
			"\n\n"
			"The algorithm uses the FFT to compute the convolution. It then computes a radial average in reciprocal and real space. "
			"This gives the correlation function up to half of the cell size. The modifier can additionally compute the short-ranged part of the "
			"correlation function from a direct summation over neighbors."
			"\n\n"
			"Usage example:"
			"\n\n"
			".. literalinclude:: ../example_snippets/correlation_function_modifier.py\n"
			"\n\n")
		.def_property("property1", &SpatialCorrelationFunctionModifier::sourceProperty1, &SpatialCorrelationFunctionModifier::setSourceProperty1,
				"The name of the first input particle property for which to compute the correlation, P1. "
				"For vector properties a component name must be appended in the string, e.g. ``\"Velocity.X\"``. "
				"\n\n"
				":Default: ``''``\n")
		.def_property("property2", &SpatialCorrelationFunctionModifier::sourceProperty2, &SpatialCorrelationFunctionModifier::setSourceProperty2,
				"The name of the second particle property for which to compute the correlation, P2. "
				"If this is the same as :py:attr:`.property1`, then the modifier will compute the autocorrelation. "
				"\n\n"
				":Default: ``''``\n")
		.def_property("grid_spacing", &SpatialCorrelationFunctionModifier::fftGridSpacing, &SpatialCorrelationFunctionModifier::setFFTGridSpacing,
				"Controls the approximate size of the FFT grid cell. "
				"The actual size is determined by the distance of the simulation cell faces which must contain an integer number of grid cells. "
				"\n\n"
				":Default: 3.0\n")
		.def_property("apply_window", &SpatialCorrelationFunctionModifier::applyWindow, &SpatialCorrelationFunctionModifier::setApplyWindow,
				"This flag controls whether nonperiodic directions have a Hann window applied to them. "
				"Applying a window function is necessary to remove spurios oscillations and power-law scaling of the (implicit) rectangular window of the nonperiodic domain. "
				"\n\n"
				":Default: ``True``\n")
		.def_property("direct_summation", &SpatialCorrelationFunctionModifier::doComputeNeighCorrelation, &SpatialCorrelationFunctionModifier::setComputeNeighCorrelation,
				"Flag controlling whether the real-space correlation plot will show the result of a direct calculation of the correlation function, "
				"obtained by summing over neighbors. "
				"\n\n"
				":Default: ``False``\n")
		.def_property("neighbor_cutoff", &SpatialCorrelationFunctionModifier::neighCutoff, &SpatialCorrelationFunctionModifier::setNeighCutoff,
				"This parameter determines the cutoff of the direct calculation of the real-space correlation function. "
				"\n\n"
				":Default: 5.0\n")
		.def_property("neighbor_bins", &SpatialCorrelationFunctionModifier::numberOfNeighBins, &SpatialCorrelationFunctionModifier::setNumberOfNeighBins,
				"This integer value controls the number of bins for the direct calculation of the real-space correlation function. "
				"\n\n"
				":Default: 50\n")
	;
	py::enum_<SpatialCorrelationFunctionModifier::NormalizationType>(SpatialCorrelationFunctionModifier_py, "Normalization")
		.value("ValueCorrelation", SpatialCorrelationFunctionModifier::VALUE_CORRELATION)
		.value("DifferenceCorrelation", SpatialCorrelationFunctionModifier::DIFFERENCE_CORRELATION)
	;
}

OVITO_REGISTER_PLUGIN_PYTHON_INTERFACE(CorrelationFunctionPluginPython);

}	// End of namespace
}	// End of namespace
