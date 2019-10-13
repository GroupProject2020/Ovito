////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2013 Alexander Stukowski
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
#include <ovito/gui/actions/ActionManager.h>
#include <ovito/gui/dialogs/AnimationSettingsDialog.h>
#include <ovito/gui/mainwin/MainWindow.h>
#include <ovito/core/dataset/animation/AnimationSettings.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui)

/******************************************************************************
* Handles the ACTION_GOTO_START_OF_ANIMATION command.
******************************************************************************/
void ActionManager::on_AnimationGotoStart_triggered()
{
	_dataset->animationSettings()->jumpToAnimationStart();
}

/******************************************************************************
* Handles the ACTION_GOTO_END_OF_ANIMATION command.
******************************************************************************/
void ActionManager::on_AnimationGotoEnd_triggered()
{
	_dataset->animationSettings()->jumpToAnimationEnd();
}

/******************************************************************************
* Handles the ACTION_GOTO_PREVIOUS_FRAME command.
******************************************************************************/
void ActionManager::on_AnimationGotoPreviousFrame_triggered()
{
	_dataset->animationSettings()->jumpToPreviousFrame();
}

/******************************************************************************
* Handles the ACTION_GOTO_NEXT_FRAME command.
******************************************************************************/
void ActionManager::on_AnimationGotoNextFrame_triggered()
{
	_dataset->animationSettings()->jumpToNextFrame();
}

/******************************************************************************
* Handles the ACTION_START_ANIMATION_PLAYBACK command.
******************************************************************************/
void ActionManager::on_AnimationStartPlayback_triggered()
{
	if(!getAction(ACTION_TOGGLE_ANIMATION_PLAYBACK)->isChecked())
		getAction(ACTION_TOGGLE_ANIMATION_PLAYBACK)->trigger();
}

/******************************************************************************
* Handles the ACTION_STOP_ANIMATION_PLAYBACK command.
******************************************************************************/
void ActionManager::on_AnimationStopPlayback_triggered()
{
	if(getAction(ACTION_TOGGLE_ANIMATION_PLAYBACK)->isChecked())
		getAction(ACTION_TOGGLE_ANIMATION_PLAYBACK)->trigger();
}

/******************************************************************************
* Handles the ACTION_ANIMATION_SETTINGS command.
******************************************************************************/
void ActionManager::on_AnimationSettings_triggered()
{
	AnimationSettingsDialog(_dataset->animationSettings(), mainWindow()).exec();
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
