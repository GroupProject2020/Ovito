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

#include <ovito/particles/gui/ParticlesGui.h>
#include <ovito/particles/modifier/analysis/surface/ConstructSurfaceModifier.h>
#include <ovito/gui/properties/IntegerParameterUI.h>
#include <ovito/gui/properties/IntegerRadioButtonParameterUI.h>
#include <ovito/gui/properties/FloatParameterUI.h>
#include <ovito/gui/properties/BooleanParameterUI.h>
#include <ovito/gui/properties/SubObjectParameterUI.h>
#include "ConstructSurfaceModifierEditor.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_CLASS(ConstructSurfaceModifierEditor);
SET_OVITO_OBJECT_EDITOR(ConstructSurfaceModifier, ConstructSurfaceModifierEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void ConstructSurfaceModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create the rollout.
	QWidget* rollout = createRollout(tr("Construct surface mesh"), rolloutParams, "particles.modifiers.construct_surface_mesh.html");

    QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);

	QGroupBox* methodGroupBox = new QGroupBox(tr("Method"));
	layout->addWidget(methodGroupBox);

    QGridLayout* sublayout = new QGridLayout(methodGroupBox);
	sublayout->setContentsMargins(4,4,4,4);
	sublayout->setSpacing(6);
	sublayout->setColumnStretch(2, 1);
	sublayout->setColumnMinimumWidth(0, 20);

	IntegerRadioButtonParameterUI* methodUI = new IntegerRadioButtonParameterUI(this, PROPERTY_FIELD(ConstructSurfaceModifier::method));
	QRadioButton* alphaShapeMethodBtn = methodUI->addRadioButton(ConstructSurfaceModifier::AlphaShape, tr("Alpha-shape method (default):"));
	sublayout->addWidget(alphaShapeMethodBtn, 0, 0, 1, 3);

	FloatParameterUI* probeSphereRadiusUI = new FloatParameterUI(this, PROPERTY_FIELD(ConstructSurfaceModifier::probeSphereRadius));
	probeSphereRadiusUI->setEnabled(false);
	sublayout->addWidget(probeSphereRadiusUI->label(), 1, 1);
	sublayout->addLayout(probeSphereRadiusUI->createFieldLayout(), 1, 2);
	connect(alphaShapeMethodBtn, &QRadioButton::toggled, probeSphereRadiusUI, &FloatParameterUI::setEnabled);

	IntegerParameterUI* smoothingLevelUI = new IntegerParameterUI(this, PROPERTY_FIELD(ConstructSurfaceModifier::smoothingLevel));
	smoothingLevelUI->setEnabled(false);
	sublayout->addWidget(smoothingLevelUI->label(), 2, 1);
	sublayout->addLayout(smoothingLevelUI->createFieldLayout(), 2, 2);
	connect(alphaShapeMethodBtn, &QRadioButton::toggled, smoothingLevelUI, &IntegerParameterUI::setEnabled);

	BooleanParameterUI* selectSurfaceParticlesUI = new BooleanParameterUI(this, PROPERTY_FIELD(ConstructSurfaceModifier::selectSurfaceParticles));
	selectSurfaceParticlesUI->setEnabled(false);
	sublayout->addWidget(selectSurfaceParticlesUI->checkBox(), 3, 1, 1, 2);
	connect(alphaShapeMethodBtn, &QRadioButton::toggled, selectSurfaceParticlesUI, &BooleanParameterUI::setEnabled);

	QRadioButton* gaussianDensityBtn = methodUI->addRadioButton(ConstructSurfaceModifier::GaussianDensity, tr("Gaussian density method (experimental):"));
	sublayout->setRowMinimumHeight(4, 10);
	sublayout->addWidget(gaussianDensityBtn, 5, 0, 1, 3);

	IntegerParameterUI* gridResolutionUI = new IntegerParameterUI(this, PROPERTY_FIELD(ConstructSurfaceModifier::gridResolution));
	gridResolutionUI->setEnabled(false);
	sublayout->addWidget(gridResolutionUI->label(), 6, 1);
	sublayout->addLayout(gridResolutionUI->createFieldLayout(), 6, 2);
	connect(gaussianDensityBtn, &QRadioButton::toggled, gridResolutionUI, &IntegerParameterUI::setEnabled);

	FloatParameterUI* radiusFactorUI = new FloatParameterUI(this, PROPERTY_FIELD(ConstructSurfaceModifier::radiusFactor));
	radiusFactorUI->setEnabled(false);
	sublayout->addWidget(radiusFactorUI->label(), 7, 1);
	sublayout->addLayout(radiusFactorUI->createFieldLayout(), 7, 2);
	connect(gaussianDensityBtn, &QRadioButton::toggled, radiusFactorUI, &FloatParameterUI::setEnabled);

	FloatParameterUI* isoValueUI = new FloatParameterUI(this, PROPERTY_FIELD(ConstructSurfaceModifier::isoValue));
	isoValueUI->setEnabled(false);
	sublayout->addWidget(isoValueUI->label(), 8, 1);
	sublayout->addLayout(isoValueUI->createFieldLayout(), 8, 2);
	connect(gaussianDensityBtn, &QRadioButton::toggled, isoValueUI, &FloatParameterUI::setEnabled);

	QGroupBox* generalGroupBox = new QGroupBox(tr("Options"));
	layout->addWidget(generalGroupBox);

    sublayout = new QGridLayout(generalGroupBox);
	sublayout->setContentsMargins(4,4,4,4);
	sublayout->setSpacing(6);
	sublayout->setColumnStretch(1, 1);

	BooleanParameterUI* onlySelectedUI = new BooleanParameterUI(this, PROPERTY_FIELD(ConstructSurfaceModifier::onlySelectedParticles));
	sublayout->addWidget(onlySelectedUI->checkBox(), 1, 0, 1, 2);

	// Status label.
	layout->addWidget(statusLabel());
	statusLabel()->setMinimumHeight(100);

	// Open a sub-editor for the surface mesh vis element.
	new SubObjectParameterUI(this, PROPERTY_FIELD(ConstructSurfaceModifier::surfaceMeshVis), rolloutParams.after(rollout));
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
