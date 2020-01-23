////////////////////////////////////////////////////////////////////////////////////////
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

#include <ovito/gui/desktop/GUI.h>
#include <ovito/gui/desktop/actions/ViewportModeAction.h>
#include <ovito/gui/desktop/mainwin/MainWindow.h>
#include <ovito/gui/base/viewport/ViewportInputManager.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(ViewportInput)

/******************************************************************************
* Initializes the action object.
******************************************************************************/
ViewportModeAction::ViewportModeAction(MainWindow* mainWindow, const QString& text, QObject* parent, ViewportInputMode* inputMode, const QColor& highlightColor)
	: QAction(text, parent), _inputMode(inputMode), _highlightColor(highlightColor), _viewportInputManager(*mainWindow->viewportInputManager())
{
	OVITO_CHECK_POINTER(inputMode);

	setCheckable(true);
	setChecked(inputMode->isActive());

	connect(inputMode, &ViewportInputMode::statusChanged, this, &ViewportModeAction::setChecked);
	connect(this, &ViewportModeAction::toggled, this, &ViewportModeAction::onActionToggled);
	connect(this, &ViewportModeAction::triggered, this, &ViewportModeAction::onActionTriggered);
}

/******************************************************************************
* Is called when the user or the program have triggered the action's state.
******************************************************************************/
void ViewportModeAction::onActionToggled(bool checked)
{
	// Activate/deactivate the input mode.
	if(checked && !_inputMode->isActive()) {
		_viewportInputManager.pushInputMode(_inputMode);
	}
	else if(!checked) {
		if(_viewportInputManager.activeMode() == _inputMode && _inputMode->modeType() == ViewportInputMode::ExclusiveMode) {
			// Make sure that an exclusive input mode cannot be deactivated by the user.
			setChecked(true);
		}
	}
}

/******************************************************************************
* Is called when the user has triggered the action's state.
******************************************************************************/
void ViewportModeAction::onActionTriggered(bool checked)
{
	if(!checked) {
		if(_inputMode->modeType() != ViewportInputMode::ExclusiveMode) {
			_viewportInputManager.removeInputMode(_inputMode);
		}
	}
}

/******************************************************************************
* Create a push button that activates this action.
******************************************************************************/
QPushButton* ViewportModeAction::createPushButton(QWidget* parent)
{
	// Define a specialized QPushButton class, which will automatically deactive the viewport input
	// mode wheneven the button widget is hidden. This is to prevent the viewport mode from remaining
	// active when the user switches to another command panel tab.
	class MyPushButton : public QPushButton {
	public:
		using QPushButton::QPushButton;
	protected:
		virtual void hideEvent(QHideEvent* event) override {
			if(!event->spontaneous() && isChecked()) click();
			QPushButton::hideEvent(event);
		}
	};

	QPushButton* button = new MyPushButton(text(), parent);
	button->setCheckable(true);
	button->setChecked(isChecked());

#ifndef Q_OS_MACX
	if(_highlightColor.isValid())
		button->setStyleSheet("QPushButton:checked { background-color: " + _highlightColor.name() + " }");
	else
		button->setStyleSheet("QPushButton:checked { background-color: moccasin; }");
#endif

	connect(this, &ViewportModeAction::toggled, button, &QPushButton::setChecked);
	connect(button, &QPushButton::clicked, this, &ViewportModeAction::trigger);
	return button;
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
