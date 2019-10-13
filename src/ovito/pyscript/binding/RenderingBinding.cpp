////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2014 Alexander Stukowski
//
//  This file is part of OVITO (Open Visualization Tool).
//
//  OVITO is free software; you can redistribute it and/or modify it either under the
//  terms of the GNU General Public License version 3 as published by the Free Software
//  Foundation (the "GPL") or, at your option, under the terms of the MIT License.
//  If you do not alter this notice, a recipient may use your version of this
//  file under either the GPL or the MIT License.
//
//  You should have received a copy of the GPL along with this program in a
//  file LICENSE.GPL.txt.  You should have received a copy of the MIT License along
//  with this program in a file LICENSE.MIT.txt
//
//  This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND,
//  either express or implied. See the GPL or the MIT License for the specific language
//  governing rights and limitations.
//
////////////////////////////////////////////////////////////////////////////////////////

#include <ovito/pyscript/PyScript.h>
#include <ovito/core/rendering/RenderSettings.h>
#include <ovito/core/rendering/SceneRenderer.h>
#include <ovito/core/rendering/noninteractive/NonInteractiveSceneRenderer.h>
#include <ovito/core/rendering/ParticlePrimitive.h>
#include <ovito/core/rendering/ArrowPrimitive.h>
#include <ovito/core/rendering/FrameBuffer.h>
#include <ovito/core/dataset/data/DataVis.h>
#include <ovito/core/dataset/data/TransformingDataVis.h>
#include <ovito/opengl/StandardSceneRenderer.h>
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

	py::object RenderSettings_py = ovito_class<RenderSettings, RefTarget>(m /*,
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
			"  * :py:class:`OSPRayRenderer` -- Another software-based, high-quality raytracing renderer.\n"
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
			"To render an animation, the rendering :py:attr:`.range` must be set to ``RenderSettings.Range.Animation``. "
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
			*/)
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
				"  * ``RenderSettings.Range.CurrentFrame`` (default): Renders a single image at the current animation time.\n"
				"  * ``RenderSettings.Range.Animation``: Renders a movie of the entire animation sequence.\n"
				"  * ``RenderSettings.Range.CustomInterval``: Renders a movie of the animation interval given by the :py:attr:`.custom_range` attribute.\n")
		// Required by RenderSettings.filename implementation:
		.def_property("output_image_width", &RenderSettings::outputImageWidth, &RenderSettings::setOutputImageWidth)
		.def_property("output_image_height", &RenderSettings::outputImageHeight, &RenderSettings::setOutputImageHeight)
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
		.def_property("custom_range_start", &RenderSettings::customRangeStart, &RenderSettings::setCustomRangeStart)
		.def_property("custom_range_end", &RenderSettings::customRangeEnd, &RenderSettings::setCustomRangeEnd)
		.def_property("custom_frame", &RenderSettings::customFrame, &RenderSettings::setCustomFrame)
		.def_property("every_nth_frame", &RenderSettings::everyNthFrame, &RenderSettings::setEveryNthFrame)
		.def_property("file_number_base", &RenderSettings::fileNumberBase, &RenderSettings::setFileNumberBase)
		.def_property("frames_per_second", &RenderSettings::framesPerSecond, &RenderSettings::setFramesPerSecond)
	;

	ovito_enum<RenderSettings::RenderingRangeType>(RenderSettings_py, "Range")
		.value("CurrentFrame", RenderSettings::CURRENT_FRAME)
		.value("Animation", RenderSettings::ANIMATION_INTERVAL)
		.value("CustomInterval", RenderSettings::CUSTOM_INTERVAL)
		.value("CustomFrame", RenderSettings::CUSTOM_FRAME)
		// For backward compatibility with OVITO 2.9.0:
		.value("CURRENT_FRAME", RenderSettings::CURRENT_FRAME)
		.value("ANIMATION", RenderSettings::ANIMATION_INTERVAL)
		.value("CUSTOM_INTERVAL", RenderSettings::CUSTOM_INTERVAL)
	;

	ovito_abstract_class<SceneRenderer, RefTarget>{m}
	;

	ovito_abstract_class<NonInteractiveSceneRenderer, SceneRenderer>{m}
	;

	ovito_class<StandardSceneRenderer, SceneRenderer>(m,
			"The standard OpenGL-based renderer."
			"\n\n"
			"This is the default built-in rendering engine that is also used by OVITO to render the contents of the interactive viewports. "
			"Since it accelerates the generation of images by using the computer's graphics hardware, it is very fast. "
			"See the corresponding :ovitoman:`user manual page <../../rendering.opengl_renderer>` for more information on this rendering engine. "
			"\n\n"
			"Note that this renderer requires OpenGL graphics support, and Python scripts may be running in environments where it is not available. "
			"A typical example for such situations are remote SSH connections, which can prevent OVITO from accessing the X window and OpenGL systems. "
			"In this case, the OpenGL renderer will refuse to work and you have to use one of the software-based rendering engines instead. "
			"See the :py:meth:`Viewport.render_image` method. "
			,
			// Python class name:
			"OpenGLRenderer")
		.def_property("antialiasing_level", &StandardSceneRenderer::antialiasingLevel, &StandardSceneRenderer::setAntialiasingLevel,
				"A positive integer controlling the level of supersampling. If 1, no supersampling is performed. For larger values, "
				"the image in rendered at a higher resolution and then scaled back to the output size to reduce aliasing artifacts."
				"\n\n"
				":Default: 3")
	;

	ovito_abstract_class<DataVis, RefTarget>(m,
			"Abstract base class for visualization elements that are reponsible for the visual appearance of data objects in the visualization. "
			"Some :py:class:`DataObjects <ovito.data.DataObject>` are associated with a corresponding :py:class:`!DataVis` element "
			"(see :py:attr:`DataObject.vis <ovito.data.DataObject.vis>` property), making them *visual* data objects that appear "
			"in the viewports and in rendered images. "
			"\n\n"
			"See the :py:mod:`ovito.vis` module for the list of visual element types available in OVITO. ")
		.def_property("enabled", &DataVis::isEnabled, &DataVis::setEnabled,
				"Boolean flag controlling the visibility of the data. If set to ``False``, the "
				"data will not be visible in the viewports or in rendered images."
				"\n\n"
				":Default: ``True``\n")
		.def_property("title", &DataVis::title, &DataVis::setTitle,
				"A custom title string assigned to the visual element, which will show in the pipeline editor of OVITO. "
				"\n\n"
				":Default: ``''``\n")
	;

	ovito_abstract_class<TransformingDataVis, DataVis>{m};

	ovito_enum<ParticlePrimitive::ShadingMode>(m, "ParticleShadingMode")
		.value("Normal", ParticlePrimitive::NormalShading)
		.value("Flat", ParticlePrimitive::FlatShading)
	;

	ovito_enum<ParticlePrimitive::RenderingQuality>(m, "ParticleRenderingQuality")
		.value("LowQuality", ParticlePrimitive::LowQuality)
		.value("MediumQuality", ParticlePrimitive::MediumQuality)
		.value("HighQuality", ParticlePrimitive::HighQuality)
		.value("AutoQuality", ParticlePrimitive::AutoQuality)
	;

	ovito_enum<ParticlePrimitive::ParticleShape>(m, "ParticleShape")
		.value("Round", ParticlePrimitive::SphericalShape)
		.value("Square", ParticlePrimitive::SquareCubicShape)
	;

	ovito_enum<ArrowPrimitive::ShadingMode>(m, "ArrowShadingMode")
		.value("Normal", ArrowPrimitive::NormalShading)
		.value("Flat", ArrowPrimitive::FlatShading)
	;

	ovito_enum<ArrowPrimitive::RenderingQuality>(m, "ArrowRenderingQuality")
		.value("LowQuality", ArrowPrimitive::LowQuality)
		.value("MediumQuality", ArrowPrimitive::MediumQuality)
		.value("HighQuality", ArrowPrimitive::HighQuality)
	;

	ovito_enum<ArrowPrimitive::Shape>(m, "ArrowShape")
		.value("CylinderShape", ArrowPrimitive::CylinderShape)
		.value("ArrowShape", ArrowPrimitive::ArrowShape)
	;
}

}
