///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2013) Alexander Stukowski
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
#include <core/dataset/UndoStack.h>
#include <core/dataset/DataSetContainer.h>
#include <core/dataset/scene/SelectionSet.h>
#include <core/dataset/scene/RootSceneNode.h>
#include <core/dataset/animation/AnimationSettings.h>
#include <gui/viewport/input/NavigationModes.h>
#include <gui/viewport/input/ViewportInputMode.h>
#include <gui/viewport/input/ViewportInputManager.h>
#include <gui/actions/ViewportModeAction.h>
#include <gui/mainwin/MainWindow.h>
#include <gui/dialogs/ClonePipelineDialog.h>
#include "ActionManager.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui)

/******************************************************************************
* Initializes the ActionManager.
******************************************************************************/
ActionManager::ActionManager(MainWindow* mainWindow) : QObject(mainWindow)
{
	// Actions need to be updated whenever a new dataset is loaded or the current selection changes.
	connect(&mainWindow->datasetContainer(), &DataSetContainer::dataSetChanged, this, &ActionManager::onDataSetChanged);
	connect(&mainWindow->datasetContainer(), &DataSetContainer::animationSettingsReplaced, this, &ActionManager::onAnimationSettingsReplaced);
	connect(&mainWindow->datasetContainer(), &DataSetContainer::selectionChangeComplete, this, &ActionManager::onSelectionChangeComplete);

	createCommandAction(ACTION_QUIT, tr("Exit"), ":/gui/actions/file/file_quit.bw.svg", tr("Quit the application."), QKeySequence::Quit);
	createCommandAction(ACTION_FILE_NEW, tr("Reset State"), ":/gui/actions/file/file_new.bw.svg", tr("Resets the program to its initial state."), QKeySequence::New);
	createCommandAction(ACTION_FILE_OPEN, tr("Load Program State"), ":/gui/actions/file/file_open.bw.svg", tr("Load a saved state from a file."), QKeySequence::Open);
	createCommandAction(ACTION_FILE_SAVE, tr("Save Program State"), ":/gui/actions/file/file_save.bw.svg", tr("Save the current program state to a file."), QKeySequence::Save);
	createCommandAction(ACTION_FILE_SAVEAS, tr("Save Program State As"), ":/gui/actions/file/file_save_as.bw.svg", tr("Save the current program state to a new file."), QKeySequence::SaveAs);
	createCommandAction(ACTION_FILE_IMPORT, tr("Load File"), ":/gui/actions/file/file_import.bw.svg", tr("Import data from a file on this computer."), Qt::CTRL + Qt::Key_I);
	createCommandAction(ACTION_FILE_REMOTE_IMPORT, tr("Load Remote File"), ":/gui/actions/file/file_import_remote.bw.svg", tr("Import a file from a remote location."), Qt::CTRL + Qt::SHIFT + Qt::Key_I);
	createCommandAction(ACTION_FILE_EXPORT, tr("Export File"), ":/gui/actions/file/file_export.bw.svg", tr("Export data to a file."), Qt::CTRL + Qt::Key_E);
	createCommandAction(ACTION_FILE_NEW_WINDOW, tr("New Program Window"), ":/gui/actions/file/new_window.bw.svg", tr("Opens a new OVITO window."));
	createCommandAction(ACTION_HELP_ABOUT, tr("About Ovito"), ":/gui/actions/file/about.bw.svg", tr("Show information about the application."));
	createCommandAction(ACTION_HELP_SHOW_ONLINE_HELP, tr("User Manual"), ":/gui/actions/file/user_manual.bw.svg", tr("Open the user manual."), QKeySequence::HelpContents);
	createCommandAction(ACTION_HELP_SHOW_SCRIPTING_HELP, tr("Scripting Reference"), ":/gui/actions/file/scripting_manual.bw.svg", tr("Open the scripting reference."));
	createCommandAction(ACTION_HELP_OPENGL_INFO, tr("OpenGL Information"), ":/gui/actions/file/opengl_info.bw.svg", tr("Display OpenGL graphics driver information."));

	createCommandAction(ACTION_EDIT_UNDO, tr("Undo"), ":/gui/actions/edit/edit_undo.bw.svg", tr("Reverse a user action."), QKeySequence::Undo);
	createCommandAction(ACTION_EDIT_REDO, tr("Redo"), ":/gui/actions/edit/edit_redo.bw.svg", tr("Redo the previously undone user action."), QKeySequence::Redo);
	createCommandAction(ACTION_EDIT_CLEAR_UNDO_STACK, tr("Clear Undo Stack"), nullptr, tr("Discards all existing undo records."));

	createCommandAction(ACTION_EDIT_CLONE_PIPELINE, tr("Clone Pipeline"), ":/gui/actions/edit/clone_pipeline.bw.svg", tr("Duplicates the current pipeline to show multiple datasets side by side."));
	createCommandAction(ACTION_EDIT_DELETE, tr("Delete Pipeline"), ":/gui/actions/edit/edit_delete.bw.svg", tr("Deletes the selected object from the scene."));

	createCommandAction(ACTION_SETTINGS_DIALOG, tr("&Application Settings..."), ":/gui/actions/file/preferences.bw.svg", QString(), QKeySequence::Preferences);

	createCommandAction(ACTION_RENDER_ACTIVE_VIEWPORT, tr("Render Active Viewport"), ":/gui/actions/rendering/render_active_viewport.bw.svg");

	createCommandAction(ACTION_VIEWPORT_MAXIMIZE, tr("Maximize Active Viewport"), ":/gui/actions/viewport/maximize_viewport.bw.svg", tr("Enlarge/reduce the active viewport."));
	createCommandAction(ACTION_VIEWPORT_ZOOM_SCENE_EXTENTS, tr("Zoom Scene Extents"), ":/gui/actions/viewport/zoom_scene_extents.bw.svg", tr("Zoom active viewport to show everything."));
	createCommandAction(ACTION_VIEWPORT_ZOOM_SCENE_EXTENTS_ALL, tr("Zoom Scene Extents All"), ":/gui/actions/viewport/zoom_scene_extents_all.png", tr("Zoom all viewports to show everything."));
	createCommandAction(ACTION_VIEWPORT_ZOOM_SELECTION_EXTENTS, tr("Zoom Selection Extents"), ":/gui/actions/viewport/zoom_selection_extents.png", tr("Zoom active viewport to show the selected objects."));
	createCommandAction(ACTION_VIEWPORT_ZOOM_SELECTION_EXTENTS_ALL, tr("Zoom Selection Extents All"), ":/gui/actions/viewport/zoom_selection_extents.png", tr("Zoom all viewports to show the selected objects."));

	ViewportInputManager* vpInputManager = mainWindow->viewportInputManager();
	createViewportModeAction(ACTION_VIEWPORT_ZOOM, vpInputManager->zoomMode(), tr("Zoom"), ":/gui/actions/viewport/mode_zoom.bw.svg", tr("Activate zoom mode."));
	createViewportModeAction(ACTION_VIEWPORT_PAN, vpInputManager->panMode(), tr("Pan"), ":/gui/actions/viewport/mode_pan.bw.svg", tr("Activate pan mode to shift the region visible in the viewports."));
	createViewportModeAction(ACTION_VIEWPORT_ORBIT, vpInputManager->orbitMode(), tr("Orbit Camera"), ":/gui/actions/viewport/mode_orbit.bw.svg", tr("Activate orbit mode to rotate the camera around the scene."));
	createViewportModeAction(ACTION_VIEWPORT_FOV, vpInputManager->fovMode(), tr("Change Field Of View"), ":/gui/actions/viewport/mode_fov.bw.svg", tr("Activate field of view mode to change the perspective projection."));
	createViewportModeAction(ACTION_VIEWPORT_PICK_ORBIT_CENTER, vpInputManager->pickOrbitCenterMode(), tr("Set Orbit Center"), ":/gui/actions/viewport/mode_set_orbit_center.png", tr("Set the center of rotation."));

	createViewportModeAction(ACTION_SELECTION_MODE, vpInputManager->selectionMode(), tr("Select"), ":/gui/actions/edit/mode_select.bw.svg", tr("Select objects in the viewports."));
	createViewportModeAction(ACTION_XFORM_MOVE_MODE, vpInputManager->moveMode(), tr("Move"), ":/gui/actions/edit/mode_move.bw.svg", tr("Move objects."));
	createViewportModeAction(ACTION_XFORM_ROTATE_MODE, vpInputManager->rotateMode(), tr("Rotate"), ":/gui/actions/edit/mode_rotate.bw.svg", tr("Rotate objects."));

	createCommandAction(ACTION_GOTO_START_OF_ANIMATION, tr("Go to Start of Animation"), ":/gui/actions/animation/goto_animation_start.bw.svg", QString(), Qt::Key_Home);
	createCommandAction(ACTION_GOTO_END_OF_ANIMATION, tr("Go to End of Animation"), ":/gui/actions/animation/goto_animation_end.bw.svg", QString(), Qt::Key_End);
	createCommandAction(ACTION_GOTO_PREVIOUS_FRAME, tr("Go to Previous Frame"), ":/gui/actions/animation/goto_previous_frame.bw.svg", QString(), Qt::ALT + Qt::Key_Left);
	createCommandAction(ACTION_GOTO_NEXT_FRAME, tr("Go to Next Frame"), ":/gui/actions/animation/goto_next_frame.bw.svg", QString(), Qt::ALT + Qt::Key_Right);
	createCommandAction(ACTION_START_ANIMATION_PLAYBACK, tr("Start Animation Playback"), ":/gui/actions/animation/play_animation.bw.svg");
	createCommandAction(ACTION_STOP_ANIMATION_PLAYBACK, tr("Stop Animation Playback"), ":/gui/actions/animation/stop_animation.bw.svg");
	createCommandAction(ACTION_ANIMATION_SETTINGS, tr("Animation Settings"), ":/gui/actions/animation/animation_settings.bw.svg");
	createCommandAction(ACTION_TOGGLE_ANIMATION_PLAYBACK, tr("Play Animation"), ":/gui/actions/animation/play_animation.bw.svg", tr("Start/stop animation playback. Hold down Shift key to play backwards."), Qt::Key_Space)->setCheckable(true);
	createCommandAction(ACTION_AUTO_KEY_MODE_TOGGLE, tr("Auto Key Mode"), ":/gui/actions/animation/animation_mode.bw.svg")->setCheckable(true);

	QMetaObject::connectSlotsByName(this);
}

