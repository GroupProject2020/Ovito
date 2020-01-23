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

#include <ovito/gui_wasm/GUI.h>
#include <ovito/gui_wasm/mainwin/MainWindow.h>
#include <ovito/gui_wasm/viewport/ViewportWindow.h>
#include <ovito/core/dataset/UndoStack.h>
#include <ovito/core/dataset/animation/AnimationSettings.h>
#include <ovito/core/dataset/animation/controller/PRSTransformationController.h>
#include <ovito/core/dataset/animation/controller/KeyframeController.h>
#include <ovito/core/dataset/scene/SelectionSet.h>
#include <ovito/core/viewport/ViewportConfiguration.h>
#include <ovito/core/viewport/Viewport.h>
#include "ViewportInputManager.h"
#include "XFormModes.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/// The cursor shown while the mouse cursor is over an object.
boost::optional<QCursor> SelectionMode::_hoverCursor;

/******************************************************************************
* Handles the mouse down event for the given viewport.
******************************************************************************/
void SelectionMode::mousePressEvent(ViewportWindow* vpwin, QMouseEvent* event)
{
	if(event->button() == Qt::LeftButton) {
		_viewport = vpwin->viewport();
		_clickPoint = event->localPos();
	}
	else if(event->button() == Qt::RightButton) {
		_viewport = nullptr;
	}
	ViewportInputMode::mousePressEvent(vpwin, event);
}

/******************************************************************************
* Handles the mouse up event for the given viewport.
******************************************************************************/
void SelectionMode::mouseReleaseEvent(ViewportWindow* vpwin, QMouseEvent* event)
{
	if(_viewport != nullptr) {
		// Select object under mouse cursor.
		ViewportPickResult pickResult = vpwin->pick(_clickPoint);
		if(pickResult.isValid()) {
			_viewport->dataset()->undoStack().beginCompoundOperation(tr("Select"));
			_viewport->dataset()->selection()->setNode(pickResult.pipelineNode());
			_viewport->dataset()->undoStack().endCompoundOperation();
		}
		_viewport = nullptr;
	}
	ViewportInputMode::mouseReleaseEvent(vpwin, event);
}

/******************************************************************************
* This is called by the system after the input handler is
* no longer the active handler.
******************************************************************************/
void SelectionMode::deactivated(bool temporary)
{
	if(inputManager()->mainWindow())
		inputManager()->mainWindow()->clearStatusBarMessage();
	_viewport = nullptr;
	ViewportInputMode::deactivated(temporary);
}

/******************************************************************************
* Handles the mouse move event for the given viewport.
******************************************************************************/
void SelectionMode::mouseMoveEvent(ViewportWindow* vpwin, QMouseEvent* event)
{
	// Change mouse cursor while hovering over an object.
	ViewportPickResult pickResult = vpwin->pick(event->localPos());
	setCursor(pickResult.isValid() ? selectionCursor() : QCursor());

	// Display a description of the object under the mouse cursor in the status bar.
	if(inputManager()->mainWindow()) {
		if(pickResult.isValid() && pickResult.pickInfo())
			inputManager()->mainWindow()->showStatusBarMessage(pickResult.pickInfo()->infoString(pickResult.pipelineNode(), pickResult.subobjectId()));
		else
			inputManager()->mainWindow()->clearStatusBarMessage();
	}

	ViewportInputMode::mouseMoveEvent(vpwin, event);
}

/******************************************************************************
* This is called by the system after the input handler is
* no longer the active handler.
******************************************************************************/
void XFormMode::deactivated(bool temporary)
{
	if(_viewport) {
		// Restore old state if change has not been committed.
		_viewport->dataset()->undoStack().endCompoundOperation(false);
		_viewport->dataset()->undoStack().endCompoundOperation(false);
		_viewport = nullptr;
	}
	_selectedNode.setTarget(nullptr);
	ViewportInputMode::deactivated(temporary);
}

/******************************************************************************
* Handles the mouse down event for the given viewport.
******************************************************************************/
void XFormMode::mousePressEvent(ViewportWindow* vpwin, QMouseEvent* event)
{
	if(event->button() == Qt::LeftButton) {
		if(_viewport == nullptr) {

			// Select object under mouse cursor.
			ViewportPickResult pickResult = vpwin->pick(event->localPos());
			if(pickResult.isValid()) {
				_viewport = vpwin->viewport();
				_startPoint = event->localPos();
				_viewport->dataset()->undoStack().beginCompoundOperation(undoDisplayName());
				_viewport->dataset()->selection()->setNode(pickResult.pipelineNode());
				_viewport->dataset()->undoStack().beginCompoundOperation(undoDisplayName());
				startXForm();
			}
		}
		return;
	}
	else if(event->button() == Qt::RightButton) {
		if(_viewport != nullptr) {
			// Restore old state when aborting the operation.
			_viewport->dataset()->undoStack().endCompoundOperation(false);
			_viewport->dataset()->undoStack().endCompoundOperation(false);
			_viewport = nullptr;
			return;
		}
	}
	ViewportInputMode::mousePressEvent(vpwin, event);
}

/******************************************************************************
* Handles the mouse up event for the given viewport.
******************************************************************************/
void XFormMode::mouseReleaseEvent(ViewportWindow* vpwin, QMouseEvent* event)
{
	if(_viewport) {
		// Commit change.
		_viewport->dataset()->undoStack().endCompoundOperation();
		_viewport->dataset()->undoStack().endCompoundOperation();
		_viewport = nullptr;
	}
	ViewportInputMode::mouseReleaseEvent(vpwin, event);
}

/******************************************************************************
* Handles the mouse move event for the given viewport.
******************************************************************************/
void XFormMode::mouseMoveEvent(ViewportWindow* vpwin, QMouseEvent* event)
{
	if(_viewport == vpwin->viewport()) {
		_currentPoint = event->localPos();

		_viewport->dataset()->undoStack().resetCurrentCompoundOperation();
		doXForm();

		// Force immediate viewport repaints.
		_viewport->dataset()->viewportConfig()->processViewportUpdates();
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
void XFormMode::focusOutEvent(ViewportWindow* vpwin, QFocusEvent* event)
{
	if(_viewport) {
		// Restore old state if change has not been committed.
		_viewport->dataset()->undoStack().endCompoundOperation(false);
		_viewport->dataset()->undoStack().endCompoundOperation(false);
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

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
