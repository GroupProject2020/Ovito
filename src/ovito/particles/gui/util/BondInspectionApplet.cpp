////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2018 Alexander Stukowski
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

#include <ovito/particles/gui/ParticlesGui.h>
#include <ovito/particles/objects/BondsObject.h>
#include <ovito/gui/desktop/actions/ViewportModeAction.h>
#include <ovito/gui/desktop/mainwin/MainWindow.h>
#include <ovito/gui/desktop/widgets/general/AutocompleteLineEdit.h>
#include "BondInspectionApplet.h"

namespace Ovito { namespace Particles {

IMPLEMENT_OVITO_CLASS(BondInspectionApplet);

/******************************************************************************
* Lets the applet create the UI widget that is to be placed into the data
* inspector panel.
******************************************************************************/
QWidget* BondInspectionApplet::createWidget(MainWindow* mainWindow)
{
	createBaseWidgets();

	QWidget* panel = new QWidget();
	QGridLayout* layout = new QGridLayout(panel);
	layout->setContentsMargins(0,0,0,0);
	layout->setSpacing(0);

	_pickingMode = new PickingMode(this);
	connect(this, &QObject::destroyed, _pickingMode, &ViewportInputMode::removeMode);
	ViewportModeAction* pickModeAction = new ViewportModeAction(mainWindow, tr("Select in viewports"), this, _pickingMode);
	pickModeAction->setIcon(QIcon(":/particles/icons/select_mode.svg"));

	QToolBar* toolbar = new QToolBar();
	toolbar->setOrientation(Qt::Horizontal);
	toolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);
	toolbar->setIconSize(QSize(18,18));
	toolbar->setStyleSheet("QToolBar { padding: 0px; margin: 0px; border: 0px none black; spacing: 0px; }");
	toolbar->addAction(pickModeAction);
	toolbar->addAction(resetFilterAction());
	layout->addWidget(toolbar, 0, 0);

	layout->addWidget(filterExpressionEdit(), 0, 1);
	layout->addWidget(tableView(), 1, 0, 1, 2);
	layout->setRowStretch(1, 1);

	QWidget* pickModeButton = toolbar->widgetForAction(pickModeAction);
	connect(_pickingMode, &ViewportInputMode::statusChanged, pickModeButton, [pickModeButton,this](bool active) {
		if(active) {
			QToolTip::showText(pickModeButton->mapToGlobal(pickModeButton->rect().bottomRight()),
#ifndef Q_OS_MACX
				tr("Pick a bond in the viewports. Hold down the CONTROL key to select multiple bonds."),
#else
				tr("Pick a bond in the viewports. Hold down the COMMAND key to select multiple bonds."),
#endif
				pickModeButton, QRect(), 2000);
		}
	});

	connect(filterExpressionEdit(), &AutocompleteLineEdit::editingFinished, this, [this]() {
		_pickingMode->resetSelection();
	});

	return panel;
}

/******************************************************************************
* Updates the contents displayed in the inspector.
******************************************************************************/
void BondInspectionApplet::updateDisplay(const PipelineFlowState& state, PipelineSceneNode* sceneNode)
{
	// Clear selection when a different scene node has been selected.
	if(sceneNode != currentSceneNode())
		_pickingMode->resetSelection();

	PropertyInspectionApplet::updateDisplay(state, sceneNode);
}

/******************************************************************************
* This is called when the applet is no longer visible.
******************************************************************************/
void BondInspectionApplet::deactivate(MainWindow* mainWindow)
{
	_pickingMode->removeMode();
}

/******************************************************************************
* Handles the mouse up events for a Viewport.
******************************************************************************/
void BondInspectionApplet::PickingMode::mouseReleaseEvent(ViewportWindowInterface* vpwin, QMouseEvent* event)
{
	if(event->button() == Qt::LeftButton) {
		PickResult pickResult;
		pickBond(vpwin, event->pos(), pickResult);
		if(!event->modifiers().testFlag(Qt::ControlModifier))
			_pickedElements.clear();
		if(pickResult.sceneNode == _applet->currentSceneNode()) {
			// Don't select the same bond twice. Instead, toggle selection.
			bool alreadySelected = false;
			for(auto p = _pickedElements.begin(); p != _pickedElements.end(); ++p) {
				if(p->sceneNode == pickResult.sceneNode && p->bondIndex == pickResult.bondIndex) {
					alreadySelected = true;
					_pickedElements.erase(p);
					break;
				}
			}
			if(!alreadySelected)
				_pickedElements.push_back(pickResult);
		}
		QString filterExpression;
		for(const auto& element : _pickedElements) {
			if(!filterExpression.isEmpty()) filterExpression += QStringLiteral(" ||\n");
			filterExpression += QStringLiteral("BondIndex==%1").arg(element.bondIndex);
		}
		_applet->setFilterExpression(filterExpression);
		requestViewportUpdate();
	}
	ViewportInputMode::mouseReleaseEvent(vpwin, event);
}

/******************************************************************************
* Handles the mouse move event for the given viewport.
******************************************************************************/
void BondInspectionApplet::PickingMode::mouseMoveEvent(ViewportWindowInterface* vpwin, QMouseEvent* event)
{
	// Change mouse cursor while hovering over a bond.
	PickResult pickResult;
	if(pickBond(vpwin, event->pos(), pickResult) && pickResult.sceneNode == _applet->currentSceneNode())
		setCursor(SelectionMode::selectionCursor());
	else
		setCursor(QCursor());

	ViewportInputMode::mouseMoveEvent(vpwin, event);
}

}	// End of namespace
}	// End of namespace
