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

#include <gui/GUI.h>
#include <gui/properties/PropertyReferenceParameterUI.h>
#include <gui/properties/PropertyClassParameterUI.h>
#include <gui/properties/IntegerParameterUI.h>
#include <gui/properties/IntegerRadioButtonParameterUI.h>
#include <gui/properties/FloatParameterUI.h>
#include <gui/properties/BooleanParameterUI.h>
#include <gui/mainwin/MainWindow.h>
#include <core/dataset/pipeline/modifiers/HistogramModifier.h>
#include "HistogramModifierEditor.h"

#include <qwt/qwt_plot.h>
#include <qwt/qwt_plot_curve.h>
#include <qwt/qwt_plot_zoneitem.h>
#include <qwt/qwt_plot_grid.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Scene) OVITO_BEGIN_INLINE_NAMESPACE(StdObj) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_CLASS(HistogramModifierEditor);
SET_OVITO_OBJECT_EDITOR(HistogramModifier, HistogramModifierEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void HistogramModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Histogram"), rolloutParams, "particles.modifiers.histogram.html");

    // Create the rollout contents.
	QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);

	PropertyClassParameterUI* pclassUI = new PropertyClassParameterUI(this, PROPERTY_FIELD(GenericPropertyModifier::propertyClass));
	layout->addWidget(new QLabel(tr("Operate on:")));
	layout->addWidget(pclassUI->comboBox());

	PropertyReferenceParameterUI* sourcePropertyUI = new PropertyReferenceParameterUI(this, PROPERTY_FIELD(HistogramModifier::sourceProperty), nullptr);
	layout->addWidget(new QLabel(tr("Property:")));
	layout->addWidget(sourcePropertyUI->comboBox());
	connect(this, &PropertiesEditor::contentsChanged, this, [sourcePropertyUI](RefTarget* editObject) {
		sourcePropertyUI->setPropertyClass(editObject ? static_object_cast<GenericPropertyModifier>(editObject)->propertyClass() : nullptr);
	});

	QGridLayout* gridlayout = new QGridLayout();
	gridlayout->setContentsMargins(4,4,4,4);
	gridlayout->setColumnStretch(1, 1);

	// Number of bins parameter.
	IntegerParameterUI* numBinsPUI = new IntegerParameterUI(this, PROPERTY_FIELD(HistogramModifier::numberOfBins));
	gridlayout->addWidget(numBinsPUI->label(), 0, 0);
	gridlayout->addLayout(numBinsPUI->createFieldLayout(), 0, 1);

	layout->addLayout(gridlayout);

	_histogramPlot = new QwtPlot();
	_histogramPlot->setMinimumHeight(240);
	_histogramPlot->setMaximumHeight(240);
	_histogramPlot->setCanvasBackground(Qt::white);
	_histogramPlot->setAxisTitle(QwtPlot::yLeft, tr("Count"));

	layout->addWidget(new QLabel(tr("Histogram:")));
	layout->addWidget(_histogramPlot);
	connect(this, &HistogramModifierEditor::contentsReplaced, this, &HistogramModifierEditor::plotHistogram);

	QPushButton* saveDataButton = new QPushButton(tr("Save histogram data"));
	layout->addWidget(saveDataButton);
	connect(saveDataButton, &QPushButton::clicked, this, &HistogramModifierEditor::onSaveData);

	// Input.
	QGroupBox* inputBox = new QGroupBox(tr("Input"), rollout);
	QVBoxLayout* sublayout = new QVBoxLayout(inputBox);
	sublayout->setContentsMargins(4,4,4,4);
	layout->addWidget(inputBox);

	BooleanParameterUI* onlySelectedUI = new BooleanParameterUI(this, PROPERTY_FIELD(HistogramModifier::onlySelected));
	sublayout->addWidget(onlySelectedUI->checkBox());

	// Create selection.
	QGroupBox* selectionBox = new QGroupBox(tr("Create selection"), rollout);
	sublayout = new QVBoxLayout(selectionBox);
	sublayout->setContentsMargins(4,4,4,4);
	layout->addWidget(selectionBox);

	BooleanParameterUI* selectInRangeUI = new BooleanParameterUI(this, PROPERTY_FIELD(HistogramModifier::selectInRange));
	sublayout->addWidget(selectInRangeUI->checkBox());

	QHBoxLayout* hlayout = new QHBoxLayout();
	sublayout->addLayout(hlayout);
	FloatParameterUI* selRangeStartPUI = new FloatParameterUI(this, PROPERTY_FIELD(HistogramModifier::selectionRangeStart));
	FloatParameterUI* selRangeEndPUI = new FloatParameterUI(this, PROPERTY_FIELD(HistogramModifier::selectionRangeEnd));
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
		BooleanParameterUI* rangeUI = new BooleanParameterUI(this, PROPERTY_FIELD(HistogramModifier::fixXAxisRange));
		axesSublayout->addWidget(rangeUI->checkBox());

		QHBoxLayout* hlayout = new QHBoxLayout();
		axesSublayout->addLayout(hlayout);
		FloatParameterUI* startPUI = new FloatParameterUI(this, PROPERTY_FIELD(HistogramModifier::xAxisRangeStart));
		FloatParameterUI* endPUI = new FloatParameterUI(this, PROPERTY_FIELD(HistogramModifier::xAxisRangeEnd));
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
		BooleanParameterUI* rangeUI = new BooleanParameterUI(this, PROPERTY_FIELD(HistogramModifier::fixYAxisRange));
		axesSublayout->addWidget(rangeUI->checkBox());

		QHBoxLayout* hlayout = new QHBoxLayout();
		axesSublayout->addLayout(hlayout);
		FloatParameterUI* startPUI = new FloatParameterUI(this, PROPERTY_FIELD(HistogramModifier::yAxisRangeStart));
		FloatParameterUI* endPUI = new FloatParameterUI(this, PROPERTY_FIELD(HistogramModifier::yAxisRangeEnd));
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
}

