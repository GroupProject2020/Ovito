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

#include <ovito/particles/gui/ParticlesGui.h>
#include <ovito/oxdna/NucleotidesVis.h>
#include <ovito/gui/desktop/properties/FloatParameterUI.h>
#include "NucleotidesVisEditor.h"

namespace Ovito { namespace Particles {

IMPLEMENT_OVITO_CLASS(NucleotidesVisEditor);
SET_OVITO_OBJECT_EDITOR(NucleotidesVis, NucleotidesVisEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void NucleotidesVisEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Nucleotide display"), rolloutParams);

    // Create the rollout contents.
	QGridLayout* layout = new QGridLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);
	layout->setColumnStretch(1, 1);

	// Particle radius.
	FloatParameterUI* radiusUI = new FloatParameterUI(this, PROPERTY_FIELD(ParticlesVis::defaultParticleRadius));
	layout->addWidget(new QLabel(tr("Backbone centers radius:")), 0, 0);
	layout->addLayout(radiusUI->createFieldLayout(), 0, 1);

	// Cylinder radius.
	FloatParameterUI* cylinderRadiusUI = new FloatParameterUI(this, PROPERTY_FIELD(NucleotidesVis::cylinderRadius));
	layout->addWidget(cylinderRadiusUI->label(), 1, 0);
	layout->addLayout(cylinderRadiusUI->createFieldLayout(), 1, 1);
}

}	// End of namespace
}	// End of namespace
