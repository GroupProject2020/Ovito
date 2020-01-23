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
#include <ovito/particles/objects/BondsVis.h>
#include <ovito/gui/desktop/properties/FloatParameterUI.h>
#include <ovito/gui/desktop/properties/BooleanParameterUI.h>
#include <ovito/gui/desktop/properties/VariantComboBoxParameterUI.h>
#include <ovito/gui/desktop/properties/ColorParameterUI.h>
#include "BondsVisEditor.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_CLASS(BondsVisEditor);
SET_OVITO_OBJECT_EDITOR(BondsVis, BondsVisEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void BondsVisEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Bonds display"), rolloutParams, "visual_elements.bonds.html");

    // Create the rollout contents.
	QGridLayout* layout = new QGridLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);
	layout->setColumnStretch(1, 1);

	// Shading mode.
	VariantComboBoxParameterUI* shadingModeUI = new VariantComboBoxParameterUI(this, PROPERTY_FIELD(BondsVis::shadingMode));
	shadingModeUI->comboBox()->addItem(tr("Normal"), QVariant::fromValue(ArrowPrimitive::NormalShading));
	shadingModeUI->comboBox()->addItem(tr("Flat"), QVariant::fromValue(ArrowPrimitive::FlatShading));
	layout->addWidget(new QLabel(tr("Shading mode:")), 0, 0);
	layout->addWidget(shadingModeUI->comboBox(), 0, 1);

	// Rendering quality.
	VariantComboBoxParameterUI* renderingQualityUI = new VariantComboBoxParameterUI(this, PROPERTY_FIELD(BondsVis::renderingQuality));
	renderingQualityUI->comboBox()->addItem(tr("Low"), QVariant::fromValue(ArrowPrimitive::LowQuality));
	renderingQualityUI->comboBox()->addItem(tr("Medium"), QVariant::fromValue(ArrowPrimitive::MediumQuality));
	renderingQualityUI->comboBox()->addItem(tr("High"), QVariant::fromValue(ArrowPrimitive::HighQuality));
	layout->addWidget(new QLabel(tr("Rendering quality:")), 1, 0);
	layout->addWidget(renderingQualityUI->comboBox(), 1, 1);

	// Bond width.
	FloatParameterUI* bondWidthUI = new FloatParameterUI(this, PROPERTY_FIELD(BondsVis::bondWidth));
	layout->addWidget(bondWidthUI->label(), 2, 0);
	layout->addLayout(bondWidthUI->createFieldLayout(), 2, 1);

	// Bond color.
	ColorParameterUI* bondColorUI = new ColorParameterUI(this, PROPERTY_FIELD(BondsVis::bondColor));
	layout->addWidget(bondColorUI->label(), 3, 0);
	layout->addWidget(bondColorUI->colorPicker(), 3, 1);

	// Use particle colors.
	BooleanParameterUI* useParticleColorsUI = new BooleanParameterUI(this, PROPERTY_FIELD(BondsVis::useParticleColors));
	layout->addWidget(useParticleColorsUI->checkBox(), 4, 1);
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
