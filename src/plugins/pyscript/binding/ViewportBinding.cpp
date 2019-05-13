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
#include <core/dataset/DataSet.h>
#include <core/dataset/scene/SceneNode.h>
#include <core/rendering/RenderSettings.h>
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
			"See also the documentation of the :ovitoman:`Adjust View <../../viewports.adjust_view_dialog>` dialog of OVITO to learn more about "
			"the role of these settings. "
			"\n\n"
			"After the viewport's virtual camera has been set up, you can render an image or movie using the "
			":py:meth:`.render_image` and :py:meth:`.render_anim` methods. For example: "
			"\n\n"
			".. literalinclude:: ../example_snippets/viewport.py"
			"\n\n"
			"Furthermore, so-called *overlays* may be assigned to a viewport. "
			"Overlays are function objects that draw additional two-dimensional content on top of the rendered scene, e.g. "
			"a coordinate axis tripod or a color legend. See the the :py:attr:`.overlays` property for more information. ")
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
		.def_property("camera_tm", &Viewport::cameraTransformation, &Viewport::setCameraTransformation)
		.def_property("camera_dir", &Viewport::cameraDirection, &Viewport::setCameraDirection,
				"The viewing direction vector of the viewport's camera.")
		.def_property("camera_pos", &Viewport::cameraPosition, &Viewport::setCameraPosition,
				"The position of the viewport's camera in the three-dimensional scene.")
		.def_property("camera_up", &Viewport::cameraUpDirection, &Viewport::setCameraUpDirection,
				"Direction vector specifying which coordinate axis will point upward in rendered images. "
				"Set this parameter to a non-zero vector in order to rotate the camera around the viewing direction and "
				"align the vertical direction in rendered images with a different simulation coordinate axis. "
				"If set to ``(0,0,0)``, then the upward axis is determined by the current user settings set in OVITO's application settings dialog (z-axis by default). "
				"\n\n"
				":Default: (0,0,0)\n")
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
								"A list of :py:class:`ViewportOverlay` objects that are attached to this viewport. "
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

	ovito_abstract_class<ViewportOverlay, RefTarget>(m,
			"Abstract base class for viewport overlays, which render two-dimensional graphics on top of (or behind) the three-dimensional scene. "
			"Examples are :py:class:`CoordinateTripodOverlay`, :py:class:`TextLabelOverlay` and :py:class:`ColorLegendOverlay`. ")
		.def_property("enabled", &ViewportOverlay::isEnabled, &ViewportOverlay::setEnabled,
				"Controls whether the overlay gets rendered. An overlay "
				"can be hidden by setting its :py:attr:`!enabled` property to ``False``. "
				"\n\n"
				":Default: ``True``\n")
		.def_property("behind_scene", &ViewportOverlay::renderBehindScene, &ViewportOverlay::setRenderBehindScene,
				"This option allows to put the overlay behind the three-dimensional scene, i.e. it becomes an \"underlay\" instead of an \"overlay\". "
				"If set to ``True``, objects of the three-dimensional scene will occclude the graphics of the overlay. "
				"\n\n"
				":Default: ``False``")
	;

	ovito_class<CoordinateTripodOverlay, ViewportOverlay>(m,
			":Base class: :py:class:`ovito.vis.ViewportOverlay`\n\n"
			"Displays a coordinate tripod in the rendered image of a viewport. "
			"You can attach an instance of this class to a viewport by adding it to the viewport's "
			":py:attr:`~ovito.vis.Viewport.overlays` collection:"
			"\n\n"
			".. literalinclude:: ../example_snippets/coordinate_tripod_overlay.py"
			"\n\n")
		.def_property("alignment", &CoordinateTripodOverlay::alignment, &CoordinateTripodOverlay::setAlignment,
				"Selects the corner of the viewport where the tripod is displayed. This must be a valid `Qt.Alignment value <https://www.riverbankcomputing.com/static/Docs/PyQt5/api/qtcore/qt.html#AlignmentFlag>`__ value as shown in the example above."
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
			":Base class: :py:class:`ovito.vis.ViewportOverlay`\n\n"
			"Displays a text label in a viewport and in rendered images. "
			"You can attach an instance of this class to a viewport by adding it to the viewport's "
			":py:attr:`~ovito.vis.Viewport.overlays` collection:"
			"\n\n"
			".. literalinclude:: ../example_snippets/text_label_overlay.py"
			"\n\n"
			"Text labels can display dynamically computed values. See the :py:attr:`.text` property for an example.")
		.def_property("alignment", &TextLabelOverlay::alignment, &TextLabelOverlay::setAlignment,
				"Selects the corner of the viewport where the text is displayed (anchor position). This must be a valid `Qt.Alignment value <https://www.riverbankcomputing.com/static/Docs/PyQt5/api/qtcore/qt.html#AlignmentFlag>`__ as shown in the example above. "
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

	auto PythonViewportOverlay_py = ovito_class<PythonViewportOverlay, ViewportOverlay>(m,
			":Base class: :py:class:`ovito.vis.ViewportOverlay`\n\n"
			"This type of viewport overlay runs a custom Python script function every time an "
			"image of the viewport is rendered. The user-defined script function can paint arbitrary graphics on top of the "
			"three-dimensional scene. "
			"\n\n"
			"Note that instead of using a :py:class:`!PythonViewportOverlay` it is also possible to directly manipulate the "
			"image returned by the :py:meth:`Viewport.render_image` method before saving the image to disk. "
			"A :py:class:`!PythonViewportOverlay` is only necessary when rendering animations or if you want the overlay "
			"to be usable from in the graphical program version. "
			"\n\n"
			"You can attach the Python overlay to a viewport by adding it to the viewport's "
			":py:attr:`~ovito.vis.Viewport.overlays` collection:"
			"\n\n"
			".. literalinclude:: ../example_snippets/python_viewport_overlay.py"
			"\n\n"
			"The user-defined Python function must accept a single argument (named ``args`` in the example above). "
			"The system will pass in an instance of the :py:class:`.Arguments` class to the function, which contains "
			"various state information, including the current animation frame number and the viewport being rendered "
			"as well as a `QPainter <https://www.riverbankcomputing.com/static/Docs/PyQt5/api/qtgui/qpainter.html>`__ object, which the "
			"function should use to issue drawing calls. ")
		.def_property("function", &PythonViewportOverlay::scriptFunction, &PythonViewportOverlay::setScriptFunction,
				"A reference to the Python function to be called every time the viewport is repainted or when an output image is rendered."
				"\n\n"
				"The user-defined function must accept exactly one argument as shown in the example above. The system will "
				"pass an :py:class:`.Arguments` object to the function, providing various contextual information on the current frame being rendered. "
				"\n\n"
				"Implementation note: Exceptions raised within the custom rendering function are *not* propagated to the calling context. "
				"\n\n"
				":Default: ``None``\n")
	;

	py::class_<ViewportOverlayArguments>(PythonViewportOverlay_py, "Arguments",
		"This is the type of data structure passed by the system to the user-defined ``render()`` function of the viewport overlay. "
		"It holds various context information about the frame being rendered and provides utility methods for projecting points from 3d to 2d space. ")
		.def_property_readonly("viewport", &ViewportOverlayArguments::viewport,
			"The :py:class:`~ovito.vis.Viewport` being rendered.")
		.def_property_readonly("is_perspective", [](const ViewportOverlayArguments& args) { return args.projParams().isPerspective; },
			"Flag indicating whether the viewport uses a perspective projection or parallel projection.")
		.def_property_readonly("fov", [](const ViewportOverlayArguments& args) { return args.projParams().fieldOfView; },
			"The field of view of the viewport’s camera. For perspective projections, this is the frustum angle in the vertical direction "
			"(in radians). For orthogonal projections this is the visible range in the vertical direction (in world units). ")
		.def_property_readonly("view_tm", [](const ViewportOverlayArguments& args) { return args.projParams().viewMatrix; },
			"The affine camera transformation matrix. This 3x4 matrix transforms points/vectors from world space to camera space.")
		.def_property_readonly("proj_tm", [](const ViewportOverlayArguments& args) { return args.projParams().projectionMatrix; },
			"The projection matrix. This 4x4 matrix transforms points from camera space to screen space.")
		.def_property_readonly("frame", &ViewportOverlayArguments::frame,
			"The animation frame number being rendered (0-based).")
		.def_property_readonly("painter", &ViewportOverlayArguments::pypainter,
			"The `QPainter <https://www.riverbankcomputing.com/static/Docs/PyQt5/api/qtgui/qpainter.html>`__ object, which provides painting methods "
			"for drawing on top of the image canvas. ")
		.def_property_readonly("size", [](const ViewportOverlayArguments& args) {
				return py::make_tuple(args.renderSettings()->outputImageWidth(), args.renderSettings()->outputImageHeight());
			},
			"A tuple with the width and height of the image being rendered in pixels.")
		.def("project_point", [](ViewportOverlayArguments& args, const Point3& worldPos) -> py::object {
				auto screenPos = args.projectPoint(worldPos);
				if(!screenPos) return py::none();
				return py::make_tuple(screenPos->x(), screenPos->y());
			},
			"project_point(world_xyz)"
			"\n\n"
			"Projects a point, given in world-space coordinates, to screen space. This method can be used to determine "
			"where a 3d point would appear in the rendered image."
			"\n\n"
			"Note that the projected point may lay outside of the visible viewport region. Furthermore, for viewports with a "
			"perspective projection, the input point may lie behind the virtual camera. In this case no corresponding "
			"projected point in 2d screen space exists and the method returns ``None``. "
			"\n\n"
			":param world_xyz: The (x,y,z) coordinates of the input point\n"
			":return: A (x,y) pair of pixel coordinates; or ``None`` if *world_xyz* is behind the viewer.\n",
			py::arg("world_xyz"))
		.def("project_size", &ViewportOverlayArguments::projectSize,
			"project_size(world_xyz, r)"
			"\n\n"
			"Projects a size from 3d world space to 2d screen space. This method can be used to determine "
			"how large a 3d object, for example a sphere with the given radius *r*, would appear in the rendered image. "
			"\n\n"
			"Additionally to the size *r* to be projected, the method takes a coordinate triplet (x,y,z) "
			"as input. It specifies the location of the base point from where the distance is measured. "
			"\n\n"
			":param world_xyz: The (x,y,z) world-space coordinates of the base point\n"
			":param r: The world-space size or distance to be converted to screen-space\n"
			":return: The computed screen-space size measured in pixels.\n",
			py::arg("world_xyz"), py::arg("r"))
		.def_property_readonly("scene", [](ViewportOverlayArguments& args) {
				return ovito_class_initialization_helper::getCurrentDataset();
			}, py::return_value_policy::reference,
			"The current three-dimensional :py:class:`~ovito.Scene` being rendered, including all visible data pipelines. ")
	;
}

}
