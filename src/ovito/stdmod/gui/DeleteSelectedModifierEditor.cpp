////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2016 Alexander Stukowski
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

#include <ovito/stdmod/gui/StdModGui.h>
#include <ovito/gui/desktop/properties/ModifierDelegateListParameterUI.h>
#include <ovito/stdmod/modifiers/DeleteSelectedModifier.h>
#include "DeleteSelectedModifierEditor.h"

namespace Ovito { namespace StdMod {

IMPLEMENT_OVITO_CLASS(DeleteSelectedModifierEditor);
SET_OVITO_OBJECT_EDITOR(DeleteSelectedModifier, DeleteSelectedModifierEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void DeleteSelectedModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Delete selected"), rolloutParams, "particles.modifiers.delete_selected_particles.html");

    // Create the rollout contents.
	QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(8);

	ModifierDelegateListParameterUI* delegatesPUI = new ModifierDelegateListParameterUI(this, rolloutParams.after(rollout));
	layout->addWidget(delegatesPUI->listWidget());

	// Status label.
	layout->addWidget(statusLabel());
}

}	// End of namespace
}	// End of namespace
