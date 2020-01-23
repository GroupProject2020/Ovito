////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2019 Alexander Stukowski
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
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/stdobj/properties/PropertyAccess.h>
#include <ovito/gui/desktop/mainwin/MainWindow.h>
#include <ovito/gui/desktop/actions/ViewportModeAction.h>
#include <ovito/gui/desktop/widgets/general/AutocompleteLineEdit.h>
#include <ovito/gui/viewport/ViewportWindow.h>
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
	connect(this, &QObject::destroyed, _pickingMode, &ViewportInputMode::removeMode);
	ViewportModeAction* pickModeAction = new ViewportModeAction(mainWindow, tr("Select in viewports"), this, _pickingMode);
	pickModeAction->setIcon(QIcon(":/particles/icons/select_mode.svg"));

	_measuringModeAction = new QAction(QIcon(":/particles/icons/measure_distances.svg"), tr("Show distances and angles"), this);
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
	QSplitter* splitter = new QSplitter();
	splitter->setChildrenCollapsible(false);
	splitter->addWidget(tableView());
	layout->addWidget(splitter, 1, 0, 1, 2);
	layout->setRowStretch(1, 1);

	_distanceTable = new QTableWidget(0, 3);
	_distanceTable->hide();
	_distanceTable->setHorizontalHeaderLabels(QStringList() << tr("Pair A-B") << tr("Distance") << tr("Vector"));
	_distanceTable->horizontalHeader()->setStretchLastSection(true);
	_distanceTable->verticalHeader()->hide();
	_distanceTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
	splitter->addWidget(_distanceTable);

	_angleTable = new QTableWidget(0, 2);
	_angleTable->hide();
	_angleTable->setHorizontalHeaderLabels(QStringList() << tr("Triplet A-B-C") << tr("Angle"));
	_angleTable->horizontalHeader()->setStretchLastSection(true);
	_angleTable->verticalHeader()->hide();
	_angleTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
	splitter->addWidget(_angleTable);

	connect(filterExpressionEdit(), &AutocompleteLineEdit::editingFinished, this, [this]() {
		_pickingMode->resetSelection();
	});
	connect(_measuringModeAction, &QAction::toggled, _distanceTable, &QWidget::setVisible);
	connect(_measuringModeAction, &QAction::toggled, _angleTable, &QWidget::setVisible);
	connect(_measuringModeAction, &QAction::toggled, this, &ParticleInspectionApplet::updateDistanceTable);
	connect(_measuringModeAction, &QAction::toggled, this, &ParticleInspectionApplet::updateAngleTable);
	connect(this, &PropertyInspectionApplet::filterChanged, this, &ParticleInspectionApplet::updateDistanceTable);
	connect(this, &PropertyInspectionApplet::filterChanged, this, &ParticleInspectionApplet::updateAngleTable);

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

	if(_measuringModeAction->isChecked()) {
		updateDistanceTable();
		updateAngleTable();
	}
}

/******************************************************************************
* Computes the inter-particle distances for the selected particles.
******************************************************************************/
void ParticleInspectionApplet::updateDistanceTable()
{
	if(currentState().isEmpty()) return;

	// Limit distance computation to the first 4 particles:
	int n = std::min(4, visibleElementCount());

	const ParticlesObject* particles = currentState().getObject<ParticlesObject>();
	ConstPropertyAccess<Point3> posProperty = particles ? particles->getProperty(ParticlesObject::PositionProperty) : nullptr;
	_distanceTable->setRowCount(std::max(1, n * (n-1) / 2));
	int row = 0;
	for(int i = 0; i < n; i++) {
		size_t i_index = visibleElementAt(i);
		for(int j = i+1; j < n; j++, row++) {
			size_t j_index = visibleElementAt(j);
			_distanceTable->setItem(row, 0, new QTableWidgetItem(QStringLiteral("%1 - %2").arg(i_index).arg(j_index)));
			if(posProperty && i_index < posProperty.size() && j_index < posProperty.size()) {
				Vector3 delta = posProperty[j_index] - posProperty[i_index];
				_distanceTable->setItem(row, 1, new QTableWidgetItem(QString::number(delta.length())));
				_distanceTable->setItem(row, 2, new QTableWidgetItem(QString::number(delta.x()) + QChar(' ') + QString::number(delta.y()) + QChar(' ') + QString::number(delta.z())));
			}
		}
	}
	if(row == 0) {
		_distanceTable->setItem(0, 0, new QTableWidgetItem(tr("Please pick two particles")));
		_distanceTable->setSpan(0, 0, 1, 3);
	}
	else {
		_distanceTable->clearSpans();
	}
}

