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

#include <ovito/particles/gui/ParticlesGui.h>
#include <ovito/particles/objects/TrajectoryVis.h>
#include <ovito/gui/desktop/properties/FloatParameterUI.h>
#include <ovito/gui/desktop/properties/BooleanParameterUI.h>
#include <ovito/gui/desktop/properties/VariantComboBoxParameterUI.h>
#include <ovito/gui/desktop/properties/ColorParameterUI.h>
#include "TrajectoryVisEditor.h"

namespace Ovito { namespace Particles {

IMPLEMENT_OVITO_CLASS(TrajectoryVisEditor);
SET_OVITO_OBJECT_EDITOR(TrajectoryVis, TrajectoryVisEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void TrajectoryVisEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Trajectory lines"), rolloutParams, "visual_elements.trajectory_lines.html");

    // Create the rollout contents.
	QGridLayout* layout = new QGridLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);
	layout->setColumnStretch(1, 1);

	// Shading mode.
	VariantComboBoxParameterUI* shadingModeUI = new VariantComboBoxParameterUI(this, PROPERTY_FIELD(TrajectoryVis::shadingMode));
	shadingModeUI->comboBox()->addItem(tr("Normal"), QVariant::fromValue(ArrowPrimitive::NormalShading));
	shadingModeUI->comboBox()->addItem(tr("Flat"), QVariant::fromValue(ArrowPrimitive::FlatShading));
	layout->addWidget(new QLabel(tr("Shading:")), 0, 0);
	layout->addWidget(shadingModeUI->comboBox(), 0, 1);

	// Line width.
	FloatParameterUI* lineWidthUI = new FloatParameterUI(this, PROPERTY_FIELD(TrajectoryVis::lineWidth));
	layout->addWidget(lineWidthUI->label(), 1, 0);
	layout->addLayout(lineWidthUI->createFieldLayout(), 1, 1);

	// Line color.
	ColorParameterUI* lineColorUI = new ColorParameterUI(this, PROPERTY_FIELD(TrajectoryVis::lineColor));
	layout->addWidget(lineColorUI->label(), 2, 0);
	layout->addWidget(lineColorUI->colorPicker(), 2, 1);

	// Wrapped line display.
	BooleanParameterUI* wrappedLinesUI = new BooleanParameterUI(this, PROPERTY_FIELD(TrajectoryVis::wrappedLines));
	layout->addWidget(wrappedLinesUI->checkBox(), 3, 0, 1, 2);

	// Up to current time.
	BooleanParameterUI* showUpToCurrentTimeUI = new BooleanParameterUI(this, PROPERTY_FIELD(TrajectoryVis::showUpToCurrentTime));
	layout->addWidget(showUpToCurrentTimeUI->checkBox(), 4, 0, 1, 2);
}

}	// End of namespace
}	// End of namespace
