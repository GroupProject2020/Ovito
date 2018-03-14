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

#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include <plugins/crystalanalysis/objects/dislocations/DislocationNetworkObject.h>
#include <core/dataset/pipeline/PipelineFlowState.h>
#include <gui/actions/ViewportModeAction.h>
#include <gui/viewport/ViewportWindow.h>
#include <gui/rendering/ViewportSceneRenderer.h>
#include <gui/mainwin/MainWindow.h>
#include "DislocationInspectionApplet.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

IMPLEMENT_OVITO_CLASS(DislocationInspectionApplet);

/******************************************************************************
* Determines whether the given pipeline flow state contains data that can be 
* displayed by this applet.
******************************************************************************/
bool DislocationInspectionApplet::appliesTo(const PipelineFlowState& state)
{
	return state.findObject<DislocationNetworkObject>() != nullptr;
}

/******************************************************************************
* Lets the applet create the UI widget that is to be placed into the data 
* inspector panel. 
******************************************************************************/
QWidget* DislocationInspectionApplet::createWidget(MainWindow* mainWindow)
{
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
	layout->addWidget(toolbar, 0, 0);

	QWidget* pickModeButton = toolbar->widgetForAction(pickModeAction);
	connect(_pickingMode, &ViewportInputMode::statusChanged, pickModeButton, [pickModeButton,this](bool active) {
		if(active) {
			QToolTip::showText(pickModeButton->mapToGlobal(pickModeButton->rect().bottomRight()), 
#ifndef Q_OS_MACX
				tr("Pick a dislocation in the viewports. Hold down the CONTROL key to select multiple dislocations."),
#else
				tr("Pick a dislocation in the viewports. Hold down the COMMAND key to select multiple dislocations."),
#endif
				pickModeButton, QRect(), 2000);
		}
	});
	
	_tableView = new QTableView();
	_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
	_tableModel = new DislocationTableModel(_tableView);
	_tableView->setModel(_tableModel);
	_tableView->setWordWrap(false);
	_tableView->horizontalHeader()->resizeSection(0, 60);
	_tableView->horizontalHeader()->resizeSection(1, 140);
	_tableView->horizontalHeader()->resizeSection(2, 200);
	_tableView->horizontalHeader()->resizeSection(4, 60);
	_tableView->horizontalHeader()->resizeSection(6, 200);
	_tableView->horizontalHeader()->resizeSection(7, 200);
	_tableView->verticalHeader()->hide();
	layout->addWidget(_tableView, 1, 0);
	layout->setRowStretch(1, 1);

	connect(_tableView->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this]() {
		if(_pickingMode->isActive())
			_pickingMode->requestViewportUpdate();
	});

	return panel;
}

/******************************************************************************
* Updates the contents displayed in the inspector.
******************************************************************************/
void DislocationInspectionApplet::updateDisplay(const PipelineFlowState& state, PipelineSceneNode* sceneNode)
{
	_tableModel->setContents(state);
	_sceneNode = sceneNode;
}

/******************************************************************************
* This is called when the applet is no longer visible.
******************************************************************************/
void DislocationInspectionApplet::deactivate(MainWindow* mainWindow)
{
	mainWindow->viewportInputManager()->removeInputMode(_pickingMode);
}

/******************************************************************************
* Handles the mouse up events for a Viewport.
******************************************************************************/
void DislocationInspectionApplet::PickingMode::mouseReleaseEvent(ViewportWindow* vpwin, QMouseEvent* event)
{
	if(event->button() == Qt::LeftButton) {
		int pickedSegment = pickDislocationSegment(vpwin, event->pos());
		if(pickedSegment != -1) {
			if(!event->modifiers().testFlag(Qt::ControlModifier)) {
				_applet->_tableView->selectRow(pickedSegment);
				_applet->_tableView->scrollTo(_applet->_tableView->model()->index(pickedSegment, 0));
			}
			else {
				_applet->_tableView->selectionModel()->select(_applet->_tableView->model()->index(pickedSegment, 0), 
					QItemSelectionModel::Toggle | QItemSelectionModel::Rows);
			}
		}
	}
	ViewportInputMode::mouseReleaseEvent(vpwin, event);
}

/******************************************************************************
* Determines the dislocation segment under the mouse cursor.
******************************************************************************/
int DislocationInspectionApplet::PickingMode::pickDislocationSegment(ViewportWindow* vpwin, const QPoint& pos) const
{
	ViewportPickResult vpPickResult = vpwin->pick(pos);

	// Check if user has clicked on something.
	if(vpPickResult) {
		// Check if that was a dislocation.
		DislocationPickInfo* pickInfo = dynamic_object_cast<DislocationPickInfo>(vpPickResult.pickInfo);
		if(pickInfo && vpPickResult.objectNode == _applet->_sceneNode) {
			int segmentIndex = pickInfo->segmentIndexFromSubObjectID(vpPickResult.subobjectId);
			if(segmentIndex >= 0 && segmentIndex < pickInfo->dislocationObj()->segments().size()) {
				return segmentIndex;
			}
		}
	}

	return -1;
}

/******************************************************************************
* Handles the mouse move event for the given viewport.
******************************************************************************/
void DislocationInspectionApplet::PickingMode::mouseMoveEvent(ViewportWindow* vpwin, QMouseEvent* event)
{
	// Change mouse cursor while hovering over a dislocation.
	if(pickDislocationSegment(vpwin, event->pos()) != -1)
		setCursor(SelectionMode::selectionCursor());
	else
		setCursor(QCursor());

	ViewportInputMode::mouseMoveEvent(vpwin, event);
}

/******************************************************************************
* Lets the input mode render its overlay content in a viewport.
******************************************************************************/
void DislocationInspectionApplet::PickingMode::renderOverlay3D(Viewport* vp, ViewportSceneRenderer* renderer)
{
	if(!_applet->_sceneNode) return;

	const PipelineFlowState& flowState = _applet->_sceneNode->evaluatePipelinePreliminary(true);
	DislocationNetworkObject* dislocationObj = flowState.findObject<DislocationNetworkObject>();
	if(!dislocationObj) return;
	DislocationVis* vis = dynamic_object_cast<DislocationVis>(dislocationObj->visElement());
	if(!vis) return;
	
	for(const QModelIndex& index : _applet->_tableView->selectionModel()->selectedRows()) {
		int segmentIndex = index.row();
		if(segmentIndex >= 0 && segmentIndex < dislocationObj->segments().size())
			vis->renderOverlayMarker(vp->dataset()->animationSettings()->time(), dislocationObj, flowState, segmentIndex, renderer, _applet->_sceneNode);
	}
}

}	// End of namespace
}	// End of namespace
}	// End of namespace
