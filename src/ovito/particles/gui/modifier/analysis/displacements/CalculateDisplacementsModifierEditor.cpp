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
#include <ovito/particles/modifier/analysis/displacements/CalculateDisplacementsModifier.h>
#include <ovito/gui/desktop/properties/BooleanParameterUI.h>
#include <ovito/gui/desktop/properties/BooleanRadioButtonParameterUI.h>
#include <ovito/gui/desktop/properties/IntegerParameterUI.h>
#include <ovito/gui/desktop/properties/IntegerRadioButtonParameterUI.h>
#include <ovito/gui/desktop/properties/SubObjectParameterUI.h>
#include <ovito/gui/desktop/properties/VariantComboBoxParameterUI.h>
#include <ovito/core/dataset/io/FileSource.h>
#include "CalculateDisplacementsModifierEditor.h"

namespace Ovito { namespace Particles {

IMPLEMENT_OVITO_CLASS(CalculateDisplacementsModifierEditor);
SET_OVITO_OBJECT_EDITOR(CalculateDisplacementsModifier, CalculateDisplacementsModifierEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void CalculateDisplacementsModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Calculate displacements"), rolloutParams, "particles.modifiers.displacement_vectors.html");

    // Create the rollout contents.
	QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);

	QGroupBox* mappingGroupBox = new QGroupBox(tr("Affine mapping of simulation cell"));
	layout->addWidget(mappingGroupBox);

	QGridLayout* sublayout = new QGridLayout(mappingGroupBox);
	sublayout->setContentsMargins(4,4,4,4);
	sublayout->setSpacing(4);

	IntegerRadioButtonParameterUI* affineMappingUI = new IntegerRadioButtonParameterUI(this, PROPERTY_FIELD(ReferenceConfigurationModifier::affineMapping));
    sublayout->addWidget(affineMappingUI->addRadioButton(ReferenceConfigurationModifier::NO_MAPPING, tr("Off")), 0, 0);
	sublayout->addWidget(affineMappingUI->addRadioButton(ReferenceConfigurationModifier::TO_REFERENCE_CELL, tr("To reference")), 0, 1);
    sublayout->addWidget(affineMappingUI->addRadioButton(ReferenceConfigurationModifier::TO_CURRENT_CELL, tr("To current")), 1, 1);

	BooleanParameterUI* useMinimumImageConventionUI = new BooleanParameterUI(this, PROPERTY_FIELD(ReferenceConfigurationModifier::useMinimumImageConvention));
	sublayout->addWidget(useMinimumImageConventionUI->checkBox(), 2, 0, 1, 2);

	QGroupBox* referenceFrameGroupBox = new QGroupBox(tr("Reference animation frame"));
	layout->addWidget(referenceFrameGroupBox);

	sublayout = new QGridLayout(referenceFrameGroupBox);
	sublayout->setContentsMargins(4,4,4,4);
	sublayout->setSpacing(4);
	sublayout->setColumnStretch(0, 5);
	sublayout->setColumnStretch(2, 95);

	// Add box for selection between absolute and relative reference frames.
	BooleanRadioButtonParameterUI* useFrameOffsetUI = new BooleanRadioButtonParameterUI(this, PROPERTY_FIELD(ReferenceConfigurationModifier::useReferenceFrameOffset));
	useFrameOffsetUI->buttonFalse()->setText(tr("Constant reference configuration"));
	sublayout->addWidget(useFrameOffsetUI->buttonFalse(), 0, 0, 1, 3);

	IntegerParameterUI* frameNumberUI = new IntegerParameterUI(this, PROPERTY_FIELD(ReferenceConfigurationModifier::referenceFrameNumber));
	frameNumberUI->label()->setText(tr("Frame number:"));
	sublayout->addWidget(frameNumberUI->label(), 1, 1, 1, 1);
	sublayout->addLayout(frameNumberUI->createFieldLayout(), 1, 2, 1, 1);
	frameNumberUI->setEnabled(false);
	connect(useFrameOffsetUI->buttonFalse(), &QRadioButton::toggled, frameNumberUI, &IntegerParameterUI::setEnabled);

	useFrameOffsetUI->buttonTrue()->setText(tr("Relative to current frame"));
	sublayout->addWidget(useFrameOffsetUI->buttonTrue(), 2, 0, 1, 3);
	IntegerParameterUI* frameOffsetUI = new IntegerParameterUI(this, PROPERTY_FIELD(ReferenceConfigurationModifier::referenceFrameOffset));
	frameOffsetUI->label()->setText(tr("Frame offset:"));
	sublayout->addWidget(frameOffsetUI->label(), 3, 1, 1, 1);
	sublayout->addLayout(frameOffsetUI->createFieldLayout(), 3, 2, 1, 1);
	frameOffsetUI->setEnabled(false);
	connect(useFrameOffsetUI->buttonTrue(), &QRadioButton::toggled, frameOffsetUI, &IntegerParameterUI::setEnabled);

	QGroupBox* referenceSourceGroupBox = new QGroupBox(tr("Reference configuration source"));
	layout->addWidget(referenceSourceGroupBox);

	sublayout = new QGridLayout(referenceSourceGroupBox);
	sublayout->setContentsMargins(4,4,4,4);
	sublayout->setSpacing(6);

	_sourceButtonGroup = new QButtonGroup(this);
	connect(_sourceButtonGroup, static_cast<void (QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked), this, &CalculateDisplacementsModifierEditor::onSourceButtonClicked);
	QRadioButton* upstreamPipelineBtn = new QRadioButton(tr("Upstream pipeline"));
	QRadioButton* externalFileBtn = new QRadioButton(tr("External file"));
	_sourceButtonGroup->addButton(upstreamPipelineBtn, 0);
	_sourceButtonGroup->addButton(externalFileBtn, 1);
	sublayout->addWidget(upstreamPipelineBtn, 0, 0);
	sublayout->addWidget(externalFileBtn, 1, 0);

	// Status label.
	layout->addSpacing(6);
	layout->addWidget(statusLabel());

	// Open a sub-editor for the reference object.
	new SubObjectParameterUI(this, PROPERTY_FIELD(CalculateDisplacementsModifier::referenceConfiguration), rolloutParams.after(rollout).setTitle(tr("Reference")));

	connect(this, &PropertiesEditor::contentsChanged, this, &CalculateDisplacementsModifierEditor::onContentsChanged);
}

/******************************************************************************
* Is called when the user clicks one of the source mode buttons.
******************************************************************************/
void CalculateDisplacementsModifierEditor::onSourceButtonClicked(int id)
{
	CalculateDisplacementsModifier* mod = static_object_cast<CalculateDisplacementsModifier>(editObject());
	if(!mod) return;

	undoableTransaction(tr("Set reference source mode"), [mod,id]() {
		if(id == 1) {
			// Create a file source object, which can be used for loading
			// the reference configuration from a separate file.
			OORef<FileSource> fileSource(new FileSource(mod->dataset()));
			mod->setReferenceConfiguration(fileSource);
		}
		else {
			mod->setReferenceConfiguration(nullptr);
		}
	});
}

/******************************************************************************
* Is called when the object being edited changes.
******************************************************************************/
void CalculateDisplacementsModifierEditor::onContentsChanged(RefTarget* editObject)
{
	CalculateDisplacementsModifier* mod = static_object_cast<CalculateDisplacementsModifier>(editObject);
	if(mod) {
		_sourceButtonGroup->button(0)->setEnabled(true);
		_sourceButtonGroup->button(1)->setEnabled(true);
		_sourceButtonGroup->button(mod->referenceConfiguration() ? 1 : 0)->setChecked(true);
	}
	else {
		_sourceButtonGroup->button(0)->setEnabled(false);
		_sourceButtonGroup->button(1)->setEnabled(false);
	}
}


}	// End of namespace
}	// End of namespace
