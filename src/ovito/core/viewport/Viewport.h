////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2019 Alexander Stukowski
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

#pragma once


#include <ovito/core/Core.h>
#include <ovito/core/oo/RefTarget.h>
#include <ovito/core/dataset/animation/TimeInterval.h>
#include <ovito/core/dataset/scene/PipelineSceneNode.h>
#include <ovito/core/viewport/overlays/ViewportOverlay.h>
#include "ViewportSettings.h"
#include "ViewportWindowInterface.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(View)

/******************************************************************************
* This data structure describes a projection parameters used to render
* the 3D contents in a viewport.
******************************************************************************/
struct ViewProjectionParameters
{
	/// The aspect ratio (height/width) of the viewport.
	FloatType aspectRatio;

	/// Indicates whether this is a orthogonal or perspective projection.
	bool isPerspective;

	/// The distance of the front clipping plane in world units.
	FloatType znear;

	/// The distance of the back clipping plane in world units.
	FloatType zfar;

	/// For orthogonal projections this is the vertical field of view in world units.
	/// For perspective projections this is the vertical field of view angle in radians.
	FloatType fieldOfView;

	/// The world to view space transformation matrix.
	AffineTransformation viewMatrix;

	/// The view space to world space transformation matrix.
	AffineTransformation inverseViewMatrix;

	/// The view space to screen space projection matrix.
	Matrix4 projectionMatrix;

	/// The screen space to view space transformation matrix.
	Matrix4 inverseProjectionMatrix;

	/// The bounding box of the scene.
	Box3 boundingBox;

	/// Specifies the time interval during which the stored parameters stay constant.
	TimeInterval validityInterval;
};

/**
 * \brief A viewport window that displays the current scene.
 */
class OVITO_CORE_EXPORT Viewport : public RefTarget
{
	Q_OBJECT
	OVITO_CLASS(Viewport)

	Q_PROPERTY(QString title READ viewportTitle NOTIFY viewportChanged);
	Q_PROPERTY(bool gridVisible READ isGridVisible WRITE setGridVisible NOTIFY viewportChanged);
	Q_PROPERTY(bool previewMode READ renderPreviewMode WRITE setRenderPreviewMode NOTIFY viewportChanged);
	Q_PROPERTY(Ovito::Viewport::ViewType viewType READ viewType WRITE setViewType NOTIFY viewportChanged);

public:

	/// View types.
	enum ViewType {
		VIEW_NONE,
		VIEW_TOP,
		VIEW_BOTTOM,
		VIEW_FRONT,
		VIEW_BACK,
		VIEW_LEFT,
		VIEW_RIGHT,
		VIEW_ORTHO,
		VIEW_PERSPECTIVE,
		VIEW_SCENENODE,
	};
	Q_ENUMS(ViewType);

public:

	/// \brief Constructs a new viewport.
	Q_INVOKABLE Viewport(DataSet* dataset);

	/// \brief Destructor.
	virtual ~Viewport();

	/// \brief Computes the projection matrix and other parameters.
	/// \param time The animation time for which the view is requested.
	/// \param aspectRatio Specifies the desired aspect ratio (height/width) of the output image.
	/// \param sceneBoundingBox The bounding box of the scene in world coordinates. This is used to calculate the near and far z-clipping planes.
	/// \return The returned structure describes the projection used to render the contents of the viewport.
	ViewProjectionParameters computeProjectionParameters(TimePoint time, FloatType aspectRatio, const Box3& sceneBoundingBox = Box3());

    /// \brief Puts an update request event for this viewport on the event loop.
    ///
	/// Calling this method is going to redraw the viewport contents unless the viewport is hidden.
	/// This function does not cause an immediate repaint; instead it schedules an
	/// update request event, which is processed when execution returns to the main event loop.
	///
	/// To update all viewports at once you should use ViewportConfiguration::updateViewports().
	void updateViewport();

	/// \brief Immediately redraws the contents of this viewport.
	void redrawViewport();

	/// \brief If an update request is pending for this viewport, immediately processes it and redraw the viewport.
	void processUpdateRequest();

