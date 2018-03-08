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

#include <plugins/pyscript/PyScript.h>
#include <core/viewport/ViewportConfiguration.h>
#include <core/viewport/Viewport.h>
#include <core/viewport/overlays/ViewportOverlay.h>
#include <core/viewport/overlays/CoordinateTripodOverlay.h>
#include <core/viewport/overlays/TextLabelOverlay.h>
#include <core/dataset/scene/SceneNode.h>
#include <plugins/pyscript/extensions/PythonViewportOverlay.h>
#include "PythonBinding.h"

namespace PyScript {

using namespace Ovito;

void defineViewportSubmodule(py::module m)
{
	auto Viewport_py = ovito_class<Viewport, RefTarget>(m,
			"A viewport represents a virtual \"window\" to the three-dimensional scene, "
			"showing the objects in the scene from a certain view point, which is determined by the viewport's camera. "
			"\n\n"
			"The virtual camera's position and orientation are controlled by the :py:attr:`.camera_pos` and :py:attr:`.camera_dir` fields. "
			"Additionally, the :py:attr:`.type` field allows you to switch between perspective and parallel projection modes "
			"or reset the camera to one of the standard axis-aligned orientations that are also found in the graphical version of OVITO. "
			"The :py:meth:`.zoom_all` method repositions the camera automatically such that the entire scene becomes fully visible within the view. "
			"\n\n"
			"After the viewport camera has been set up, you can render an image or movie using the "
			":py:meth:`.render_image` and :py:meth:`.render_anim` methods. For example: "
			"\n\n"
			".. literalinclude:: ../example_snippets/viewport.py"
			"\n\n"
			"Furthermore, so-called *overlays* may be assigned to a viewport. "
			"Overlays are function objects that paint additional two-dimensional content on top of the rendered scene, e.g. "
			"a coordinate axis tripod or a color legend. See the documentation of the :py:attr:`.overlays` property for more information. ")
		.def_property("type", &Viewport::viewType, [](Viewport& vp, Viewport::ViewType vt) { vp.setViewType(vt); },
				"Specifies the projection type of the viewport. The following standard modes are available:"
				"\n\n"
				"  * ``Viewport.Type.Perspective``\n"
				"  * ``Viewport.Type.Ortho``\n"
				"  * ``Viewport.Type.Top``\n"
				"  * ``Viewport.Type.Bottom``\n"
				"  * ``Viewport.Type.Front``\n"
				"  * ``Viewport.Type.Back``\n"
				"  * ``Viewport.Type.Left``\n"
				"  * ``Viewport.Type.Right``\n"
				"\n"
				"The first two types (``Perspective`` and ``Ortho``) allow you to set up custom views with arbitrary camera orientations.\n")
		.def_property("fov", &Viewport::fieldOfView, &Viewport::setFieldOfView,
				"The field of view of the viewport's camera. "
				"For perspective projections this is the camera's angle in the vertical direction (in radians). For orthogonal projections this is the visible range in the vertical direction (in world units).")
		.def_property("cameraTransformation", &Viewport::cameraTransformation, &Viewport::setCameraTransformation)
		.def_property("camera_dir", &Viewport::cameraDirection, &Viewport::setCameraDirection,
				"The viewing direction vector of the viewport's camera.")
		.def_property("camera_pos", &Viewport::cameraPosition, &Viewport::setCameraPosition,
				"The position of the viewport's camera in the three-dimensional scene.")
		.def_property_readonly("viewMatrix", [](Viewport& vp) -> const AffineTransformation& { return vp.projectionParams().viewMatrix; })
		.def_property_readonly("inverseViewMatrix", [](Viewport& vp) -> const AffineTransformation& { return vp.projectionParams().inverseViewMatrix; })
		.def_property_readonly("projectionMatrix", [](Viewport& vp) -> const Matrix4& { return vp.projectionParams().projectionMatrix; })
		.def_property_readonly("inverseProjectionMatrix", [](Viewport& vp) -> const Matrix4& { return vp.projectionParams().inverseProjectionMatrix; })
		.def("zoom_all", &Viewport::zoomToSceneExtents,
				"Repositions the viewport camera such that all objects in the scene become completely visible. "
				"The camera direction is maintained by the method.")
	;
	expose_mutable_subobject_list(Viewport_py,
								  std::mem_fn(&Viewport::overlays), 
								  std::mem_fn(&Viewport::insertOverlay), 
								  std::mem_fn(&Viewport::removeOverlay), "overlays", "ViewportOverlayList",
								"A list of viewport overlay objects that are attached to this viewport. "
								"Overlays render graphical content on top of the three-dimensional scene. "
								"See the following classes for more information:"
								"\n\n"
								"   * :py:class:`TextLabelOverlay`\n"
								"   * :py:class:`ColorLegendOverlay`\n"
								"   * :py:class:`CoordinateTripodOverlay`\n"
								"   * :py:class:`PythonViewportOverlay`\n"
								"\n\n"
								"To attach a new overlay to the viewport, use the ``append()`` method:"
								"\n\n"
								".. literalinclude:: ../example_snippets/viewport_add_overlay.py"
								"\n\n");		

	py::enum_<Viewport::ViewType>(Viewport_py, "Type")
		.value("Top", Viewport::VIEW_TOP)
		.value("Bottom", Viewport::VIEW_BOTTOM)
		.value("Front", Viewport::VIEW_FRONT)
		.value("Back", Viewport::VIEW_BACK)
		.value("Left", Viewport::VIEW_LEFT)
		.value("Right", Viewport::VIEW_RIGHT)
		.value("Ortho", Viewport::VIEW_ORTHO)
		.value("Perspective", Viewport::VIEW_PERSPECTIVE)
		.value("SceneNode", Viewport::VIEW_SCENENODE)
		// For backward compatibility with OVITO 2.9.0:
		.value("NONE", Viewport::VIEW_NONE)
		.value("TOP", Viewport::VIEW_TOP)
		.value("BOTTOM", Viewport::VIEW_BOTTOM)
		.value("FRONT", Viewport::VIEW_FRONT)
		.value("BACK", Viewport::VIEW_BACK)
		.value("LEFT", Viewport::VIEW_LEFT)
		.value("RIGHT", Viewport::VIEW_RIGHT)
		.value("ORTHO", Viewport::VIEW_ORTHO)
		.value("PERSPECTIVE", Viewport::VIEW_PERSPECTIVE)
	;

	py::class_<ViewProjectionParameters>(m, "ViewProjectionParameters")
		.def_readwrite("aspectRatio", &ViewProjectionParameters::aspectRatio)
		.def_readwrite("isPerspective", &ViewProjectionParameters::isPerspective)
		.def_readwrite("znear", &ViewProjectionParameters::znear)
		.def_readwrite("zfar", &ViewProjectionParameters::zfar)
		.def_readwrite("fieldOfView", &ViewProjectionParameters::fieldOfView)
		.def_readwrite("viewMatrix", &ViewProjectionParameters::viewMatrix)
		.def_readwrite("inverseViewMatrix", &ViewProjectionParameters::inverseViewMatrix)
		.def_readwrite("projectionMatrix", &ViewProjectionParameters::projectionMatrix)
		.def_readwrite("inverseProjectionMatrix", &ViewProjectionParameters::inverseProjectionMatrix)
	;

	auto ViewportConfiguration_py = ovito_class<ViewportConfiguration, RefTarget>(m)
		.def_property("active_vp", &ViewportConfiguration::activeViewport, &ViewportConfiguration::setActiveViewport,
				"The viewport that is currently active. It is marked with a colored border in OVITO's main window.")
		.def_property("maximized_vp", &ViewportConfiguration::maximizedViewport, &ViewportConfiguration::setMaximizedViewport,
				"The viewport that is currently maximized; or ``None`` if no viewport is maximized.\n"
				"Assign a viewport to this attribute to maximize it, e.g.::"
				"\n\n"
				"    dataset.viewports.maximized_vp = dataset.viewports.active_vp\n")
	;
	expose_subobject_list(ViewportConfiguration_py, std::mem_fn(&ViewportConfiguration::viewports), "viewports", "ViewportList");

	ovito_abstract_class<ViewportOverlay, RefTarget>{m};

	ovito_class<CoordinateTripodOverlay, ViewportOverlay>(m,
			"Displays a coordinate tripod in the rendered image of a viewport. "
			"You can attach an instance of this class to a viewport by adding it to the viewport's "
			":py:attr:`~ovito.vis.Viewport.overlays` collection:"
			"\n\n"
			".. literalinclude:: ../example_snippets/coordinate_tripod_overlay.py"
			"\n\n")
		.def_property("alignment", &CoordinateTripodOverlay::alignment, &CoordinateTripodOverlay::setAlignment,
				"Selects the corner of the viewport where the tripod is displayed. This must be a valid `Qt.Alignment value <http://doc.qt.io/qt-5/qt.html#AlignmentFlag-enum>`_ value as shown in the example above."
				"\n\n"
				":Default: ``PyQt5.QtCore.Qt.AlignLeft ^ PyQt5.QtCore.Qt.AlignBottom``")
		.def_property("size", &CoordinateTripodOverlay::tripodSize, &CoordinateTripodOverlay::setTripodSize,
				"Scaling factor controlling the overall size of the tripod. The size is specified as a fraction of the output image height."
				"\n\n"
				":Default: 0.075\n")
		.def_property("line_width", &CoordinateTripodOverlay::lineWidth, &CoordinateTripodOverlay::setLineWidth,
				"Controls the width of axis arrows. The line width is specified relative to the tripod size."
				"\n\n"
				":Default: 0.06\n")
		.def_property("offset_x", &CoordinateTripodOverlay::offsetX, &CoordinateTripodOverlay::setOffsetX,
				"This parameter allows to displace the tripod horizontally. The offset is specified as a fraction of the output image width."
				"\n\n"
				":Default: 0.0\n")
		.def_property("offset_y", &CoordinateTripodOverlay::offsetY, &CoordinateTripodOverlay::setOffsetY,
				"This parameter allows to displace the tripod vertically. The offset is specified as a fraction of the output image height."
				"\n\n"
				":Default: 0.0\n")
		.def_property("font_size", &CoordinateTripodOverlay::fontSize, &CoordinateTripodOverlay::setFontSize,
				"The font size for rendering the text labels of the tripod. The font size is specified in terms of the tripod size."
				"\n\n"
				":Default: 0.4\n")
		.def_property("axis1_enabled", &CoordinateTripodOverlay::axis1Enabled, &CoordinateTripodOverlay::setAxis1Enabled,
				"Enables the display of the first axis."
				"\n\n"
				":Default: True\n")
		.def_property("axis2_enabled", &CoordinateTripodOverlay::axis2Enabled, &CoordinateTripodOverlay::setAxis2Enabled,
				"Enables the display of the second axis."
				"\n\n"
				":Default: True\n")
		.def_property("axis3_enabled", &CoordinateTripodOverlay::axis3Enabled, &CoordinateTripodOverlay::setAxis3Enabled,
				"Enables the display of the third axis."
				"\n\n"
				":Default: True\n")
		.def_property("axis4_enabled", &CoordinateTripodOverlay::axis4Enabled, &CoordinateTripodOverlay::setAxis4Enabled,
				"Enables the display of the fourth axis."
				"\n\n"
				":Default: False\n")
		.def_property("axis1_label", &CoordinateTripodOverlay::axis1Label, &CoordinateTripodOverlay::setAxis1Label,
				"Text label for the first axis."
				"\n\n"
				":Default: \"x\"\n")
		.def_property("axis2_label", &CoordinateTripodOverlay::axis2Label, &CoordinateTripodOverlay::setAxis2Label,
				"Text label for the second axis."
				"\n\n"
				":Default: \"y\"\n")
		.def_property("axis3_label", &CoordinateTripodOverlay::axis3Label, &CoordinateTripodOverlay::setAxis3Label,
				"Text label for the third axis."
				"\n\n"
				":Default: \"z\"\n")
		.def_property("axis4_label", &CoordinateTripodOverlay::axis4Label, &CoordinateTripodOverlay::setAxis4Label,
				"Label for the fourth axis."
				"\n\n"
				":Default: \"w\"\n")
		.def_property("axis1_dir", &CoordinateTripodOverlay::axis1Dir, &CoordinateTripodOverlay::setAxis1Dir,
				"Vector specifying direction and length of first axis, expressed in the global Cartesian coordinate system."
				"\n\n"
				":Default: ``(1,0,0)``\n")
		.def_property("axis2_dir", &CoordinateTripodOverlay::axis2Dir, &CoordinateTripodOverlay::setAxis2Dir,
				"Vector specifying direction and length of second axis, expressed in the global Cartesian coordinate system."
				"\n\n"
				":Default: ``(0,1,0)``\n")
		.def_property("axis3_dir", &CoordinateTripodOverlay::axis3Dir, &CoordinateTripodOverlay::setAxis3Dir,
				"Vector specifying direction and length of third axis, expressed in the global Cartesian coordinate system."
				"\n\n"
				":Default: ``(0,0,1)``\n")
		.def_property("axis4_dir", &CoordinateTripodOverlay::axis4Dir, &CoordinateTripodOverlay::setAxis4Dir,
				"Vector specifying direction and length of fourth axis, expressed in the global Cartesian coordinate system."
				"\n\n"
				":Default: ``(0.7071, 0.7071, 0.0)``\n")
		.def_property("axis1_color", &CoordinateTripodOverlay::axis1Color, &CoordinateTripodOverlay::setAxis1Color,
				"RGB display color of the first axis."
				"\n\n"
				":Default: ``(1.0, 0.0, 0.0)``\n")
		.def_property("axis2_color", &CoordinateTripodOverlay::axis2Color, &CoordinateTripodOverlay::setAxis2Color,
				"RGB display color of the second axis."
				"\n\n"
				":Default: ``(0.0, 0.8, 0.0)``\n")
		.def_property("axis3_color", &CoordinateTripodOverlay::axis3Color, &CoordinateTripodOverlay::setAxis3Color,
				"RGB display color of the third axis."
				"\n\n"
				":Default: ``(0.2, 0.2, 1.0)``\n")
		.def_property("axis4_color", &CoordinateTripodOverlay::axis4Color, &CoordinateTripodOverlay::setAxis4Color,
				"RGB display color of the fourth axis."
				"\n\n"
				":Default: ``(1.0, 0.0, 1.0)``\n")

	;

	ovito_class<TextLabelOverlay, ViewportOverlay>(m,
			"Displays a text label in a viewport and in rendered images. "
			"You can attach an instance of this class to a viewport by adding it to the viewport's "
			":py:attr:`~ovito.vis.Viewport.overlays` collection:"
			"\n\n"
			".. literalinclude:: ../example_snippets/text_label_overlay.py"
			"\n\n"
			"Text labels can display dynamically computed values. See the :py:attr:`.text` property for an example.")
		.def_property("alignment", &TextLabelOverlay::alignment, &TextLabelOverlay::setAlignment,
				"Selects the corner of the viewport where the text is displayed (anchor position). This must be a valid `Qt.Alignment value <http://doc.qt.io/qt-5/qt.html#AlignmentFlag-enum>`_ as shown in the example above. "
				"\n\n"
				":Default: ``PyQt5.QtCore.Qt.AlignLeft ^ PyQt5.QtCore.Qt.AlignTop``")
		.def_property("offset_x", &TextLabelOverlay::offsetX, &TextLabelOverlay::setOffsetX,
				"This parameter allows to displace the label horizontally from its anchor position. The offset is specified as a fraction of the output image width."
				"\n\n"
				":Default: 0.0\n")
		.def_property("offset_y", &TextLabelOverlay::offsetY, &TextLabelOverlay::setOffsetY,
				"This parameter allows to displace the label vertically from its anchor position. The offset is specified as a fraction of the output image height."
				"\n\n"
				":Default: 0.0\n")
		.def_property("font_size", &TextLabelOverlay::fontSize, &TextLabelOverlay::setFontSize,
				"The font size, which is specified as a fraction of the output image height."
				"\n\n"
				":Default: 0.02\n")
		.def_property("text", &TextLabelOverlay::labelText, &TextLabelOverlay::setLabelText,
				"The text string to be rendered."
				"\n\n"
				"The string can contain placeholder references to dynamically computed attributes of the form ``[attribute]``, which will be replaced "
				"by their actual value before rendering the text label. "
				"Attributes are taken from the pipeline output of the :py:class:`~ovito.pipeline.Pipeline` assigned to the overlay's :py:attr:`.source_pipeline` property. "
				"\n\n"
				"The following example demonstrates how to insert a text label that displays the number of currently selected particles: "
				"\n\n"
				".. literalinclude:: ../example_snippets/text_label_overlay_with_attributes.py"
				"\n\n"
				":Default: \"Text label\"")
		.def_property("source_pipeline", &TextLabelOverlay::sourceNode, &TextLabelOverlay::setSourceNode,
				"The :py:class:`~ovito.pipeline.Pipeline` that is queried to obtain the attribute values referenced "
				"in the text string. See the :py:attr:`.text` property for more information. ")
		.def_property("text_color", &TextLabelOverlay::textColor, &TextLabelOverlay::setTextColor,
				"The text rendering color."
				"\n\n"
				":Default: ``(0.0,0.0,0.5)``\n")
		.def_property("outline_color", &TextLabelOverlay::outlineColor, &TextLabelOverlay::setOutlineColor,
				"The text outline color. This is used only if :py:attr:`.outline_enabled` is set."
				"\n\n"
				":Default: ``(1.0,1.0,1.0)``\n")
		.def_property("outline_enabled", &TextLabelOverlay::outlineEnabled, &TextLabelOverlay::setOutlineEnabled,
				"Enables the painting of a font outline to make the text easier to read."
				"\n\n"
				":Default: ``False``\n")
	;

	ovito_class<PythonViewportOverlay, ViewportOverlay>(m,
			"This overlay type can be attached to a viewport to run a Python script every time an "
			"image of the viewport is rendered. The Python script can issue arbitrary drawing commands to "
			"paint on top of the three-dimensional scene. "
			"\n\n"
			"Note that an alternative to using the :py:class:`!PythonViewportOverlay` class is to directly manipulate the "
			"image returned by the :py:meth:`Viewport.render_image` method before saving the image to disk. "
			"\n\n"
			"You can attach a Python overlay to a viewport by adding an instance of this class to the viewport's "
			":py:attr:`~ovito.vis.Viewport.overlays` collection:"
			"\n\n"
			".. literalinclude:: ../example_snippets/python_viewport_overlay.py")
		.def_property("function", &PythonViewportOverlay::scriptFunction, &PythonViewportOverlay::setScriptFunction,
				"The Python function to be called every time the viewport is repainted or when an output image is being rendered."
				"\n\n"
				"The function must have a signature as shown in the example above. The *painter* parameter "
				"passed to the user-defined function contains a `QPainter <http://pyqt.sourceforge.net/Docs/PyQt5/api/qpainter.html>`_ object, which provides "
				"painting methods to draw arbitrary 2D graphics on top of the image rendered by OVITO. "
				"\n\n"
				"Additional keyword arguments are passed to the function in the *args* dictionary. "
				"The following keys are defined: \n\n"
				"   * ``viewport``: The :py:class:`~ovito.vis.Viewport` being rendered.\n"
				"   * ``is_perspective``: Flag indicating whether projection is perspective or parallel.\n"
				"   * ``fov``: The field of view.\n"
				"   * ``view_tm``: The camera transformation matrix.\n"
				"   * ``proj_tm``: The projection matrix.\n"
				"\n\n"
				"Implementation note: Exceptions raised by the custom rendering function are not propagated to the calling context. "
				"\n\n"
				":Default: ``None``\n")
	;
}

};
