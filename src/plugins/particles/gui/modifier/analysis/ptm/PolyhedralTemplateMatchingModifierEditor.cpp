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

#include <plugins/particles/gui/ParticlesGui.h>
#include <plugins/particles/modifier/analysis/ptm/PolyhedralTemplateMatchingModifier.h>
#include <plugins/particles/gui/modifier/analysis/StructureListParameterUI.h>
#include <gui/properties/BooleanParameterUI.h>
#include <gui/properties/IntegerRadioButtonParameterUI.h>
#include <gui/properties/FloatParameterUI.h>
#include "PolyhedralTemplateMatchingModifierEditor.h"

#include <qwt/qwt_plot_zoneitem.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_CLASS(PolyhedralTemplateMatchingModifierEditor);
SET_OVITO_OBJECT_EDITOR(PolyhedralTemplateMatchingModifier, PolyhedralTemplateMatchingModifierEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void PolyhedralTemplateMatchingModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Polyhedral template matching"), rolloutParams, "particles.modifiers.polyhedral_template_matching.html");

	// Create the rollout contents.
	QVBoxLayout* layout1 = new QVBoxLayout(rollout);
	layout1->setContentsMargins(4,4,4,4);
	layout1->setSpacing(6);

	QGroupBox* paramsBox = new QGroupBox(tr("Parameters"), rollout);
	QGridLayout* gridlayout = new QGridLayout(paramsBox);
	gridlayout->setContentsMargins(4,4,4,4);
	gridlayout->setColumnStretch(1, 1);
	layout1->addWidget(paramsBox);

	// RMSD cutoff parameter.
	FloatParameterUI* rmsdCutoffPUI = new FloatParameterUI(this, PROPERTY_FIELD(PolyhedralTemplateMatchingModifier::rmsdCutoff));
	gridlayout->addWidget(rmsdCutoffPUI->label(), 0, 0);
	gridlayout->addLayout(rmsdCutoffPUI->createFieldLayout(), 0, 1);

	// Use only selected particles.
	BooleanParameterUI* onlySelectedParticlesUI = new BooleanParameterUI(this, PROPERTY_FIELD(StructureIdentificationModifier::onlySelectedParticles));
	gridlayout->addWidget(onlySelectedParticlesUI->checkBox(), 1, 0, 1, 2);

	QGroupBox* outputBox = new QGroupBox(tr("Output"), rollout);
	QGridLayout* sublayout = new QGridLayout(outputBox);
	sublayout->setContentsMargins(4,4,4,4);
	sublayout->setColumnStretch(1, 1);
	sublayout->setColumnMinimumWidth(0, 12);
	layout1->addWidget(outputBox);

	// Output controls.
	BooleanParameterUI* outputRmsdUI = new BooleanParameterUI(this, PROPERTY_FIELD(PolyhedralTemplateMatchingModifier::outputRmsd));
	sublayout->addWidget(outputRmsdUI->checkBox(), 0, 0, 1, 2);
	outputRmsdUI->checkBox()->setText(tr("RMSD values"));
	BooleanParameterUI* outputInteratomicDistanceUI = new BooleanParameterUI(this, PROPERTY_FIELD(PolyhedralTemplateMatchingModifier::outputInteratomicDistance));
	sublayout->addWidget(outputInteratomicDistanceUI->checkBox(), 1, 0, 1, 2);
	outputInteratomicDistanceUI->checkBox()->setText(tr("Interatomic distances"));

	BooleanParameterUI* outputDeformationGradientUI = new BooleanParameterUI(this, PROPERTY_FIELD(PolyhedralTemplateMatchingModifier::outputDeformationGradient));
	sublayout->addWidget(outputDeformationGradientUI->checkBox(), 2, 0, 1, 2);
	outputDeformationGradientUI->checkBox()->setText(tr("Elastic deformation gradients"));
	BooleanParameterUI* outputOrderingTypesUI = new BooleanParameterUI(this, PROPERTY_FIELD(PolyhedralTemplateMatchingModifier::outputOrderingTypes));
	sublayout->addWidget(outputOrderingTypesUI->checkBox(), 3, 0, 1, 2);
	outputOrderingTypesUI->checkBox()->setText(tr("Ordering types"));

	// Lattice orientations
	BooleanParameterUI* outputOrientationUI = new BooleanParameterUI(this, PROPERTY_FIELD(PolyhedralTemplateMatchingModifier::outputOrientation));
	sublayout->addWidget(outputOrientationUI->checkBox(), 4, 0, 1, 2);
	outputOrientationUI->checkBox()->setText(tr("Lattice orientations"));

	// Selection of reference configuration for lattice orientation calculation:
	IntegerRadioButtonParameterUI* referenceOrientationUI = new IntegerRadioButtonParameterUI(this, PROPERTY_FIELD(PolyhedralTemplateMatchingModifier::useStandardOrientations));
	referenceOrientationUI->setEnabled(false);
	QRadioButton* standardModeBtn = referenceOrientationUI->addRadioButton(true, tr("Use standard reference orientations"));
	QRadioButton* templateModeBtn = referenceOrientationUI->addRadioButton(false, tr("Use PTM template orientations"));
	sublayout->addWidget(standardModeBtn, 5, 1);
	sublayout->addWidget(templateModeBtn, 6, 1);
	connect(outputOrientationUI->checkBox(), &QCheckBox::toggled, referenceOrientationUI, &IntegerRadioButtonParameterUI::setEnabled);

	// Color by type
	BooleanParameterUI* colorByTypeUI = new BooleanParameterUI(this, PROPERTY_FIELD(StructureIdentificationModifier::colorByType));
	sublayout->addWidget(colorByTypeUI->checkBox(), 7, 0, 1, 2);

	StructureListParameterUI* structureTypesPUI = new StructureListParameterUI(this, true);
	layout1->addSpacing(10);
	layout1->addWidget(structureTypesPUI->tableWidget());
	QLabel* label = new QLabel(tr("<p style=\"font-size: small;\">Double-click to change colors. Defaults can be set in the application settings.</p>"));
	label->setWordWrap(true);
	layout1->addWidget(label);

	// Create plot widget for RMSD distribution.
	_rmsdPlotWidget = new DataSeriesPlotWidget();
	_rmsdPlotWidget->setMinimumHeight(200);
	_rmsdPlotWidget->setMaximumHeight(200);
	_rmsdRangeIndicator = new QwtPlotZoneItem();
	_rmsdRangeIndicator->setOrientation(Qt::Vertical);
	_rmsdRangeIndicator->setZ(1);
	_rmsdRangeIndicator->attach(_rmsdPlotWidget);
	_rmsdRangeIndicator->hide();
	layout1->addSpacing(10);
	layout1->addWidget(_rmsdPlotWidget);
	connect(this, &PolyhedralTemplateMatchingModifierEditor::contentsReplaced, this, &PolyhedralTemplateMatchingModifierEditor::plotHistogram);

	// Status label.
	layout1->addSpacing(10);
	layout1->addWidget(statusLabel());
}