	/// \brief Returns whether the rendering of the viewport's contents is currently in progress.
	bool isRendering() const { return _isRendering; }

	/// \brief Changes the view type.
	/// \param type The new view type.
	/// \param keepViewParams When setting the view type to ViewType::VIEW_ORTHO or ViewType::VIEW_PERSPECTIVE,
	///        this controls whether the camera is reset to the default position/orientation.
	/// \note if \a type is set to ViewType::VIEW_SCENENODE then a view node should be set
	///       using setViewNode().
	void setViewType(ViewType type, bool keepCurrentView = false);

	/// \brief Returns true if the viewport is using a perspective project;
	///        returns false if it is using an orthogonal projection.
	bool isPerspectiveProjection() const;

	/// \brief Sets the zoom of the viewport.
	/// \param fov Vertical camera angle in radians if the viewport uses a perspective projection or
	///            the field of view in the vertical direction in world units if the viewport
	///            uses an orthogonal projection.
	void setFieldOfView(FloatType fov) {
		// Clamp FOV to reasonable interval.
		_fieldOfView.set(this, PROPERTY_FIELD(fieldOfView), qBound(FloatType(-1e12), fov, FloatType(1e12)));
	}

	/// \brief Returns the viewing direction of the camera.
	Vector3 cameraDirection() const {
		if(cameraTransformation().column(2) == Vector3::Zero()) return Vector3(0,0,1);
		else return -cameraTransformation().column(2);
	}

	/// \brief Changes the viewing direction of the camera.
	void setCameraDirection(const Vector3& newDir);

	/// \brief Returns the position of the camera.
	Point3 cameraPosition() const {
		return Point3::Origin() + cameraTransformation().translation();
	}

	/// \brief Sets the position of the camera.
	void setCameraPosition(const Point3& p) {
		AffineTransformation tm = cameraTransformation();
		tm.translation() = p - Point3::Origin();
		setCameraTransformation(tm);
	}

	/// Return the current 3D projection used to render the contents of the viewport.
	const ViewProjectionParameters& projectionParams() const { return _projParams; }

	/// \brief Returns the current orbit center for this viewport.
	Point3 orbitCenter();

	/// \brief Computes the scaling factor of an object that should always appear in the same size on the screen, independent of its
	///        position with respect to the camera.
	FloatType nonScalingSize(const Point3& worldPosition);

	/// \brief Computes a point in the given coordinate system based on the given screen position and the current snapping settings.
	/// \param[in] screenPoint A screen point relative to the upper left corner of the viewport.
	/// \param[out] snapPoint The resulting point in the coordinate system specified by \a snapSystem. If the method returned
	///                       \c false then the value of this output variable is undefined.
	/// \param[in] snapSystem Specifies the coordinate system in which the snapping point should be determined.
	/// \return \c true if a snapping point has been found; \c false if no snapping point was found for the given screen position.
	bool snapPoint(const QPointF& screenPoint, Point3& snapPoint, const AffineTransformation& snapSystem);

	/// \brief Computes a point in the grid coordinate system based on a screen position and the current snap settings.
	/// \param[in] screenPoint A screen point relative to the upper left corner of the 3d window.
	/// \param[out] snapPoint The resulting snap point in the viewport's grid coordinate system. If the method returned
	///                       \c false then the value of this output variable is undefined.
	/// \return \c true if a snapping point has been found; \c false if no snapping point was found for the given screen position.
	bool snapPoint(const QPointF& screenPoint, Point3& snapPoint) {
		return this->snapPoint(screenPoint, snapPoint, gridMatrix());
	}

	/// \brief Computes a ray in world space going through a pixel of the viewport window.
	/// \param screenPoint A screen point relative to the upper left corner of the viewport window.
	/// \return The ray that goes from the camera point through the specified pixel of the viewport window.
	Ray3 screenRay(const QPointF& screenPoint);