/******************************************************************************
* This is called when a new dataset has been loaded.
******************************************************************************/
void ActionManager::onDataSetChanged(DataSet* newDataSet)
{
	disconnect(_canUndoChangedConnection);
	disconnect(_canRedoChangedConnection);
	disconnect(_undoTextChangedConnection);
	disconnect(_redoTextChangedConnection);
	disconnect(_undoTriggeredConnection);
	disconnect(_redoTriggeredConnection);
	disconnect(_clearUndoStackTriggeredConnection);
	_dataset = newDataSet;
	QAction* undoAction = getAction(ACTION_EDIT_UNDO);
	QAction* redoAction = getAction(ACTION_EDIT_REDO);
	QAction* clearUndoStackAction = getAction(ACTION_EDIT_CLEAR_UNDO_STACK);
	if(newDataSet) {
		undoAction->setEnabled(newDataSet->undoStack().canUndo());
		redoAction->setEnabled(newDataSet->undoStack().canRedo());
		clearUndoStackAction->setEnabled(true);
		undoAction->setText(tr("Undo %1").arg(newDataSet->undoStack().undoText()));
		redoAction->setText(tr("Redo %1").arg(newDataSet->undoStack().redoText()));
		_canUndoChangedConnection = connect(&newDataSet->undoStack(), &UndoStack::canUndoChanged, undoAction, &QAction::setEnabled);
		_canRedoChangedConnection = connect(&newDataSet->undoStack(), &UndoStack::canRedoChanged, redoAction, &QAction::setEnabled);
		_undoTextChangedConnection = connect(&newDataSet->undoStack(), &UndoStack::undoTextChanged, [this,undoAction](const QString& undoText) {
			undoAction->setText(tr("Undo %1").arg(undoText));
		});
		_redoTextChangedConnection = connect(&newDataSet->undoStack(), &UndoStack::redoTextChanged, [this,redoAction](const QString& redoText) {
			redoAction->setText(tr("Redo %1").arg(redoText));
		});
		_undoTriggeredConnection = connect(undoAction, &QAction::triggered, &newDataSet->undoStack(), &UndoStack::undo);
		_redoTriggeredConnection = connect(redoAction, &QAction::triggered, &newDataSet->undoStack(), &UndoStack::redo);
		_clearUndoStackTriggeredConnection = connect(clearUndoStackAction, &QAction::triggered, &newDataSet->undoStack(), &UndoStack::clear);
	}
	else {
		undoAction->setEnabled(false);
		redoAction->setEnabled(false);
		clearUndoStackAction->setEnabled(false);
	}
}