/******************************************************************************
* Computes the angles formed by selected particles.
******************************************************************************/
void ParticleInspectionApplet::updateAngleTable()
{
	if(currentState().isEmpty()) return;

	// Limit angle computation to the first 3 particles:
	int n = std::min(3, visibleElementCount());

	const ParticlesObject* particles = currentState().getObject<ParticlesObject>();
	ConstPropertyAccess<Point3> posProperty = particles ? particles->getProperty(ParticlesObject::PositionProperty) : nullptr;
	_angleTable->setRowCount(n == 3 ? 3 : 1);
	int row = 0;
	for(int i = 0; i < n; i++) {
		size_t i_index = visibleElementAt(i);
		for(int j = 0; j < n; j++) {
			if(j == i) continue;
			size_t j_index = visibleElementAt(j);
			for(int k = j+1; k < n; k++) {
				if(k == i) continue;
				size_t k_index = visibleElementAt(k);
				_angleTable->setItem(row, 0, new QTableWidgetItem(QStringLiteral("%1 - %2 - %3").arg(j_index).arg(i_index).arg(k_index)));
				if(posProperty && i_index < posProperty.size() && j_index < posProperty.size() && k_index < posProperty.size()) {
					Vector3 delta1 = posProperty[j_index] - posProperty[i_index];
					Vector3 delta2 = posProperty[k_index] - posProperty[i_index];
					if(!delta1.isZero() && !delta2.isZero()) {
						FloatType angle = std::acos(delta1.dot(delta2) / delta1.length() / delta2.length());
						_angleTable->setItem(row, 1, new QTableWidgetItem(QString::number(qRadiansToDegrees(angle))));
					}
				}
				row++;
			}
		}
	}
	if(row == 0) {
		_angleTable->setItem(0, 0, new QTableWidgetItem(tr("Please pick three particles")));
		_angleTable->setSpan(0, 0, 1, 2);
	}
	else {
		_angleTable->clearSpans();
	}
}

/******************************************************************************
* This is called when the applet is no longer visible.
******************************************************************************/
void ParticleInspectionApplet::deactivate(MainWindow* mainWindow)
{
	_pickingMode->removeMode();
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
	if(!renderer->isInteractive() || renderer->isPicking())
		return;

	// Render the highlight markers for the selected particles.
	for(const auto& element : _pickedElements) {
		renderSelectionMarker(vp, renderer, element);
	}

	// Render pair-wise connection lines between selected particles.
	if(_applet->_measuringModeAction->isChecked() && !renderer->isBoundingBoxPass()) {
		renderer->setWorldTransform(AffineTransformation::Identity());

		// Collect world space coordinates of selected particles.
		std::array<Point3,4> vertices;
		auto outVertex = vertices.begin();
		for(auto& element : _pickedElements) {
			const PipelineFlowState& flowState = element.objNode->evaluatePipelinePreliminary(true);
			if(const ParticlesObject* particles = flowState.getObject<ParticlesObject>()) {
				// If particle selection is based on ID, find particle with the given ID.
				size_t particleIndex = element.particleIndex;
				if(element.particleId >= 0) {
					if(ConstPropertyAccess<qlonglong> identifierProperty = particles->getProperty(ParticlesObject::IdentifierProperty)) {
						if(particleIndex >= identifierProperty.size() || identifierProperty[particleIndex] != element.particleId) {
							auto iter = boost::find(identifierProperty, element.particleId);
							if(iter == identifierProperty.cend()) continue;
							element.particleIndex = particleIndex = (iter - identifierProperty.cbegin());
						}
					}
				}
				if(ConstPropertyAccess<Point3> posProperty = particles->getProperty(ParticlesObject::PositionProperty)) {
					if(particleIndex < posProperty.size())
						*outVertex++ = posProperty[particleIndex];
				}
			}
			if(outVertex == vertices.end())
				break;
		}

		// Generate pair-wise line elements.
		std::vector<Point3> lines;
		lines.reserve(vertices.size() * (vertices.size() - 1));
		for(auto v1 = vertices.begin(); v1 != outVertex; ++v1) {
			for(auto v2 = v1 + 1; v2 != outVertex; ++v2) {
				lines.push_back(*v1);
				lines.push_back(*v2);
			}
		}

		// Render line elements.
		std::shared_ptr<LinePrimitive> linesPrimitive = renderer->createLinePrimitive();
		linesPrimitive->setVertexCount(lines.size());
		linesPrimitive->setVertexPositions(lines.data());
		linesPrimitive->setLineColor(ViewportSettings::getSettings().viewportColor(ViewportSettings::COLOR_UNSELECTED));
		linesPrimitive->render(renderer);
	}
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
