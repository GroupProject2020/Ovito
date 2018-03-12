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
#include <plugins/particles/objects/ParticleProperty.h>
#include <gui/mainwin/MainWindow.h>
#include <gui/actions/ViewportModeAction.h>
#include <gui/widgets/general/AutocompleteLineEdit.h>
#include <gui/viewport/ViewportWindow.h>
#include "ParticleInspectionApplet.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_CLASS(ParticleInspectionApplet);

/******************************************************************************
* Lets the applet create the UI widget that is to be placed into the data 
* inspector panel. 
******************************************************************************/
QWidget* ParticleInspectionApplet::createWidget(MainWindow* mainWindow)
{
	createBaseWidgets();

	QWidget* panel = new QWidget();
	QGridLayout* layout = new QGridLayout(panel);
	layout->setContentsMargins(0,0,0,0);
	layout->setSpacing(0);

	_pickingMode = new PickingMode(this);
	ViewportModeAction* pickModeAction = new ViewportModeAction(mainWindow, tr("Select in viewports"), this, _pickingMode);	
	pickModeAction->setIcon(QIcon(":/particles/icons/select_mode.svg"));

	_measuringModeAction = new QAction(QIcon(":/particles/icons/measure_distances.svg"), tr("Show distances"), this);
	_measuringModeAction->setCheckable(true);

	QToolBar* toolbar = new QToolBar();
	toolbar->setOrientation(Qt::Horizontal);
	toolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);
	toolbar->setIconSize(QSize(18,18));
	toolbar->setStyleSheet("QToolBar { padding: 0px; margin: 0px; border: 0px none black; spacing: 0px; }");
	toolbar->addAction(pickModeAction);
	toolbar->addAction(_measuringModeAction);
	toolbar->addAction(resetFilterAction());
	layout->addWidget(toolbar, 0, 0);

	QWidget* pickModeButton = toolbar->widgetForAction(pickModeAction);
	connect(_pickingMode, &ViewportInputMode::statusChanged, pickModeButton, [pickModeButton,this](bool active) {
		if(active) {
			QToolTip::showText(pickModeButton->mapToGlobal(pickModeButton->rect().bottomRight()), 
#ifndef Q_OS_MACX
				tr("Pick a particle in the viewports. Hold down the CONTROL key to select multiple particles."),
#else
				tr("Pick a particle in the viewports. Hold down the COMMAND key to select multiple particles."),
#endif
				pickModeButton, QRect(), 2000);
		}
	});

	layout->addWidget(filterExpressionEdit(), 0, 1);
	QHBoxLayout* sublayout = new QHBoxLayout();
	sublayout->setContentsMargins(0,0,0,0);
	sublayout->setSpacing(4);
	sublayout->addWidget(tableView(), 2);
	layout->addLayout(sublayout, 1, 0, 1, 2);
	layout->setRowStretch(1, 1);

	_distanceTable = new QTableWidget(0, 3);
	_distanceTable->hide();
	_distanceTable->setHorizontalHeaderLabels(QStringList() << tr("Particle 1") << tr("Particle 2") << tr("Distance")); 
	_distanceTable->horizontalHeader()->setStretchLastSection(true);
	_distanceTable->verticalHeader()->hide();
	sublayout->addWidget(_distanceTable, 1);

	connect(filterExpressionEdit(), &AutocompleteLineEdit::editingFinished, this, [this]() {
		_pickingMode->resetSelection();
	});
	connect(_measuringModeAction, &QAction::toggled, _distanceTable, &QWidget::setVisible);
	connect(_measuringModeAction, &QAction::toggled, this, &ParticleInspectionApplet::updateDistanceTable);
	connect(this, &PropertyInspectionApplet::filterChanged, this, &ParticleInspectionApplet::updateDistanceTable);

	return panel;
}

