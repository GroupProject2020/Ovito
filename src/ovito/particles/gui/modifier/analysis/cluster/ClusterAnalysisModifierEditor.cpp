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
#include <ovito/particles/modifier/analysis/cluster/ClusterAnalysisModifier.h>
#include <ovito/gui/properties/BooleanParameterUI.h>
#include <ovito/gui/properties/FloatParameterUI.h>
#include <ovito/gui/properties/IntegerRadioButtonParameterUI.h>
#include "ClusterAnalysisModifierEditor.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_CLASS(ClusterAnalysisModifierEditor);
SET_OVITO_OBJECT_EDITOR(ClusterAnalysisModifier, ClusterAnalysisModifierEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void ClusterAnalysisModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Cluster analysis"), rolloutParams, "particles.modifiers.cluster_analysis.html");

    // Create the rollout contents.
	QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);

	QGridLayout* gridlayout = new QGridLayout();
	gridlayout->setContentsMargins(4,4,4,4);
	gridlayout->setColumnStretch(2, 1);
	gridlayout->setColumnMinimumWidth(0, 10);
	gridlayout->setRowMinimumHeight(3, 6);

	gridlayout->addWidget(new QLabel(tr("Neighbor mode:")), 0, 0, 1, 3);

	IntegerRadioButtonParameterUI* neighborModePUI = new IntegerRadioButtonParameterUI(this, PROPERTY_FIELD(ClusterAnalysisModifier::neighborMode));
	QRadioButton* cutoffModeBtn = neighborModePUI->addRadioButton(ClusterAnalysisModifier::CutoffRange, tr("Cutoff distance:"));
	gridlayout->addWidget(cutoffModeBtn, 1, 1);
	QRadioButton* bondModeBtn = neighborModePUI->addRadioButton(ClusterAnalysisModifier::Bonding, tr("Bonds"));
	gridlayout->addWidget(bondModeBtn, 2, 1, 1, 2);

	// Cutoff parameter.
	FloatParameterUI* cutoffRadiusPUI = new FloatParameterUI(this, PROPERTY_FIELD(ClusterAnalysisModifier::cutoff));
	gridlayout->addLayout(cutoffRadiusPUI->createFieldLayout(), 1, 2);
	cutoffRadiusPUI->setEnabled(false);
	connect(cutoffModeBtn, &QRadioButton::toggled, cutoffRadiusPUI, &FloatParameterUI::setEnabled);

	// Sort by size
	BooleanParameterUI* sortBySizeUI = new BooleanParameterUI(this, PROPERTY_FIELD(ClusterAnalysisModifier::sortBySize));
	gridlayout->addWidget(sortBySizeUI->checkBox(), 4, 0, 1, 3);

	// Use only selected particles.
	BooleanParameterUI* onlySelectedParticlesUI = new BooleanParameterUI(this, PROPERTY_FIELD(ClusterAnalysisModifier::onlySelectedParticles));
	gridlayout->addWidget(onlySelectedParticlesUI->checkBox(), 5, 0, 1, 3);

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
