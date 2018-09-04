///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2018) Alexander Stukowski
//
//  This file is part of OVITO (Open Visualization Tool).
//
//  OVITO is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  OVITO is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////

#include <plugins/particles/gui/ParticlesGui.h>
#include <plugins/particles/objects/BondsObject.h>
#include <gui/actions/ViewportModeAction.h>
#include <gui/mainwin/MainWindow.h>
#include <gui/widgets/general/AutocompleteLineEdit.h>
#include "BondInspectionApplet.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

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
	mainWindow->viewportInputManager()->removeInputMode(_pickingMode);
}

/******************************************************************************
* Handles the mouse up events for a Viewport.
******************************************************************************/
void BondInspectionApplet::PickingMode::mouseReleaseEvent(ViewportWindow* vpwin, QMouseEvent* event)
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
void BondInspectionApplet::PickingMode::mouseMoveEvent(ViewportWindow* vpwin, QMouseEvent* event)
{
	// Change mouse cursor while hovering over a bond.
	PickResult pickResult;
	if(pickBond(vpwin, event->pos(), pickResult) && pickResult.sceneNode == _applet->currentSceneNode())
		setCursor(SelectionMode::selectionCursor());
	else
		setCursor(QCursor());

	ViewportInputMode::mouseMoveEvent(vpwin, event);
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
