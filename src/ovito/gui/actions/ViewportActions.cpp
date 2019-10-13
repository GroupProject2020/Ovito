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

#include <ovito/gui/GUI.h>
#include <ovito/gui/actions/ActionManager.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/viewport/ViewportConfiguration.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui)

/******************************************************************************
* Handles the ACTION_VIEWPORT_MAXIMIZE command.
******************************************************************************/
void ActionManager::on_ViewportMaximize_triggered()
{
	ViewportConfiguration* vpconf = _dataset->viewportConfig();
	if(vpconf->maximizedViewport()) {
		vpconf->setMaximizedViewport(nullptr);
	}
	else if(vpconf->activeViewport()) {
		vpconf->setMaximizedViewport(vpconf->activeViewport());
	}
	// Remember which viewport was maximized across program sessions.
	// The same viewport will be maximized next time OVITO is started.
	ViewportSettings::getSettings().setDefaultMaximizedViewportType(vpconf->maximizedViewport() ? vpconf->maximizedViewport()->viewType() : Viewport::VIEW_NONE);
	ViewportSettings::getSettings().save();
}

/******************************************************************************
* Handles the ACTION_VIEWPORT_ZOOM_SCENE_EXTENTS command.
******************************************************************************/
void ActionManager::on_ViewportZoomSceneExtents_triggered()
{
	ViewportConfiguration* vpconf = _dataset->viewportConfig();

	if(vpconf->activeViewport() && !QGuiApplication::keyboardModifiers().testFlag(Qt::ControlModifier))
		vpconf->activeViewport()->zoomToSceneExtents();
	else
		vpconf->zoomToSceneExtents();
}

/******************************************************************************
* Handles the ACTION_VIEWPORT_ZOOM_SCENE_EXTENTS_ALL command.
******************************************************************************/
void ActionManager::on_ViewportZoomSceneExtentsAll_triggered()
{
	_dataset->viewportConfig()->zoomToSceneExtents();
}

/******************************************************************************
* Handles the ACTION_VIEWPORT_ZOOM_SELECTION_EXTENTS command.
******************************************************************************/
void ActionManager::on_ViewportZoomSelectionExtents_triggered()
{
	ViewportConfiguration* vpconf = _dataset->viewportConfig();
	if(vpconf->activeViewport())
		vpconf->activeViewport()->zoomToSelectionExtents();
}

/******************************************************************************
* Handles the ACTION_VIEWPORT_ZOOM_SELECTION_EXTENTS_ALL command.
******************************************************************************/
void ActionManager::on_ViewportZoomSelectionExtentsAll_triggered()
{
	_dataset->viewportConfig()->zoomToSelectionExtents();
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