	/// \brief Computes a ray in world space going through a viewport pixel.
	/// \param viewportPoint Viewport coordinates of the point in the range [-1,+1].
	/// \return The ray that goes from the viewpoint through the specified position in the viewport.
	Ray3 viewportRay(const Point2& viewportPoint);

	/// \brief Computes the intersection point of a ray going through a point in the
	///        viewport plane with the construction grid plane.
	/// \param[in] viewportPosition A 2d point in viewport coordinates (in the range [-1,+1]).
	/// \param[out] intersectionPoint The coordinates of the intersection point in grid plane coordinates.
	///                               The point can be transformed to world coordinates using the gridMatrix() transform.
	/// \param[in] epsilon This threshold value is used to test whether the ray is parallel to the grid plane.
	/// \return \c true if an intersection has been found; \c false if not.
	bool computeConstructionPlaneIntersection(const Point2& viewportPosition, Point3& intersectionPoint, FloatType epsilon = FLOATTYPE_EPSILON);

	/// Returns the geometry of the render frame, i.e., the region of the viewport that
	/// will be visible in a rendered image.
	/// The returned box is given in viewport coordinates (interval [-1,+1]).
	Box2 renderFrameRect() const;

	/// \brief Returns a color value for drawing something in the viewport. The user can configure the color for each element.
	/// \param which The enum constant that specifies what type of element to draw.
	/// \return The color that should be used for the given element type.
	static const Color& viewportColor(ViewportSettings::ViewportColor which) {
		return ViewportSettings::getSettings().viewportColor(which);
	}

	/// \brief Inserts an overlay into this viewport's list of overlays.
	/// \param index The position at which to insert the overlay.
	/// \param layer The overlay to insert.
	/// \undoable
	void insertOverlay(int index, ViewportOverlay* layer) {
		_overlays.insert(this, PROPERTY_FIELD(overlays), index, layer);
	}

	/// \brief Removes an overlay from this viewport.
	/// \param index The index of the overlay to remove.
	/// \undoable
	void removeOverlay(int index) {
		_overlays.remove(this, PROPERTY_FIELD(overlays), index);
	}

	/// \brief Inserts an underlay into this viewport's list of underlays.
	/// \param index The position at which to insert the underlay into the stack.
	/// \param layer The underlay to insert.
	/// \undoable
	void insertUnderlay(int index, ViewportOverlay* layer) {
		_underlays.insert(this, PROPERTY_FIELD(underlays), index, layer);
	}

	/// \brief Removes an underlay from this viewport.
	/// \param index The index of the underlay to remove.
	/// \undoable
	void removeUnderlay(int index) {
		_underlays.remove(this, PROPERTY_FIELD(underlays), index);
	}

	/// \brief Returns the size of the viewport's screen window (in device pixels).
	QSize windowSize() const {
		return window() ? window()->viewportWindowDeviceSize() : QSize(0,0);
	}

	/// \brief Returns the GUI window associated with this viewport (may be NULL).
	ViewportWindowInterface* window() const { return _window; }

	/// \brief Associates this viewport with a GUI window. This is an internal method.
	void setWindow(ViewportWindowInterface* window) { _window = window; }

	/// Renders the contents of the interactive viewport in a window.
	/// This is an internal method, which should not be called by user code.
	void renderInteractive(SceneRenderer* renderer);

public Q_SLOTS:

	/// \brief Zooms to the extents of the scene.
	void zoomToSceneExtents();

	/// \brief Zooms to the extents of the currently selected nodes.
	void zoomToSelectionExtents();

	/// \brief Zooms to the extents of the given bounding box.
	void zoomToBox(const Box3& box);

Q_SIGNALS:

	/// This signal is emitted whenever any of the UI properties of the viewport change.
	void viewportChanged();

protected:

	/// Is called when the value of a property field of this object has changed.
	virtual void propertyChanged(const PropertyFieldDescriptor& field) override;

	/// Is called when a RefTarget referenced by this object has generated an event.
	virtual bool referenceEvent(RefTarget* source, const ReferenceEvent& event) override;

