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


#include <ovito/gui/base/GUIBase.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(ViewportInput)

/**
 * \brief Abstract base class for viewport input modes that handle mouse input
 *        in the viewports.
 *
 * The ViewportInputManager keeps a stack of ViewportInputMode objects.
 * The topmost handler is the active one and handles all mouse events for the viewports.
 */
class OVITO_GUIBASE_EXPORT ViewportInputMode : public QObject
{
	Q_OBJECT

public:

	/// \brief These are the activation behavior types for input modes.
	enum InputModeType {
		NormalMode,				///< The mode is temporarily suspended when another mode becomes active.
		TemporaryMode,			///< The mode is completely removed from the stack when another mode becomes active.
		ExclusiveMode			///< The stack is cleared before the mode becomes active.
	};

	/// \brief Constructor.
	ViewportInputMode(QObject* parent = nullptr) : QObject(parent) {}

	/// \brief Destructor.
	virtual ~ViewportInputMode();

	/// \brief Returns a pointer to the viewport input manager that has a reference to this mode.
	ViewportInputManager* inputManager() const {
		OVITO_ASSERT_MSG(_manager != nullptr, "ViewportInputMode::inputManager()", "Cannot access input manager while mode is not on the input stack.");
		return _manager;
	}

	/// \brief Checks whether this mode is currently active.
	bool isActive() const;

	/// \brief Returns the activation behavior of this input mode.
	/// \return The activation type controls what happens when the mode is activated and deactivated.
	///         The returned value is used by the ViewportInputManager when managing the stack of modes.
	///
	/// The default implementation returns InputModeType::NormalMode.
	virtual InputModeType modeType() { return NormalMode; }

	/// \brief Handles mouse press events for a Viewport.
	/// \param vpwin The viewport window in which the mouse event occurred.
	/// \param event The mouse event.
	///
	/// The default implementation of this method deactivates the
	/// input handler when the user presses the right mouse button.
	/// It also activates temporary viewport navigation modes like
	/// pan, zoom and orbit when the user uses the corresponding
	/// mouse+key combination.
	virtual void mousePressEvent(ViewportWindowInterface* vpwin, QMouseEvent* event);

	/// \brief Handles mouse release events for a Viewport.
	/// \param vpwin The viewport window in which the mouse event occurred.
	/// \param event The mouse event.
	///
	/// The default implementation deactivates any
	/// temporary viewport navigation mode like pan, zoom and orbit
	/// when they have been activated by the mousePressEvent() method.
	virtual void mouseReleaseEvent(ViewportWindowInterface* vpwin, QMouseEvent* event);

	/// \brief Handles mouse move events for a Viewport.
	/// \param vpwin The viewport window in which the mouse event occurred.
	/// \param event The mouse event.
	///
	/// The default implementation delegates the event to the
	/// temporary viewport navigation mode like pan, zoom and orbit
	/// when it has been activated in the mousePressEvent() method.
	virtual void mouseMoveEvent(ViewportWindowInterface* vpwin, QMouseEvent* event);

	/// \brief Handles mouse wheel events for a Viewport.
	/// \param vpwin The viewport window in which the mouse event occurred.
	/// \param event The mouse event.
	///
	/// The default implementation zooms in or out according to the wheel rotation.
	virtual void wheelEvent(ViewportWindowInterface* vpwin, QWheelEvent* event);

	/// \brief Handles double click events for a Viewport.
	/// \param vpwin The viewport window in which the mouse event occurred.
	/// \param event The mouse event.
	virtual void mouseDoubleClickEvent(ViewportWindowInterface* vpwin, QMouseEvent* event);

	/// \brief Is called when a viewport looses the input focus.
	/// \param vpwin The viewport window.
	/// \param event The focus event.
	virtual void focusOutEvent(ViewportWindowInterface* vpwin, QFocusEvent* event);

	/// \brief Return the mouse cursor shown in the viewport windows
	///        while this input handler is active.
	const QCursor& cursor() { return _cursor; }

	/// \brief Sets the mouse cursor shown in the viewport windows
	///        while this input handler is active.
	void setCursor(const QCursor& cursor);

	/// \brief Activates the given temporary navigation mode.
	///
	/// This method can be overridden by subclasses to prevent the activation of temporary navigation modes.
	virtual void activateTemporaryNavigationMode(ViewportInputMode* navigationMode);

	/// \brief Redraws all viewports.
	void requestViewportUpdate();

public Q_SLOTS:

	/// Removes this input mode from the mode stack of the ViewportInputManager.
	void removeMode();

Q_SIGNALS:

	/// \brief This signal is emitted when the input mode has become the active mode or is no longer the active mode.
	void statusChanged(bool isActive);

	/// \brief This signal is emitted when the current curser of this mode has changed.
	void curserChanged(const QCursor& cursor);

protected:

	/// \brief This is called by the system after the input handler has
	///        become the active handler.
	///
	/// Implementations of this virtual method in sub-classes should call the base implementation.
	virtual void activated(bool temporaryActivation);

	/// \brief This is called by the system after the input handler is
	///        no longer the active handler.
	///
	/// Implementations of this virtual method in sub-classes should call the base implementation.
	virtual void deactivated(bool temporary);

private:

	/// Stores a copy of the last mouse-press event.
	std::unique_ptr<QMouseEvent> _lastMousePressEvent;

	/// The cursor shown while this mode is active.
	QCursor _cursor;

	/// The viewport input manager that has a reference to this mode.
	ViewportInputManager* _manager = nullptr;

	friend class ViewportInputManager;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
