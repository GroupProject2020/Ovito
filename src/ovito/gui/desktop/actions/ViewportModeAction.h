/////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2013 Alexander Stukowski
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


#include <ovito/gui/desktop/GUI.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(ViewportInput)

/**
 * An Qt action that activates a ViewportInputMode.
 */
class OVITO_GUI_EXPORT ViewportModeAction : public QAction
{
	Q_OBJECT

public:

	/// \brief Initializes the action object.
	ViewportModeAction(MainWindow* mainWindow, const QString& text, QObject* parent, ViewportInputMode* inputMode, const QColor& highlightColor = QColor());

	/// \brief Create a push button that activates this action.
	QPushButton* createPushButton(QWidget* parent = nullptr);

public Q_SLOTS:

	/// \brief Activates the viewport input mode.
	void activateMode() {
		onActionToggled(true);
	}

	/// \brief Deactivates the viewport input mode.
	void deactivateMode() {
		onActionToggled(false);
	}

protected Q_SLOTS:

	/// Is called when the user or the program have triggered the action's state.
	void onActionToggled(bool checked);

	/// Is called when the user has triggered the action's state.
	void onActionTriggered(bool checked);

private:

	/// The viewport input mode activated by this action.
	ViewportInputMode* _inputMode;

	/// The highlight color for the button controls.
	QColor _highlightColor;

	/// The viewport input manager.
	ViewportInputManager& _viewportInputManager;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace


