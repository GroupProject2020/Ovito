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

#include <ovito/gui/GUI.h>
#include <ovito/core/dataset/scene/RootSceneNode.h>
#include <ovito/core/dataset/scene/PipelineSceneNode.h>
#include <ovito/core/dataset/data/camera/AbstractCameraObject.h>
#include <ovito/core/dataset/pipeline/StaticSource.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/dataset/animation/AnimationSettings.h>
#include <ovito/core/app/PluginManager.h>
#include <ovito/gui/dialogs/AdjustCameraDialog.h>
#include <ovito/gui/mainwin/MainWindow.h>
#include "ViewportMenu.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/******************************************************************************
* Initializes the menu.
******************************************************************************/
ViewportMenu::ViewportMenu(ViewportWindow* vpWindow) : QMenu(vpWindow), _viewport(vpWindow->viewport()), _vpWindow(vpWindow)
{
	QAction* action;

	// Build menu.
	action = addAction(tr("Preview Mode"), this, SLOT(onRenderPreviewMode(bool)));
	action->setCheckable(true);
	action->setChecked(_viewport->renderPreviewMode());
	action = addAction(tr("Show Grid"), this, SLOT(onShowGrid(bool)));
	action->setCheckable(true);
	action->setChecked(_viewport->isGridVisible());
	action = addAction(tr("Constrain Rotation"), this, SLOT(onConstrainRotation(bool)));
	action->setCheckable(true);
	action->setChecked(ViewportSettings::getSettings().constrainCameraRotation());
	addSeparator();

	_viewTypeMenu = addMenu(tr("View Type"));
	connect(_viewTypeMenu, &QMenu::aboutToShow, this, &ViewportMenu::onShowViewTypeMenu);

	QActionGroup* viewTypeGroup = new QActionGroup(this);
	action = viewTypeGroup->addAction(tr("Top"));
	action->setCheckable(true);
	action->setChecked(_viewport->viewType() == Viewport::VIEW_TOP);
	action->setData((int)Viewport::VIEW_TOP);
	action = viewTypeGroup->addAction(tr("Bottom"));
	action->setCheckable(true);
	action->setChecked(_viewport->viewType() == Viewport::VIEW_BOTTOM);
	action->setData((int)Viewport::VIEW_BOTTOM);
	action = viewTypeGroup->addAction(tr("Front"));
	action->setCheckable(true);
	action->setChecked(_viewport->viewType() == Viewport::VIEW_FRONT);
	action->setData((int)Viewport::VIEW_FRONT);
	action = viewTypeGroup->addAction(tr("Back"));
	action->setCheckable(true);
	action->setChecked(_viewport->viewType() == Viewport::VIEW_BACK);
	action->setData((int)Viewport::VIEW_BACK);
	action = viewTypeGroup->addAction(tr("Left"));
	action->setCheckable(true);
	action->setChecked(_viewport->viewType() == Viewport::VIEW_LEFT);
	action->setData((int)Viewport::VIEW_LEFT);
	action = viewTypeGroup->addAction(tr("Right"));
	action->setCheckable(true);
	action->setChecked(_viewport->viewType() == Viewport::VIEW_RIGHT);
	action->setData((int)Viewport::VIEW_RIGHT);
	action = viewTypeGroup->addAction(tr("Ortho"));
	action->setCheckable(true);
	action->setChecked(_viewport->viewType() == Viewport::VIEW_ORTHO);
	action->setData((int)Viewport::VIEW_ORTHO);
	action = viewTypeGroup->addAction(tr("Perspective"));
	action->setCheckable(true);
	action->setChecked(_viewport->viewType() == Viewport::VIEW_PERSPECTIVE);
	action->setData((int)Viewport::VIEW_PERSPECTIVE);
	_viewTypeMenu->addActions(viewTypeGroup->actions());
	connect(viewTypeGroup, &QActionGroup::triggered, this, &ViewportMenu::onViewType);

	addSeparator();
	addAction(tr("Adjust View..."), this, SLOT(onAdjustView()))->setEnabled(_viewport->viewType() != Viewport::VIEW_SCENENODE);
}

/******************************************************************************
* Displays the menu.
******************************************************************************/
void ViewportMenu::show(const QPoint& pos)
{
	// Make sure deleteLater() calls are executed first.
	QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

	// Show context menu.
	exec(_vpWindow->mapToGlobal(pos));
}

/******************************************************************************
* Is called just before the "View Type" sub-menu is shown.
******************************************************************************/
void ViewportMenu::onShowViewTypeMenu()
{
	QActionGroup* viewNodeGroup = new QActionGroup(this);
	connect(viewNodeGroup, &QActionGroup::triggered, this, &ViewportMenu::onViewNode);

	// Find all camera nodes in the scene.
	_viewport->dataset()->sceneRoot()->visitObjectNodes([this, viewNodeGroup](PipelineSceneNode* node) -> bool {
		const PipelineFlowState& state = node->evaluatePipelinePreliminary(false);
		if(state.data() && state.data()->containsObject<AbstractCameraObject>()) {
			// Add a menu entry for this camera node.
			QAction* action = viewNodeGroup->addAction(node->nodeName());
			action->setCheckable(true);
			action->setChecked(_viewport->viewNode() == node);
			action->setData(QVariant::fromValue((void*)node));
		}
		return true;
	});

	// Add menu entries to menu.
	if(viewNodeGroup->actions().isEmpty() == false) {
		_viewTypeMenu->addSeparator();
		_viewTypeMenu->addActions(viewNodeGroup->actions());
	}

	_viewTypeMenu->addSeparator();
	_viewTypeMenu->addAction(tr("Create Camera"), this, SLOT(onCreateCamera()))->setEnabled(_viewport->viewNode() == nullptr);

	disconnect(_viewTypeMenu, &QMenu::aboutToShow, this, &ViewportMenu::onShowViewTypeMenu);
}

