///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2017) Alexander Stukowski
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

#include <plugins/stdmod/gui/StdModGui.h>
#include <gui/properties/ColorParameterUI.h>
#include <gui/properties/BooleanParameterUI.h>
#include <gui/properties/ModifierDelegateParameterUI.h>
#include <plugins/stdmod/modifiers/AssignColorModifier.h>
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
	layout->setSpacing(0);
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
