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
#include <ovito/particles/modifier/properties/SmoothTrajectoryModifier.h>
#include <ovito/gui/desktop/properties/BooleanParameterUI.h>
#include <ovito/gui/desktop/properties/IntegerParameterUI.h>
#include "SmoothTrajectoryModifierEditor.h"

namespace Ovito { namespace Particles {

IMPLEMENT_OVITO_CLASS(SmoothTrajectoryModifierEditor);
SET_OVITO_OBJECT_EDITOR(SmoothTrajectoryModifier, SmoothTrajectoryModifierEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void SmoothTrajectoryModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	QWidget* rollout = createRollout(tr("Smooth trajectory"), rolloutParams, "particles.modifiers.interpolate_trajectory.html");

    // Create the rollout contents.
	QGridLayout* layout = new QGridLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);
	layout->setColumnStretch(1, 1);

	// Smoothing window size parameter.
	IntegerParameterUI* smoothingWindowSizeUI = new IntegerParameterUI(this, PROPERTY_FIELD(SmoothTrajectoryModifier::smoothingWindowSize));
	layout->addWidget(smoothingWindowSizeUI->label(), 0, 0);
	layout->addLayout(smoothingWindowSizeUI->createFieldLayout(), 0, 1);

	BooleanParameterUI* useMinimumImageConventionUI = new BooleanParameterUI(this, PROPERTY_FIELD(SmoothTrajectoryModifier::useMinimumImageConvention));
	layout->addWidget(useMinimumImageConventionUI->checkBox(), 1, 0, 1, 2);

	// Status label.
	layout->setRowMinimumHeight(2, 8);
	layout->addWidget(statusLabel(), 3, 0, 1, 2);
}

}	// End of namespace
}	// End of namespace
