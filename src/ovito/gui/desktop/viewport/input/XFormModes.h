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

#pragma once


#include <ovito/gui/desktop/GUI.h>
#include <ovito/gui/base/viewport/ViewportInputMode.h>
#include <ovito/core/dataset/animation/TimeInterval.h>
#include <ovito/core/oo/RefTargetListener.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/******************************************************************************
* Base class for selection, move, rotate and scale modes.
******************************************************************************/
class OVITO_GUI_EXPORT XFormMode : public ViewportInputMode
{
	Q_OBJECT

public:

	/// \brief Handles the mouse down event for the given viewport.
	virtual void mousePressEvent(ViewportWindowInterface* vpwin, QMouseEvent* event) override;

	/// \brief Handles the mouse up event for the given viewport.
	virtual void mouseReleaseEvent(ViewportWindowInterface* vpwin, QMouseEvent* event) override;

	/// \brief Handles the mouse move event for the given viewport.
	virtual void mouseMoveEvent(ViewportWindowInterface* vpwin, QMouseEvent* event) override;

	/// Is called when a viewport looses the input focus.
	virtual void focusOutEvent(ViewportWindowInterface* vpwin, QFocusEvent* event) override;

	/// \brief Returns the origin of the transformation system to use for xform modes.
	Point3 transformationCenter();

	/// \brief Determines the coordinate system to use for transformation.
	AffineTransformation transformationSystem();

protected:

	/// Protected constructor.
	XFormMode(QObject* parent, const QString& cursorImagePath) : ViewportInputMode(parent), _xformCursor(QPixmap(cursorImagePath)) {
		connect(&_selectedNode, &RefTargetListener<SceneNode>::notificationEvent, this, &XFormMode::onSceneNodeEvent);
	}

	/// \brief This is called by the system after the input handler has
	///        become the active handler.
	virtual void activated(bool temporaryActivation) override;

	/// \brief This is called by the system after the input handler is
	///        no longer the active handler.
	virtual void deactivated(bool temporary) override;

	/// Returns the current viewport we are working in.
	Viewport* viewport() const { return _viewport; }

	/// Is called when the transformation operation begins.
	virtual void startXForm() {}

	/// Is repeatedly called during the transformation operation.
	virtual void doXForm() {}

	/// Returns the display name for undoable operations performed by this input mode.
	virtual QString undoDisplayName() = 0;

	/// Applies the current transformation to a set of nodes.
	virtual void applyXForm(const QVector<SceneNode*>& nodeSet, FloatType multiplier) {}

	/// Updates the values displayed in the coordinate display widget.
	virtual void updateCoordinateDisplay(CoordinateDisplayWidget* coordDisplay) {}

protected Q_SLOT:

	/// Is called when the user has selected a different scene node.
	void onSelectionChangeComplete(SelectionSet* selection);

	/// Is called when the selected scene node generates a notification event.
	void onSceneNodeEvent(const ReferenceEvent& event);

	/// Is called when the current animation time has changed.
	void onTimeChanged(TimePoint time);

	/// This signal handler is called by the coordinate display widget when the user
	/// has changed the value of one of the vector components.
	virtual void onCoordinateValueEntered(int component, FloatType value) {}

	/// This signal handler is called by the coordinate display widget when the user
	/// has pressed the "Animate" button.
	virtual void onAnimateTransformationButton() {}

protected:

	/// Mouse position at first click.
	QPointF _startPoint;

	/// The current mouse position.
	QPointF _currentPoint;

	/// The current viewport we are working in.
	Viewport* _viewport = nullptr;

	/// The cursor shown while the mouse cursor is over an object.
	QCursor _xformCursor;

	/// This monitors the selected node to update the coordinate display.
	RefTargetListener<SceneNode> _selectedNode;
};

/******************************************************************************
* This mode lets the user move scene nodes.
******************************************************************************/
class OVITO_GUI_EXPORT MoveMode : public XFormMode
{
	Q_OBJECT

public:

	/// Constructor.
	MoveMode(QObject* parent) : XFormMode(parent, QStringLiteral(":/gui/cursor/editing/cursor_mode_move.png")) {}

protected:

	/// Returns the display name for undoable operations performed by this input mode.
	virtual QString undoDisplayName() override { return tr("Move"); }

	/// Is called when the transformation operation begins.
	virtual void startXForm() override;

	/// Is repeatedly called during the transformation operation.
	virtual void doXForm() override;

	/// Applies the current transformation to a set of nodes.
	virtual void applyXForm(const QVector<SceneNode*>& nodeSet, FloatType multiplier) override;

	/// Updates the values displayed in the coordinate display widget.
	virtual void updateCoordinateDisplay(CoordinateDisplayWidget* coordDisplay) override;

	/// This signal handler is called by the coordinate display widget when the user
	/// has changed the value of one of the vector components.
	virtual void onCoordinateValueEntered(int component, FloatType value) override;

	/// This signal handler is called by the coordinate display widget when the user
	/// has pressed the "Animate" button.
	virtual void onAnimateTransformationButton() override;

private:

	/// The coordinate system to use for translations.
	AffineTransformation _translationSystem;

	/// The starting position.
	Point3 _initialPoint;

	/// The translation vector.
	Vector3 _delta;
};

/******************************************************************************
* This mode lets the user rotate scene nodes.
******************************************************************************/
class OVITO_GUI_EXPORT RotateMode : public XFormMode
{
	Q_OBJECT

public:

	/// Constructor.
	RotateMode(QObject* parent) : XFormMode(parent, QStringLiteral(":/gui/cursor/editing/cursor_mode_rotate.png")) {}

protected:

	/// Returns the display name for undoable operations performed by this input mode.
	virtual QString undoDisplayName() override { return tr("Rotate"); }

	/// Is called when the transformation operation begins.
	virtual void startXForm() override;

	/// Is repeatedly called during the transformation operation.
	virtual void doXForm() override;

	/// Applies the current transformation to a set of nodes.
	virtual void applyXForm(const QVector<SceneNode*>& nodeSet, FloatType multiplier) override;

	/// Updates the values displayed in the coordinate display widget.
	virtual void updateCoordinateDisplay(CoordinateDisplayWidget* coordDisplay) override;

	/// This signal handler is called by the coordinate display widget when the user
	/// has changed the value of one of the vector components.
	virtual void onCoordinateValueEntered(int component, FloatType value) override;

	/// This signal handler is called by the coordinate display widget when the user
	/// has pressed the "Animate" button.
	virtual void onAnimateTransformationButton() override;

private:

	/// The cached transformation center for off-center rotation.
	Point3 _transformationCenter;

	/// The current rotation
	Rotation _rotation;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
