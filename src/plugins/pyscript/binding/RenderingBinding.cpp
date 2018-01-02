///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2014) Alexander Stukowski
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

#include <plugins/pyscript/PyScript.h>
#include <core/rendering/RenderSettings.h>
#include <core/rendering/SceneRenderer.h>
#include <core/rendering/noninteractive/NonInteractiveSceneRenderer.h>
#include <core/rendering/ParticlePrimitive.h>
#include <core/rendering/ArrowPrimitive.h>
#include <core/rendering/FrameBuffer.h>
#include <core/dataset/data/DisplayObject.h>
#include <opengl_renderer/StandardSceneRenderer.h>
#include "PythonBinding.h"

namespace PyScript {

using namespace Ovito;

void defineRenderingSubmodule(py::module m)
{
	py::class_<FrameBuffer, std::shared_ptr<FrameBuffer>>(m, "FrameBuffer")
		.def(py::init<>())
		.def(py::init<int, int>())
		.def_property_readonly("width", &FrameBuffer::width)
		.def_property_readonly("height", &FrameBuffer::height)
		.def_property_readonly("_image", [](const FrameBuffer& fb) { return reinterpret_cast<std::uintptr_t>(&fb.image()); })
	;

	py::object RenderSettings_py = ovito_class<RenderSettings, RefTarget>(m,
			"A data structure with parameters that control image and movie generation."
			"\n\n"
			"You typically pass an instance of this class to the :py:meth:`Viewport.render` method to specify various render settings "
			"such as the resolution of the output image and the rendering engine to use: "
			"\n\n"
			".. literalinclude:: ../example_snippets/render_settings.py\n"
			"   :lines: 1-8\n"
			"\n\n"
			"The rendering engine, which produces the two-dimensional rendering of the three-dimensional scene, is part of the :py:class:`!RenderSettings` structure. "
			"You can choose from three different rendering backends: "
			"\n\n"
			"  * :py:class:`OpenGLRenderer` -- The quick renderer which is also used by the interactive viewports of OVITO.\n"
			"  * :py:class:`TachyonRenderer` -- A software-based, high-quality raytracing renderer.\n"
			"  * :py:class:`POVRayRenderer` -- A rendering backend that calls the external POV-Ray raytracing program.\n"
			"\n"
			"To render an image, one must create a :py:class:`Viewport`, set up its virtual camera, and finally invoke its :py:meth:`~Viewport.render` method with "
			"the :py:class:`!RenderSettings` structure: "
			"\n\n"
			".. literalinclude:: ../example_snippets/render_settings.py\n"
			"   :lines: 10-12\n"
			"\n\n"
			"This will render a single frame at the current animation time position, which is given by the global "
			":py:attr:`AnimationSettings.current_frame <ovito.anim.AnimationSettings.current_frame>` setting (frame 0 by default). "
			"\n\n"
			"**Rendering animations**"
			"\n\n"
			"To render an animation, the rendering :py:attr:`.range` must be set to ``RenderSettings.Range.ANIMATION``. "
			"The chosen output :py:attr:`.filename` determines the kind of file(s) that will be produced: "
			"If the name suffix is :file:`.mp4`, :file:`.avi` or :file:`.mov`, a single encoded movie file "
			"will be produced from all rendered frames. The playback speed of the final movie is determined by the "
			"global :py:attr:`AnimationSettings.frames_per_second <ovito.anim.AnimationSettings.frames_per_second>` setting in this case: "
			"\n\n"
			".. literalinclude:: ../example_snippets/render_settings.py\n"
			"   :lines: 14-21\n"
			"\n\n"
			"Alternatively, a series of images can be rendered, which may subsequently be combined into a movie with an external video encoding tool: "
			"\n\n"
			".. literalinclude:: ../example_snippets/render_settings.py\n"
			"   :lines: 23-26\n"
			"\n\n"
			"This produces image files named :file:`frame0000.png`, :file:`frame0001.png`, etc. "
			)
		.def_property("renderer", &RenderSettings::renderer, &RenderSettings::setRenderer,
				"The renderer that is used to generate the image or movie. Depending on the selected renderer you "
				"can use this to set additional parameters such as the anti-aliasing level."
				"\n\n"
				"See the :py:class:`OpenGLRenderer`, :py:class:`TachyonRenderer` and :py:class:`POVRayRenderer` classes "
				"for the list of parameters specific to each rendering backend.")
		.def_property("range", &RenderSettings::renderingRangeType, &RenderSettings::setRenderingRangeType,
				"Selects the animation frames to be rendered."
				"\n\n"
				"Possible values:\n"
				"  * ``RenderSettings.Range.CURRENT_FRAME`` (default): Renders a single image at the current animation time.\n"
				"  * ``RenderSettings.Range.ANIMATION``: Renders a movie of the entire animation sequence.\n"
				"  * ``RenderSettings.Range.CUSTOM_INTERVAL``: Renders a movie of the animation interval given by the :py:attr:`.custom_range` attribute.\n")
		// Required by RenderSettings.filename implementation:
		.def_property("outputImageWidth", &RenderSettings::outputImageWidth, &RenderSettings::setOutputImageWidth)
		.def_property("outputImageHeight", &RenderSettings::outputImageHeight, &RenderSettings::setOutputImageHeight)
		.def_property("background_color", &RenderSettings::backgroundColor, &RenderSettings::setBackgroundColor,
				"Controls the background color of the rendered image."
				"\n\n"
				":Default: ``(1,1,1)`` -- white")
		.def_property("generate_alpha", &RenderSettings::generateAlphaChannel, &RenderSettings::setGenerateAlphaChannel,
				"When saving the image to a file format that supports transparency information (e.g. PNG), this option will make "
				"those parts of the output image transparent which are not covered by an object."
				"\n\n"
				":Default: ``False``")
		// Required by RenderSettings.filename implementation:
		.def_property("save_to_file", &RenderSettings::saveToFile, &RenderSettings::setSaveToFile)
		.def_property("output_filename", &RenderSettings::imageFilename, &RenderSettings::setImageFilename)
		.def_property("skip_existing_images", &RenderSettings::skipExistingImages, &RenderSettings::setSkipExistingImages,
				"Controls whether animation frames for which the output image file already exists will be skipped "
				"when rendering an animation sequence. This flag is ignored when directly rendering to a movie file and not an image file sequence. "
				"Use this flag when the image sequence has already been partially rendered and you want to render just the missing frames. "
				"\n\n"
				":Default: ``False``")
		.def_property("customRangeStart", &RenderSettings::customRangeStart, &RenderSettings::setCustomRangeStart)
		.def_property("customRangeEnd", &RenderSettings::customRangeEnd, &RenderSettings::setCustomRangeEnd)
		.def_property("everyNthFrame", &RenderSettings::everyNthFrame, &RenderSettings::setEveryNthFrame)
		.def_property("fileNumberBase", &RenderSettings::fileNumberBase, &RenderSettings::setFileNumberBase)
	;

	py::enum_<RenderSettings::RenderingRangeType>(RenderSettings_py, "Range")
		.value("CURRENT_FRAME", RenderSettings::CURRENT_FRAME)
		.value("ANIMATION", RenderSettings::ANIMATION_INTERVAL)
		.value("CUSTOM_INTERVAL", RenderSettings::CUSTOM_INTERVAL)
	;

	ovito_abstract_class<SceneRenderer, RefTarget>(m)
		.def_property_readonly("isInteractive", &SceneRenderer::isInteractive)
	;

	ovito_abstract_class<NonInteractiveSceneRenderer, SceneRenderer>{m}
	;

	ovito_class<StandardSceneRenderer, SceneRenderer>(m,
			"The standard OpenGL-based renderer."
			"\n\n"
			"This is the default built-in rendering engine that is also used by OVITO to render the contents of the interactive viewports. "
			"Since it accelerates the generation of images by using the computer's graphics hardware, it is very fast.",
			"OpenGLRenderer")
		.def_property("antialiasing_level", &StandardSceneRenderer::antialiasingLevel, &StandardSceneRenderer::setAntialiasingLevel,
				"A positive integer controlling the level of supersampling. If 1, no supersampling is performed. For larger values, "
				"the image in rendered at a higher resolution and then scaled back to the output size to reduce aliasing artifacts."
				"\n\n"
				":Default: 3")
	;

	ovito_abstract_class<DisplayObject, RefTarget>(m,
			"Abstract base class for display objects that render and control the visual appearance of data objects. "
			"A :py:class:`~ovito.data.DataObject` may be associated with a corresponding :py:class:`!Display` object (see " ":py:attr:`DataObject.display <ovito.data.DataObject.display>` property), making it a *visual* data object that appears in the viewports and in rendered images. "
			"\n\n" 
			"See the :py:mod:`ovito.vis` module for the list of display object classes available in OVITO. ",
			// Python class name:
			"Display")
		.def_property("enabled", &DisplayObject::isEnabled, &DisplayObject::setEnabled,
				"Boolean flag controlling the visibility of the data. If set to ``False``, the "
				"data will not be visible in the viewports or in rendered images."
				"\n\n"
				":Default: ``True``\n")
	;

	py::enum_<ParticlePrimitive::ShadingMode>(m, "ParticleShadingMode")
		.value("Normal", ParticlePrimitive::NormalShading)
		.value("Flat", ParticlePrimitive::FlatShading)
	;

	py::enum_<ParticlePrimitive::RenderingQuality>(m, "ParticleRenderingQuality")
		.value("LowQuality", ParticlePrimitive::LowQuality)
		.value("MediumQuality", ParticlePrimitive::MediumQuality)
		.value("HighQuality", ParticlePrimitive::HighQuality)
		.value("AutoQuality", ParticlePrimitive::AutoQuality)
	;

	py::enum_<ParticlePrimitive::ParticleShape>(m, "ParticleShape")
		.value("Round", ParticlePrimitive::SphericalShape)
		.value("Square", ParticlePrimitive::SquareShape)
	;

	py::enum_<ArrowPrimitive::ShadingMode>(m, "ArrowShadingMode")
		.value("Normal", ArrowPrimitive::NormalShading)
		.value("Flat", ArrowPrimitive::FlatShading)
	;

	py::enum_<ArrowPrimitive::RenderingQuality>(m, "ArrowRenderingQuality")
		.value("LowQuality", ArrowPrimitive::LowQuality)
		.value("MediumQuality", ArrowPrimitive::MediumQuality)
		.value("HighQuality", ArrowPrimitive::HighQuality)
	;

	py::enum_<ArrowPrimitive::Shape>(m, "ArrowShape")
		.value("CylinderShape", ArrowPrimitive::CylinderShape)
		.value("ArrowShape", ArrowPrimitive::ArrowShape)
	;
}

};
