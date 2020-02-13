////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2019 Alexander Stukowski
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

#include <ovito/gui/desktop/GUI.h>
#include <ovito/gui/desktop/properties/BooleanGroupBoxParameterUI.h>
#include <ovito/gui/desktop/properties/StringParameterUI.h>
#include <ovito/gui/desktop/properties/BooleanParameterUI.h>
#include <ovito/gui/desktop/properties/ColorParameterUI.h>
#include <ovito/gui/desktop/properties/FontParameterUI.h>
#include <ovito/gui/desktop/properties/FloatParameterUI.h>
#include <ovito/gui/desktop/properties/Vector3ParameterUI.h>
#include <ovito/gui/desktop/properties/VariantComboBoxParameterUI.h>
#include <ovito/gui/desktop/properties/IntegerRadioButtonParameterUI.h>
#include <ovito/gui/desktop/actions/ViewportModeAction.h>
#include <ovito/gui/desktop/viewport/overlays/MoveOverlayInputMode.h>
#include <ovito/core/viewport/overlays/CoordinateTripodOverlay.h>
#include "CoordinateTripodOverlayEditor.h"

namespace Ovito {

IMPLEMENT_OVITO_CLASS(CoordinateTripodOverlayEditor);
SET_OVITO_OBJECT_EDITOR(CoordinateTripodOverlay, CoordinateTripodOverlayEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void CoordinateTripodOverlayEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Coordinate tripod"), rolloutParams);

    // Create the rollout contents.
	QGridLayout* layout = new QGridLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);
	layout->setColumnStretch(1, 1);
	int row = 0;

	VariantComboBoxParameterUI* alignmentPUI = new VariantComboBoxParameterUI(this, PROPERTY_FIELD(CoordinateTripodOverlay::alignment));
	layout->addWidget(new QLabel(tr("Position:")), row, 0);
	layout->addWidget(alignmentPUI->comboBox(), row++, 1);
	alignmentPUI->comboBox()->addItem(tr("Top left"), QVariant::fromValue((int)(Qt::AlignTop | Qt::AlignLeft)));
	alignmentPUI->comboBox()->addItem(tr("Top right"), QVariant::fromValue((int)(Qt::AlignTop | Qt::AlignRight)));
	alignmentPUI->comboBox()->addItem(tr("Bottom left"), QVariant::fromValue((int)(Qt::AlignBottom | Qt::AlignLeft)));
	alignmentPUI->comboBox()->addItem(tr("Bottom right"), QVariant::fromValue((int)(Qt::AlignBottom | Qt::AlignRight)));

	FloatParameterUI* offsetXPUI = new FloatParameterUI(this, PROPERTY_FIELD(CoordinateTripodOverlay::offsetX));
	layout->addWidget(offsetXPUI->label(), row, 0);
	layout->addLayout(offsetXPUI->createFieldLayout(), row++, 1);

	FloatParameterUI* offsetYPUI = new FloatParameterUI(this, PROPERTY_FIELD(CoordinateTripodOverlay::offsetY));
	layout->addWidget(offsetYPUI->label(), row, 0);
	layout->addLayout(offsetYPUI->createFieldLayout(), row++, 1);

	ViewportInputMode* moveOverlayMode = new MoveOverlayInputMode(this);
	connect(this, &QObject::destroyed, moveOverlayMode, &ViewportInputMode::removeMode);
	ViewportModeAction* moveOverlayAction = new ViewportModeAction(mainWindow(), tr("Move using mouse"), this, moveOverlayMode);
	layout->addWidget(moveOverlayAction->createPushButton(), row++, 1);

	FloatParameterUI* sizePUI = new FloatParameterUI(this, PROPERTY_FIELD(CoordinateTripodOverlay::tripodSize));
	layout->addWidget(sizePUI->label(), row, 0);
	layout->addLayout(sizePUI->createFieldLayout(), row++, 1);

	FloatParameterUI* lineWidthPUI = new FloatParameterUI(this, PROPERTY_FIELD(CoordinateTripodOverlay::lineWidth));
	layout->addWidget(lineWidthPUI->label(), row, 0);
	layout->addLayout(lineWidthPUI->createFieldLayout(), row++, 1);

