///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2017) Alexander Stukowski
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

#include <ovito/gui/GUI.h>
#include <ovito/stdobj/gui/widgets/PropertyContainerParameterUI.h>
#include <ovito/stdobj/gui/widgets/PropertyReferenceParameterUI.h>
#include <ovito/grid/objects/VoxelGrid.h>
#include <ovito/gui/properties/BooleanParameterUI.h>
#include <ovito/gui/properties/IntegerParameterUI.h>
#include <ovito/gui/properties/FloatParameterUI.h>
#include <ovito/gui/properties/SubObjectParameterUI.h>
#include <ovito/grid/modifier/CreateIsosurfaceModifier.h>
#include "CreateIsosurfaceModifierEditor.h"

#include <qwt/qwt_plot_marker.h>

namespace Ovito { namespace Grid { OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_CLASS(CreateIsosurfaceModifierEditor);
SET_OVITO_OBJECT_EDITOR(CreateIsosurfaceModifier, CreateIsosurfaceModifierEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void CreateIsosurfaceModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Create isosurface"), rolloutParams, "particles.modifiers.create_isosurface.html");

    // Create the rollout contents.
	QVBoxLayout* layout1 = new QVBoxLayout(rollout);
	layout1->setContentsMargins(4,4,4,4);
	layout1->setSpacing(4);

	QGridLayout* layout2 = new QGridLayout();
	layout2->setContentsMargins(0,0,0,0);
	layout2->setSpacing(4);
	layout2->setColumnStretch(1, 1);
	layout1->addLayout(layout2);

	PropertyContainerParameterUI* pclassUI = new PropertyContainerParameterUI(this, PROPERTY_FIELD(CreateIsosurfaceModifier::subject));
	pclassUI->setContainerFilter([](const PropertyContainer* container) {
		return VoxelGrid::OOClass().isMember(container);
	});
	layout2->addWidget(new QLabel(tr("Operate on:")), 0, 0);
	layout2->addWidget(pclassUI->comboBox(), 0, 1);

	PropertyReferenceParameterUI* fieldQuantityUI = new PropertyReferenceParameterUI(this, PROPERTY_FIELD(CreateIsosurfaceModifier::sourceProperty), nullptr);
	layout2->addWidget(new QLabel(tr("Field quantity:")), 1, 0);
	layout2->addWidget(fieldQuantityUI->comboBox(), 1, 1);
	connect(this, &PropertiesEditor::contentsChanged, this, [fieldQuantityUI](RefTarget* editObject) {
		if(CreateIsosurfaceModifier* modifier = static_object_cast<CreateIsosurfaceModifier>(editObject)) {
			fieldQuantityUI->setContainerRef(modifier->subject());
		}
		else
			fieldQuantityUI->setContainerRef(nullptr);
	});

	// Isolevel parameter.
	FloatParameterUI* isolevelPUI = new FloatParameterUI(this, PROPERTY_FIELD(CreateIsosurfaceModifier::isolevelController));
	layout2->addWidget(isolevelPUI->label(), 2, 0);
	layout2->addLayout(isolevelPUI->createFieldLayout(), 2, 1);

	_plotWidget = new StdObj::DataSeriesPlotWidget();
	_plotWidget->setMinimumHeight(200);
	_plotWidget->setMaximumHeight(200);
	_isoLevelIndicator = new QwtPlotMarker();
	_isoLevelIndicator->setLineStyle(QwtPlotMarker::VLine);
	_isoLevelIndicator->setLinePen(Qt::blue, 1, Qt::DashLine);
	_isoLevelIndicator->setZ(1);
	_isoLevelIndicator->attach(_plotWidget);
	_isoLevelIndicator->hide();

	layout2->addWidget(new QLabel(tr("Histogram:")), 3, 0, 1, 2);
	layout2->addWidget(_plotWidget, 4, 0, 1, 2);

	// Status label.
	layout1->addSpacing(8);
	layout1->addWidget(statusLabel());

	// Open a sub-editor for the mesh vis element.
	new SubObjectParameterUI(this, PROPERTY_FIELD(CreateIsosurfaceModifier::surfaceMeshVis), rolloutParams.after(rollout));

	// Update data plot whenever the modifier has calculated new results.
	connect(this, &ModifierPropertiesEditor::contentsReplaced, this, &CreateIsosurfaceModifierEditor::plotHistogram);
	connect(this, &ModifierPropertiesEditor::modifierEvaluated, this, [this]() {
		plotHistogramLater(this);
	});
}

/******************************************************************************
* Replots the histogram computed by the modifier.
******************************************************************************/
void CreateIsosurfaceModifierEditor::plotHistogram()
{
	CreateIsosurfaceModifier* modifier = static_object_cast<CreateIsosurfaceModifier>(editObject());

	if(modifier && modifierApplication()) {
		_isoLevelIndicator->setXValue(modifier->isolevel());
		_isoLevelIndicator->show();

		// Request the modifier's pipeline output.
		const PipelineFlowState& state = getModifierOutput();

		// Look up the generated data series in the modifier's pipeline output.
		const DataSeriesObject* series = state.getObjectBy<DataSeriesObject>(modifierApplication(), QStringLiteral("isosurface-histogram"));
		_plotWidget->setSeries(series);
	}
	else {
		_isoLevelIndicator->hide();
		_plotWidget->reset();
	}
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