/******************************************************************************
* This is called when new animation settings have been loaded.
******************************************************************************/
void ActionManager::onAnimationSettingsReplaced(AnimationSettings* newAnimationSettings)
{
	disconnect(_autoKeyModeChangedConnection);
	disconnect(_autoKeyModeToggledConnection);
	disconnect(_animationIntervalChangedConnection);
	disconnect(_animationPlaybackChangedConnection);
	disconnect(_animationPlaybackToggledConnection);
	QAction* autoKeyModeAction = getAction(ACTION_AUTO_KEY_MODE_TOGGLE);
	QAction* animationPlaybackAction = getAction(ACTION_TOGGLE_ANIMATION_PLAYBACK);
	if(newAnimationSettings) {
		autoKeyModeAction->setEnabled(true);
		autoKeyModeAction->setChecked(newAnimationSettings->autoKeyMode());
		animationPlaybackAction->setEnabled(true);
		animationPlaybackAction->setChecked(newAnimationSettings->isPlaybackActive());
		_autoKeyModeChangedConnection = connect(newAnimationSettings, &AnimationSettings::autoKeyModeChanged, autoKeyModeAction, &QAction::setChecked);
		_autoKeyModeToggledConnection = connect(autoKeyModeAction, &QAction::toggled, newAnimationSettings, &AnimationSettings::setAutoKeyMode);
		_animationIntervalChangedConnection = connect(newAnimationSettings, &AnimationSettings::intervalChanged, this, &ActionManager::onAnimationIntervalChanged);
		_animationPlaybackChangedConnection = connect(newAnimationSettings, &AnimationSettings::playbackChanged, animationPlaybackAction, &QAction::setChecked);
		_animationPlaybackToggledConnection = connect(animationPlaybackAction, &QAction::toggled, newAnimationSettings, &AnimationSettings::setAnimationPlayback);
		onAnimationIntervalChanged(newAnimationSettings->animationInterval());
	}
	else {
		autoKeyModeAction->setEnabled(false);
		animationPlaybackAction->setEnabled(false);
		onAnimationIntervalChanged(TimeInterval(0));
	}
}