/******************************************************************************
* This method is called when a reference target changes.
******************************************************************************/
bool PolyhedralTemplateMatchingModifierEditor::referenceEvent(RefTarget* source, const ReferenceEvent& event)
{
	if(source == modifierApplication() && event.type() == ReferenceEvent::PipelineCacheUpdated) {
		plotHistogramLater(this);
	}
	return ModifierPropertiesEditor::referenceEvent(source, event);
}

/******************************************************************************
* Replots the histogram computed by the modifier.
******************************************************************************/
void PolyhedralTemplateMatchingModifierEditor::plotHistogram()
{
	PolyhedralTemplateMatchingModifier* modifier = static_object_cast<PolyhedralTemplateMatchingModifier>(editObject());
	if(modifier && modifier->rmsdCutoff() > 0) {
		_rmsdRangeIndicator->setInterval(0, modifier->rmsdCutoff());
		_rmsdRangeIndicator->show();
	}
	else {
		_rmsdRangeIndicator->hide();
	}

	if(modifierApplication()) {
		// Request the modifier's pipeline output.
		const PipelineFlowState& state = getModifierOutput();

		// Look up the data series in the modifier's pipeline output.
		_rmsdPlotWidget->setSeries(state.getObjectBy<DataSeriesObject>(modifierApplication(), QStringLiteral("ptm-rmsd")));
	}
	else {
		_rmsdPlotWidget->reset();
	}
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
