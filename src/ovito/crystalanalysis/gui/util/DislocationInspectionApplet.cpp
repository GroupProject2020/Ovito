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

#include <ovito/crystalanalysis/CrystalAnalysis.h>
#include <ovito/crystalanalysis/objects/DislocationNetworkObject.h>
#include <ovito/crystalanalysis/objects/Microstructure.h>
#include <ovito/core/dataset/pipeline/PipelineFlowState.h>
#include <ovito/gui/actions/ViewportModeAction.h>
#include <ovito/gui/viewport/ViewportWindow.h>
#include <ovito/gui/rendering/ViewportSceneRenderer.h>
#include <ovito/gui/mainwin/MainWindow.h>
#include "DislocationInspectionApplet.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

IMPLEMENT_OVITO_CLASS(DislocationInspectionApplet);

/******************************************************************************
* Determines whether the given pipeline dataset contains data that can be
* displayed by this applet.
******************************************************************************/
bool DislocationInspectionApplet::appliesTo(const DataCollection& data)
{
	return data.containsObject<DislocationNetworkObject>() || data.containsObject<Microstructure>();
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
	connect(this, &QObject::destroyed, _pickingMode, &ViewportInputMode::removeMode);
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

	_tableView = new TableView();
	_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
	_tableModel = new DislocationTableModel(_tableView);
	_tableView->setModel(_tableModel);
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
	_pickingMode->removeMode();
}

/******************************************************************************
* Returns the data stored under the given 'role' for the item referred to by the 'index'.
******************************************************************************/
QVariant DislocationInspectionApplet::DislocationTableModel::data(const QModelIndex& index, int role) const
{
	if(role == Qt::DisplayRole) {
		if(_dislocationObj) {
			DislocationSegment* segment = _dislocationObj->segments()[index.row()];
			switch(index.column()) {
			case 0: return _dislocationObj->segments()[index.row()]->id;
			case 1: return DislocationVis::formatBurgersVector(segment->burgersVector.localVec(), _dislocationObj->structureById(segment->burgersVector.cluster()->structure));
			case 2: { Vector3 b = segment->burgersVector.toSpatialVector();
					return QStringLiteral("%1 %2 %3")
								.arg(QLocale::c().toString(b.x(), 'f', 4), 7)
								.arg(QLocale::c().toString(b.y(), 'f', 4), 7)
								.arg(QLocale::c().toString(b.z(), 'f', 4), 7); }
			case 3: return QLocale::c().toString(segment->calculateLength());
			case 4: return segment->burgersVector.cluster()->id;
			case 5: return _dislocationObj->structureById(segment->burgersVector.cluster()->structure)->name();
			case 6: { Point3 headLocation = segment->backwardNode().position();
						if(_dislocationObj->domain()) headLocation = _dislocationObj->domain()->data().wrapPoint(headLocation);
						return QStringLiteral("%1 %2 %3")
							.arg(QLocale::c().toString(headLocation.x(), 'f', 4), 7)
							.arg(QLocale::c().toString(headLocation.y(), 'f', 4), 7)
							.arg(QLocale::c().toString(headLocation.z(), 'f', 4), 7); }
			case 7: { Point3 tailLocation = segment->forwardNode().position();
						if(_dislocationObj->domain()) tailLocation = _dislocationObj->domain()->data().wrapPoint(tailLocation);
						return QStringLiteral("%1 %2 %3")
							.arg(QLocale::c().toString(tailLocation.x(), 'f', 4), 7)
							.arg(QLocale::c().toString(tailLocation.y(), 'f', 4), 7)
							.arg(QLocale::c().toString(tailLocation.z(), 'f', 4), 7); }
			}
		}
		else if(_microstructure) {
			const PropertyObject* burgersVectorProperty = _microstructure->faces()->getProperty(SurfaceMeshFaces::BurgersVectorProperty);
			const PropertyObject* faceRegionProperty = _microstructure->faces()->getProperty(SurfaceMeshFaces::RegionProperty);
			const PropertyObject* phaseProperty = _microstructure->regions()->getProperty(SurfaceMeshRegions::PhaseProperty);
			if(burgersVectorProperty && faceRegionProperty && phaseProperty && index.row() < burgersVectorProperty->size()) {
				const MicrostructurePhase* phase = nullptr;
				int region = faceRegionProperty->getInt(index.row());
				if(region >= 0 && region < phaseProperty->size()) {
					int phaseId = phaseProperty->getInt(region);
					if(const MicrostructurePhase* phase = dynamic_object_cast<MicrostructurePhase>(phaseProperty->elementType(phaseId))) {
						switch(index.column()) {
						case 0: return index.row();
						case 1: return DislocationVis::formatBurgersVector(burgersVectorProperty->getVector3(index.row()), phase);
						case 2:
							if(const PropertyObject* correspondenceProperty = _microstructure->regions()->getProperty(SurfaceMeshRegions::LatticeCorrespondenceProperty)) {
								Vector3 transformedVector = correspondenceProperty->getMatrix3(region) * burgersVectorProperty->getVector3(index.row());
								return QStringLiteral("%1 %2 %3")
										.arg(QLocale::c().toString(transformedVector.x(), 'f', 4), 7)
										.arg(QLocale::c().toString(transformedVector.y(), 'f', 4), 7)
										.arg(QLocale::c().toString(transformedVector.z(), 'f', 4), 7);
							}
							break;
						case 4: return region;
						case 5: return phase->name();
						}
					}
				}
			}
		}
	}
	else if(role == Qt::DecorationRole && index.column() == 1) {
		if(_dislocationObj) {
			DislocationSegment* segment = _dislocationObj->segments()[index.row()];
			MicrostructurePhase* crystalStructure = _dislocationObj->structureById(segment->burgersVector.cluster()->structure);
			BurgersVectorFamily* family = crystalStructure->defaultBurgersVectorFamily();
			for(BurgersVectorFamily* f : crystalStructure->burgersVectorFamilies()) {
				if(f->isMember(segment->burgersVector.localVec(), crystalStructure)) {
					family = f;
					break;
				}
			}
			if(family) return (QColor)family->color();
		}
	}
	return {};
}