	FloatParameterUI* fontSizePUI = new FloatParameterUI(this, PROPERTY_FIELD(CoordinateTripodOverlay::fontSize));
	layout->addWidget(fontSizePUI->label(), row, 0);
	layout->addLayout(fontSizePUI->createFieldLayout(), row++, 1);

	FontParameterUI* labelFontPUI = new FontParameterUI(this, PROPERTY_FIELD(CoordinateTripodOverlay::font));
	layout->addWidget(labelFontPUI->label(), row, 0);
	layout->addWidget(labelFontPUI->fontPicker(), row++, 1);

	IntegerRadioButtonParameterUI* tripodStyleUI = new IntegerRadioButtonParameterUI(this, PROPERTY_FIELD(CoordinateTripodOverlay::tripodStyle));
	layout->addWidget(new QLabel(tr("Style:")), row, 0);
	QHBoxLayout* hlayout = new QHBoxLayout();
	hlayout->setContentsMargins(0,0,0,0);
	hlayout->addWidget(tripodStyleUI->addRadioButton(CoordinateTripodOverlay::FlatArrows, tr("Flat")));
	hlayout->addWidget(tripodStyleUI->addRadioButton(CoordinateTripodOverlay::SolidArrows, tr("Solid")));
	layout->addLayout(hlayout, row++, 1);

	// Create a second rollout.
	rollout = createRollout(tr("Coordinate axes"), rolloutParams);

