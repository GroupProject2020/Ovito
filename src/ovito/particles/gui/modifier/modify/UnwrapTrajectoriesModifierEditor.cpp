///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2019) Alexander Stukowski
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

#include <ovito/particles/gui/ParticlesGui.h>
#include <ovito/particles/modifier/modify/UnwrapTrajectoriesModifier.h>
#include <ovito/gui/mainwin/MainWindow.h>
#include <ovito/gui/utilities/concurrent/ProgressDialog.h>
#include "UnwrapTrajectoriesModifierEditor.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_CLASS(UnwrapTrajectoriesModifierEditor);
SET_OVITO_OBJECT_EDITOR(UnwrapTrajectoriesModifier, UnwrapTrajectoriesModifierEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void UnwrapTrajectoriesModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Unwrap trajectories"), rolloutParams, "particles.modifiers.unwrap_trajectories.html");

    // Create the rollout contents.
	QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(6);

	// Status label.
	layout->addWidget(statusLabel());
	layout->addSpacing(6);

	QPushButton* unwrapTrajectoriesButton = new QPushButton(tr("Update"));
	layout->addWidget(unwrapTrajectoriesButton);
	connect(unwrapTrajectoriesButton, &QPushButton::clicked, this, &UnwrapTrajectoriesModifierEditor::onUnwrapTrajectories);
}

/******************************************************************************
* Is called when the user clicks the 'Unwrap trajectories' button.
******************************************************************************/
void UnwrapTrajectoriesModifierEditor::onUnwrapTrajectories()
{
	UnwrapTrajectoriesModifier* modifier = static_object_cast<UnwrapTrajectoriesModifier>(editObject());
	if(!modifier) return;

	undoableTransaction(tr("Unwrap trajectories"), [this,modifier]() {
		ProgressDialog progressDialog(container(), modifier->dataset()->taskManager(), tr("Unwrapping trajectories"));
		modifier->detectPeriodicCrossings(progressDialog.taskManager());
	});
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
