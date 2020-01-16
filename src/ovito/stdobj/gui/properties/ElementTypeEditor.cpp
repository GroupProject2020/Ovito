////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2020 Alexander Stukowski
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

#include <ovito/stdobj/gui/StdObjGui.h>
#include <ovito/stdobj/properties/ElementType.h>
#include <ovito/gui/properties/ColorParameterUI.h>
#include <ovito/gui/properties/FloatParameterUI.h>
#include <ovito/gui/properties/IntegerParameterUI.h>
#include <ovito/gui/properties/StringParameterUI.h>
#include <ovito/gui/properties/BooleanParameterUI.h>
#include "ElementTypeEditor.h"

namespace Ovito { namespace StdObj {

IMPLEMENT_OVITO_CLASS(ElementTypeEditor);
SET_OVITO_OBJECT_EDITOR(ElementType, ElementTypeEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void ElementTypeEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Element Type"), rolloutParams);

    // Create the rollout contents.
	QVBoxLayout* layout1 = new QVBoxLayout(rollout);
	layout1->setContentsMargins(4,4,4,4);

	QGroupBox* nameBox = new QGroupBox(tr("Type"), rollout);
	QGridLayout* gridLayout = new QGridLayout(nameBox);
	gridLayout->setContentsMargins(4,4,4,4);
	gridLayout->setColumnStretch(1, 1);
	layout1->addWidget(nameBox);

	// Name.
	StringParameterUI* namePUI = new StringParameterUI(this, PROPERTY_FIELD(ElementType::name));
	gridLayout->addWidget(new QLabel(tr("Name:")), 0, 0);
	gridLayout->addWidget(namePUI->textBox(), 0, 1);

	// Numeric ID.
	gridLayout->addWidget(new QLabel(tr("Numeric ID:")), 1, 0);
	QLabel* numericIdLabel = new QLabel();
	gridLayout->addWidget(numericIdLabel, 1, 1);
	connect(this, &PropertiesEditor::contentsReplaced, [numericIdLabel](RefTarget* newEditObject) {
		if(ElementType* ptype = static_object_cast<ElementType>(newEditObject))
			numericIdLabel->setText(QString::number(ptype->numericId()));
		else
			numericIdLabel->setText({});
	});

	QGroupBox* appearanceBox = new QGroupBox(tr("Appearance"), rollout);
	gridLayout = new QGridLayout(appearanceBox);
	gridLayout->setContentsMargins(4,4,4,4);
	gridLayout->setColumnStretch(1, 1);
	layout1->addWidget(appearanceBox);

	// Display color parameter.
	ColorParameterUI* colorPUI = new ColorParameterUI(this, PROPERTY_FIELD(ElementType::color));
	gridLayout->addWidget(colorPUI->label(), 0, 0);
	gridLayout->addWidget(colorPUI->colorPicker(), 0, 1);

	connect(this, &PropertiesEditor::contentsReplaced, [namePUI](RefTarget* newEditObject) {
		// Update the placeholder text of the name input field to reflect the numeric ID of the current particle type.
		if(QLineEdit* lineEdit = qobject_cast<QLineEdit*>(namePUI->textBox())) {
			if(ElementType* ptype = dynamic_object_cast<ElementType>(newEditObject))
				lineEdit->setPlaceholderText(QStringLiteral("[%1]").arg(ElementType::generateDefaultTypeName(ptype->numericId())));
			else
				lineEdit->setPlaceholderText({});
		}
	});
}

}	// End of namespace
}	// End of namespace
