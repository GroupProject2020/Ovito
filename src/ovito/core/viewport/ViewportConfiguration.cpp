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

#include <ovito/core/Core.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/viewport/ViewportConfiguration.h>
#include <ovito/core/viewport/Viewport.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/dataset/scene/SelectionSet.h>
#include <ovito/core/dataset/scene/RootSceneNode.h>
#include <ovito/core/dataset/animation/AnimationSettings.h>

namespace Ovito {

IMPLEMENT_OVITO_CLASS(ViewportConfiguration);
DEFINE_REFERENCE_FIELD(ViewportConfiguration, viewports);
DEFINE_REFERENCE_FIELD(ViewportConfiguration, activeViewport);
DEFINE_REFERENCE_FIELD(ViewportConfiguration, maximizedViewport);
DEFINE_PROPERTY_FIELD(ViewportConfiguration, orbitCenterMode);
DEFINE_PROPERTY_FIELD(ViewportConfiguration, userOrbitCenter);

/******************************************************************************
* Constructor.
******************************************************************************/
ViewportSuspender::ViewportSuspender(RefMaker* object) : ViewportSuspender(object->dataset()->viewportConfig())
{
}

/******************************************************************************
* Constructor.
******************************************************************************/
ViewportConfiguration::ViewportConfiguration(DataSet* dataset) : RefTarget(dataset),
	_orbitCenterMode(ORBIT_SELECTION_CENTER),
	_userOrbitCenter(Point3::Origin())
{
	// Repaint viewports when the camera orbit center changed.
	connect(this, &ViewportConfiguration::cameraOrbitCenterChanged, this, &ViewportConfiguration::updateViewports);
}

/******************************************************************************
* Is called when the value of a reference field of this RefMaker changes.
******************************************************************************/
void ViewportConfiguration::referenceReplaced(const PropertyFieldDescriptor& field, RefTarget* oldTarget, RefTarget* newTarget)
{
	if(field == PROPERTY_FIELD(activeViewport)) {
		Q_EMIT activeViewportChanged(_activeViewport);
	}
	else if(field == PROPERTY_FIELD(maximizedViewport)) {
		Q_EMIT maximizedViewportChanged(_maximizedViewport);
	}
	RefTarget::referenceReplaced(field, oldTarget, newTarget);
}

/******************************************************************************
* Is called when the value of a property of this object has changed.
******************************************************************************/
void ViewportConfiguration::propertyChanged(const PropertyFieldDescriptor& field)
{
	if(field == PROPERTY_FIELD(orbitCenterMode) || field == PROPERTY_FIELD(userOrbitCenter)) {
		Q_EMIT cameraOrbitCenterChanged();
	}
	RefTarget::propertyChanged(field);
}

/******************************************************************************
* This flags all viewports for redrawing.
******************************************************************************/
void ViewportConfiguration::updateViewports()
{
	// Check if viewport updates are suppressed.
	if(_viewportSuspendCount > 0) {
		_viewportsNeedUpdate = true;
		return;
	}
	_viewportsNeedUpdate = false;

	for(Viewport* vp : viewports())
		vp->updateViewport();
}

/******************************************************************************
* This immediately redraws the viewports reflecting all
* changes made to the scene.
******************************************************************************/
void ViewportConfiguration::processViewportUpdates()
{
	if(isSuspended())
		return;

	for(Viewport* vp : viewports())
		vp->processUpdateRequest();
}

/******************************************************************************
* Return true if there is currently a rendering operation going on.
* No new windows or dialogs should be shown during this phase
* to prevent an infinite update loop.
******************************************************************************/
bool ViewportConfiguration::isRendering() const
{
	// Check if any of the viewport windows is rendering.
	for(Viewport* vp : viewports())
		if(vp->isRendering()) return true;

	return false;
}

/******************************************************************************
* This will resume redrawing of the viewports after a call to suspendViewportUpdates().
******************************************************************************/
void ViewportConfiguration::resumeViewportUpdates()
{
	OVITO_ASSERT(_viewportSuspendCount > 0);
	_viewportSuspendCount--;
	if(_viewportSuspendCount == 0) {
		Q_EMIT viewportUpdateResumed();
		if(_viewportsNeedUpdate)
			updateViewports();
	}
}

/******************************************************************************
* Returns the world space point around which the viewport camera orbits.
******************************************************************************/
Point3 ViewportConfiguration::orbitCenter()
{
	// Update orbiting center.
	if(orbitCenterMode() == ORBIT_SELECTION_CENTER) {
		TimePoint time = dataset()->animationSettings()->time();
		Box3 selectionBoundingBox;
		for(SceneNode* node : dataset()->selection()->nodes()) {
			selectionBoundingBox.addBox(node->worldBoundingBox(time));
		}
		if(!selectionBoundingBox.isEmpty())
			return selectionBoundingBox.center();
		else {
			Box3 sceneBoundingBox = dataset()->sceneRoot()->worldBoundingBox(time);
			if(!sceneBoundingBox.isEmpty())
				return sceneBoundingBox.center();
		}
	}
	else if(orbitCenterMode() == ORBIT_USER_DEFINED) {
		return _userOrbitCenter;
	}
	return Point3::Origin();
}

}	// End of namespace
