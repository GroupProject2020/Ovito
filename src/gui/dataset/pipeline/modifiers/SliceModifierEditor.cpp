///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2016) Alexander Stukowski
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

#include <gui/GUI.h>
#include <core/dataset/data/simcell/SimulationCellObject.h>
#include <core/dataset/animation/AnimationSettings.h>
#include <core/dataset/scene/ObjectNode.h>
#include <core/dataset/scene/SelectionSet.h>
#include <core/viewport/Viewport.h>
#include <core/viewport/ViewportConfiguration.h>
#include <core/rendering/MarkerPrimitive.h>
#include <core/rendering/LinePrimitive.h>
#include <core/rendering/MeshPrimitive.h>
#include <core/utilities/mesh/TriMesh.h>
#include <core/app/PluginManager.h>
#include <gui/actions/ViewportModeAction.h>
#include <gui/mainwin/MainWindow.h>
#include <gui/properties/FloatParameterUI.h>
#include <gui/properties/Vector3ParameterUI.h>
#include <gui/properties/BooleanParameterUI.h>
#include <gui/properties/ModifierDelegateListParameterUI.h>
#include <gui/rendering/ViewportSceneRenderer.h>
#include <gui/viewport/ViewportWindow.h>
#include <gui/viewport/input/XFormModes.h>
#include "SliceModifierEditor.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Scene) OVITO_BEGIN_INLINE_NAMESPACE(StdObj) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_CLASS(SliceModifierEditor);
SET_OVITO_OBJECT_EDITOR(SliceModifier, SliceModifierEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
*******************************************************************************/
void SliceModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Slice"), rolloutParams, "particles.modifiers.slice.html");

    // Create the rollout contents.
	QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);

	QGridLayout* gridlayout = new QGridLayout();
	gridlayout->setContentsMargins(0,0,0,0);
	gridlayout->setColumnStretch(1, 1);

	// Distance parameter.
	FloatParameterUI* distancePUI = new FloatParameterUI(this, PROPERTY_FIELD(SliceModifier::distanceController));
	gridlayout->addWidget(distancePUI->label(), 0, 0);
	gridlayout->addLayout(distancePUI->createFieldLayout(), 0, 1);

	// Normal parameter.
	static const QString axesNames[3] = { QStringLiteral("X"), QStringLiteral("Y"), QStringLiteral("Z") };
	for(int i = 0; i < 3; i++) {
		Vector3ParameterUI* normalPUI = new Vector3ParameterUI(this, PROPERTY_FIELD(SliceModifier::normalController), i);
		normalPUI->label()->setTextFormat(Qt::RichText);
		normalPUI->label()->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
		normalPUI->label()->setText(QStringLiteral("<a href=\"%1\">%2</a>").arg(i).arg(normalPUI->label()->text()));
		normalPUI->label()->setToolTip(tr("Click here to align plane normal with %1 axis").arg(axesNames[i]));
		connect(normalPUI->label(), &QLabel::linkActivated, this, &SliceModifierEditor::onXYZNormal);
		gridlayout->addWidget(normalPUI->label(), i+1, 0);
		gridlayout->addLayout(normalPUI->createFieldLayout(), i+1, 1);
	}

	// Slice width parameter.
	FloatParameterUI* widthPUI = new FloatParameterUI(this, PROPERTY_FIELD(SliceModifier::widthController));
	gridlayout->addWidget(widthPUI->label(), 4, 0);
	gridlayout->addLayout(widthPUI->createFieldLayout(), 4, 1);

	layout->addLayout(gridlayout);
	layout->addSpacing(8);

	// Invert parameter.
	BooleanParameterUI* invertPUI = new BooleanParameterUI(this, PROPERTY_FIELD(SliceModifier::inverse));
	layout->addWidget(invertPUI->checkBox());

	// Create selection parameter.
	BooleanParameterUI* createSelectionPUI = new BooleanParameterUI(this, PROPERTY_FIELD(SliceModifier::createSelection));
	layout->addWidget(createSelectionPUI->checkBox());

	// Apply to selection only parameter.
	BooleanParameterUI* applyToSelectionPUI = new BooleanParameterUI(this, PROPERTY_FIELD(SliceModifier::applyToSelection));
	layout->addWidget(applyToSelectionPUI->checkBox());

	layout->addSpacing(8);
	QPushButton* centerPlaneBtn = new QPushButton(tr("Move plane to simulation box center"), rollout);
	connect(centerPlaneBtn, &QPushButton::clicked, this, &SliceModifierEditor::onCenterOfBox);
	layout->addWidget(centerPlaneBtn);

	// Add buttons for view alignment functions.
	QPushButton* alignViewToPlaneBtn = new QPushButton(tr("Align view direction to plane normal"), rollout);
	connect(alignViewToPlaneBtn, &QPushButton::clicked, this, &SliceModifierEditor::onAlignViewToPlane);
	layout->addWidget(alignViewToPlaneBtn);
	QPushButton* alignPlaneToViewBtn = new QPushButton(tr("Align plane normal to view direction"), rollout);
	connect(alignPlaneToViewBtn, &QPushButton::clicked, this, &SliceModifierEditor::onAlignPlaneToView);
	layout->addWidget(alignPlaneToViewBtn);

	_pickPlanePointsInputMode = new PickPlanePointsInputMode(this);
	_pickPlanePointsInputModeAction = new ViewportModeAction(mainWindow(), tr("Pick three points"), this, _pickPlanePointsInputMode);
	layout->addWidget(_pickPlanePointsInputModeAction->createPushButton());

	// Deactivate input mode when editor is reset.
	connect(this, &PropertiesEditor::contentsReplaced, _pickPlanePointsInputModeAction, &ViewportModeAction::deactivateMode);

	// Status label.
	layout->addSpacing(12);
	layout->addWidget(statusLabel());

	// Create a second rollout.
	rollout = createRollout(tr("Operate on"), rolloutParams.after(rollout), "particles.modifiers.slice.html");
	
	// Create the rollout contents.
	layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);

	ModifierDelegateListParameterUI* delegatesPUI = new ModifierDelegateListParameterUI(this, rolloutParams.after(rollout));
	layout->addWidget(delegatesPUI->listWidget());
}