/******************************************************************************
* This method is called when a reference target changes.
******************************************************************************/
bool HistogramModifierEditor::referenceEvent(RefTarget* source, ReferenceEvent* event)
{
	if(event->sender() == editObject() && (event->type() == ReferenceEvent::ObjectStatusChanged || event->type() == ReferenceEvent::TargetChanged)) {
		plotHistogramLater(this);
	}
	return ModifierPropertiesEditor::referenceEvent(source, event);
}

/******************************************************************************
* Replots the histogram computed by the modifier.
******************************************************************************/
void HistogramModifierEditor::plotHistogram()
{
	HistogramModifier* modifier = static_object_cast<HistogramModifier>(editObject());
	HistogramModifierApplication* modApp = dynamic_object_cast<HistogramModifierApplication>(someModifierApplication());
	if(!modifier || !modApp || !modifier->isEnabled() || modApp->histogramData().empty()) {
		if(_plotCurve) _plotCurve->hide();
		_histogramPlot->replot();
		return;
	}

	QString axisTitle = modifier->sourceProperty().nameWithComponent();
	_histogramPlot->setAxisTitle(QwtPlot::xBottom, axisTitle);

	if(modifier->fixXAxisRange() == false) {
		modifier->setXAxisRange(modApp->intervalStart(), modApp->intervalEnd());
	}
	
	size_t binCount = modApp->histogramData().size();
	FloatType binSize = (modifier->xAxisRangeEnd() - modifier->xAxisRangeStart()) / binCount;
	QVector<QPointF> plotData(binCount);
	for(size_t i = 0; i < binCount; i++) {
		plotData[i].rx() = binSize * (i + FloatType(0.5)) + modifier->xAxisRangeStart();
		plotData[i].ry() = modApp->histogramData()[i];
	}

	if(modifier->fixYAxisRange() == false) {
		auto minmaxHistogramData = std::minmax_element(modApp->histogramData().begin(), modApp->histogramData().end());
		modifier->setYAxisRange(*minmaxHistogramData.first, *minmaxHistogramData.second);
	}

	if(!_plotCurve) {
		_plotCurve = new QwtPlotCurve();
	    _plotCurve->setRenderHint(QwtPlotItem::RenderAntialiased, true);
		_plotCurve->setBrush(QColor(255, 160, 100));
		_plotCurve->attach(_histogramPlot);
		QwtPlotGrid* plotGrid = new QwtPlotGrid();
		plotGrid->setPen(Qt::gray, 0, Qt::DotLine);
		plotGrid->attach(_histogramPlot);
	}
	_plotCurve->setSamples(plotData);
	_plotCurve->show();

	if(!modifier->fixXAxisRange())
		_histogramPlot->setAxisAutoScale(QwtPlot::xBottom);
	else
		_histogramPlot->setAxisScale(QwtPlot::xBottom, modifier->xAxisRangeStart(), modifier->xAxisRangeEnd());

	if(!modifier->fixYAxisRange())
		_histogramPlot->setAxisAutoScale(QwtPlot::yLeft);
	else
		_histogramPlot->setAxisScale(QwtPlot::yLeft, modifier->yAxisRangeStart(), modifier->yAxisRangeEnd());

	if(modifier->selectInRange()) {
		if(!_selectionRange) {
			_selectionRange = new QwtPlotZoneItem();
			_selectionRange->setOrientation(Qt::Vertical);
			_selectionRange->setZ(_plotCurve->z() + 1);
			_selectionRange->attach(_histogramPlot);
		}
		_selectionRange->show();
		auto minmax = std::minmax(modifier->selectionRangeStart(), modifier->selectionRangeEnd());
		_selectionRange->setInterval(minmax.first, minmax.second);
	}
	else if(_selectionRange) {
		_selectionRange->hide();
	}

	_histogramPlot->replot();
}

/******************************************************************************
* This is called when the user has clicked the "Save Data" button.
******************************************************************************/
void HistogramModifierEditor::onSaveData()
{
	HistogramModifier* modifier = static_object_cast<HistogramModifier>(editObject());
	HistogramModifierApplication* modApp = dynamic_object_cast<HistogramModifierApplication>(someModifierApplication());
	if(!modifier || !modApp)
		return;

	if(modApp->histogramData().empty())
		return;

	QString fileName = QFileDialog::getSaveFileName(mainWindow(),
	    tr("Save Histogram"), QString(), tr("Text files (*.txt);;All files (*)"));
	if(fileName.isEmpty())
		return;

	try {

		QFile file(fileName);
		if(!file.open(QIODevice::WriteOnly | QIODevice::Text))
			modifier->throwException(tr("Could not open file for writing: %1").arg(file.errorString()));

		QTextStream stream(&file);

		QString sourceTitle = modifier->sourceProperty().nameWithComponent();

		FloatType binSize = (modifier->xAxisRangeEnd() - modifier->xAxisRangeStart()) / modApp->histogramData().size();
		stream << "# " << sourceTitle << " histogram (bin size: " << binSize << ")" << endl;
		for(int i = 0; i < modApp->histogramData().size(); i++) {
			stream << (binSize * (FloatType(i) + FloatType(0.5)) + modifier->xAxisRangeStart()) << " " <<
						modApp->histogramData()[i] << endl;
		}
	}
	catch(const Exception& ex) {
		ex.reportError();
	}
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace