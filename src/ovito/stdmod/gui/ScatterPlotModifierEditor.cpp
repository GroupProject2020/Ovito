////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2013 Alexander Stukowski
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

#include <ovito/stdmod/gui/StdModGui.h>
#include <ovito/stdobj/gui/widgets/PropertyReferenceParameterUI.h>
#include <ovito/stdobj/gui/widgets/PropertyContainerParameterUI.h>
#include <ovito/stdobj/table/DataTable.h>
#include <ovito/gui/properties/IntegerParameterUI.h>
#include <ovito/gui/properties/FloatParameterUI.h>
#include <ovito/gui/properties/BooleanParameterUI.h>
#include <ovito/gui/mainwin/MainWindow.h>
#include <ovito/stdmod/modifiers/ScatterPlotModifier.h>
#include "ScatterPlotModifierEditor.h"

#include <qwt/qwt_plot.h>
#include <qwt/qwt_plot_spectrocurve.h>
#include <qwt/qwt_plot_zoneitem.h>
#include <qwt/qwt_plot_grid.h>
#include <qwt/qwt_color_map.h>

namespace Ovito { namespace StdMod {

IMPLEMENT_OVITO_CLASS(ScatterPlotModifierEditor);
SET_OVITO_OBJECT_EDITOR(ScatterPlotModifier, ScatterPlotModifierEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void ScatterPlotModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Scatter plot"), rolloutParams, "particles.modifiers.scatter_plot.html");

    // Create the rollout contents.
	QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);

	PropertyContainerParameterUI* pclassUI = new PropertyContainerParameterUI(this, PROPERTY_FIELD(GenericPropertyModifier::subject));
	layout->addWidget(new QLabel(tr("Operate on:")));
	layout->addWidget(pclassUI->comboBox());
	layout->addSpacing(6);

	// Do not list data tables as available inputs.
	pclassUI->setContainerFilter([](const PropertyContainer* container) {
		return DataTable::OOClass().isMember(container) == false;
	});


	PropertyReferenceParameterUI* xPropertyUI = new PropertyReferenceParameterUI(this, PROPERTY_FIELD(ScatterPlotModifier::xAxisProperty), nullptr);
	layout->addWidget(new QLabel(tr("X-axis property:"), rollout));
	layout->addWidget(xPropertyUI->comboBox());
	PropertyReferenceParameterUI* yPropertyUI = new PropertyReferenceParameterUI(this, PROPERTY_FIELD(ScatterPlotModifier::yAxisProperty), nullptr);
	layout->addWidget(new QLabel(tr("Y-axis property:"), rollout));
	layout->addWidget(yPropertyUI->comboBox());
	connect(this, &PropertiesEditor::contentsChanged, this, [xPropertyUI,yPropertyUI](RefTarget* editObject) {
		GenericPropertyModifier* modifier = static_object_cast<GenericPropertyModifier>(editObject);
		if(modifier) {
			xPropertyUI->setContainerRef(modifier->subject());
			yPropertyUI->setContainerRef(modifier->subject());
		}
		else {
			xPropertyUI->setContainerRef({});
			yPropertyUI->setContainerRef({});
		}
	});
	layout->addSpacing(6);

	_plotWidget = new DataTablePlotWidget();
	_plotWidget->setMinimumHeight(240);
	_plotWidget->setMaximumHeight(240);
	_selectionRangeIndicatorX = new QwtPlotZoneItem();
	_selectionRangeIndicatorX->setOrientation(Qt::Vertical);
	_selectionRangeIndicatorX->setZ(1);
	_selectionRangeIndicatorX->attach(_plotWidget);
	_selectionRangeIndicatorX->hide();
	_selectionRangeIndicatorY = new QwtPlotZoneItem();
	_selectionRangeIndicatorY->setOrientation(Qt::Horizontal);
	_selectionRangeIndicatorY->setZ(1);
	_selectionRangeIndicatorY->attach(_plotWidget);
	_selectionRangeIndicatorY->hide();

	layout->addWidget(new QLabel(tr("Scatter plot:")));
	layout->addWidget(_plotWidget);

	QPushButton* btn = new QPushButton(tr("Show in data inspector"));
	connect(btn, &QPushButton::clicked, this, [this]() {
		if(modifierApplication())
			mainWindow()->openDataInspector(modifierApplication());
	});
	layout->addWidget(btn);

