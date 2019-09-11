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

#include <ovito/pyscript/PyScript.h>
#include <ovito/pyscript/binding/PythonBinding.h>
#include <ovito/ospray/renderer/OSPRayRenderer.h>
#include <ovito/core/app/PluginManager.h>

namespace Ovito { namespace OSPRay { OVITO_BEGIN_INLINE_NAMESPACE(Internal)

using namespace Ovito;
using namespace PyScript;

PYBIND11_MODULE(OSPRayRendererPython, m)
{
	// Register the classes of this plugin with the global PluginManager.
	PluginManager::instance().registerLoadedPluginClasses();

	py::options options;
	options.disable_function_signatures();

	ovito_class<OSPRayRenderer, NonInteractiveSceneRenderer>(m,
			"This is one of the software-based rendering backends of OVITO. OSPRay is an open-source raytracing system integrated into OVITO."
			"\n\n"
			"An instance of this class can be passed to the :py:meth:`Viewport.render_image` or :py:meth:`Viewport.render_anim` methods. "
			"\n\n"
			"OSPRay can render scenes with ambient occlusion lighting, semi-transparent objects, and depth-of-field focal blur. "
			"For technical details of the supported rendering algorithms and parameters, see the `www.ospray.org <http://www.ospray.org>`__ website. "
			"See also the corresponding :ovitoman:`user manual page <../../rendering.ospray_renderer>` for more information on this rendering engine. ")
		.def_property("refinement_iterations", &OSPRayRenderer::refinementIterations, &OSPRayRenderer::setRefinementIterations,
				"The OSPRay renderer supports a feature called adaptive accumulation, which is a progressive rendering method. "
				"During each rendering pass, the rendered image is progressively refined. "
          		"This parameter controls the number of iterations until the refinement stops. "
				"\n\n"
				":Default: 8")
		.def_property("samples_per_pixel", &OSPRayRenderer::samplesPerPixel, &OSPRayRenderer::setSamplesPerPixel,
				"The number of raytracing samples computed per pixel. Larger values can help to reduce aliasing artifacts. "
				"\n\n"
				":Default: 4")
		.def_property("max_ray_recursion", &OSPRayRenderer::maxRayRecursion, &OSPRayRenderer::setMaxRayRecursion,
				"The maximum number of recursion steps during raytracing. Normally, 1 or 2 is enough, but when rendering semi-transparent "
          		"objects, a larger recursion depth is needed. "
				"\n\n"
				":Default: 20")
		.def_property("direct_light_enabled", &OSPRayRenderer::directLightSourceEnabled, &OSPRayRenderer::setDirectLightSourceEnabled,
				"Enables the default directional light source that is positioned behind the camera and is pointing roughly along the viewing direction. "
				"The brightness of the light source is controlled by the :py:attr:`.default_light_intensity` parameter. "
				"\n\n"
				":Default: ``True``")
		.def_property("default_light_intensity", &OSPRayRenderer::defaultLightSourceIntensity, &OSPRayRenderer::setDefaultLightSourceIntensity,
				"The intensity of the default directional light source. The light source must be enabled by setting :py:attr:`.direct_light_enabled`. "
				"\n\n"
				":Default: 3.0")
		.def_property("default_light_angular_diameter", &OSPRayRenderer::defaultLightSourceAngularDiameter, &OSPRayRenderer::setDefaultLightSourceAngularDiameter,
				"Specifies the apparent size (angle in radians) of the default directional light source. "
          		"Setting the angular diameter to a value greater than zero will result in soft shadows when the rendering backend uses stochastic sampling "
          		"(which is only the case for the *Path Tracer* backend). "
				"\n\n"
				":Default: 0.0")
		.def_property("ambient_light_enabled", &OSPRayRenderer::ambientLightEnabled, &OSPRayRenderer::setAmbientLightEnabled,
				"Enables the ambient light, which surrounds the scene and illuminates it from infinity with constant radiance. "
				"\n\n"
				":Default: ``True``")
		.def_property("ambient_brightness", &OSPRayRenderer::ambientBrightness, &OSPRayRenderer::setAmbientBrightness,
				"Controls the radiance of the ambient light. "
				"\n\n"
				":Default: 0.8")
		.def_property("dof_enabled", &OSPRayRenderer::depthOfFieldEnabled, &OSPRayRenderer::setDepthOfFieldEnabled,
				"Enables the depth-of-field effect. Only objects exactly at the distance from the camera specified by the :py:attr:`.focal_length` will appear "
				"sharp when depth-of-field rendering is active. Objects closer to or further from the camera will appear blurred. "
				"\n\n"
				":Default: ``False``")
		.def_property("focal_length", &OSPRayRenderer::dofFocalLength, &OSPRayRenderer::setDofFocalLength,
				"Only objects exactly at this distance from the camera will appear sharp when :py:attr:`.dof_enabled` is set. "
				"Objects closer to or further from the camera will appear blurred. "
				"\n\n"
				":Default: 40.0")
		.def_property("aperture", &OSPRayRenderer::dofAperture, &OSPRayRenderer::setDofAperture,
				"The aperture radius controls how blurred objects will appear that are out of focus if :py:attr:`.dof_enabled` was set. "
				"\n\n"
				":Default: 0.5")
		.def_property("material_shininess", &OSPRayRenderer::materialShininess, &OSPRayRenderer::setMaterialShininess,
				"Specular Phong exponent value for the default material. Usually in the range between 2.0 and 10,000. "
				"\n\n"
				":Default: 10.0")
		.def_property("material_specular_brightness", &OSPRayRenderer::materialSpecularBrightness, &OSPRayRenderer::setMaterialSpecularBrightness,
				"Controls the specular reflectivity of the default material. "
				"\n\n"
				":Default: 0.05")
	;
}

OVITO_REGISTER_PLUGIN_PYTHON_INTERFACE(OSPRayRendererPython);

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
