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

#include <ovito/gui/desktop/GUI.h>
#include <ovito/gui/desktop/mainwin/MainWindow.h>
#include <ovito/gui/desktop/widgets/display/CoordinateDisplayWidget.h>
#include <ovito/gui/desktop/dialogs/AnimationKeyEditorDialog.h>
#include <ovito/gui/desktop/viewport/ViewportWindow.h>
#include <ovito/gui/base/viewport/ViewportInputManager.h>
#include <ovito/core/dataset/UndoStack.h>
#include <ovito/core/dataset/animation/AnimationSettings.h>
#include <ovito/core/dataset/animation/controller/PRSTransformationController.h>
#include <ovito/core/dataset/animation/controller/KeyframeController.h>
#include <ovito/core/dataset/scene/SelectionSet.h>
#include <ovito/core/viewport/ViewportConfiguration.h>
#include <ovito/core/viewport/Viewport.h>
#include <ovito/core/viewport/ViewportWindowInterface.h>
#include "XFormModes.h"

namespace Ovito {

/******************************************************************************
* This is called by the system after the input handler has
* become the active handler.
******************************************************************************/
void XFormMode::activated(bool temporaryActivation)
{
	ViewportInputMode::activated(temporaryActivation);

	// Listen to selection change events to update the coordinate display.
	DataSetContainer& datasetContainer = inputManager()->datasetContainer();
	connect(&datasetContainer, &DataSetContainer::selectionChangeComplete, this, &XFormMode::onSelectionChangeComplete);
	connect(&datasetContainer, &DataSetContainer::timeChanged, this, &XFormMode::onTimeChanged);
	onSelectionChangeComplete(datasetContainer.currentSet() ? datasetContainer.currentSet()->selection() : nullptr);
}

/******************************************************************************
* This is called by the system after the input handler is
* no longer the active handler.
******************************************************************************/
void XFormMode::deactivated(bool temporary)
{
	if(viewport()) {
		// Restore old state if change has not been committed.
		viewport()->dataset()->undoStack().endCompoundOperation(false);
		viewport()->dataset()->undoStack().endCompoundOperation(false);
		_viewport = nullptr;
	}
	disconnect(&inputManager()->datasetContainer(), &DataSetContainer::selectionChangeComplete, this, &XFormMode::onSelectionChangeComplete);
	disconnect(&inputManager()->datasetContainer(), &DataSetContainer::timeChanged, this, &XFormMode::onTimeChanged);
	_selectedNode.setTarget(nullptr);
	onSelectionChangeComplete(nullptr);
	ViewportInputMode::deactivated(temporary);
}

/******************************************************************************
* Is called when the user has selected a different scene node.
******************************************************************************/
void XFormMode::onSelectionChangeComplete(SelectionSet* selection)
{
	MainWindow* mainWindow = static_cast<MainWindow*>(inputManager()->mainWindow());
	CoordinateDisplayWidget* coordDisplay = mainWindow ? mainWindow->coordinateDisplay() : nullptr;

	if(selection) {
		if(selection->nodes().size() == 1) {
			_selectedNode.setTarget(selection->nodes().front());
			if(coordDisplay) {
				updateCoordinateDisplay(coordDisplay);
				coordDisplay->activate(undoDisplayName());
				connect(coordDisplay, &CoordinateDisplayWidget::valueEntered, this, &XFormMode::onCoordinateValueEntered, Qt::ConnectionType(Qt::AutoConnection | Qt::UniqueConnection));
				connect(coordDisplay, &CoordinateDisplayWidget::animatePressed, this, &XFormMode::onAnimateTransformationButton, Qt::ConnectionType(Qt::AutoConnection | Qt::UniqueConnection));
			}
			return;
		}
	}
	_selectedNode.setTarget(nullptr);
	if(coordDisplay) {
		disconnect(coordDisplay, &CoordinateDisplayWidget::valueEntered, this, &XFormMode::onCoordinateValueEntered);
		disconnect(coordDisplay, &CoordinateDisplayWidget::animatePressed, this, &XFormMode::onAnimateTransformationButton);
		coordDisplay->deactivate();
	}
}

/******************************************************************************
* Is called when the selected scene node generates a notification event.
******************************************************************************/
void XFormMode::onSceneNodeEvent(const ReferenceEvent& event)
{
	if(event.type() == ReferenceEvent::TransformationChanged) {
		if(MainWindow* mainWindow = static_cast<MainWindow*>(inputManager()->mainWindow()))
			updateCoordinateDisplay(mainWindow->coordinateDisplay());
	}
}

/******************************************************************************
* Is called when the current animation time has changed.
******************************************************************************/
void XFormMode::onTimeChanged(TimePoint time)
{
	if(MainWindow* mainWindow = static_cast<MainWindow*>(inputManager()->mainWindow()))
		updateCoordinateDisplay(mainWindow->coordinateDisplay());
}

/******************************************************************************
* Handles the mouse down event for the given viewport.
******************************************************************************/
void XFormMode::mousePressEvent(ViewportWindowInterface* vpwin, QMouseEvent* event)
{
	if(event->button() == Qt::LeftButton) {
		if(viewport() == nullptr) {

			// Select object under mouse cursor.
			ViewportPickResult pickResult = vpwin->pick(event->localPos());
			if(pickResult.isValid()) {
				_viewport = vpwin->viewport();
				_startPoint = event->localPos();
				viewport()->dataset()->undoStack().beginCompoundOperation(undoDisplayName());
				viewport()->dataset()->selection()->setNode(pickResult.pipelineNode());
				viewport()->dataset()->undoStack().beginCompoundOperation(undoDisplayName());
				startXForm();
			}
		}
		return;
	}
	else if(event->button() == Qt::RightButton) {
		if(viewport()) {
			// Restore old state when aborting the operation.
			viewport()->dataset()->undoStack().endCompoundOperation(false);
			viewport()->dataset()->undoStack().endCompoundOperation(false);
			_viewport = nullptr;
			return;
		}
	}
	ViewportInputMode::mousePressEvent(vpwin, event);
}

/******************************************************************************
* Handles the mouse up event for the given viewport.
******************************************************************************/
void XFormMode::mouseReleaseEvent(ViewportWindowInterface* vpwin, QMouseEvent* event)
{
	if(viewport()) {
		// Commit change.
		viewport()->dataset()->undoStack().endCompoundOperation();
		viewport()->dataset()->undoStack().endCompoundOperation();
		_viewport = nullptr;
	}
	ViewportInputMode::mouseReleaseEvent(vpwin, event);
}

/******************************************************************************
* Handles the mouse move event for the given viewport.
******************************************************************************/
void XFormMode::mouseMoveEvent(ViewportWindowInterface* vpwin, QMouseEvent* event)
{
	if(viewport() == vpwin->viewport()) {
		// Take the current mouse cursor position to make the input mode
		// look more responsive. The cursor position recorded when the mouse event was
		// generates may be too old.
		_currentPoint = static_cast<ViewportWindow*>(vpwin)->mapFromGlobal(QCursor::pos());

		viewport()->dataset()->undoStack().resetCurrentCompoundOperation();
		doXForm();

		// Force immediate viewport repaints.
		viewport()->dataset()->viewportConfig()->processViewportUpdates();
	}
	else {
		// Change mouse cursor while hovering over an object.
		setCursor(vpwin->pick(event->localPos()).isValid() ? _xformCursor : QCursor());
	}
	ViewportInputMode::mouseMoveEvent(vpwin, event);
}

/******************************************************************************
* Is called when a viewport looses the input focus.
******************************************************************************/
void XFormMode::focusOutEvent(ViewportWindowInterface* vpwin, QFocusEvent* event)
{
	if(viewport()) {
		// Restore old state if change has not been committed.
		viewport()->dataset()->undoStack().endCompoundOperation(false);
		viewport()->dataset()->undoStack().endCompoundOperation(false);
		_viewport = nullptr;
	}
}

/******************************************************************************
* Returns the origin of the transformation system to use for xform modes.
******************************************************************************/
Point3 XFormMode::transformationCenter()
{
	Point3 center = Point3::Origin();
	SelectionSet* selection = viewport()->dataset()->selection();
	if(selection && !selection->nodes().empty()) {
		TimeInterval interval;
		TimePoint time = selection->dataset()->animationSettings()->time();
		for(SceneNode* node : selection->nodes()) {
			const AffineTransformation& nodeTM = node->getWorldTransform(time, interval);
			center += nodeTM.translation();
		}
		center /= (FloatType)selection->nodes().size();
	}
	return center;
}

/******************************************************************************
* Determines the coordinate system to use for transformation.
*******************************************************************************/
AffineTransformation XFormMode::transformationSystem()
{
	return viewport()->gridMatrix();
}

/******************************************************************************
* Is called when the transformation operation begins.
******************************************************************************/
void MoveMode::startXForm()
{
	_translationSystem = transformationSystem();
	_initialPoint = Point3::Origin();
	viewport()->snapPoint(_startPoint, _initialPoint, _translationSystem);
}

/******************************************************************************
* Is repeatedly called during the transformation operation.
******************************************************************************/
void MoveMode::doXForm()
{
    Point3 point2;
	if(viewport()->snapPoint(_currentPoint, point2, _translationSystem)) {

		// Get movement in world space.
		_delta = _translationSystem * (point2 - _initialPoint);

		// Apply transformation to selected nodes.
		applyXForm(viewport()->dataset()->selection()->nodes(), 1);
	}
}

/******************************************************************************
* Applies the current transformation to a set of nodes.
******************************************************************************/
void MoveMode::applyXForm(const QVector<SceneNode*>& nodeSet, FloatType multiplier)
{
	for(SceneNode* node : nodeSet) {
		OVITO_CHECK_OBJECT_POINTER(node);
		OVITO_CHECK_OBJECT_POINTER(node->transformationController());

		// Get node's transformation system.
		AffineTransformation transformSystem = _translationSystem;

		// Get parent's system.
		TimeInterval iv;
		TimePoint time = node->dataset()->animationSettings()->time();
		const AffineTransformation& translationSys = node->parentNode()->getWorldTransform(time, iv);

		// Move node in parent's system.
		node->transformationController()->translate(time, _delta * multiplier, translationSys.inverse());
	}
}

/******************************************************************************
* Updates the values displayed in the coordinate display widget.
******************************************************************************/
void MoveMode::updateCoordinateDisplay(CoordinateDisplayWidget* coordDisplay)
{
	if(_selectedNode.target()) {
		DataSet* dataset = _selectedNode.target()->dataset();
		coordDisplay->setUnit(dataset->unitsManager().worldUnit());
		Controller* ctrl = _selectedNode.target()->transformationController();
		if(ctrl) {
			TimeInterval iv;
			Vector3 translation;
			ctrl->getPositionValue(dataset->animationSettings()->time(), translation, iv);
			coordDisplay->setValues(translation);
			return;
		}
	}
	coordDisplay->setValues(Vector3::Zero());
}

/******************************************************************************
* This signal handler is called by the coordinate display widget when the user
* has changed the value of one of the vector components.
******************************************************************************/
void MoveMode::onCoordinateValueEntered(int component, FloatType value)
{
	if(_selectedNode.target()) {
		Controller* ctrl = _selectedNode.target()->transformationController();
		if(ctrl) {
			TimeInterval iv;
			Vector3 translation;
			DataSet* dataset = _selectedNode.target()->dataset();
			ctrl->getPositionValue(dataset->animationSettings()->time(), translation, iv);
			translation[component] = value;
			ctrl->setPositionValue(dataset->animationSettings()->time(), translation, true);
		}
	}
}

/******************************************************************************
* This signal handler is called by the coordinate display widget when the user
* has pressed the "Animate" button.
******************************************************************************/
void MoveMode::onAnimateTransformationButton()
{
	if(_selectedNode.target()) {
		PRSTransformationController* prs_ctrl = dynamic_object_cast<PRSTransformationController>(_selectedNode.target()->transformationController());
		if(prs_ctrl) {
			KeyframeController* ctrl = dynamic_object_cast<KeyframeController>(prs_ctrl->positionController());
			if(ctrl) {
				MainWindow* mainWindow = static_cast<MainWindow*>(inputManager()->mainWindow());
				AnimationKeyEditorDialog dlg(ctrl, &PROPERTY_FIELD(PRSTransformationController::positionController), mainWindow, mainWindow);
				dlg.exec();
			}
		}
	}
}

/******************************************************************************
* Is called when the transformation operation begins.
******************************************************************************/
void RotateMode::startXForm()
{
	_transformationCenter = transformationCenter();
}

/******************************************************************************
* Is repeatedly called during the transformation operation.
******************************************************************************/
void RotateMode::doXForm()
{
	FloatType angle1 = (FloatType)(_currentPoint.y() - _startPoint.y()) / 100;
	FloatType angle2 = (FloatType)(_currentPoint.x() - _startPoint.x()) / 100;

	// Constrain rotation to z-axis.
	_rotation = Rotation(Vector3(0,0,1), angle1);

	// Apply transformation to selected nodes.
	applyXForm(viewport()->dataset()->selection()->nodes(), 1);
}

/******************************************************************************
* Applies the current transformation to a set of nodes.
******************************************************************************/
void RotateMode::applyXForm(const QVector<SceneNode*>& nodeSet, FloatType multiplier)
{
	for(SceneNode* node : nodeSet) {
		OVITO_CHECK_OBJECT_POINTER(node);
		OVITO_CHECK_OBJECT_POINTER(node->transformationController());

		// Get transformation system.
		AffineTransformation transformSystem = transformationSystem();
		transformSystem.translation() = _transformationCenter - Point3::Origin();

		// Make transformation system relative to parent's tm.
		TimeInterval iv;
		TimePoint time = node->dataset()->animationSettings()->time();
		const AffineTransformation& parentTM = node->parentNode()->getWorldTransform(time, iv);
		transformSystem = transformSystem * parentTM.inverse();

		// Rotate node in transformation system.
		Rotation scaledRot = Rotation(_rotation.axis(), _rotation.angle() * multiplier);
		node->transformationController()->rotate(time, scaledRot, transformSystem);

#if 0
		// Translate node for off-center rotation.
		if(!ANIM_MANAGER.isAnimating()) {
			AffineTransformation inverseSys = transformSystem.inverse();
			/// Get node position in parent's space.
			AffineTransformation curTM;
			node->transformationController()->getValue(time, curTM, iv);
			Point3 nodePos = Point3::Origin() + curTM.translation();
			nodePos = inverseSys * nodePos;
			Vector3 translation = (AffineTransformation::rotation(scaledRot) * nodePos) - nodePos;
			node->transformationController()->translate(time, translation, transformSystem);
		}
#endif
	}
}

/******************************************************************************
* Updates the values displayed in the coordinate display widget.
******************************************************************************/
void RotateMode::updateCoordinateDisplay(CoordinateDisplayWidget* coordDisplay)
{
	if(_selectedNode.target()) {
		DataSet* dataset = _selectedNode.target()->dataset();
		coordDisplay->setUnit(dataset->unitsManager().angleUnit());
		Controller* ctrl = _selectedNode.target()->transformationController();
		if(ctrl) {
			TimeInterval iv;
			Rotation rotation;
			ctrl->getRotationValue(dataset->animationSettings()->time(), rotation, iv);
			Vector3 euler = rotation.toEuler(Matrix3::szyx);
			coordDisplay->setValues(Vector3(euler[2], euler[1], euler[0]));
			return;
		}
	}
	coordDisplay->setValues(Vector3::Zero());
}

/******************************************************************************
* This signal handler is called by the coordinate display widget when the user
* has changed the value of one of the vector components.
******************************************************************************/
void RotateMode::onCoordinateValueEntered(int component, FloatType value)
{
	if(_selectedNode.target()) {
		Controller* ctrl = _selectedNode.target()->transformationController();
		if(ctrl) {
			TimeInterval iv;
			Vector3 translation;
			DataSet* dataset = _selectedNode.target()->dataset();
			MainWindow* mainWindow = static_cast<MainWindow*>(inputManager()->mainWindow());
			CoordinateDisplayWidget* coordDisplay = mainWindow->coordinateDisplay();
			Vector3 euler = coordDisplay->getValues();
			Rotation rotation = Rotation::fromEuler(Vector3(euler[2], euler[1], euler[0]), Matrix3::szyx);
			ctrl->setRotationValue(dataset->animationSettings()->time(), rotation, true);
		}
	}
}

/******************************************************************************
* This signal handler is called by the coordinate display widget when the user
* has pressed the "Animate" button.
******************************************************************************/
void RotateMode::onAnimateTransformationButton()
{
	if(_selectedNode.target()) {
		PRSTransformationController* prs_ctrl = dynamic_object_cast<PRSTransformationController>(_selectedNode.target()->transformationController());
		if(prs_ctrl) {
			KeyframeController* ctrl = dynamic_object_cast<KeyframeController>(prs_ctrl->rotationController());
			if(ctrl) {
				MainWindow* mainWindow = static_cast<MainWindow*>(inputManager()->mainWindow());
				AnimationKeyEditorDialog dlg(ctrl, &PROPERTY_FIELD(PRSTransformationController::rotationController), mainWindow, mainWindow);
				dlg.exec();
			}
		}
	}
}

}	// End of namespace
