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

#include <ovito/crystalanalysis/CrystalAnalysis.h>
#include <ovito/crystalanalysis/objects/DislocationNetworkObject.h>
#include <ovito/crystalanalysis/objects/Microstructure.h>
#include <ovito/core/dataset/pipeline/PipelineFlowState.h>
#include <ovito/gui/desktop/actions/ViewportModeAction.h>
#include <ovito/core/viewport/ViewportWindowInterface.h>
#include <ovito/gui/base/rendering/ViewportSceneRenderer.h>
#include <ovito/gui/desktop/mainwin/MainWindow.h>
#include "DislocationInspectionApplet.h"

namespace Ovito { namespace CrystalAnalysis {

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
			ConstPropertyAccess<Vector3> burgersVectorProperty = _microstructure->faces()->getProperty(SurfaceMeshFaces::BurgersVectorProperty);
			ConstPropertyAccess<int> faceRegionProperty = _microstructure->faces()->getProperty(SurfaceMeshFaces::RegionProperty);
			const PropertyObject* phaseProperty = _microstructure->regions()->getProperty(SurfaceMeshRegions::PhaseProperty);
			ConstPropertyAccess<int> phaseArray(faceRegionProperty);
			if(burgersVectorProperty && faceRegionProperty && phaseProperty && index.row() < burgersVectorProperty.size()) {
				const MicrostructurePhase* phase = nullptr;
				int region = faceRegionProperty[index.row()];
				if(region >= 0 && region < phaseArray.size()) {
					int phaseId = phaseArray[region];
					if(const MicrostructurePhase* phase = dynamic_object_cast<MicrostructurePhase>(phaseProperty->elementType(phaseId))) {
						switch(index.column()) {
						case 0: return index.row();
						case 1: return DislocationVis::formatBurgersVector(burgersVectorProperty[index.row()], phase);
						case 2:
							if(ConstPropertyAccess<Matrix3> correspondenceProperty = _microstructure->regions()->getProperty(SurfaceMeshRegions::LatticeCorrespondenceProperty)) {
								Vector3 transformedVector = correspondenceProperty[region] * burgersVectorProperty[index.row()];
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
void DislocationInspectionApplet::PickingMode::mouseReleaseEvent(ViewportWindowInterface* vpwin, QMouseEvent* event)
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
int DislocationInspectionApplet::PickingMode::pickDislocation(ViewportWindowInterface* vpwin, const QPoint& pos) const
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
void DislocationInspectionApplet::PickingMode::mouseMoveEvent(ViewportWindowInterface* vpwin, QMouseEvent* event)
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
void DislocationInspectionApplet::PickingMode::renderOverlay3D(Viewport* vp, SceneRenderer* renderer)
{
	if(!_applet->_sceneNode) return;

	const PipelineFlowState& flowState = _applet->_sceneNode->evaluatePipelineSynchronous(true);
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
