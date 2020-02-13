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
#include <ovito/particles/objects/VectorVis.h>
#include <ovito/gui/desktop/properties/FloatParameterUI.h>
#include <ovito/gui/desktop/properties/VariantComboBoxParameterUI.h>
#include <ovito/gui/desktop/properties/ColorParameterUI.h>
#include <ovito/gui/desktop/properties/BooleanParameterUI.h>
#include "VectorVisEditor.h"

namespace Ovito { namespace Particles {

IMPLEMENT_OVITO_CLASS(VectorVisEditor);
SET_OVITO_OBJECT_EDITOR(VectorVis, VectorVisEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void VectorVisEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Vector display"), rolloutParams, "visual_elements.vectors.html");

    // Create the rollout contents.
	QGridLayout* layout = new QGridLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);
	layout->setColumnStretch(1, 1);
	int row = 0;

	// Shading mode.
	VariantComboBoxParameterUI* shadingModeUI = new VariantComboBoxParameterUI(this, PROPERTY_FIELD(VectorVis::shadingMode));
	shadingModeUI->comboBox()->addItem(tr("Normal"), QVariant::fromValue(ArrowPrimitive::NormalShading));
	shadingModeUI->comboBox()->addItem(tr("Flat"), QVariant::fromValue(ArrowPrimitive::FlatShading));
	layout->addWidget(new QLabel(tr("Shading mode:")), row, 0);
	layout->addWidget(shadingModeUI->comboBox(), row++, 1);

	// Scaling factor.
	FloatParameterUI* scalingFactorUI = new FloatParameterUI(this, PROPERTY_FIELD(VectorVis::scalingFactor));
	layout->addWidget(scalingFactorUI->label(), row, 0);
	layout->addLayout(scalingFactorUI->createFieldLayout(), row++, 1);

	// Arrow width factor.
	FloatParameterUI* arrowWidthUI = new FloatParameterUI(this, PROPERTY_FIELD(VectorVis::arrowWidth));
	layout->addWidget(arrowWidthUI->label(), row, 0);
	layout->addLayout(arrowWidthUI->createFieldLayout(), row++, 1);

	VariantComboBoxParameterUI* arrowPositionUI = new VariantComboBoxParameterUI(this, PROPERTY_FIELD(VectorVis::arrowPosition));
	arrowPositionUI->comboBox()->addItem(QIcon(":/particles/icons/arrow_alignment_base.png"), tr("Base"), QVariant::fromValue(VectorVis::Base));
	arrowPositionUI->comboBox()->addItem(QIcon(":/particles/icons/arrow_alignment_center.png"), tr("Center"), QVariant::fromValue(VectorVis::Center));
	arrowPositionUI->comboBox()->addItem(QIcon(":/particles/icons/arrow_alignment_head.png"), tr("Head"), QVariant::fromValue(VectorVis::Head));
	layout->addWidget(new QLabel(tr("Alignment:")), row, 0);
	layout->addWidget(arrowPositionUI->comboBox(), row++, 1);

	ColorParameterUI* arrowColorUI = new ColorParameterUI(this, PROPERTY_FIELD(VectorVis::arrowColor));
	layout->addWidget(arrowColorUI->label(), row, 0);
	layout->addWidget(arrowColorUI->colorPicker(), row++, 1);

	BooleanParameterUI* reverseArrowDirectionUI = new BooleanParameterUI(this, PROPERTY_FIELD(VectorVis::reverseArrowDirection));
	layout->addWidget(reverseArrowDirectionUI->checkBox(), row++, 1, 1, 1);
}

}	// End of namespace
}	// End of namespace
