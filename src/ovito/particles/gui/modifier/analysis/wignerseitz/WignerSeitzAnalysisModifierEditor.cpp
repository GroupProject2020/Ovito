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

#include <ovito/particles/gui/ParticlesGui.h>
#include <ovito/particles/modifier/analysis/wignerseitz/WignerSeitzAnalysisModifier.h>
#include <ovito/gui/properties/BooleanParameterUI.h>
#include <ovito/gui/properties/BooleanRadioButtonParameterUI.h>
#include <ovito/gui/properties/IntegerParameterUI.h>
#include <ovito/gui/properties/IntegerRadioButtonParameterUI.h>
#include <ovito/gui/properties/SubObjectParameterUI.h>
#include <ovito/core/dataset/io/FileSource.h>
#include "WignerSeitzAnalysisModifierEditor.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_CLASS(WignerSeitzAnalysisModifierEditor);
SET_OVITO_OBJECT_EDITOR(WignerSeitzAnalysisModifier, WignerSeitzAnalysisModifierEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void WignerSeitzAnalysisModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Wigner-Seitz defect analysis"), rolloutParams, "particles.modifiers.wigner_seitz_analysis.html");

    // Create the rollout contents.
	QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);

	QGroupBox* optionsGroupBox = new QGroupBox(tr("Output options"));
	layout->addWidget(optionsGroupBox);

	QGridLayout* sublayout = new QGridLayout(optionsGroupBox);
	sublayout->setContentsMargins(4,4,4,4);
	sublayout->setSpacing(4);

	BooleanParameterUI* perTypeOccupancyUI = new BooleanParameterUI(this, PROPERTY_FIELD(WignerSeitzAnalysisModifier::perTypeOccupancy));

	sublayout->setColumnMinimumWidth(0, 16);
	sublayout->addWidget(new QLabel(tr("Output mode:")), 0, 0, 1, 2);
	BooleanRadioButtonParameterUI* outputCurrentConfigUI = new BooleanRadioButtonParameterUI(this, PROPERTY_FIELD(WignerSeitzAnalysisModifier::outputCurrentConfig));
	outputCurrentConfigUI->buttonFalse()->setText(tr("Sites (reference config)"));
	sublayout->addWidget(outputCurrentConfigUI->buttonFalse(), 1, 1);
	outputCurrentConfigUI->buttonTrue()->setText(tr("Atoms (displaced config)"));
	sublayout->addWidget(outputCurrentConfigUI->buttonTrue(), 2, 1);

	sublayout->setRowMinimumHeight(3, 6);
	sublayout->addWidget(perTypeOccupancyUI->checkBox(), 4, 0, 1, 2);

	QGroupBox* mappingGroupBox = new QGroupBox(tr("Affine mapping of simulation cell"));
	layout->addWidget(mappingGroupBox);

	sublayout = new QGridLayout(mappingGroupBox);
	sublayout->setContentsMargins(4,4,4,4);
	sublayout->setSpacing(4);

	IntegerRadioButtonParameterUI* affineMappingUI = new IntegerRadioButtonParameterUI(this, PROPERTY_FIELD(ReferenceConfigurationModifier::affineMapping));
    sublayout->addWidget(affineMappingUI->addRadioButton(ReferenceConfigurationModifier::NO_MAPPING, tr("Off")), 0, 0);
	sublayout->addWidget(affineMappingUI->addRadioButton(ReferenceConfigurationModifier::TO_REFERENCE_CELL, tr("To reference")), 0, 1);

	QGroupBox* referenceSourceGroupBox = new QGroupBox(tr("Reference configuration source"));
	layout->addWidget(referenceSourceGroupBox);

	sublayout = new QGridLayout(referenceSourceGroupBox);
	sublayout->setContentsMargins(4,4,4,4);
	sublayout->setSpacing(6);
	sublayout->setColumnStretch(1, 1);

	_sourceButtonGroup = new QButtonGroup(this);
	connect(_sourceButtonGroup, static_cast<void (QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked), this, &WignerSeitzAnalysisModifierEditor::onSourceButtonClicked);
	QRadioButton* upstreamPipelineBtn = new QRadioButton(tr("Upstream pipeline"));
	QRadioButton* externalFileBtn = new QRadioButton(tr("External file"));
	_sourceButtonGroup->addButton(upstreamPipelineBtn, 0);
	_sourceButtonGroup->addButton(externalFileBtn, 1);
	sublayout->addWidget(upstreamPipelineBtn, 0, 0, 1, 2);
	sublayout->addWidget(externalFileBtn, 1, 0, 1, 2);

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

	// Status label.
	layout->addSpacing(6);
	layout->addWidget(statusLabel());

	// Open a sub-editor for the reference object.
	new SubObjectParameterUI(this, PROPERTY_FIELD(WignerSeitzAnalysisModifier::referenceConfiguration), RolloutInsertionParameters().setTitle(tr("Reference")));

	connect(this, &PropertiesEditor::contentsChanged, this, &WignerSeitzAnalysisModifierEditor::onContentsChanged);
}

/******************************************************************************
* Is called when the user clicks one of the source mode buttons.
******************************************************************************/
void WignerSeitzAnalysisModifierEditor::onSourceButtonClicked(int id)
{
	ReferenceConfigurationModifier* mod = static_object_cast<ReferenceConfigurationModifier>(editObject());
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
void WignerSeitzAnalysisModifierEditor::onContentsChanged(RefTarget* editObject)
{
	ReferenceConfigurationModifier* mod = static_object_cast<ReferenceConfigurationModifier>(editObject);
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


OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