/******************************************************************************
* Aligns the normal of the slicing plane with the X, Y, or Z axis.
******************************************************************************/
void SliceModifierEditor::onXYZNormal(const QString& link)
{
	SliceModifier* mod = static_object_cast<SliceModifier>(editObject());
	if(!mod) return;

	undoableTransaction(tr("Set plane normal"), [mod, &link]() {
		if(link == "0")
			mod->setNormal(Vector3(1,0,0));
		else if(link == "1")
			mod->setNormal(Vector3(0,1,0));
		else if(link == "2")
			mod->setNormal(Vector3(0,0,1));
	});
}

/******************************************************************************
* Aligns the slicing plane to the viewing direction.
******************************************************************************/
void SliceModifierEditor::onAlignPlaneToView()
{
	TimeInterval interval;

	Viewport* vp = dataset()->viewportConfig()->activeViewport();
	if(!vp) return;

	// Get the object to world transformation for the currently selected object.
	if(dataset()->selection()->nodes().empty()) return;
	ObjectNode* node = dynamic_object_cast<ObjectNode>(dataset()->selection()->nodes().front());
	if(!node) return;
	const AffineTransformation& nodeTM = node->getWorldTransform(dataset()->animationSettings()->time(), interval);

	// Get the base point of the current slicing plane in local coordinates.
	SliceModifier* mod = static_object_cast<SliceModifier>(editObject());
	if(!mod) return;
	Plane3 oldPlaneLocal = std::get<Plane3>(mod->slicingPlane(dataset()->animationSettings()->time(), interval));
	Point3 basePoint = Point3::Origin() + oldPlaneLocal.normal * oldPlaneLocal.dist;

	// Get the orientation of the projection plane of the current viewport.
	Vector3 dirWorld = -vp->cameraDirection();
	Plane3 newPlaneLocal(basePoint, nodeTM.inverse() * dirWorld);
	if(std::abs(newPlaneLocal.normal.x()) < FLOATTYPE_EPSILON) newPlaneLocal.normal.x() = 0;
	if(std::abs(newPlaneLocal.normal.y()) < FLOATTYPE_EPSILON) newPlaneLocal.normal.y() = 0;
	if(std::abs(newPlaneLocal.normal.z()) < FLOATTYPE_EPSILON) newPlaneLocal.normal.z() = 0;

	undoableTransaction(tr("Align plane to view"), [mod, &newPlaneLocal]() {
		mod->setNormal(newPlaneLocal.normal.normalized());
		mod->setDistance(newPlaneLocal.dist);
	});
}

