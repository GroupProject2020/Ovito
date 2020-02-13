////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2020 Alexander Stukowski
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

#include <ovito/gui/base/GUIBase.h>
#include <ovito/gui/base/mainwin/MainWindowInterface.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/dataset/UndoStack.h>
#include <ovito/core/dataset/scene/SelectionSet.h>
#include <ovito/core/viewport/ViewportConfiguration.h>
#include <ovito/core/viewport/Viewport.h>
#include "ViewportInputManager.h"
#include "SelectionMode.h"

namespace Ovito {

/// The cursor shape shown while the mouse is positioned over an object.
boost::optional<QCursor> SelectionMode::_hoverCursor;

/******************************************************************************
* Handles the mouse down event for the given viewport.
******************************************************************************/
void SelectionMode::mousePressEvent(ViewportWindowInterface* vpwin, QMouseEvent* event)
{
	if(event->button() == Qt::LeftButton) {
		_viewport = vpwin->viewport();
		_clickPoint = event->localPos();
	}
	else if(event->button() == Qt::RightButton) {
		_viewport = nullptr;
	}
	ViewportInputMode::mousePressEvent(vpwin, event);
}

/******************************************************************************
* Handles the mouse up event for the given viewport.
******************************************************************************/
void SelectionMode::mouseReleaseEvent(ViewportWindowInterface* vpwin, QMouseEvent* event)
{
	if(_viewport != nullptr) {
		// Select object under mouse cursor.
		ViewportPickResult pickResult = vpwin->pick(_clickPoint);
		if(pickResult.isValid()) {
			_viewport->dataset()->undoStack().beginCompoundOperation(tr("Select"));
			_viewport->dataset()->selection()->setNode(pickResult.pipelineNode());
			_viewport->dataset()->undoStack().endCompoundOperation();
		}
		_viewport = nullptr;
	}
	ViewportInputMode::mouseReleaseEvent(vpwin, event);
}

/******************************************************************************
* This is called by the system after the input handler is
* no longer the active handler.
******************************************************************************/
void SelectionMode::deactivated(bool temporary)
{
	if(inputManager()->mainWindow())
		inputManager()->mainWindow()->clearStatusBarMessage();
	_viewport = nullptr;
	ViewportInputMode::deactivated(temporary);
}

/******************************************************************************
* Handles the mouse move event for the given viewport.
******************************************************************************/
void SelectionMode::mouseMoveEvent(ViewportWindowInterface* vpwin, QMouseEvent* event)
{
	// Change mouse cursor while hovering over an object.
	ViewportPickResult pickResult = vpwin->pick(event->localPos());
	setCursor(pickResult.isValid() ? selectionCursor() : QCursor());

	// Display a description of the object under the mouse cursor in the status bar.
	if(inputManager()->mainWindow()) {
		if(pickResult.isValid() && pickResult.pickInfo())
			inputManager()->mainWindow()->showStatusBarMessage(pickResult.pickInfo()->infoString(pickResult.pipelineNode(), pickResult.subobjectId()));
		else
			inputManager()->mainWindow()->clearStatusBarMessage();
	}

	ViewportInputMode::mouseMoveEvent(vpwin, event);
}

}	// End of namespace
