///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2018) Alexander Stukowski
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

#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include <plugins/crystalanalysis/modifier/microstructure/SimplifyMicrostructureModifier.h>
#include <gui/properties/IntegerParameterUI.h>
#include <gui/properties/FloatParameterUI.h>
#include <gui/properties/BooleanParameterUI.h>
#include "SimplifyMicrostructureModifierEditor.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

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
}	// End of namespace