/******************************************************************************
* Aligns the current viewing direction to the slicing plane.
******************************************************************************/
void SliceModifierEditor::onAlignViewToPlane()
{
	TimeInterval interval;

	Viewport* vp = dataset()->viewportConfig()->activeViewport();
	if(!vp) return;

	// Get the object to world transformation for the currently selected object
	if(dataset()->selection()->nodes().empty()) return;
	ObjectNode* node = dynamic_object_cast<ObjectNode>(dataset()->selection()->nodes().front());
	if(!node) return;
	const AffineTransformation& nodeTM = node->getWorldTransform(dataset()->animationSettings()->time(), interval);

	// Transform the current slicing plane to the world coordinate system.
	SliceModifier* mod = static_object_cast<SliceModifier>(editObject());
	if(!mod) return;
	Plane3 planeLocal = std::get<Plane3>(mod->slicingPlane(dataset()->animationSettings()->time(), interval));
	Plane3 planeWorld = nodeTM * planeLocal;

	// Calculate the intersection point of the current viewing direction with the current slicing plane.
	Ray3 viewportRay(vp->cameraPosition(), vp->cameraDirection());
	FloatType t = planeWorld.intersectionT(viewportRay);
	Point3 intersectionPoint;
	if(t != FLOATTYPE_MAX)
		intersectionPoint = viewportRay.point(t);
	else
		intersectionPoint = Point3::Origin() + nodeTM.translation();

	if(vp->isPerspectiveProjection()) {
		FloatType distance = (vp->cameraPosition() - intersectionPoint).length();
		vp->setViewType(Viewport::VIEW_PERSPECTIVE);
		vp->setCameraDirection(-planeWorld.normal);
		vp->setCameraPosition(intersectionPoint + planeWorld.normal * distance);
	}
	else {
		vp->setViewType(Viewport::VIEW_ORTHO);
		vp->setCameraDirection(-planeWorld.normal);
	}

	vp->zoomToSelectionExtents();
}

/******************************************************************************
* Moves the plane to the center of the simulation box.
******************************************************************************/
void SliceModifierEditor::onCenterOfBox()
{
	SliceModifier* mod = static_object_cast<SliceModifier>(editObject());
	if(!mod) return;

	// Get the simulation cell from the input object to center the slicing plane in
	// the center of the simulation cell.
	PipelineFlowState input = getSomeModifierInput();
	if(SimulationCellObject* cell = input.findObject<SimulationCellObject>()) {

		Point3 centerPoint = cell->cellMatrix() * Point3(0.5, 0.5, 0.5);
		FloatType centerDistance = mod->normal().dot(centerPoint - Point3::Origin());

		undoableTransaction(tr("Set plane position"), [mod, centerDistance]() {
			mod->setDistance(centerDistance);
		});
	}
}

/******************************************************************************
* This is called by the system after the input handler has become the active handler.
******************************************************************************/
void PickPlanePointsInputMode::activated(bool temporary)
{
	ViewportInputMode::activated(temporary);
	inputManager()->mainWindow()->statusBar()->showMessage(tr("Pick three points to define a new slicing plane."));
	if(!temporary)
		_numPickedPoints = 0;
}

/******************************************************************************
* This is called by the system after the input handler is no longer the active handler.
******************************************************************************/
void PickPlanePointsInputMode::deactivated(bool temporary)
{
	if(!temporary) {
		_numPickedPoints = 0;
		_hasPreliminaryPoint = false;
	}
	inputManager()->mainWindow()->statusBar()->clearMessage();
	ViewportInputMode::deactivated(temporary);
}

/******************************************************************************
* Handles the mouse events for a Viewport.
******************************************************************************/
void PickPlanePointsInputMode::mouseMoveEvent(ViewportWindow* vpwin, QMouseEvent* event)
{
	ViewportInputMode::mouseMoveEvent(vpwin, event);

	ViewportPickResult pickResult = vpwin->pick(event->localPos());
	setCursor(pickResult ? SelectionMode::selectionCursor() : QCursor());
	if(pickResult && _numPickedPoints < 3) {
		_pickedPoints[_numPickedPoints] = pickResult.worldPosition;
		_hasPreliminaryPoint = true;
		vpwin->viewport()->dataset()->viewportConfig()->updateViewports();
	}
	else { 
		if(_hasPreliminaryPoint) vpwin->viewport()->dataset()->viewportConfig()->updateViewports();
		_hasPreliminaryPoint = false;
	}
}

/******************************************************************************
* Handles the mouse events for a Viewport.
******************************************************************************/
void PickPlanePointsInputMode::mouseReleaseEvent(ViewportWindow* vpwin, QMouseEvent* event)
{
	if(event->button() == Qt::LeftButton) {

		if(_numPickedPoints >= 3) {
			_numPickedPoints = 0;
			vpwin->viewport()->dataset()->viewportConfig()->updateViewports();
		}

		ViewportPickResult pickResult = vpwin->pick(event->localPos());
		if(pickResult) {

			// Do not select the same point twice.
			bool ignore = false;
			if(_numPickedPoints >= 1 && _pickedPoints[0].equals(pickResult.worldPosition, FLOATTYPE_EPSILON)) ignore = true;
			if(_numPickedPoints >= 2 && _pickedPoints[1].equals(pickResult.worldPosition, FLOATTYPE_EPSILON)) ignore = true;

			if(!ignore) {
				_pickedPoints[_numPickedPoints] = pickResult.worldPosition;
				_numPickedPoints++;
				_hasPreliminaryPoint = false;
				vpwin->viewport()->dataset()->viewportConfig()->updateViewports();

				if(_numPickedPoints == 3) {

					// Get the slice modifier that is currently being edited.
					SliceModifier* mod = dynamic_object_cast<SliceModifier>(_editor->editObject());
					if(mod)
						alignPlane(mod);
					_numPickedPoints = 0;
				}
			}
		}
	}

	ViewportInputMode::mouseReleaseEvent(vpwin, event);
}

