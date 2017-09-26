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

#include <plugins/stdmod/gui/StdModGui.h>
#include <gui/properties/BooleanParameterUI.h>
#include <gui/properties/IntegerParameterUI.h>
#include <gui/properties/ModifierDelegateListParameterUI.h>
#include <plugins/stdmod/modifiers/ReplicateModifier.h>
#include "ReplicateModifierEditor.h"

namespace Ovito { namespace StdMod {

IMPLEMENT_OVITO_CLASS(ReplicateModifierEditor);
SET_OVITO_OBJECT_EDITOR(ReplicateModifier, ReplicateModifierEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void ReplicateModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	QWidget* rollout = createRollout(tr("Replicate"), rolloutParams, "particles.modifiers.show_periodic_images.html");

    // Create the rollout contents.
	QGridLayout* layout = new QGridLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
#ifndef Q_OS_MACX
	layout->setHorizontalSpacing(2);
	layout->setVerticalSpacing(2);
#endif
	layout->setColumnStretch(1, 1);

	IntegerParameterUI* numImagesXPUI = new IntegerParameterUI(this, PROPERTY_FIELD(ReplicateModifier::numImagesX));
	layout->addWidget(numImagesXPUI->label(), 0, 0);
	layout->addLayout(numImagesXPUI->createFieldLayout(), 0, 1);

	IntegerParameterUI* numImagesYPUI = new IntegerParameterUI(this, PROPERTY_FIELD(ReplicateModifier::numImagesY));
	layout->addWidget(numImagesYPUI->label(), 1, 0);
	layout->addLayout(numImagesYPUI->createFieldLayout(), 1, 1);

	IntegerParameterUI* numImagesZPUI = new IntegerParameterUI(this, PROPERTY_FIELD(ReplicateModifier::numImagesZ));
	layout->addWidget(numImagesZPUI->label(), 2, 0);
	layout->addLayout(numImagesZPUI->createFieldLayout(), 2, 1);

	BooleanParameterUI* adjustBoxSizeUI = new BooleanParameterUI(this, PROPERTY_FIELD(ReplicateModifier::adjustBoxSize));
	layout->addWidget(adjustBoxSizeUI->checkBox(), 3, 0, 1, 2);

	BooleanParameterUI* uniqueIdentifiersUI = new BooleanParameterUI(this, PROPERTY_FIELD(ReplicateModifier::uniqueIdentifiers));
	layout->addWidget(uniqueIdentifiersUI->checkBox(), 4, 0, 1, 2);

	// Create a second rollout.
	rollout = createRollout(tr("Operate on"), rolloutParams.after(rollout), "particles.modifiers.show_periodic_images.html");
	
	// Create the rollout contents.
	QVBoxLayout* topLayout = new QVBoxLayout(rollout);
	topLayout->setContentsMargins(4,4,4,4);
	topLayout->setSpacing(12);

	ModifierDelegateListParameterUI* delegatesPUI = new ModifierDelegateListParameterUI(this, rolloutParams.after(rollout));
	topLayout->addWidget(delegatesPUI->listWidget());	
}

}	// End of namespace
}	// End of namespace
