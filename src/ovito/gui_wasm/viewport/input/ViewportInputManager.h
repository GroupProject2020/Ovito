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

#pragma once


#include <ovito/gui_wasm/GUI.h>
#include "ViewportInputMode.h"
#include "NavigationModes.h"
#include "XFormModes.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(ViewportInput)

/**
 * \brief Manages a stack of viewport input handlers.
 */
class OVITO_GUIWASM_EXPORT ViewportInputManager : public QObject
{
	Q_OBJECT

public:

	/// \brief Constructor.
	ViewportInputManager(QObject* owner, DataSetContainer& datasetContainer);

	/// Destructor.
	virtual ~ViewportInputManager();

	/// Returns the dataset container this input manager is associated with,
	DataSetContainer& datasetContainer() { return _datasetContainer; }

	/// Returns the main window this input manager belongs to (if any).
	MainWindow* mainWindow() const { return _mainWindow; }

	/// \brief Returns the currently active ViewportInputMode that handles the mouse events in viewports.
	/// \return The mode that is responsible for mouse event handling. Can be \c NULL when the stack is empty.
	ViewportInputMode* activeMode();

	/// \brief Returns the stack of input modes.
	/// \return The stack of input modes. The topmost mode is the active one.
	const std::vector<ViewportInputMode*>& stack() { return _inputModeStack; }

	/// \brief Pushes an input mode onto the stack and makes it active.
	/// \param mode The mode to be made active.
	/// \param temporary A flag passed to the input mode that indicates whether the activation is only temporary.
	void pushInputMode(ViewportInputMode* mode, bool temporary = false);

	/// \brief Removes an input mode from the stack and deactivates it if it is currently active.
	/// \param mode The mode to remove from the stack.
	void removeInputMode(ViewportInputMode* mode);

	/// Returns the list of active viewport gizmos.
	const std::vector<ViewportGizmo*>& viewportGizmos() const { return _viewportGizmos; }

	/// Adds a gizmo to be shown in the interactive viewports.
	void addViewportGizmo(ViewportGizmo* gizmo);

	/// Removes a gizmo, which will no longer be shown in the interactive viewports.
	void removeViewportGizmo(ViewportGizmo* gizmo);

	/// \brief Returns the zoom input mode.
	ZoomMode* zoomMode() const { return _zoomMode; }

	/// \brief Returns the pan input mode.
	PanMode* panMode() const { return _panMode; }

	/// \brief Returns the orbit input mode.
	OrbitMode* orbitMode() const { return _orbitMode; }

	/// \brief Returns the FOV input mode.
	FOVMode* fovMode() const { return _fovMode; }

	/// \brief Returns the pick orbit center input mode.
	PickOrbitCenterMode* pickOrbitCenterMode() const { return _pickOrbitCenterMode; }

	/// \brief Returns the scene node selection mode.
	SelectionMode* selectionMode() const { return _selectionMode; }

	/// \brief Returns the scene node translation mode.
	MoveMode* moveMode() const { return _moveMode; }

	/// \brief Returns the scene node rotation mode.
	RotateMode* rotateMode() const { return _rotateMode; }

public Q_SLOTS:

	/// \brief Resets the input mode stack to its default state.
	///
	/// All input mode are removed from the stack and a default input mode
	/// is activated.
	void reset();

Q_SIGNALS:

	/// \brief This signal is sent when the active viewport input mode has changed.
	/// \param oldMode The previous input handler (can be \c NULL).
	/// \param newMode The new input handler that is now active (can be \c NULL).
	void inputModeChanged(ViewportInputMode* oldMode, ViewportInputMode* newMode);

private:

	/// The dataset container this input manager is associated with,
	DataSetContainer& _datasetContainer;

	/// Pointer to the main window this input manager belongs to (if any).
	MainWindow* _mainWindow;

	/// Stack of input modes. The topmost entry is the active one.
	std::vector<ViewportInputMode*> _inputModeStack;

	/// List of active viewport gizmos.
	std::vector<ViewportGizmo*> _viewportGizmos;

	/// The default viewport input mode.
	ViewportInputMode* _defaultMode;

	/// The zoom input mode.
	ZoomMode* _zoomMode;

	/// The pan input mode.
	PanMode* _panMode;

	/// The orbit input mode.
	OrbitMode* _orbitMode;

	/// The FOV input mode.
	FOVMode* _fovMode;

	/// The pick orbit center input mode.
	PickOrbitCenterMode* _pickOrbitCenterMode;

	/// The default scene node selection mode.
	SelectionMode* _selectionMode;

	/// The scene node translation mode.
	MoveMode* _moveMode;

	/// The scene node rotation mode.
	RotateMode* _rotateMode;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