	// Selection.
	QGroupBox* selectionBox = new QGroupBox(tr("Selection"), rollout);
	QVBoxLayout* sublayout = new QVBoxLayout(selectionBox);
	sublayout->setContentsMargins(4,4,4,4);
	layout->addWidget(selectionBox);

	BooleanParameterUI* selectInRangeUI = new BooleanParameterUI(this, PROPERTY_FIELD(ScatterPlotModifier::selectXAxisInRange));
	sublayout->addWidget(selectInRangeUI->checkBox());

	QHBoxLayout* hlayout = new QHBoxLayout();
	sublayout->addLayout(hlayout);
	FloatParameterUI* selRangeStartPUI = new FloatParameterUI(this, PROPERTY_FIELD(ScatterPlotModifier::selectionXAxisRangeStart));
	FloatParameterUI* selRangeEndPUI = new FloatParameterUI(this, PROPERTY_FIELD(ScatterPlotModifier::selectionXAxisRangeEnd));
	hlayout->addWidget(new QLabel(tr("From:")));
	hlayout->addLayout(selRangeStartPUI->createFieldLayout());
	hlayout->addSpacing(12);
	hlayout->addWidget(new QLabel(tr("To:")));
	hlayout->addLayout(selRangeEndPUI->createFieldLayout());
	selRangeStartPUI->setEnabled(false);
	selRangeEndPUI->setEnabled(false);
	connect(selectInRangeUI->checkBox(), &QCheckBox::toggled, selRangeStartPUI, &FloatParameterUI::setEnabled);
	connect(selectInRangeUI->checkBox(), &QCheckBox::toggled, selRangeEndPUI, &FloatParameterUI::setEnabled);

	selectInRangeUI = new BooleanParameterUI(this, PROPERTY_FIELD(ScatterPlotModifier::selectYAxisInRange));
	sublayout->addWidget(selectInRangeUI->checkBox());

	hlayout = new QHBoxLayout();
	sublayout->addLayout(hlayout);
	selRangeStartPUI = new FloatParameterUI(this, PROPERTY_FIELD(ScatterPlotModifier::selectionYAxisRangeStart));
	selRangeEndPUI = new FloatParameterUI(this, PROPERTY_FIELD(ScatterPlotModifier::selectionYAxisRangeEnd));
	hlayout->addWidget(new QLabel(tr("From:")));
	hlayout->addLayout(selRangeStartPUI->createFieldLayout());
	hlayout->addSpacing(12);
	hlayout->addWidget(new QLabel(tr("To:")));
	hlayout->addLayout(selRangeEndPUI->createFieldLayout());
	selRangeStartPUI->setEnabled(false);
	selRangeEndPUI->setEnabled(false);
	connect(selectInRangeUI->checkBox(), &QCheckBox::toggled, selRangeStartPUI, &FloatParameterUI::setEnabled);
	connect(selectInRangeUI->checkBox(), &QCheckBox::toggled, selRangeEndPUI, &FloatParameterUI::setEnabled);

