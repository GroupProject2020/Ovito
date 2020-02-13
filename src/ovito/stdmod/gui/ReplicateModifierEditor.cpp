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
#include <ovito/gui/desktop/properties/BooleanParameterUI.h>
#include <ovito/gui/desktop/properties/IntegerParameterUI.h>
#include <ovito/gui/desktop/properties/ModifierDelegateListParameterUI.h>
#include <ovito/stdmod/modifiers/ReplicateModifier.h>
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