/******************************************************************************
* Updates the contents displayed in the inspector.
******************************************************************************/
void ParticleInspectionApplet::updateDisplay(const PipelineFlowState& state, PipelineSceneNode* sceneNode)
{
	// Clear selection when a different scene node has been selected.
	if(sceneNode != currentSceneNode())
		_pickingMode->resetSelection();
		
	PropertyInspectionApplet::updateDisplay(state, sceneNode);

	if(_measuringModeAction->isChecked())
		updateDistanceTable();
}

/******************************************************************************
* Computes the inter-particle distances for the selected particles.
******************************************************************************/
void ParticleInspectionApplet::updateDistanceTable()
{
	// Limit distance computation to the first 4 particles:
	int n = std::min(4, visibleElementCount());

	ParticleProperty* posProperty = ParticleProperty::findInState(currentData(), ParticleProperty::PositionProperty);
	_distanceTable->setRowCount(n * (n-1) / 2);
	int row = 0;
	for(int i = 0; i < n; i++) {
		size_t i_index = visibleElementAt(i);
		for(int j = i+1; j < n; j++, row++) {
			size_t j_index = visibleElementAt(j);
			_distanceTable->setItem(row, 0, new QTableWidgetItem(QString::number(i_index)));
			_distanceTable->setItem(row, 1, new QTableWidgetItem(QString::number(j_index)));
			if(posProperty && i_index < posProperty->size() && j_index < posProperty->size()) {
				Vector3 delta = posProperty->getPoint3(j_index) - posProperty->getPoint3(i_index);
				_distanceTable->setItem(row, 2, new QTableWidgetItem(QString::number(delta.length())));
			}
		}
	}
}

/******************************************************************************
* This is called when the applet is no longer visible.
******************************************************************************/
void ParticleInspectionApplet::deactivate(MainWindow* mainWindow)
{
	mainWindow->viewportInputManager()->removeInputMode(_pickingMode);
}

/******************************************************************************
* Handles the mouse up events for a Viewport.
******************************************************************************/
void ParticleInspectionApplet::PickingMode::mouseReleaseEvent(ViewportWindow* vpwin, QMouseEvent* event)
{
	if(event->button() == Qt::LeftButton) {
		PickResult pickResult;
		pickParticle(vpwin, event->pos(), pickResult);
		if(!event->modifiers().testFlag(Qt::ControlModifier))
			_pickedElements.clear();
		if(pickResult.objNode == _applet->currentSceneNode()) {
			// Don't select the same particle twice. Instead, toggle selection.
			bool alreadySelected = false;
			for(auto p = _pickedElements.begin(); p != _pickedElements.end(); ++p) {
				if(p->objNode == pickResult.objNode && p->particleIndex == pickResult.particleIndex) {
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
			if(element.particleId >= 0)
				filterExpression += QStringLiteral("ParticleIdentifier==%1").arg(element.particleId);
			else
				filterExpression += QStringLiteral("ParticleIndex==%1").arg(element.particleIndex);
		}
		_applet->setFilterExpression(filterExpression);
		requestViewportUpdate();
	}
	ViewportInputMode::mouseReleaseEvent(vpwin, event);
}

/******************************************************************************
* Handles the mouse move event for the given viewport.
******************************************************************************/
void ParticleInspectionApplet::PickingMode::mouseMoveEvent(ViewportWindow* vpwin, QMouseEvent* event)
{
	// Change mouse cursor while hovering over a particle.
	PickResult pickResult;
	if(pickParticle(vpwin, event->pos(), pickResult) && pickResult.objNode == _applet->currentSceneNode())
		setCursor(SelectionMode::selectionCursor());
	else
		setCursor(QCursor());

	ViewportInputMode::mouseMoveEvent(vpwin, event);
}

/******************************************************************************
* Lets the input mode render its overlay content in a viewport.
******************************************************************************/
void ParticleInspectionApplet::PickingMode::renderOverlay3D(Viewport* vp, ViewportSceneRenderer* renderer)
{
	for(const auto& element : _pickedElements)
		renderSelectionMarker(vp, renderer, element);
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
