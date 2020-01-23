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
#include <ovito/particles/modifier/analysis/voronoi/VoronoiAnalysisModifier.h>
#include <ovito/gui/desktop/properties/BooleanParameterUI.h>
#include <ovito/gui/desktop/properties/BooleanGroupBoxParameterUI.h>
#include <ovito/gui/desktop/properties/IntegerParameterUI.h>
#include <ovito/gui/desktop/properties/FloatParameterUI.h>
#include "VoronoiAnalysisModifierEditor.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_CLASS(VoronoiAnalysisModifierEditor);
SET_OVITO_OBJECT_EDITOR(VoronoiAnalysisModifier, VoronoiAnalysisModifierEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void VoronoiAnalysisModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Voronoi analysis"), rolloutParams, "particles.modifiers.voronoi_analysis.html");

    // Create the rollout contents.
	QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);

	QGridLayout* gridlayout = new QGridLayout();
	QGridLayout* sublayout;
	gridlayout->setContentsMargins(4,4,4,4);
	gridlayout->setSpacing(4);
	gridlayout->setColumnStretch(1, 1);
	int row = 0;

	// Face threshold.
	FloatParameterUI* faceThresholdPUI = new FloatParameterUI(this, PROPERTY_FIELD(VoronoiAnalysisModifier::faceThreshold));
	gridlayout->addWidget(faceThresholdPUI->label(), row, 0);
	gridlayout->addLayout(faceThresholdPUI->createFieldLayout(), row++, 1);

	// Relative face threshold.
	FloatParameterUI* relativeFaceThresholdPUI = new FloatParameterUI(this, PROPERTY_FIELD(VoronoiAnalysisModifier::relativeFaceThreshold));
	gridlayout->addWidget(relativeFaceThresholdPUI->label(), row, 0);
	gridlayout->addLayout(relativeFaceThresholdPUI->createFieldLayout(), row++, 1);

	// Compute indices.
	BooleanGroupBoxParameterUI* computeIndicesPUI = new BooleanGroupBoxParameterUI(this, PROPERTY_FIELD(VoronoiAnalysisModifier::computeIndices));
	gridlayout->addWidget(computeIndicesPUI->groupBox(), row++, 0, 1, 2);
	sublayout = new QGridLayout(computeIndicesPUI->childContainer());
	sublayout->setContentsMargins(4,4,4,4);
	sublayout->setSpacing(4);
	sublayout->setColumnStretch(1, 1);

	// Edge threshold.
	FloatParameterUI* edgeThresholdPUI = new FloatParameterUI(this, PROPERTY_FIELD(VoronoiAnalysisModifier::edgeThreshold));
	sublayout->addWidget(edgeThresholdPUI->label(), 0, 0);
	sublayout->addLayout(edgeThresholdPUI->createFieldLayout(), 0, 1);

	// Generate bonds.
	BooleanParameterUI* computeBondsPUI = new BooleanParameterUI(this, PROPERTY_FIELD(VoronoiAnalysisModifier::computeBonds));
	gridlayout->addWidget(computeBondsPUI->checkBox(), row++, 0, 1, 2);

	// Generate polyhedral mesh.
	BooleanParameterUI* computePolyhedraPUI = new BooleanParameterUI(this, PROPERTY_FIELD(VoronoiAnalysisModifier::computePolyhedra));
	gridlayout->addWidget(computePolyhedraPUI->checkBox(), row++, 0, 1, 2);

	// Atomic radii.
	BooleanParameterUI* useRadiiPUI = new BooleanParameterUI(this, PROPERTY_FIELD(VoronoiAnalysisModifier::useRadii));
	gridlayout->addWidget(useRadiiPUI->checkBox(), row++, 0, 1, 2);

	// Only selected particles.
	BooleanParameterUI* onlySelectedPUI = new BooleanParameterUI(this, PROPERTY_FIELD(VoronoiAnalysisModifier::onlySelected));
	gridlayout->addWidget(onlySelectedPUI->checkBox(), row++, 0, 1, 2);

	layout->addLayout(gridlayout);

	// Status label.
	layout->addSpacing(6);
	layout->addWidget(statusLabel());
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