/******************************************************************************
* Handles the menu item event.
******************************************************************************/
void ViewportMenu::onRenderPreviewMode(bool checked)
{
	_viewport->setRenderPreviewMode(checked);
}

/******************************************************************************
* Handles the menu item event.
******************************************************************************/
void ViewportMenu::onShowGrid(bool checked)
{
	_viewport->setGridVisible(checked);
}

/******************************************************************************
* Handles the menu item event.
******************************************************************************/
void ViewportMenu::onConstrainRotation(bool checked)
{
	ViewportSettings::getSettings().setConstrainCameraRotation(checked);
	ViewportSettings::getSettings().save();
}

/******************************************************************************
* Handles the menu item event.
******************************************************************************/
void ViewportMenu::onViewType(QAction* action)
{
	_viewport->setViewType((Viewport::ViewType)action->data().toInt());

	// Remember which viewport was maximized across program sessions.
	// The same viewport will be maximized next time OVITO is started.
	if(_viewport->dataset()->viewportConfig()->maximizedViewport() == _viewport) {
		ViewportSettings::getSettings().setDefaultMaximizedViewportType(_viewport->viewType());
		ViewportSettings::getSettings().save();
	}
}

/******************************************************************************
* Handles the menu item event.
******************************************************************************/
void ViewportMenu::onAdjustView()
{
	AdjustCameraDialog dialog(_viewport, _vpWindow->window());
	dialog.exec();
}

/******************************************************************************
* Handles the menu item event.
******************************************************************************/
void ViewportMenu::onViewNode(QAction* action)
{
	PipelineSceneNode* viewNode = static_cast<PipelineSceneNode*>(action->data().value<void*>());
	OVITO_CHECK_OBJECT_POINTER(viewNode);

	UndoableTransaction::handleExceptions(_viewport->dataset()->undoStack(), tr("Set camera"), [this, viewNode]() {
		_viewport->setViewType(Viewport::VIEW_SCENENODE);
		_viewport->setViewNode(viewNode);
	});
}

/******************************************************************************
* Handles the menu item event.
******************************************************************************/
void ViewportMenu::onCreateCamera()
{
	UndoableTransaction::handleExceptions(_viewport->dataset()->undoStack(), tr("Create camera"), [this]() {
		RootSceneNode* scene = _viewport->dataset()->sceneRoot();
		AnimationSuspender animSuspender(_viewport->dataset()->animationSettings());

		// Create and initialize the camera object.
		OORef<PipelineSceneNode> cameraNode;
		{
			UndoSuspender noUndo(_viewport->dataset()->undoStack());

			QVector<OvitoClassPtr> cameraTypes = PluginManager::instance().listClasses(AbstractCameraObject::OOClass());
			if(cameraTypes.empty())
				_viewport->throwException(tr("OVITO has been built without support for camera objects."));
			OORef<AbstractCameraObject> cameraObj = static_object_cast<AbstractCameraObject>(cameraTypes.front()->createInstance(_viewport->dataset()));

			cameraObj->setPerspectiveCamera(_viewport->isPerspectiveProjection());
			if(_viewport->isPerspectiveProjection())
				cameraObj->setFieldOfView(0, _viewport->fieldOfView());
			else
				cameraObj->setFieldOfView(0, _viewport->fieldOfView());

			// Create an object node with a data source for the camera.
			OORef<DataCollection> cameraDataCollection = new DataCollection(_viewport->dataset());
			cameraDataCollection->addObject(cameraObj);
			OORef<StaticSource> cameraSource = new StaticSource(_viewport->dataset(), cameraDataCollection);
			cameraNode = new PipelineSceneNode(_viewport->dataset());
			cameraNode->setDataProvider(cameraSource);

			// Give the new node a name.
			cameraNode->setNodeName(scene->makeNameUnique(tr("Camera")));

			// Position camera node to match the current view.
			AffineTransformation tm = _viewport->projectionParams().inverseViewMatrix;
			if(_viewport->isPerspectiveProjection() == false) {
				// Position camera with parallel projection outside of scene bounding box.
				tm = tm * AffineTransformation::translation(
						Vector3(0, 0, -_viewport->projectionParams().znear + FloatType(0.2) * (_viewport->projectionParams().zfar -_viewport->projectionParams().znear)));
			}
			cameraNode->transformationController()->setTransformationValue(0, tm, true);
		}

		// Insert node into scene.
		scene->addChildNode(cameraNode);

		// Set new camera as view node for current viewport.
		_viewport->setViewType(Viewport::VIEW_SCENENODE);
		_viewport->setViewNode(cameraNode);
	});
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
