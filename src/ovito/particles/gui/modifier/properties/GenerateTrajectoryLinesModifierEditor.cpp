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
#include <ovito/particles/modifier/properties/GenerateTrajectoryLinesModifier.h>
#include <ovito/gui/desktop/mainwin/MainWindow.h>
#include <ovito/gui/desktop/properties/IntegerParameterUI.h>
#include <ovito/gui/desktop/properties/StringParameterUI.h>
#include <ovito/gui/desktop/properties/BooleanParameterUI.h>
#include <ovito/gui/desktop/properties/IntegerRadioButtonParameterUI.h>
#include <ovito/gui/desktop/properties/BooleanRadioButtonParameterUI.h>
#include <ovito/gui/desktop/properties/SubObjectParameterUI.h>
#include <ovito/gui/desktop/widgets/general/ElidedTextLabel.h>
#include <ovito/gui/desktop/utilities/concurrent/ProgressDialog.h>
#include "GenerateTrajectoryLinesModifierEditor.h"

namespace Ovito { namespace Particles {

IMPLEMENT_OVITO_CLASS(GenerateTrajectoryLinesModifierEditor);
SET_OVITO_OBJECT_EDITOR(GenerateTrajectoryLinesModifier, GenerateTrajectoryLinesModifierEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void GenerateTrajectoryLinesModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Generate trajectory lines"), rolloutParams, "particles.modifiers.generate_trajectory_lines.html");

    // Create the rollout contents.
	QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(6);

	// Particle set
	{
		QGroupBox* groupBox = new QGroupBox(tr("Generate trajectories for"));
		layout->addWidget(groupBox);

		QVBoxLayout* layout2 = new QVBoxLayout(groupBox);
		layout2->setContentsMargins(4,4,4,4);
		layout2->setSpacing(4);

		BooleanRadioButtonParameterUI* onlySelectedParticlesUI = new BooleanRadioButtonParameterUI(this, PROPERTY_FIELD(GenerateTrajectoryLinesModifier::onlySelectedParticles));

		QRadioButton* allParticlesButton = onlySelectedParticlesUI->buttonFalse();
		allParticlesButton->setText(tr("All particles"));
		layout2->addWidget(allParticlesButton);

		QRadioButton* selectedParticlesButton = onlySelectedParticlesUI->buttonTrue();
		selectedParticlesButton->setText(tr("Selected particles"));
		layout2->addWidget(selectedParticlesButton);
	}

	// Periodic boundaries
	{
		QGroupBox* groupBox = new QGroupBox(tr("Periodic boundary conditions"));
		layout->addWidget(groupBox);

		QGridLayout* layout2 = new QGridLayout(groupBox);
		layout2->setContentsMargins(4,4,4,4);
		layout2->setSpacing(2);

		BooleanParameterUI* unwrapTrajectoriesUI = new BooleanParameterUI(this, PROPERTY_FIELD(GenerateTrajectoryLinesModifier::unwrapTrajectories));
		layout2->addWidget(unwrapTrajectoriesUI->checkBox(), 0, 0);
	}

	// Time range
	{
		QGroupBox* groupBox = new QGroupBox(tr("Time range"));
		layout->addWidget(groupBox);

		QVBoxLayout* layout2 = new QVBoxLayout(groupBox);
		layout2->setContentsMargins(4,4,4,4);
		layout2->setSpacing(2);
		QGridLayout* layout2c = new QGridLayout();
		layout2c->setContentsMargins(0,0,0,0);
		layout2c->setSpacing(2);
		layout2->addLayout(layout2c);

		BooleanRadioButtonParameterUI* useCustomIntervalUI = new BooleanRadioButtonParameterUI(this, PROPERTY_FIELD(GenerateTrajectoryLinesModifier::useCustomInterval));

		QRadioButton* animationIntervalButton = useCustomIntervalUI->buttonFalse();
		animationIntervalButton->setText(tr("Complete trajectory"));
		layout2c->addWidget(animationIntervalButton, 0, 0, 1, 5);

		QRadioButton* customIntervalButton = useCustomIntervalUI->buttonTrue();
		customIntervalButton->setText(tr("Frame interval:"));
		layout2c->addWidget(customIntervalButton, 1, 0, 1, 5);

		IntegerParameterUI* customRangeStartUI = new IntegerParameterUI(this, PROPERTY_FIELD(GenerateTrajectoryLinesModifier::customIntervalStart));
		customRangeStartUI->setEnabled(false);
		layout2c->addLayout(customRangeStartUI->createFieldLayout(), 2, 1);
		layout2c->addWidget(new QLabel(tr("to")), 2, 2);
		IntegerParameterUI* customRangeEndUI = new IntegerParameterUI(this, PROPERTY_FIELD(GenerateTrajectoryLinesModifier::customIntervalEnd));
		customRangeEndUI->setEnabled(false);
		layout2c->addLayout(customRangeEndUI->createFieldLayout(), 2, 3);
		layout2c->setColumnMinimumWidth(0, 30);
		layout2c->setColumnStretch(4, 1);
		connect(customIntervalButton, &QRadioButton::toggled, customRangeStartUI, &IntegerParameterUI::setEnabled);
		connect(customIntervalButton, &QRadioButton::toggled, customRangeEndUI, &IntegerParameterUI::setEnabled);

		QGridLayout* layout2a = new QGridLayout();
		layout2a->setContentsMargins(0,6,0,0);
		layout2a->setSpacing(2);
		layout2->addLayout(layout2a);
		IntegerParameterUI* everyNthFrameUI = new IntegerParameterUI(this, PROPERTY_FIELD(GenerateTrajectoryLinesModifier::everyNthFrame));
		layout2a->addWidget(everyNthFrameUI->label(), 0, 0);
		layout2a->addLayout(everyNthFrameUI->createFieldLayout(), 0, 1);
		layout2a->setColumnStretch(2, 1);
	}

	QPushButton* createTrajectoryButton = new QPushButton(tr("Generate trajectory lines"));
	layout->addWidget(createTrajectoryButton);
	connect(createTrajectoryButton, &QPushButton::clicked, this, &GenerateTrajectoryLinesModifierEditor::onRegenerateTrajectory);

	// Open a sub-editor for the trajectory vis element.
	new SubObjectParameterUI(this, PROPERTY_FIELD(GenerateTrajectoryLinesModifier::trajectoryVis), rolloutParams.after(rollout));
}

/******************************************************************************
* Is called when the user clicks the 'Regenerate trajectory' button.
******************************************************************************/
void GenerateTrajectoryLinesModifierEditor::onRegenerateTrajectory()
{
	GenerateTrajectoryLinesModifier* modifier = static_object_cast<GenerateTrajectoryLinesModifier>(editObject());
	if(!modifier) return;

	undoableTransaction(tr("Generate trajectory"), [&]() {
		ProgressDialog progressDialog(container(), modifier->dataset()->taskManager(), tr("Generating trajectory lines"));
		modifier->generateTrajectories(progressDialog.createOperation());
	});
}

}	// End of namespace
}	// End of namespace
