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
#include <ovito/core/viewport/Viewport.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(View)

/**
 * \brief This class holds a collection of Viewport objects.
 *
 * It also keeps track of the current viewport and the maximized viewport.
 */
class OVITO_CORE_EXPORT ViewportConfiguration : public RefTarget
{
	Q_OBJECT
	OVITO_CLASS(ViewportConfiguration)

public:

	enum OrbitCenterMode {
		ORBIT_SELECTION_CENTER,		///< Take the center of mass of the current selection as orbit center.
									///< If there is no selection, use scene bounding box.
		ORBIT_USER_DEFINED			///< Use the orbit center set by the user.
	};
	Q_ENUMS(OrbitCenterMode);

public:

	/// Constructor.
	Q_INVOKABLE ViewportConfiguration(DataSet* dataset);

	/// Add a record for a new viewport.
	void addViewport(const OORef<Viewport>& vp) { _viewports.push_back(this, PROPERTY_FIELD(viewports), vp); }

	/// \brief Immediately repaints all viewports that have been scheduled for an update using updateViewports().
	/// \sa updateViewports()
	void processViewportUpdates();

	/// \brief A call to this method suspends redrawing of the viewports.
	///
	/// To resume redrawing of viewports call resumeViewportUpdates().
	///
	/// Calling updateViewports() while redrawing is suspended will update the
	/// viewports as soon as redrawing is resumed.
	///
	/// Normally you should use the ViewportSuspender helper class to suspend viewport update.
	/// It has the advantage of being exception-safe.
	///
	/// \sa resumeViewportUpdates(), isSuspended(), ViewportSuspender
	void suspendViewportUpdates() { _viewportSuspendCount++; }

	/// \brief This will resume redrawing of the viewports after a call to suspendViewportUpdates().
	/// \sa suspendViewportUpdates(), isSuspended()
	void resumeViewportUpdates();

	/// \brief Returns whether viewport updates are suspended.
	/// \return \c true if suspendViewportUpdates() has been called to suspend any viewport updates.
	/// \sa suspendViewportUpdates(), resumeViewportUpdates()
	bool isSuspended() const { return _viewportSuspendCount > 0; }

	/// \brief Returns whether any of the viewports
	///        is currently being updated.
	/// \return \c true if there is currently a rendering operation going on.
	bool isRendering() const;

	/// Returns the current location around which the viewport camera orbits.
	Point3 orbitCenter();

public Q_SLOTS:

	/// \brief Sets the active viewport.
	/// \param vp The viewport to be made active.
	void setActiveViewportSlot(Viewport* vp) {
		setActiveViewport(vp);
	}

	/// \brief Maximizes a viewport.
	/// \param vp The viewport to be maximized or \c NULL to restore the currently maximized viewport to
	///           its original state.
	void setMaximizedViewportSlot(Viewport* vp) {
		setMaximizedViewport(vp);
	}

	/// \brief Zooms all viewports to the extents of the currently selected nodes.
	void zoomToSelectionExtents() {
		for(Viewport* vp : viewports())
			vp->zoomToSelectionExtents();
	}

	/// \brief Zooms to the extents of the scene.
	void zoomToSceneExtents() {
		for(Viewport* vp : viewports())
			vp->zoomToSceneExtents();
	}

	/// \brief This will flag all viewports for redrawing.
	///
	/// This function does not cause an immediate repaint of the viewports; instead it schedules a
	/// paint event for processing when Qt returns to the main event loop. You can call this method as often
	/// as you want; it will return immediately and will cause only one viewport repaint when Qt returns to the
	/// main event loop.
	///
	/// To update only a single viewport, Viewport::updateViewport() should be used.
	///
	/// To redraw all viewports immediately without waiting for the paint event to be processed,
	/// call processViewportUpdates() subsequently.
	void updateViewports();

protected:

	/// Is called when the value of a reference field of this RefMaker changes.
	virtual void referenceReplaced(const PropertyFieldDescriptor& field, RefTarget* oldTarget, RefTarget* newTarget) override;

	/// Is called when the value of a property of this object has changed.
	virtual void propertyChanged(const PropertyFieldDescriptor& field) override;

Q_SIGNALS:

	/// This signal is emitted when another viewport became active.
	void activeViewportChanged(Viewport* activeViewport);

	/// This signal is emitted when one of the viewports has been maximized.
	void maximizedViewportChanged(Viewport* maximizedViewport);

	/// This signal is emitted when the camera orbit center haa changed.
	void cameraOrbitCenterChanged();

	/// This signal is emitted whenever the updating of viewports is resumed.
	void viewportUpdateResumed();

private:

	/// The list of viewports.
	DECLARE_VECTOR_REFERENCE_FIELD_FLAGS(Viewport, viewports, PROPERTY_FIELD_NO_UNDO | PROPERTY_FIELD_ALWAYS_CLONE);

	/// The active viewport. May be NULL.
	DECLARE_MODIFIABLE_REFERENCE_FIELD_FLAGS(Viewport, activeViewport, setActiveViewport, PROPERTY_FIELD_NO_UNDO);

	/// The maximized viewport or NULL.
	DECLARE_MODIFIABLE_REFERENCE_FIELD_FLAGS(Viewport, maximizedViewport, setMaximizedViewport, PROPERTY_FIELD_NO_UNDO);

	/// Controls around which point the viewport camera should orbit.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(OrbitCenterMode, orbitCenterMode, setOrbitCenterMode, PROPERTY_FIELD_NO_UNDO);

	/// Position of the orbiting center picked by the user.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(Point3, userOrbitCenter, setUserOrbitCenter, PROPERTY_FIELD_NO_UNDO);

	/// This counter is for suspending the viewport updates.
	int _viewportSuspendCount = 0;

	/// Indicates that the viewports have been invalidated while updates were suspended.
	bool _viewportsNeedUpdate = false;
};


/**
 * \brief Small helper class that suspends viewport redrawing while it
 * exists.
 *
 * The constructor of this class calls ViewportConfiguration::suspendViewportUpdates() and
 * the destructor calls ViewportConfiguration::resumeViewportUpdates().
 *
 * Use this to make your code exception-safe.
 * Just create an instance of this class on the stack to suspend viewport updates
 * during the lifetime of the class instance.
 */
class OVITO_CORE_EXPORT ViewportSuspender {
public:
	ViewportSuspender(ViewportConfiguration* vpconf) : _vpconf(*vpconf) { _vpconf.suspendViewportUpdates(); }
	ViewportSuspender(RefMaker* object);
	~ViewportSuspender() { _vpconf.resumeViewportUpdates(); }
private:
	ViewportConfiguration& _vpconf;
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace

Q_DECLARE_METATYPE(Ovito::ViewportConfiguration::OrbitCenterMode);
Q_DECLARE_TYPEINFO(Ovito::ViewportConfiguration::OrbitCenterMode, Q_PRIMITIVE_TYPE);


