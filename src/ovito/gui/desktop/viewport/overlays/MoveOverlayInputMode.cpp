////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2016 Alexander Stukowski
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

#include <ovito/gui/desktop/GUI.h>
#include <ovito/gui/desktop/viewport/ViewportWindow.h>
#include <ovito/gui/desktop/properties/PropertiesEditor.h>
#include <ovito/gui/desktop/mainwin/MainWindow.h>
#include <ovito/gui/base/viewport/ViewportInputManager.h>
#include <ovito/core/viewport/Viewport.h>
#include <ovito/core/viewport/overlays/ViewportOverlay.h>
#include "MoveOverlayInputMode.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(View)

/******************************************************************************
* Constructor.
******************************************************************************/
MoveOverlayInputMode::MoveOverlayInputMode(PropertiesEditor* editor) :
		ViewportInputMode(editor),
		_editor(editor),
		_moveCursor(QCursor(QPixmap(QStringLiteral(":/gui/cursor/editing/cursor_mode_move.png")))),
		_forbiddenCursor(Qt::ForbiddenCursor)
{
}

/******************************************************************************
* This is called by the system after the input handler is
* no longer the active handler.
******************************************************************************/
void MoveOverlayInputMode::deactivated(bool temporary)
{
	if(viewport()) {
		// Restore old state if change has not been committed.
		viewport()->dataset()->undoStack().endCompoundOperation(false);
		_viewport = nullptr;
	}
	ViewportInputMode::deactivated(temporary);
}

/******************************************************************************
* Handles the mouse down events for a Viewport.
******************************************************************************/
void MoveOverlayInputMode::mousePressEvent(ViewportWindowInterface* vpwin, QMouseEvent* event)
{
	if(event->button() == Qt::LeftButton) {
		if(viewport() == nullptr) {
			ViewportOverlay* layer = dynamic_object_cast<ViewportOverlay>(_editor->editObject());
			if(layer && (vpwin->viewport()->overlays().contains(layer) || vpwin->viewport()->underlays().contains(layer))) {
				_viewport = vpwin->viewport();
				_startPoint = event->localPos();
				viewport()->dataset()->undoStack().beginCompoundOperation(tr("Move overlay"));
			}
		}
		return;
	}
	else if(event->button() == Qt::RightButton) {
		if(viewport()) {
			// Restore old state when aborting the move operation.
			viewport()->dataset()->undoStack().endCompoundOperation(false);
			_viewport = nullptr;
			return;
		}
	}
	ViewportInputMode::mousePressEvent(vpwin, event);
}

/******************************************************************************
* Handles the mouse move events for a Viewport.
******************************************************************************/
void MoveOverlayInputMode::mouseMoveEvent(ViewportWindowInterface* vpwin, QMouseEvent* event)
{
	// Get the viewport layer being moved.
	ViewportOverlay* layer = dynamic_object_cast<ViewportOverlay>(_editor->editObject());
	if(layer && (vpwin->viewport()->overlays().contains(layer) || vpwin->viewport()->underlays().contains(layer))) {
		setCursor(_moveCursor);

		if(viewport() == vpwin->viewport()) {
			// Take the current mouse cursor position to make the input mode
			// look more responsive. The cursor position recorded when the mouse event was
			// generates may be too old.
			_currentPoint = static_cast<ViewportWindow*>(vpwin)->mapFromGlobal(QCursor::pos());

			// Reset the layer's position first before moving it again below.
			viewport()->dataset()->undoStack().resetCurrentCompoundOperation();

			// Compute the displacement based on the new mouse position.
			Box2 renderFrameRect = viewport()->renderFrameRect();
			QSize vpSize = vpwin->viewportWindowDeviceIndependentSize();
			Vector2 delta;
			delta.x() =  (FloatType)(_currentPoint.x() - _startPoint.x()) / vpSize.width() / renderFrameRect.width() * 2;
			delta.y() = -(FloatType)(_currentPoint.y() - _startPoint.y()) / vpSize.height() / renderFrameRect.height() * 2;

			// Move the layer.
			try {
				layer->moveLayerInViewport(delta);
			}
			catch(const Exception& ex) {
				inputManager()->removeInputMode(this);
				ex.reportError();
			}

			// Force immediate viewport repaints.
			viewport()->dataset()->viewportConfig()->processViewportUpdates();
		}
	}
	else {
		setCursor(_forbiddenCursor);
	}
	ViewportInputMode::mouseMoveEvent(vpwin, event);
}

/******************************************************************************
* Handles the mouse up events for a Viewport.
******************************************************************************/
void MoveOverlayInputMode::mouseReleaseEvent(ViewportWindowInterface* vpwin, QMouseEvent* event)
{
	if(viewport()) {
		// Commit change.
		viewport()->dataset()->undoStack().endCompoundOperation();
		_viewport = nullptr;
	}
	ViewportInputMode::mouseReleaseEvent(vpwin, event);
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
