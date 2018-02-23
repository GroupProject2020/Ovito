///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2017) Alexander Stukowski
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
#include <plugins/particles/modifier/properties/InterpolateTrajectoryModifier.h>
#include <gui/properties/BooleanParameterUI.h>
#include "InterpolateTrajectoryModifierEditor.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Properties) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_CLASS(InterpolateTrajectoryModifierEditor);
SET_OVITO_OBJECT_EDITOR(InterpolateTrajectoryModifier, InterpolateTrajectoryModifierEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void InterpolateTrajectoryModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	QWidget* rollout = createRollout(tr("Interpolate trajectory"), rolloutParams, "particles.modifiers.interpolate_trajectory.html");

    // Create the rollout contents.
	QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(2);

	BooleanParameterUI* useMinimumImageConventionUI = new BooleanParameterUI(this, PROPERTY_FIELD(InterpolateTrajectoryModifier::useMinimumImageConvention));
	layout->addWidget(useMinimumImageConventionUI->checkBox());
	
	// Status label.
	layout->addSpacing(8);
	layout->addWidget(statusLabel());
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
