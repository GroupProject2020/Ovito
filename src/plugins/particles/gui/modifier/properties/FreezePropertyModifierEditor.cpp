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

#include <plugins/particles/gui/ParticlesGui.h>
#include <plugins/particles/modifier/properties/FreezePropertyModifier.h>
#include <gui/properties/PropertyReferenceParameterUI.h>
#include <gui/properties/IntegerParameterUI.h>
#include "FreezePropertyModifierEditor.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Properties) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

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

	PropertyReferenceParameterUI* sourcePropertyUI = new PropertyReferenceParameterUI(this, PROPERTY_FIELD(FreezePropertyModifier::sourceProperty), &ParticleProperty::OOClass(), false, true);
	layout->addWidget(new QLabel(tr("Property to freeze:"), rollout));
	layout->addWidget(sourcePropertyUI->comboBox());
	connect(sourcePropertyUI, &PropertyReferenceParameterUI::valueEntered, this, &FreezePropertyModifierEditor::onSourcePropertyChanged);
	layout->addSpacing(8);

	PropertyReferenceParameterUI* destPropertyUI = new PropertyReferenceParameterUI(this, PROPERTY_FIELD(FreezePropertyModifier::destinationProperty), &ParticleProperty::OOClass(), false, false);
	layout->addWidget(new QLabel(tr("Output property:"), rollout));
	layout->addWidget(destPropertyUI->comboBox());
	layout->addSpacing(8);
	
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

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