/******************************************************************************
* This is called when the active animation interval has changed.
******************************************************************************/
void ActionManager::onAnimationIntervalChanged(TimeInterval newAnimationInterval)
{
	bool isAnimationInterval = newAnimationInterval.duration() != 0;
	getAction(ACTION_GOTO_START_OF_ANIMATION)->setEnabled(isAnimationInterval);
	getAction(ACTION_GOTO_PREVIOUS_FRAME)->setEnabled(isAnimationInterval);
	getAction(ACTION_TOGGLE_ANIMATION_PLAYBACK)->setEnabled(isAnimationInterval);
	getAction(ACTION_GOTO_NEXT_FRAME)->setEnabled(isAnimationInterval);
	getAction(ACTION_GOTO_END_OF_ANIMATION)->setEnabled(isAnimationInterval);
}

/******************************************************************************
* This is called whenever the scene node selection changed.
******************************************************************************/
void ActionManager::onSelectionChangeComplete(SelectionSet* selection)
{
	getAction(ACTION_EDIT_DELETE)->setEnabled(selection && !selection->nodes().empty());
	getAction(ACTION_EDIT_CLONE_PIPELINE)->setEnabled(selection && !selection->nodes().empty());
}

/******************************************************************************
* Invokes the command action with the given ID.
******************************************************************************/
void ActionManager::invokeAction(const QString& actionId)
{
	QAction* action = getAction(actionId);
	if(!action) throw Exception(tr("Action with id '%1' is not defined.").arg(actionId), _dataset);
	action->trigger();
}

