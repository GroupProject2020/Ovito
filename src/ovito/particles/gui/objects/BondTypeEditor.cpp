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

#include <ovito/particles/gui/ParticlesGui.h>
#include <ovito/particles/objects/BondType.h>
#include <ovito/gui/desktop/properties/ColorParameterUI.h>
#include <ovito/gui/desktop/properties/FloatParameterUI.h>
#include <ovito/gui/desktop/properties/IntegerParameterUI.h>
#include <ovito/gui/desktop/properties/StringParameterUI.h>
#include "BondTypeEditor.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_CLASS(BondTypeEditor);
SET_OVITO_OBJECT_EDITOR(BondType, BondTypeEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void BondTypeEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Bond Type"), rolloutParams, "scene_objects.particles.html");

    // Create the rollout contents.
	QGridLayout* layout1 = new QGridLayout(rollout);
	layout1->setContentsMargins(4,4,4,4);
#ifndef Q_OS_MACX
	layout1->setSpacing(0);
#endif
	layout1->setColumnStretch(1, 1);

	// Text box for the name of bond type.
	StringParameterUI* namePUI = new StringParameterUI(this, PROPERTY_FIELD(BondType::name));
	layout1->addWidget(new QLabel(tr("Name:")), 0, 0);
	layout1->addWidget(namePUI->textBox(), 0, 1);

	// Display color parameter.
	ColorParameterUI* colorPUI = new ColorParameterUI(this, PROPERTY_FIELD(BondType::color));
	layout1->addWidget(colorPUI->label(), 1, 0);
	layout1->addWidget(colorPUI->colorPicker(), 1, 1);

	// Numeric ID.
	IntegerParameterUI* idPUI = new IntegerParameterUI(this, PROPERTY_FIELD(BondType::numericId));
	idPUI->setEnabled(false);
	layout1->addWidget(new QLabel(tr("Numeric ID:")), 2, 0);
	layout1->addWidget(idPUI->textBox(), 2, 1);

	connect(this, &PropertiesEditor::contentsReplaced, [namePUI](RefTarget* newEditObject) {
		// Update the placeholder text of the name input field to reflect the numeric ID of the current bond type.
		if(QLineEdit* lineEdit = qobject_cast<QLineEdit*>(namePUI->textBox())) {
			if(ElementType* ptype = dynamic_object_cast<ElementType>(newEditObject))
				lineEdit->setPlaceholderText(QStringLiteral("[%1]").arg(ElementType::generateDefaultTypeName(ptype->numericId())));
			else
				lineEdit->setPlaceholderText({});
		}
	});
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
