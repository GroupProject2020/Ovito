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

#include <ovito/stdmod/gui/StdModGui.h>
#include <ovito/stdmod/modifiers/FreezePropertyModifier.h>
#include <ovito/stdobj/gui/widgets/PropertyReferenceParameterUI.h>
#include <ovito/stdobj/gui/widgets/PropertyContainerParameterUI.h>
#include <ovito/stdobj/series/DataSeriesObject.h>
#include <ovito/gui/properties/IntegerParameterUI.h>
#include "FreezePropertyModifierEditor.h"

namespace Ovito { namespace StdMod {

IMPLEMENT_OVITO_CLASS(FreezePropertyModifierEditor);
SET_OVITO_OBJECT_EDITOR(FreezePropertyModifier, FreezePropertyModifierEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void FreezePropertyModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	QWidget* rollout = createRollout(tr("Freeze property"), rolloutParams, "particles.modifiers.freeze_property.html");

    // Create the rollout contents.
	QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(2);

	PropertyContainerParameterUI* pclassUI = new PropertyContainerParameterUI(this, PROPERTY_FIELD(GenericPropertyModifier::subject));
	layout->addWidget(new QLabel(tr("Operate on:")));
	layout->addWidget(pclassUI->comboBox());
	layout->addSpacing(8);

	// Do not list data series as available inputs.
	pclassUI->setContainerFilter([](const PropertyContainer* container) {
		return DataSeriesObject::OOClass().isMember(container) == false;
	});

	PropertyReferenceParameterUI* sourcePropertyUI = new PropertyReferenceParameterUI(this, PROPERTY_FIELD(FreezePropertyModifier::sourceProperty), nullptr, false, true);
	layout->addWidget(new QLabel(tr("Property to freeze:"), rollout));
	layout->addWidget(sourcePropertyUI->comboBox());
	connect(sourcePropertyUI, &PropertyReferenceParameterUI::valueEntered, this, &FreezePropertyModifierEditor::onSourcePropertyChanged);
	layout->addSpacing(8);

	PropertyReferenceParameterUI* destPropertyUI = new PropertyReferenceParameterUI(this, PROPERTY_FIELD(FreezePropertyModifier::destinationProperty), nullptr, false, false);
	layout->addWidget(new QLabel(tr("Output property:"), rollout));
	layout->addWidget(destPropertyUI->comboBox());
	layout->addSpacing(8);
	connect(this, &PropertiesEditor::contentsChanged, this, [sourcePropertyUI,destPropertyUI](RefTarget* editObject) {
		FreezePropertyModifier* modifier = static_object_cast<FreezePropertyModifier>(editObject);
		if(modifier) {
			sourcePropertyUI->setContainerRef(modifier->subject());
			destPropertyUI->setContainerRef(modifier->subject());
		}
		else {
			sourcePropertyUI->setContainerRef({});
			destPropertyUI->setContainerRef({});
		}
	});

	QGridLayout* gridlayout = new QGridLayout();
	gridlayout->setContentsMargins(0,0,0,0);
	gridlayout->setColumnStretch(1, 1);

	IntegerParameterUI* freezeTimePUI = new IntegerParameterUI(this, PROPERTY_FIELD(FreezePropertyModifier::freezeTime));
	gridlayout->addWidget(freezeTimePUI->label(), 0, 0);
	gridlayout->addLayout(freezeTimePUI->createFieldLayout(), 0, 1);
	layout->addLayout(gridlayout);
	layout->addSpacing(8);

	// Status label.
	layout->addSpacing(12);
	layout->addWidget(statusLabel());
}

/******************************************************************************
* Is called when the user has selected a different source property.
******************************************************************************/
void FreezePropertyModifierEditor::onSourcePropertyChanged()
{
	FreezePropertyModifier* mod = static_object_cast<FreezePropertyModifier>(editObject());
	if(!mod) return;

	undoableTransaction(tr("Freeze property"), [this,mod]() {
		// When the user selects a different source property, adjust the destination property automatically.
		mod->setDestinationProperty(mod->sourceProperty());
	});
}

}	// End of namespace
}	// End of namespace
