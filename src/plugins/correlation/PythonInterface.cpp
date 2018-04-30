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

#include <plugins/particles/Particles.h>
#include <plugins/pyscript/binding/PythonBinding.h>
#include <plugins/stdobj/scripting/PythonBinding.h>
#include <plugins/particles/scripting/PythonBinding.h>
#include <plugins/correlation/CorrelationFunctionModifier.h>
#include <core/app/PluginManager.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

using namespace PyScript;

PYBIND11_MODULE(CorrelationFunctionPlugin, m)
{
	// Register the classes of this plugin with the global PluginManager.
	PluginManager::instance().registerLoadedPluginClasses();
	
	py::options options;
	options.disable_function_signatures();

	auto CorrelationFunctionModifier_py = ovito_class<CorrelationFunctionModifier, AsynchronousModifier>(m,
			":Base class: :py:class:`ovito.pipeline.Modifier`"
			"\n\n"
			"This modifier calculates the spatial correlation function between two particle properties. "
			"See also the corresponding `user manual page <../../particles.modifiers.correlation_function.html>`__ for this modifier. "
			"\n\n"
			"The algorithm uses the FFT to compute the convolution. It then computes a radial average in reciprocal and real space. "
			"This gives the correlation function up to half of the cell size. The modifier can additionally compute the short-ranged part of the "
			"correlation function from a direct summation over neighbors."
			"\n\n"
			"Usage example:"
			"\n\n"
			".. literalinclude:: ../example_snippets/correlation_function_modifier.py\n"
			"\n\n")
		.def_property("property1", &CorrelationFunctionModifier::sourceProperty1, &CorrelationFunctionModifier::setSourceProperty1,
				"The name of the first input particle property for which to compute the correlation, P1. "
				"For vector properties a component name must be appended in the string, e.g. ``\"Velocity.X\"``. "
				"\n\n"
				":Default: ``''``\n")
		.def_property("property2", &CorrelationFunctionModifier::sourceProperty2, &CorrelationFunctionModifier::setSourceProperty2,
				"The name of the second particle property for which to compute the correlation, P2. "
				"If this is the same as :py:attr:`.property1`, then the modifier will compute the autocorrelation. "
				"\n\n"
				":Default: ``''``\n")
		.def_property("grid_spacing", &CorrelationFunctionModifier::fftGridSpacing, &CorrelationFunctionModifier::setFFTGridSpacing,
				"Controls the approximate size of the FFT grid cell. "
				"The actual size is determined by the distance of the simulation cell faces which must contain an integer number of grid cells. "
				"\n\n"
				":Default: 3.0\n")
		.def_property("apply_window", &CorrelationFunctionModifier::applyWindow, &CorrelationFunctionModifier::setApplyWindow,
				"This flag controls whether nonperiodic directions have a Hann window applied to them. "
				"Applying a window function is necessary to remove spurios oscillations and power-law scaling of the (implicit) rectangular window of the nonperiodic domain. "
				"\n\n"
				":Default: ``True``\n")
		.def_property("direct_summation", &CorrelationFunctionModifier::doComputeNeighCorrelation, &CorrelationFunctionModifier::setComputeNeighCorrelation,
				"Flag controlling whether the real-space correlation plot will show the result of a direct calculation of the correlation function, "
				"obtained by summing over neighbors. "
				"\n\n"
				":Default: ``False``\n")
		.def_property("neighbor_cutoff", &CorrelationFunctionModifier::neighCutoff, &CorrelationFunctionModifier::setNeighCutoff,
				"This parameter determines the cutoff of the direct calculation of the real-space correlation function. "
				"\n\n"
				":Default: 5.0\n")
		.def_property("neighbor_bins", &CorrelationFunctionModifier::numberOfNeighBins, &CorrelationFunctionModifier::setNumberOfNeighBins,
				"This integer value controls the number of bins for the direct calculation of the real-space correlation function. "
				"\n\n"
				":Default: 50\n")

		.def_property_readonly("mean1", py::cpp_function([](CorrelationFunctionModifier& mod) {
				CorrelationFunctionModifierApplication* modApp = dynamic_object_cast<CorrelationFunctionModifierApplication>(mod.someModifierApplication());
				if(!modApp) mod.throwException(CorrelationFunctionModifier::tr("Modifier has not been evaluated yet. Correlation function data is not yet available."));				
				return modApp->mean1();
			}),
			"Returns the computed mean value <P1> of the first input particle property. "    
			"\n\n"
			"Accessing this read-only attribute is only permitted after the modifier has computed its results as part of a data pipeline evaluation. "
			"Thus, you should typically call :py:meth:`Pipeline.compute() <ovito.pipeline.Pipeline.compute>` first to ensure that the modifier has calculated its results. ")
		.def_property_readonly("mean2", py::cpp_function([](CorrelationFunctionModifier& mod) {
				CorrelationFunctionModifierApplication* modApp = dynamic_object_cast<CorrelationFunctionModifierApplication>(mod.someModifierApplication());
				if(!modApp) mod.throwException(CorrelationFunctionModifier::tr("Modifier has not been evaluated yet. Correlation function data is not yet available."));				
				return modApp->mean2();
			}),
			"Returns the computed mean value <P2> of the second input particle property. "    
			"\n\n"
			"Accessing this read-only attribute is only permitted after the modifier has computed its results as part of a data pipeline evaluation. "
			"Thus, you should typically call :py:meth:`Pipeline.compute() <ovito.pipeline.Pipeline.compute>` first to ensure that the modifier has calculated its results. ")
		.def_property_readonly("covariance", py::cpp_function([](CorrelationFunctionModifier& mod) {
				CorrelationFunctionModifierApplication* modApp = dynamic_object_cast<CorrelationFunctionModifierApplication>(mod.someModifierApplication());
				if(!modApp) mod.throwException(CorrelationFunctionModifier::tr("Modifier has not been evaluated yet. Correlation function data is not yet available."));				
				return modApp->covariance();
			}),
			"Returns the computed co-variance value <P1P2> of the two input particle properties. "    
			"\n\n"
			"Accessing this read-only attribute is only permitted after the modifier has computed its results as part of a data pipeline evaluation. "
			"Thus, you should typically call :py:meth:`Pipeline.compute() <ovito.pipeline.Pipeline.compute>` first to ensure that the modifier has calculated its results. ")
		.def_property_readonly("_realspace_correlation", py::cpp_function([](CorrelationFunctionModifier& mod) {
				CorrelationFunctionModifierApplication* modApp = dynamic_object_cast<CorrelationFunctionModifierApplication>(mod.someModifierApplication());
				if(!modApp) mod.throwException(CorrelationFunctionModifier::tr("Modifier has not been evaluated yet. Correlation function data is not yet available."));
				py::array_t<FloatType> array(modApp->realSpaceCorrelation().size(), modApp->realSpaceCorrelation().data(), py::cast(modApp));
				// Mark array as read-only.
				reinterpret_cast<py::detail::PyArray_Proxy*>(array.ptr())->flags &= ~py::detail::npy_api::NPY_ARRAY_WRITEABLE_;
				return array;
			}))				
		.def_property_readonly("_realspace_rdf", py::cpp_function([](CorrelationFunctionModifier& mod) {
				CorrelationFunctionModifierApplication* modApp = dynamic_object_cast<CorrelationFunctionModifierApplication>(mod.someModifierApplication());
				if(!modApp) mod.throwException(CorrelationFunctionModifier::tr("Modifier has not been evaluated yet. Correlation function data is not yet available."));
				py::array_t<FloatType> array(modApp->realSpaceRDF().size(), modApp->realSpaceRDF().data(), py::cast(modApp));
				// Mark array as read-only.
				reinterpret_cast<py::detail::PyArray_Proxy*>(array.ptr())->flags &= ~py::detail::npy_api::NPY_ARRAY_WRITEABLE_;
				return array;
			}))				
		.def_property_readonly("_realspace_x", py::cpp_function([](CorrelationFunctionModifier& mod) {
				CorrelationFunctionModifierApplication* modApp = dynamic_object_cast<CorrelationFunctionModifierApplication>(mod.someModifierApplication());
				if(!modApp) mod.throwException(CorrelationFunctionModifier::tr("Modifier has not been evaluated yet. Correlation function data is not yet available."));
				py::array_t<FloatType> array(modApp->realSpaceCorrelationX().size(), modApp->realSpaceCorrelationX().data(), py::cast(modApp));
				// Mark array as read-only.
				reinterpret_cast<py::detail::PyArray_Proxy*>(array.ptr())->flags &= ~py::detail::npy_api::NPY_ARRAY_WRITEABLE_;
				return array;
			}))				
		.def_property_readonly("_reciprocspace_correlation", py::cpp_function([](CorrelationFunctionModifier& mod) {
				CorrelationFunctionModifierApplication* modApp = dynamic_object_cast<CorrelationFunctionModifierApplication>(mod.someModifierApplication());
				if(!modApp) mod.throwException(CorrelationFunctionModifier::tr("Modifier has not been evaluated yet. Correlation function data is not yet available."));
				py::array_t<FloatType> array(modApp->reciprocalSpaceCorrelation().size(), modApp->reciprocalSpaceCorrelation().data(), py::cast(modApp));
				// Mark array as read-only.
				reinterpret_cast<py::detail::PyArray_Proxy*>(array.ptr())->flags &= ~py::detail::npy_api::NPY_ARRAY_WRITEABLE_;
				return array;
			}))				
		.def_property_readonly("_reciprocspace_x", py::cpp_function([](CorrelationFunctionModifier& mod) {
				CorrelationFunctionModifierApplication* modApp = dynamic_object_cast<CorrelationFunctionModifierApplication>(mod.someModifierApplication());
				if(!modApp) mod.throwException(CorrelationFunctionModifier::tr("Modifier has not been evaluated yet. Correlation function data is not yet available."));
				py::array_t<FloatType> array(modApp->reciprocalSpaceCorrelationX().size(), modApp->reciprocalSpaceCorrelationX().data(), py::cast(modApp));
				// Mark array as read-only.
				reinterpret_cast<py::detail::PyArray_Proxy*>(array.ptr())->flags &= ~py::detail::npy_api::NPY_ARRAY_WRITEABLE_;
				return array;
			}))				
	;
	py::enum_<CorrelationFunctionModifier::NormalizationType>(CorrelationFunctionModifier_py, "Normalization")
		.value("Off", CorrelationFunctionModifier::DO_NOT_NORMALIZE)
		.value("ByCovariance", CorrelationFunctionModifier::NORMALIZE_BY_COVARIANCE)
		.value("ByRDF", CorrelationFunctionModifier::NORMALIZE_BY_RDF)
	;

	ovito_class<CorrelationFunctionModifierApplication, AsynchronousModifierApplication>{m};
}

OVITO_REGISTER_PLUGIN_PYTHON_INTERFACE(CorrelationFunctionPlugin);

}	// End of namespace
}	// End of namespace