/******************************************************************************
* Aligns the modifier's slicing plane to the three selected particles.
******************************************************************************/
void PickPlanePointsInputMode::alignPlane(SliceModifier* mod)
{
	OVITO_ASSERT(_numPickedPoints == 3);

	try {
		Plane3 worldPlane(_pickedPoints[0], _pickedPoints[1], _pickedPoints[2], true);
		if(worldPlane.normal.equals(Vector3::Zero(), FLOATTYPE_EPSILON))
			mod->throwException(tr("Cannot set the new slicing plane. The three selected points are colinear."));

		// Get the object to world transformation for the currently selected node.
		ModifierApplication* modApp = mod->someModifierApplication();
		if(!modApp) return;
		QSet<ObjectNode*> nodes = modApp->dependentNodes();
		if(nodes.empty()) return;
		ObjectNode* node = *nodes.begin();
		TimeInterval interval;
		const AffineTransformation& nodeTM = node->getWorldTransform(mod->dataset()->animationSettings()->time(), interval);

		// Transform new plane from world to object space.
		Plane3 localPlane = nodeTM.inverse() * worldPlane;

		// Flip new plane orientation if necessary to align it with old orientation.
		if(localPlane.normal.dot(mod->normal()) < 0)
			localPlane = -localPlane;

		localPlane.normalizePlane();
		UndoableTransaction::handleExceptions(mod->dataset()->undoStack(), tr("Align plane to points"), [mod, &localPlane]() {
			mod->setNormal(localPlane.normal);
			mod->setDistance(localPlane.dist);
		});
	}
	catch(const Exception& ex) {
		ex.reportError();
	}
}

/******************************************************************************
* Lets the input mode render its overlay content in a viewport.
******************************************************************************/
void PickPlanePointsInputMode::renderOverlay3D(Viewport* vp, ViewportSceneRenderer* renderer)
{
	ViewportInputMode::renderOverlay3D(vp, renderer);

	if(renderer->isPicking())
		return;

	int npoints = _numPickedPoints;
	if(_hasPreliminaryPoint && npoints < 3) npoints++;
	if(_numPickedPoints == 0)
		return;

	renderer->setWorldTransform(AffineTransformation::Identity());
	if(!renderer->isBoundingBoxPass()) {
		std::shared_ptr<MarkerPrimitive> markers = renderer->createMarkerPrimitive(MarkerPrimitive::BoxShape);
		markers->setCount(npoints);
		markers->setMarkerPositions(_pickedPoints);
		markers->setMarkerColor(ColorA(1, 1, 1));
		markers->render(renderer);

		if(npoints == 2) {
			std::shared_ptr<LinePrimitive> lines = renderer->createLinePrimitive();
			lines->setVertexCount(2);
			lines->setVertexPositions(_pickedPoints);
			lines->setLineColor(ColorA(1, 1, 1));
			lines->render(renderer);
		}
		else if(npoints == 3) {
			std::shared_ptr<MeshPrimitive> mesh = renderer->createMeshPrimitive();
			TriMesh tri;
			tri.setVertexCount(3);
			tri.setVertex(0, _pickedPoints[0]);
			tri.setVertex(1, _pickedPoints[1]);
			tri.setVertex(2, _pickedPoints[2]);
			tri.addFace().setVertices(0, 1, 2);
			mesh->setMesh(tri, ColorA(0.7f, 0.7f, 1.0f, 0.5f));
			mesh->render(renderer);

			std::shared_ptr<LinePrimitive> lines = renderer->createLinePrimitive();
			lines->setVertexCount(6);
			const Point3 vertices[6] = { _pickedPoints[0], _pickedPoints[1], _pickedPoints[1], _pickedPoints[2], _pickedPoints[2], _pickedPoints[0] };
			lines->setVertexPositions(vertices);
			lines->setLineColor(ColorA(1, 1, 1));
			lines->render(renderer);
		}
	}
	else {
		for(int i = 0; i < npoints; i++)
			renderer->addToLocalBoundingBox(_pickedPoints[i]);			
	}
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
