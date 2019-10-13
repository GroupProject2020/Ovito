////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2018 Alexander Stukowski
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

#include <ovito/crystalanalysis/CrystalAnalysis.h>
#include <ovito/crystalanalysis/modifier/microstructure/SimplifyMicrostructureModifier.h>
#include <ovito/gui/properties/IntegerParameterUI.h>
#include <ovito/gui/properties/FloatParameterUI.h>
#include <ovito/gui/properties/BooleanParameterUI.h>
#include "SimplifyMicrostructureModifierEditor.h"

namespace Ovito { namespace CrystalAnalysis {

IMPLEMENT_OVITO_CLASS(SimplifyMicrostructureModifierEditor);
SET_OVITO_OBJECT_EDITOR(SimplifyMicrostructureModifier, SimplifyMicrostructureModifierEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void SimplifyMicrostructureModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create the rollout.
	QWidget* rollout = createRollout(tr("Simplify microstructure"), rolloutParams);

    QGridLayout* layout = new QGridLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(6);
	layout->setColumnStretch(1, 1);

	IntegerParameterUI* smoothingLevelUI = new IntegerParameterUI(this, PROPERTY_FIELD(SimplifyMicrostructureModifier::smoothingLevel));
	layout->addWidget(smoothingLevelUI->label(), 0, 0);
	layout->addLayout(smoothingLevelUI->createFieldLayout(), 0, 1);

	FloatParameterUI* kPBUI = new FloatParameterUI(this, PROPERTY_FIELD(SimplifyMicrostructureModifier::kPB));
	layout->addWidget(kPBUI->label(), 1, 0);
	layout->addLayout(kPBUI->createFieldLayout(), 1, 1);

	FloatParameterUI* lambdaUI = new FloatParameterUI(this, PROPERTY_FIELD(SimplifyMicrostructureModifier::lambda));
	layout->addWidget(lambdaUI->label(), 2, 0);
	layout->addLayout(lambdaUI->createFieldLayout(), 2, 1);
}

}	// End of namespace
}	// End of namespace