    // Create the rollout contents.
	layout = new QGridLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);
	layout->setColumnStretch(1, 1);

	row = 0;
	QGridLayout* sublayout;
	StringParameterUI* axisLabelPUI;
	ColorParameterUI* axisColorPUI;
	BooleanGroupBoxParameterUI* axisPUI;

	// Axis 1.
	axisPUI = new BooleanGroupBoxParameterUI(this, PROPERTY_FIELD(CoordinateTripodOverlay::axis1Enabled));
	axisPUI->groupBox()->setTitle("Axis 1");
	layout->addWidget(axisPUI->groupBox(), row++, 0, 1, 2);
	sublayout = new QGridLayout(axisPUI->childContainer());
	sublayout->setContentsMargins(4,4,4,4);
	sublayout->setSpacing(2);

	// Axis label.
	axisLabelPUI = new StringParameterUI(this, PROPERTY_FIELD(CoordinateTripodOverlay::axis1Label));
	sublayout->addWidget(new QLabel(tr("Label:")), 0, 0);
	sublayout->addWidget(axisLabelPUI->textBox(), 0, 1, 1, 2);

	// Axis color.
	axisColorPUI = new ColorParameterUI(this, PROPERTY_FIELD(CoordinateTripodOverlay::axis1Color));
	sublayout->addWidget(new QLabel(tr("Color:")), 1, 0);
	sublayout->addWidget(axisColorPUI->colorPicker(), 1, 1, 1, 2);

	// Axis direction.
	sublayout->addWidget(new QLabel(tr("Direction:")), 2, 0, 1, 3);
	for(int dim = 0; dim < 3; dim++) {
		Vector3ParameterUI* axisDirPUI = new Vector3ParameterUI(this, PROPERTY_FIELD(CoordinateTripodOverlay::axis1Dir), dim);
		sublayout->addLayout(axisDirPUI->createFieldLayout(), 3, dim, 1, 1);
	}

	// Axis 2
	axisPUI = new BooleanGroupBoxParameterUI(this, PROPERTY_FIELD(CoordinateTripodOverlay::axis2Enabled));
	axisPUI->groupBox()->setTitle("Axis 2");
	layout->addWidget(axisPUI->groupBox(), row++, 0, 1, 2);
	sublayout = new QGridLayout(axisPUI->childContainer());
	sublayout->setContentsMargins(4,4,4,4);
	sublayout->setSpacing(2);

	// Axis label.
	axisLabelPUI = new StringParameterUI(this, PROPERTY_FIELD(CoordinateTripodOverlay::axis2Label));
	sublayout->addWidget(new QLabel(tr("Label:")), 0, 0);
	sublayout->addWidget(axisLabelPUI->textBox(), 0, 1, 1, 2);

	// Axis color.
	axisColorPUI = new ColorParameterUI(this, PROPERTY_FIELD(CoordinateTripodOverlay::axis2Color));
	sublayout->addWidget(new QLabel(tr("Color:")), 1, 0);
	sublayout->addWidget(axisColorPUI->colorPicker(), 1, 1, 1, 2);

	// Axis direction.
	sublayout->addWidget(new QLabel(tr("Direction:")), 2, 0, 1, 3);
	for(int dim = 0; dim < 3; dim++) {
		Vector3ParameterUI* axisDirPUI = new Vector3ParameterUI(this, PROPERTY_FIELD(CoordinateTripodOverlay::axis2Dir), dim);
		sublayout->addLayout(axisDirPUI->createFieldLayout(), 3, dim, 1, 1);
	}

	// Axis 3.
	axisPUI = new BooleanGroupBoxParameterUI(this, PROPERTY_FIELD(CoordinateTripodOverlay::axis3Enabled));
	axisPUI->groupBox()->setTitle("Axis 3");
	layout->addWidget(axisPUI->groupBox(), row++, 0, 1, 2);
	sublayout = new QGridLayout(axisPUI->childContainer());
	sublayout->setContentsMargins(4,4,4,4);
	sublayout->setSpacing(2);

	// Axis label.
	axisLabelPUI = new StringParameterUI(this, PROPERTY_FIELD(CoordinateTripodOverlay::axis3Label));
	sublayout->addWidget(new QLabel(tr("Label:")), 0, 0);
	sublayout->addWidget(axisLabelPUI->textBox(), 0, 1, 1, 2);

	// Axis color.
	axisColorPUI = new ColorParameterUI(this, PROPERTY_FIELD(CoordinateTripodOverlay::axis3Color));
	sublayout->addWidget(new QLabel(tr("Color:")), 1, 0);
	sublayout->addWidget(axisColorPUI->colorPicker(), 1, 1, 1, 2);

	// Axis direction.
	sublayout->addWidget(new QLabel(tr("Direction:")), 2, 0, 1, 3);
	for(int dim = 0; dim < 3; dim++) {
		Vector3ParameterUI* axisDirPUI = new Vector3ParameterUI(this, PROPERTY_FIELD(CoordinateTripodOverlay::axis3Dir), dim);
		sublayout->addLayout(axisDirPUI->createFieldLayout(), 3, dim, 1, 1);
	}

	// Axis 4.
	axisPUI = new BooleanGroupBoxParameterUI(this, PROPERTY_FIELD(CoordinateTripodOverlay::axis4Enabled));
	axisPUI->groupBox()->setTitle("Axis 4");
	layout->addWidget(axisPUI->groupBox(), row++, 0, 1, 2);
	sublayout = new QGridLayout(axisPUI->childContainer());
	sublayout->setContentsMargins(4,4,4,4);
	sublayout->setSpacing(2);

	// Axis label.
	axisLabelPUI = new StringParameterUI(this, PROPERTY_FIELD(CoordinateTripodOverlay::axis4Label));
	sublayout->addWidget(new QLabel(tr("Label:")), 0, 0);
	sublayout->addWidget(axisLabelPUI->textBox(), 0, 1, 1, 2);

	// Axis color.
	axisColorPUI = new ColorParameterUI(this, PROPERTY_FIELD(CoordinateTripodOverlay::axis4Color));
	sublayout->addWidget(new QLabel(tr("Color:")), 1, 0);
	sublayout->addWidget(axisColorPUI->colorPicker(), 1, 1, 1, 2);

	// Axis direction.
	sublayout->addWidget(new QLabel(tr("Direction:")), 2, 0, 1, 3);
	for(int dim = 0; dim < 3; dim++) {
		Vector3ParameterUI* axisDirPUI = new Vector3ParameterUI(this, PROPERTY_FIELD(CoordinateTripodOverlay::axis4Dir), dim);
		sublayout->addLayout(axisDirPUI->createFieldLayout(), 3, dim, 1, 1);
	}
}

}	// End of namespace
