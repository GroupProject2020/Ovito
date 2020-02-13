////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 Alexander Stukowski
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
#include <ovito/crystalanalysis/objects/BurgersVectorFamily.h>
#include <ovito/gui/desktop/properties/ColorParameterUI.h>
#include <ovito/gui/desktop/properties/StringParameterUI.h>
#include "BurgersVectorFamilyEditor.h"

namespace Ovito { namespace CrystalAnalysis {

IMPLEMENT_OVITO_CLASS(BurgersVectorFamilyEditor);
SET_OVITO_OBJECT_EDITOR(BurgersVectorFamily, BurgersVectorFamilyEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void BurgersVectorFamilyEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Burgers vector family"), rolloutParams);

    // Create the rollout contents.
	QGridLayout* layout1 = new QGridLayout(rollout);
	layout1->setContentsMargins(4,4,4,4);
	layout1->setSpacing(4);
	layout1->setColumnStretch(1, 1);

	// Text box for the name of type.
	StringParameterUI* namePUI = new StringParameterUI(this, PROPERTY_FIELD(BurgersVectorFamily::name));
	layout1->addWidget(new QLabel(tr("Name:")), 0, 0);
	layout1->addWidget(namePUI->textBox(), 0, 1);

	// Color parameter.
	ColorParameterUI* colorPUI = new ColorParameterUI(this, PROPERTY_FIELD(BurgersVectorFamily::color));
	layout1->addWidget(colorPUI->label(), 1, 0);
	layout1->addWidget(colorPUI->colorPicker(), 1, 1);
}

}	// End of namespace
}	// End of namespace