	// Axes.
	QGroupBox* axesBox = new QGroupBox(tr("Plot axes"), rollout);
	QVBoxLayout* axesSublayout = new QVBoxLayout(axesBox);
	axesSublayout->setContentsMargins(4,4,4,4);
	layout->addWidget(axesBox);
	// x-axis.
	{
		BooleanParameterUI* rangeUI = new BooleanParameterUI(this, PROPERTY_FIELD(ScatterPlotModifier::fixXAxisRange));
		axesSublayout->addWidget(rangeUI->checkBox());

		QHBoxLayout* hlayout = new QHBoxLayout();
		axesSublayout->addLayout(hlayout);
		FloatParameterUI* startPUI = new FloatParameterUI(this, PROPERTY_FIELD(ScatterPlotModifier::xAxisRangeStart));
		FloatParameterUI* endPUI = new FloatParameterUI(this, PROPERTY_FIELD(ScatterPlotModifier::xAxisRangeEnd));
		hlayout->addWidget(new QLabel(tr("From:")));
		hlayout->addLayout(startPUI->createFieldLayout());
		hlayout->addSpacing(12);
		hlayout->addWidget(new QLabel(tr("To:")));
		hlayout->addLayout(endPUI->createFieldLayout());
		startPUI->setEnabled(false);
		endPUI->setEnabled(false);
		connect(rangeUI->checkBox(), &QCheckBox::toggled, startPUI, &FloatParameterUI::setEnabled);
		connect(rangeUI->checkBox(), &QCheckBox::toggled, endPUI, &FloatParameterUI::setEnabled);
	}
	// y-axis.
	{
		BooleanParameterUI* rangeUI = new BooleanParameterUI(this, PROPERTY_FIELD(ScatterPlotModifier::fixYAxisRange));
		axesSublayout->addWidget(rangeUI->checkBox());

		QHBoxLayout* hlayout = new QHBoxLayout();
		axesSublayout->addLayout(hlayout);
		FloatParameterUI* startPUI = new FloatParameterUI(this, PROPERTY_FIELD(ScatterPlotModifier::yAxisRangeStart));
		FloatParameterUI* endPUI = new FloatParameterUI(this, PROPERTY_FIELD(ScatterPlotModifier::yAxisRangeEnd));
		hlayout->addWidget(new QLabel(tr("From:")));
		hlayout->addLayout(startPUI->createFieldLayout());
		hlayout->addSpacing(12);
		hlayout->addWidget(new QLabel(tr("To:")));
		hlayout->addLayout(endPUI->createFieldLayout());
		startPUI->setEnabled(false);
		endPUI->setEnabled(false);
		connect(rangeUI->checkBox(), &QCheckBox::toggled, startPUI, &FloatParameterUI::setEnabled);
		connect(rangeUI->checkBox(), &QCheckBox::toggled, endPUI, &FloatParameterUI::setEnabled);
	}

	// Status label.
	layout->addSpacing(6);
	layout->addWidget(statusLabel());

	// Update data plot whenever the modifier has calculated new results.
	connect(this, &ModifierPropertiesEditor::contentsReplaced, this, &ScatterPlotModifierEditor::plotScatterPlot);
	connect(this, &ModifierPropertiesEditor::modifierEvaluated, this, [this]() {
		plotLater(this);
	});
}

/******************************************************************************
* Replots the scatter plot computed by the modifier.
******************************************************************************/
void ScatterPlotModifierEditor::plotScatterPlot()
{
	ScatterPlotModifier* modifier = static_object_cast<ScatterPlotModifier>(editObject());

	if(modifier && modifier->fixXAxisRange()) {
		_plotWidget->setAxisScale(QwtPlot::xBottom, modifier->xAxisRangeStart(), modifier->xAxisRangeEnd());
	}
	else {
		_plotWidget->setAxisAutoScale(QwtPlot::xBottom);
	}

	if(modifier && modifier->fixYAxisRange()) {
		_plotWidget->setAxisScale(QwtPlot::yLeft, modifier->yAxisRangeStart(), modifier->yAxisRangeEnd());
	}
	else {
		_plotWidget->setAxisAutoScale(QwtPlot::yLeft);
	}

	if(modifier && modifier->selectXAxisInRange()) {
		auto minmax = std::minmax(modifier->selectionXAxisRangeStart(), modifier->selectionXAxisRangeEnd());
		_selectionRangeIndicatorX->setInterval(minmax.first, minmax.second);
		_selectionRangeIndicatorX->show();
	}
	else {
		_selectionRangeIndicatorX->hide();
	}

	if(modifier && modifier->selectYAxisInRange()) {
		auto minmax = std::minmax(modifier->selectionYAxisRangeStart(), modifier->selectionYAxisRangeEnd());
		_selectionRangeIndicatorY->setInterval(minmax.first, minmax.second);
		_selectionRangeIndicatorY->show();
	}
	else {
		_selectionRangeIndicatorY->hide();
	}

	if(modifier && modifierApplication()) {
		// Request the modifier's pipeline output.
		const PipelineFlowState& state = getModifierOutput();

		// Look up the generated data table in the modifier's pipeline output.
		const DataTable* table = state.getObjectBy<DataTable>(modifierApplication(), QStringLiteral("scatter"));
		_plotWidget->setTable(table);
	}
	else {
		_plotWidget->reset();
	}
}

}	// End of namespace
}	// End of namespace