/******************************************************************************
* Registers an action with the ActionManager.
******************************************************************************/
void ActionManager::addAction(QAction* action)
{
	OVITO_CHECK_POINTER(action);
	OVITO_ASSERT_MSG(action->parent() == this || findAction(action->objectName()) == nullptr, "ActionManager::addAction()", "There is already an action with the same ID.");

	// Make it a child of this manager.
	action->setParent(this);
}

/******************************************************************************
* Creates and registers a new command action with the ActionManager.
******************************************************************************/
QAction* ActionManager::createCommandAction(const QString& id, const QString& title, const char* iconPath, const QString& statusTip, const QKeySequence& shortcut)
{
	QAction* action = new QAction(title, this);
	action->setObjectName(id);
	if(!shortcut.isEmpty())
		action->setShortcut(shortcut);
	if(!statusTip.isEmpty())
		action->setStatusTip(statusTip);
	if(!shortcut.isEmpty())
		action->setToolTip(QStringLiteral("%1 [%2]").arg(title).arg(shortcut.toString(QKeySequence::NativeText)));
	if(iconPath)
		action->setIcon(QIcon(QString(iconPath)));
	addAction(action);
	return action;
}

/******************************************************************************
* Creates and registers a new viewport mode action with the ActionManager.
******************************************************************************/
QAction* ActionManager::createViewportModeAction(const QString& id, ViewportInputMode* inputHandler, const QString& title, const char* iconPath, const QString& statusTip, const QKeySequence& shortcut)
{
	QAction* action = new ViewportModeAction(mainWindow(), title, this, inputHandler);
	action->setObjectName(id);
	if(!shortcut.isEmpty())
		action->setShortcut(shortcut);
	action->setStatusTip(statusTip);
	if(!shortcut.isEmpty())
		action->setToolTip(QStringLiteral("%1 [%2]").arg(title).arg(shortcut.toString(QKeySequence::NativeText)));
	if(iconPath)
		action->setIcon(QIcon(QString(iconPath)));
	addAction(action);
	return action;
}

/******************************************************************************
* Handles ACTION_EDIT_DELETE command
******************************************************************************/
void ActionManager::on_EditDelete_triggered()
{
	UndoableTransaction::handleExceptions(_dataset->undoStack(), tr("Delete pipeline"), [this]() {
		// Delete all nodes in selection set.
		for(SceneNode* node : _dataset->selection()->nodes())
			node->deleteNode();

		// Automatically select one of the remaining nodes.
		if(_dataset->sceneRoot()->children().isEmpty() == false)
			_dataset->selection()->setNode(_dataset->sceneRoot()->children().front());
	});
}

/******************************************************************************
* Handles ACTION_EDIT_CLONE_PIPELINE command
******************************************************************************/
void ActionManager::on_ClonePipeline_triggered()
{
	if(PipelineSceneNode* node = dynamic_object_cast<PipelineSceneNode>(_dataset->selection()->firstNode())) {
		ClonePipelineDialog dialog(node, mainWindow());
		dialog.exec();
	}
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
