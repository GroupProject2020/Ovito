///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2019) Alexander Stukowski
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

#include <ovito/crystalanalysis/CrystalAnalysis.h>
#include <ovito/crystalanalysis/modifier/ConstructSurfaceModifier.h>
#include <ovito/gui/properties/IntegerParameterUI.h>
#include <ovito/gui/properties/IntegerRadioButtonParameterUI.h>
#include <ovito/gui/properties/FloatParameterUI.h>
#include <ovito/gui/properties/BooleanParameterUI.h>
#include <ovito/gui/properties/SubObjectParameterUI.h>
#include "ConstructSurfaceModifierEditor.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

IMPLEMENT_OVITO_CLASS(ConstructSurfaceModifierEditor);
SET_OVITO_OBJECT_EDITOR(ConstructSurfaceModifier, ConstructSurfaceModifierEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void ConstructSurfaceModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create the rollout.
	QWidget* rollout = createRollout(tr("Construct surface mesh"), rolloutParams, "particles.modifiers.construct_surface_mesh.html");

    QGridLayout* layout = new QGridLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(6);
	layout->setColumnStretch(1, 1);

	FloatParameterUI* radiusUI = new FloatParameterUI(this, PROPERTY_FIELD(ConstructSurfaceModifier::probeSphereRadius));
	layout->addWidget(radiusUI->label(), 0, 0);
	layout->addLayout(radiusUI->createFieldLayout(), 0, 1);

	BooleanParameterUI* onlySelectedUI = new BooleanParameterUI(this, PROPERTY_FIELD(ConstructSurfaceModifier::onlySelectedParticles));
	layout->addWidget(onlySelectedUI->checkBox(), 1, 0, 1, 2);

	IntegerRadioButtonParameterUI* methodUI = new IntegerRadioButtonParameterUI(this, PROPERTY_FIELD(ConstructSurfaceModifier::method));
	QRadioButton* alphaShapeMethodBtn = methodUI->addRadioButton(ConstructSurfaceModifier::AlphaShape, tr("Use alpha-shape method:"));
	layout->addWidget(alphaShapeMethodBtn, 2, 0, 1, 2);

	IntegerParameterUI* smoothingLevelUI = new IntegerParameterUI(this, PROPERTY_FIELD(ConstructSurfaceModifier::smoothingLevel));
	layout->addWidget(smoothingLevelUI->label(), 3, 0);
	layout->addLayout(smoothingLevelUI->createFieldLayout(), 3, 1);

	BooleanParameterUI* selectSurfaceParticlesUI = new BooleanParameterUI(this, PROPERTY_FIELD(ConstructSurfaceModifier::selectSurfaceParticles));
	layout->addWidget(selectSurfaceParticlesUI->checkBox(), 4, 0, 1, 2);

	QRadioButton* gaussianDensityBtn = methodUI->addRadioButton(ConstructSurfaceModifier::GaussianDensity, tr("Use Gaussian density method:"));
	layout->addWidget(gaussianDensityBtn, 5, 0, 1, 2);

	// Status label.
	layout->setRowMinimumHeight(5, 10);
	layout->addWidget(statusLabel(), 6, 0, 1, 2);
	statusLabel()->setMinimumHeight(100);

	// Open a sub-editor for the surface mesh vis element.
	new SubObjectParameterUI(this, PROPERTY_FIELD(ConstructSurfaceModifier::surfaceMeshVis), rolloutParams.after(rollout));
}

}	// End of namespace
}	// End of namespace
}	// End of namespace

