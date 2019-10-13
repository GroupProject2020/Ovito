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

#include <ovito/stdobj/gui/StdObjGui.h>
#include <ovito/gui/properties/FloatParameterUI.h>
#include <ovito/gui/properties/BooleanRadioButtonParameterUI.h>
#include <ovito/gui/properties/VariantComboBoxParameterUI.h>
#include <ovito/stdobj/camera/CameraObject.h>
#include "CameraObjectEditor.h"

namespace Ovito { namespace StdObj {

IMPLEMENT_OVITO_CLASS(CameraObjectEditor);
SET_OVITO_OBJECT_EDITOR(CameraObject, CameraObjectEditor);

/******************************************************************************
* Constructor that creates the UI controls for the editor.
******************************************************************************/
void CameraObjectEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create the rollout.
	QWidget* rollout = createRollout(tr("Camera"), rolloutParams);

	QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(2);

	QGridLayout* sublayout = new QGridLayout();
	sublayout->setContentsMargins(0,0,0,0);
	sublayout->setColumnStretch(2, 1);
	sublayout->setColumnMinimumWidth(0, 12);
	layout->addLayout(sublayout);

	// Camera projection parameter.
	BooleanRadioButtonParameterUI* isPerspectivePUI = new BooleanRadioButtonParameterUI(this, PROPERTY_FIELD(CameraObject::isPerspective));
	isPerspectivePUI->buttonTrue()->setText(tr("Perspective camera:"));
	sublayout->addWidget(isPerspectivePUI->buttonTrue(), 0, 0, 1, 3);

	// FOV parameter.
	FloatParameterUI* fovPUI = new FloatParameterUI(this, PROPERTY_FIELD(CameraObject::fovController));
	sublayout->addWidget(fovPUI->label(), 1, 1);
	sublayout->addLayout(fovPUI->createFieldLayout(), 1, 2);

	isPerspectivePUI->buttonFalse()->setText(tr("Orthographic camera:"));
	sublayout->addWidget(isPerspectivePUI->buttonFalse(), 2, 0, 1, 3);

	// Zoom parameter.
	FloatParameterUI* zoomPUI = new FloatParameterUI(this, PROPERTY_FIELD(CameraObject::zoomController));
	sublayout->addWidget(zoomPUI->label(), 3, 1);
	sublayout->addLayout(zoomPUI->createFieldLayout(), 3, 2);

	fovPUI->setEnabled(false);
	zoomPUI->setEnabled(false);
	connect(isPerspectivePUI->buttonTrue(), &QRadioButton::toggled, fovPUI, &FloatParameterUI::setEnabled);
	connect(isPerspectivePUI->buttonFalse(), &QRadioButton::toggled, zoomPUI, &FloatParameterUI::setEnabled);

	// Camera type.
	layout->addSpacing(10);
	VariantComboBoxParameterUI* typePUI = new VariantComboBoxParameterUI(this, "isTargetCamera");
	typePUI->comboBox()->addItem(tr("Free camera"), QVariant::fromValue(false));
	typePUI->comboBox()->addItem(tr("Target camera"), QVariant::fromValue(true));
	layout->addWidget(new QLabel(tr("Camera type:")));
	layout->addWidget(typePUI->comboBox());
}

}	// End of namespace
}	// End of namespace