	/// Is called when the value of a reference field of this RefMaker changes.
	virtual void referenceReplaced(const PropertyFieldDescriptor& field, RefTarget* oldTarget, RefTarget* newTarget) override;

	/// Is called when a RefTarget has been added to a VectorReferenceField.
	virtual void referenceInserted(const PropertyFieldDescriptor& field, RefTarget* newTarget, int listIndex) override;

	/// Is called when a RefTarget has been removed from a VectorReferenceField.
	virtual void referenceRemoved(const PropertyFieldDescriptor& field, RefTarget* oldTarget, int listIndex) override;

protected:

	/// Updates the title text of the viewport based on the current view type.
	void updateViewportTitle();

	/// Modifies the projection such that the render frame painted over the 3d scene exactly
	/// matches the true visible area.
	void adjustProjectionForRenderFrame(ViewProjectionParameters& params);

	/// Renders the viewport overlays to an image buffer.
	void renderLayers(SceneRenderer* renderer, TimePoint time, RenderSettings* renderSettings, QSize vpSize, const Box3& boundingBox, const QVector<ViewportOverlay*>& layers, AsyncOperation& operation);

private Q_SLOTS:

	/// This is called when the global viewport settings have changed.
	void viewportSettingsChanged(ViewportSettings* newSettings);

private:

	/// The type of the viewport (top, left, perspective, etc.)
	DECLARE_PROPERTY_FIELD_FLAGS(ViewType, viewType, PROPERTY_FIELD_NO_UNDO);

	/// The orientation of the grid.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(AffineTransformation, gridMatrix, setGridMatrix, PROPERTY_FIELD_NO_UNDO);

	/// The zoom or field of view.
	DECLARE_PROPERTY_FIELD_FLAGS(FloatType, fieldOfView, PROPERTY_FIELD_NO_UNDO);

	/// The orientation of the camera.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(AffineTransformation, cameraTransformation, setCameraTransformation, PROPERTY_FIELD_NO_UNDO);

	/// Selectes the upward pointing direction of the virtual camera.
	/// If nonzero, this viewport parameter overrides the global user settings.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(Vector3, cameraUpDirection, setCameraUpDirection, PROPERTY_FIELD_NO_UNDO);

	/// Indicates whether the rendering frame is shown.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(bool, renderPreviewMode, setRenderPreviewMode, PROPERTY_FIELD_NO_UNDO);

	/// Indicates whether the grid is shown.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(bool, isGridVisible, setGridVisible, PROPERTY_FIELD_NO_UNDO);

	/// Enables stereoscopic rendering.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(bool, stereoscopicMode, setStereoscopicMode, PROPERTY_FIELD_NO_UNDO);

	/// The scene node (camera) that has been selected as the view node.
	DECLARE_MODIFIABLE_REFERENCE_FIELD_FLAGS(PipelineSceneNode, viewNode, setViewNode, PROPERTY_FIELD_NEVER_CLONE_TARGET | PROPERTY_FIELD_NO_SUB_ANIM | PROPERTY_FIELD_DONT_PROPAGATE_MESSAGES);

	/// The title of the viewport.
	DECLARE_PROPERTY_FIELD_FLAGS(QString, viewportTitle, PROPERTY_FIELD_NO_UNDO);

	/// The list of layers which are painted above the 3d scene.
	DECLARE_VECTOR_REFERENCE_FIELD(ViewportOverlay, overlays);

	/// The list of layers which are painted under the 3d scene.
	DECLARE_VECTOR_REFERENCE_FIELD(ViewportOverlay, underlays);

	/// This flag is true during the rendering phase.
	bool _isRendering = false;

	/// Describes the current 3D projection used to render the contents of the viewport.
	ViewProjectionParameters _projParams;

	/// The GUI window associated with this viewport.
	ViewportWindowInterface* _window = nullptr;
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace

Q_DECLARE_METATYPE(Ovito::Viewport::ViewType);
Q_DECLARE_TYPEINFO(Ovito::Viewport::ViewType, Q_PRIMITIVE_TYPE);

#include <ovito/core/rendering/SceneRenderer.h>
