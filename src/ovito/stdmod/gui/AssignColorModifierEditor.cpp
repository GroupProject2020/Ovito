////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2017 Alexander Stukowski
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
#include <ovito/gui/properties/ColorParameterUI.h>
#include <ovito/gui/properties/BooleanParameterUI.h>
#include <ovito/gui/properties/ModifierDelegateParameterUI.h>
#include <ovito/stdmod/modifiers/AssignColorModifier.h>
#include "AssignColorModifierEditor.h"

namespace Ovito { namespace StdMod {

IMPLEMENT_OVITO_CLASS(AssignColorModifierEditor);
SET_OVITO_OBJECT_EDITOR(AssignColorModifier, AssignColorModifierEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void AssignColorModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Assign color"), rolloutParams, "particles.modifiers.assign_color.html");

    // Create the rollout contents.
	QGridLayout* layout = new QGridLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(2);
	layout->setColumnStretch(1, 1);

	// Operate on.
	ModifierDelegateParameterUI* delegateUI = new ModifierDelegateParameterUI(this, AssignColorModifierDelegate::OOClass());
	layout->addWidget(new QLabel(tr("Operate on:")), 0, 0);
	layout->addWidget(delegateUI->comboBox(), 0, 1);

	// Color parameter.
	ColorParameterUI* constColorPUI = new ColorParameterUI(this, PROPERTY_FIELD(AssignColorModifier::colorController));
	layout->addWidget(constColorPUI->label(), 1, 0);
	layout->addWidget(constColorPUI->colorPicker(), 1, 1);

	// Keep selection parameter.
	BooleanParameterUI* keepSelectionPUI = new BooleanParameterUI(this, PROPERTY_FIELD(AssignColorModifier::keepSelection));
	layout->addWidget(keepSelectionPUI->checkBox(), 2, 0, 1, 2);
}

}	// End of namespace
}	// End of namespace
