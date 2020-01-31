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
#include <ovito/core/viewport/Viewport.h>
#include <ovito/core/viewport/ViewportConfiguration.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/dataset/DataSetContainer.h>
#include "ViewportInputManager.h"
#include "ViewportInputMode.h"
#include "NavigationModes.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(ViewportInput)

/******************************************************************************
* Destructor.
******************************************************************************/
ViewportInputMode::~ViewportInputMode()
{
	// Mode must not be in use when it gets destroyed.
	OVITO_ASSERT(!_manager || std::find(_manager->stack().begin(), _manager->stack().end(), this) == _manager->stack().end());
}

/******************************************************************************
* Removes this input mode from the mode stack of the ViewportInputManager.
******************************************************************************/
void ViewportInputMode::removeMode()
{
	if(_manager)
		_manager->removeInputMode(this);
}

/******************************************************************************
* This is called by the system after the input handler has become the active handler.
******************************************************************************/
void ViewportInputMode::activated(bool temporaryActivation)
{
	Q_EMIT statusChanged(true);
}

/******************************************************************************
* This is called by the system after the input handler is no longer the active handler.
******************************************************************************/
void ViewportInputMode::deactivated(bool temporary)
{
	inputManager()->removeViewportGizmo(inputManager()->pickOrbitCenterMode());
	Q_EMIT statusChanged(false);
}

/******************************************************************************
* Checks whether this mode is currently active.
******************************************************************************/
bool ViewportInputMode::isActive() const
{
	if(!_manager) return false;
	return _manager->activeMode() == this;
}

/******************************************************************************
* Activates the given temporary navigation mode.
******************************************************************************/
void ViewportInputMode::activateTemporaryNavigationMode(ViewportInputMode* mode)
{
	inputManager()->pushInputMode(mode, true);
}

/******************************************************************************
* Sets the mouse cursor shown in the viewport windows
* while this input handler is active.
******************************************************************************/
void ViewportInputMode::setCursor(const QCursor& cursor)
{
	_cursor = cursor;
	Q_EMIT curserChanged(_cursor);
}

/******************************************************************************
* Handles the mouse down event for the given viewport.
******************************************************************************/
void ViewportInputMode::mousePressEvent(ViewportWindowInterface* vpwin, QMouseEvent* event)
{
	_lastMousePressEvent.reset();
	ViewportInputManager* manager = inputManager();
	if(event->button() == Qt::RightButton) {
		if(modeType() != ExclusiveMode) {
			manager->removeInputMode(this);
		}
		else {
			activateTemporaryNavigationMode(manager->panMode());
			if(manager->activeMode() == manager->panMode()) {
				QMouseEvent leftMouseEvent(event->type(), event->localPos(), event->windowPos(), event->screenPos(), Qt::LeftButton, Qt::LeftButton, event->modifiers());
				manager->activeMode()->mousePressEvent(vpwin, &leftMouseEvent);
			}
		}
	}
	else if(event->button() == Qt::LeftButton) {
		_lastMousePressEvent.reset(new QMouseEvent(event->type(), event->localPos(), event->windowPos(), event->screenPos(), event->button(), event->buttons(), event->modifiers()));
	}
	else if(event->button() == Qt::MidButton) {
		activateTemporaryNavigationMode(manager->panMode());
		if(manager->activeMode() == manager->panMode())
			manager->activeMode()->mousePressEvent(vpwin, event);
	}
}

/******************************************************************************
* Handles the mouse up event for the given viewport.
******************************************************************************/
void ViewportInputMode::mouseReleaseEvent(ViewportWindowInterface* vpwin, QMouseEvent* event)
{
	_lastMousePressEvent.reset();
}

/******************************************************************************
* Handles the mouse move event for the given viewport.
******************************************************************************/
void ViewportInputMode::mouseMoveEvent(ViewportWindowInterface* vpwin, QMouseEvent* event)
{
	if(_lastMousePressEvent && (event->pos() - _lastMousePressEvent->pos()).manhattanLength() > 2) {
		if(this != inputManager()->orbitMode()) {
			ViewportInputManager* manager = inputManager();
			activateTemporaryNavigationMode(inputManager()->orbitMode());
			if(manager->activeMode() == manager->orbitMode())
				manager->activeMode()->mousePressEvent(vpwin, _lastMousePressEvent.get());
		}
		_lastMousePressEvent.reset();
	}
}

/******************************************************************************
* Handles the mouse wheel event for the given viewport.
******************************************************************************/
void ViewportInputMode::wheelEvent(ViewportWindowInterface* vpwin, QWheelEvent* event)
{
	_lastMousePressEvent.reset();

	FloatType delta = event->angleDelta().y();
	if(event->inverted()) delta = -delta;
#ifdef Q_OS_WASM
	// In the web browser, the wheel direction seems to be inverted. 
	delta = -delta;
#endif	
	inputManager()->zoomMode()->zoom(vpwin->viewport(), delta);
	event->accept();
}

/******************************************************************************
* Handles the mouse double-click events for the given viewport.
******************************************************************************/
void ViewportInputMode::mouseDoubleClickEvent(ViewportWindowInterface* vpwin, QMouseEvent* event)
{
	_lastMousePressEvent.reset();
	if(event->button() == Qt::LeftButton) {
		inputManager()->pickOrbitCenterMode()->pickOrbitCenter(vpwin, event->pos());
		inputManager()->addViewportGizmo(inputManager()->pickOrbitCenterMode());
		event->accept();
	}
}

/******************************************************************************
* Handles the mouse up event for the given viewport.
******************************************************************************/
void ViewportInputMode::focusOutEvent(ViewportWindowInterface* vpwin, QFocusEvent* event)
{
	_lastMousePressEvent.reset();
}

/******************************************************************************
* Redraws all viewports.
******************************************************************************/
void ViewportInputMode::requestViewportUpdate()
{
	if(isActive()) {
		DataSet* dataset = inputManager()->datasetContainer().currentSet();
		if(dataset && dataset->viewportConfig()) {
			dataset->viewportConfig()->updateViewports();
		}
	}
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
