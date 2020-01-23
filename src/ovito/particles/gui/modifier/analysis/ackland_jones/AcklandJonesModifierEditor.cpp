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
#include <ovito/particles/gui/modifier/analysis/StructureListParameterUI.h>
#include <ovito/particles/modifier/analysis/ackland_jones/AcklandJonesModifier.h>
#include <ovito/gui/desktop/properties/BooleanParameterUI.h>
#include "AcklandJonesModifierEditor.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_CLASS(AcklandJonesModifierEditor);
SET_OVITO_OBJECT_EDITOR(AcklandJonesModifier, AcklandJonesModifierEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void AcklandJonesModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Ackland-Jones analysis"), rolloutParams, "particles.modifiers.bond_angle_analysis.html");

    // Create the rollout contents.
	QVBoxLayout* layout1 = new QVBoxLayout(rollout);
	layout1->setContentsMargins(4,4,4,4);
	layout1->setSpacing(4);

	// Use only selected particles.
	BooleanParameterUI* onlySelectedParticlesUI = new BooleanParameterUI(this, PROPERTY_FIELD(StructureIdentificationModifier::onlySelectedParticles));
	layout1->addWidget(onlySelectedParticlesUI->checkBox());

	// Color by type
	BooleanParameterUI* colorByTypeUI = new BooleanParameterUI(this, PROPERTY_FIELD(StructureIdentificationModifier::colorByType));
	layout1->addWidget(colorByTypeUI->checkBox());

	// Status label.
	layout1->addSpacing(10);
	layout1->addWidget(statusLabel());

	StructureListParameterUI* structureTypesPUI = new StructureListParameterUI(this, true);
	layout1->addSpacing(10);
	layout1->addWidget(new QLabel(tr("Structure types:")));
	layout1->addWidget(structureTypesPUI->tableWidget());
	QLabel* label = new QLabel(tr("<p style=\"font-size: small;\">Double-click to change colors. Defaults can be set in the application settings.</p>"));
	label->setWordWrap(true);
	layout1->addWidget(label);
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
