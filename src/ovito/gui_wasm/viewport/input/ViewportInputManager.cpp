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

#include <ovito/gui_wasm/GUI.h>
#include <ovito/gui_wasm/mainwin/MainWindow.h>
#include <ovito/core/dataset/DataSetContainer.h>
#include <ovito/core/viewport/ViewportConfiguration.h>
#include "ViewportInputManager.h"
#include "ViewportInputMode.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(ViewportInput)

/******************************************************************************
* Initializes the viewport input manager.
******************************************************************************/
ViewportInputManager::ViewportInputManager(QObject* owner, DataSetContainer& datasetContainer) : QObject(owner),
	_datasetContainer(datasetContainer)
{
	_mainWindow = qobject_cast<MainWindow*>(owner);
	_zoomMode = new ZoomMode(this);
	_panMode = new PanMode(this);
	_orbitMode = new OrbitMode(this);
	_fovMode = new FOVMode(this);
	_pickOrbitCenterMode = new PickOrbitCenterMode(this);
	_selectionMode = new SelectionMode(this);
	_moveMode = new MoveMode(this);
	_rotateMode = new RotateMode(this);

	// Set the scene node selection mode as the default.
	_defaultMode = _selectionMode;

	// Reset the viewport input manager when a new scene has been loaded.
	connect(&_datasetContainer, &DataSetContainer::dataSetChanged, this, &ViewportInputManager::reset);
}

/******************************************************************************
* Destructor
******************************************************************************/
ViewportInputManager::~ViewportInputManager()
{
	for(ViewportInputMode* mode : _inputModeStack)
		mode->_manager = nullptr;
	_inputModeStack.clear();
}

/******************************************************************************
* Returns the currently active ViewportInputMode that handles the mouse events in viewports.
******************************************************************************/
ViewportInputMode* ViewportInputManager::activeMode()
{
	if(_inputModeStack.empty()) return nullptr;
	return _inputModeStack.back();
}

/******************************************************************************
* Pushes a mode onto the stack and makes it active.
******************************************************************************/
void ViewportInputManager::pushInputMode(ViewportInputMode* newMode, bool temporary)
{
    OVITO_CHECK_POINTER(newMode);

    ViewportInputMode* oldMode = activeMode();
	if(newMode == oldMode) return;

	bool oldModeRemoved = false;
	if(oldMode) {
		if(newMode->modeType() == ViewportInputMode::ExclusiveMode) {
			// Remove all existing input modes from the stack before activating the exclusive mode.
			while(_inputModeStack.size() > 1)
				removeInputMode(activeMode());
			oldMode = activeMode();
			if(oldMode == newMode) return;
			oldModeRemoved = true;
			_inputModeStack.clear();
		}
		else if(newMode->modeType() == ViewportInputMode::NormalMode) {
			// Remove all non-exclusive handlers from the stack before activating the new mode.
			while(_inputModeStack.size() > 1 && activeMode()->modeType() != ViewportInputMode::ExclusiveMode)
				removeInputMode(activeMode());
			oldMode = activeMode();
			if(oldMode == newMode) return;
			if(oldMode->modeType() != ViewportInputMode::ExclusiveMode) {
				_inputModeStack.pop_back();
				oldModeRemoved = true;
			}
		}
		else if(newMode->modeType() == ViewportInputMode::TemporaryMode) {
			// Remove all temporary handlers from the stack before activating the new mode.
			if(oldMode->modeType() == ViewportInputMode::TemporaryMode) {
				_inputModeStack.pop_back();
				oldModeRemoved = true;
			}
		}
	}

	// Put new handler on the stack.
	OVITO_ASSERT(newMode->_manager == nullptr);
	newMode->_manager = this;
	_inputModeStack.push_back(newMode);

	if(oldMode) {
		OVITO_ASSERT(oldMode->_manager == this);
		oldMode->deactivated(!oldModeRemoved);
		if(oldModeRemoved)
			oldMode->_manager = nullptr;
	}
	newMode->activated(temporary);

	Q_EMIT inputModeChanged(oldMode, newMode);
}

/******************************************************************************
* Removes a mode from the stack and deactivates it if it is currently active.
******************************************************************************/
void ViewportInputManager::removeInputMode(ViewportInputMode* mode)
{
	OVITO_CHECK_POINTER(mode);

	auto iter = std::find(_inputModeStack.begin(), _inputModeStack.end(), mode);
	if(iter == _inputModeStack.end()) return;

	OVITO_ASSERT(mode->_manager == this);

	if(iter == _inputModeStack.end() - 1) {
		_inputModeStack.erase(iter);
		mode->deactivated(false);
		if(!_inputModeStack.empty())
			activeMode()->activated(false);
		mode->_manager = nullptr;

		Q_EMIT inputModeChanged(mode, activeMode());

		// Activate default mode when stack becomes empty.
		if(_inputModeStack.empty())
			pushInputMode(_defaultMode);
	}
	else {
		_inputModeStack.erase(iter);
		mode->deactivated(false);
		mode->_manager = nullptr;
	}
}

/******************************************************************************
* Adds a gizmo to be shown in the interactive viewports.
******************************************************************************/
void ViewportInputManager::addViewportGizmo(ViewportGizmo* gizmo)
{
	OVITO_ASSERT(gizmo);
	if(std::find(viewportGizmos().begin(), viewportGizmos().end(), gizmo) == viewportGizmos().end()) {
		_viewportGizmos.push_back(gizmo);

		// Update viewports to show displays overlays.
		DataSet* dataset = _datasetContainer.currentSet();
		if(dataset && dataset->viewportConfig()) {
			dataset->viewportConfig()->updateViewports();
		}
	}
}

/******************************************************************************
* Removes a gizmo, which will no longer be shown in the interactive viewports.
******************************************************************************/
void ViewportInputManager::removeViewportGizmo(ViewportGizmo* gizmo)
{
	OVITO_ASSERT(gizmo);
	auto iter = std::find(_viewportGizmos.begin(), _viewportGizmos.end(), gizmo);
	if(iter != _viewportGizmos.end()) {
		_viewportGizmos.erase(iter);

		// Update viewports to show displays overlays.
		DataSet* dataset = _datasetContainer.currentSet();
		if(dataset && dataset->viewportConfig()) {
			dataset->viewportConfig()->updateViewports();
		}
	}
}

/******************************************************************************
* Resets the input mode stack to its initial state on application startup.
******************************************************************************/
void ViewportInputManager::reset()
{
	// Remove all input modes from the stack.
	for(int i = _inputModeStack.size() - 1; i >= 0; i--)
		removeInputMode(_inputModeStack[i]);

	// Activate default mode when stack is empty.
	if(_inputModeStack.empty())
		pushInputMode(_defaultMode);
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