/******************************************************************************
* Handles the mouse up events for a Viewport.
******************************************************************************/
void DislocationInspectionApplet::PickingMode::mouseReleaseEvent(ViewportWindow* vpwin, QMouseEvent* event)
{
	if(event->button() == Qt::LeftButton) {
		int pickedDislocationIndex = pickDislocation(vpwin, event->pos());
		if(pickedDislocationIndex != -1) {
			if(!event->modifiers().testFlag(Qt::ControlModifier)) {
				_applet->_tableView->selectRow(pickedDislocationIndex);
				_applet->_tableView->scrollTo(_applet->_tableView->model()->index(pickedDislocationIndex, 0));
			}
			else {
				_applet->_tableView->selectionModel()->select(_applet->_tableView->model()->index(pickedDislocationIndex, 0),
					QItemSelectionModel::Toggle | QItemSelectionModel::Rows);
			}
		}
	}
	ViewportInputMode::mouseReleaseEvent(vpwin, event);
}

/******************************************************************************
* Determines the dislocation under the mouse cursor.
******************************************************************************/
int DislocationInspectionApplet::PickingMode::pickDislocation(ViewportWindow* vpwin, const QPoint& pos) const
{
	ViewportPickResult vpPickResult = vpwin->pick(pos);

	// Check if user has clicked on something.
	if(vpPickResult.isValid()) {
		// Check if that was a dislocation.
		DislocationPickInfo* pickInfo = dynamic_object_cast<DislocationPickInfo>(vpPickResult.pickInfo());
		if(pickInfo && vpPickResult.pipelineNode() == _applet->_sceneNode) {
			return pickInfo->segmentIndexFromSubObjectID(vpPickResult.subobjectId());
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
	if(pickDislocation(vpwin, event->pos()) != -1)
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
	const DislocationNetworkObject* dislocationObj = flowState.getObject<DislocationNetworkObject>();
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
