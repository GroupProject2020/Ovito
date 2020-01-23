////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2019 Alexander Stukowski
//  Copyright 2017 Lars Pastewka
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
#include <ovito/correlation/SpatialCorrelationFunctionModifier.h>
#include <ovito/stdobj/gui/widgets/PropertyReferenceParameterUI.h>
#include <ovito/stdobj/properties/PropertyAccess.h>
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/gui/desktop/mainwin/MainWindow.h>
#include <ovito/gui/desktop/properties/BooleanParameterUI.h>
#include <ovito/gui/desktop/properties/IntegerParameterUI.h>
#include <ovito/gui/desktop/properties/IntegerRadioButtonParameterUI.h>
#include <ovito/gui/desktop/properties/FloatParameterUI.h>
#include <ovito/gui/desktop/properties/VariantComboBoxParameterUI.h>
#include <ovito/core/oo/CloneHelper.h>
#include "SpatialCorrelationFunctionModifierEditor.h"

#include <qwt/qwt_plot.h>
#include <qwt/qwt_plot_curve.h>
#include <qwt/qwt_plot_grid.h>
#include <qwt/qwt_scale_engine.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_CLASS(SpatialCorrelationFunctionModifierEditor);
SET_OVITO_OBJECT_EDITOR(SpatialCorrelationFunctionModifier, SpatialCorrelationFunctionModifierEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void SpatialCorrelationFunctionModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Spatial correlation function"), rolloutParams, "particles.modifiers.correlation_function.html");

	// Create the rollout contents.
	QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);

	PropertyReferenceParameterUI* sourceProperty1UI = new PropertyReferenceParameterUI(this, PROPERTY_FIELD(SpatialCorrelationFunctionModifier::sourceProperty1), &ParticlesObject::OOClass());
	layout->addWidget(new QLabel(tr("First property:"), rollout));
	layout->addWidget(sourceProperty1UI->comboBox());

	PropertyReferenceParameterUI* sourceProperty2UI = new PropertyReferenceParameterUI(this, PROPERTY_FIELD(SpatialCorrelationFunctionModifier::sourceProperty2), &ParticlesObject::OOClass());
	layout->addWidget(new QLabel(tr("Second property:"), rollout));
	layout->addWidget(sourceProperty2UI->comboBox());

	QGridLayout* gridlayout = new QGridLayout();
	gridlayout->setContentsMargins(4,4,4,4);
	gridlayout->setColumnStretch(1, 1);

	// FFT grid spacing parameter.
	FloatParameterUI* fftGridSpacingRadiusPUI = new FloatParameterUI(this, PROPERTY_FIELD(SpatialCorrelationFunctionModifier::fftGridSpacing));
	gridlayout->addWidget(fftGridSpacingRadiusPUI->label(), 0, 0);
	gridlayout->addLayout(fftGridSpacingRadiusPUI->createFieldLayout(), 0, 1);

	layout->addLayout(gridlayout);

	BooleanParameterUI* applyWindowUI = new BooleanParameterUI(this, PROPERTY_FIELD(SpatialCorrelationFunctionModifier::applyWindow));
	layout->addWidget(applyWindowUI->checkBox());

#if 0
	gridlayout = new QGridLayout();
	gridlayout->addWidget(new QLabel(tr("Average:"), rollout), 0, 0);
	VariantComboBoxParameterUI* averagingDirectionPUI = new VariantComboBoxParameterUI(this, PROPERTY_FIELD(SpatialCorrelationFunctionModifier::averagingDirection));
    averagingDirectionPUI->comboBox()->addItem("radial", QVariant::fromValue(SpatialCorrelationFunctionModifier::RADIAL));
    averagingDirectionPUI->comboBox()->addItem("cell vector 1", QVariant::fromValue(SpatialCorrelationFunctionModifier::CELL_VECTOR_1));
    averagingDirectionPUI->comboBox()->addItem("cell vector 2", QVariant::fromValue(SpatialCorrelationFunctionModifier::CELL_VECTOR_2));
    averagingDirectionPUI->comboBox()->addItem("cell vector 3", QVariant::fromValue(SpatialCorrelationFunctionModifier::CELL_VECTOR_3));
    gridlayout->addWidget(averagingDirectionPUI->comboBox(), 0, 1);
    layout->addLayout(gridlayout);
#endif

	QGroupBox* realSpaceGroupBox = new QGroupBox(tr("Real-space correlation function"));
	layout->addWidget(realSpaceGroupBox);

	BooleanParameterUI* doComputeNeighCorrelationUI = new BooleanParameterUI(this, PROPERTY_FIELD(SpatialCorrelationFunctionModifier::doComputeNeighCorrelation));

	QGridLayout* realSpaceGridLayout = new QGridLayout();
	realSpaceGridLayout->setContentsMargins(4,4,4,4);
	realSpaceGridLayout->setColumnStretch(1, 1);

	// Neighbor cutoff parameter.
	FloatParameterUI *neighCutoffRadiusPUI = new FloatParameterUI(this, PROPERTY_FIELD(SpatialCorrelationFunctionModifier::neighCutoff));
	neighCutoffRadiusPUI->setEnabled(false);
	realSpaceGridLayout->addWidget(neighCutoffRadiusPUI->label(), 1, 0);
	realSpaceGridLayout->addLayout(neighCutoffRadiusPUI->createFieldLayout(), 1, 1);

	// Number of bins parameter.
	IntegerParameterUI* numberOfNeighBinsPUI = new IntegerParameterUI(this, PROPERTY_FIELD(SpatialCorrelationFunctionModifier::numberOfNeighBins));
	numberOfNeighBinsPUI->setEnabled(false);
	realSpaceGridLayout->addWidget(numberOfNeighBinsPUI->label(), 2, 0);
	realSpaceGridLayout->addLayout(numberOfNeighBinsPUI->createFieldLayout(), 2, 1);

	connect(doComputeNeighCorrelationUI->checkBox(), &QCheckBox::toggled, neighCutoffRadiusPUI, &FloatParameterUI::setEnabled);
	connect(doComputeNeighCorrelationUI->checkBox(), &QCheckBox::toggled, numberOfNeighBinsPUI, &IntegerParameterUI::setEnabled);

	QGridLayout *normalizeRealSpaceLayout = new QGridLayout();
	normalizeRealSpaceLayout->addWidget(new QLabel(tr("Type of plot:"), rollout), 0, 0);
	VariantComboBoxParameterUI* normalizeRealSpacePUI = new VariantComboBoxParameterUI(this, PROPERTY_FIELD(SpatialCorrelationFunctionModifier::normalizeRealSpace));
    normalizeRealSpacePUI->comboBox()->addItem("Value correlation", QVariant::fromValue(SpatialCorrelationFunctionModifier::VALUE_CORRELATION));
    normalizeRealSpacePUI->comboBox()->addItem("Difference correlation", QVariant::fromValue(SpatialCorrelationFunctionModifier::DIFFERENCE_CORRELATION));
    normalizeRealSpaceLayout->addWidget(normalizeRealSpacePUI->comboBox(), 0, 1);

	BooleanParameterUI* normalizeRealSpaceByRDFUI = new BooleanParameterUI(this, PROPERTY_FIELD(SpatialCorrelationFunctionModifier::normalizeRealSpaceByRDF));
	BooleanParameterUI* normalizeRealSpaceByCovarianceUI = new BooleanParameterUI(this, PROPERTY_FIELD(SpatialCorrelationFunctionModifier::normalizeRealSpaceByCovariance));

	QGridLayout* typeOfRealSpacePlotLayout = new QGridLayout();
	IntegerRadioButtonParameterUI *typeOfRealSpacePlotPUI = new IntegerRadioButtonParameterUI(this, PROPERTY_FIELD(SpatialCorrelationFunctionModifier::typeOfRealSpacePlot));
	typeOfRealSpacePlotLayout->addWidget(new QLabel(tr("Display as:")), 0, 0);
	typeOfRealSpacePlotLayout->addWidget(typeOfRealSpacePlotPUI->addRadioButton(0, tr("lin-lin")), 0, 1);
	typeOfRealSpacePlotLayout->addWidget(typeOfRealSpacePlotPUI->addRadioButton(1, tr("log-lin")), 0, 2);
	typeOfRealSpacePlotLayout->addWidget(typeOfRealSpacePlotPUI->addRadioButton(3, tr("log-log")), 0, 3);

	_realSpacePlot = new DataTablePlotWidget();
	_realSpacePlot->setMinimumHeight(200);
	_realSpacePlot->setMaximumHeight(200);
	_neighCurve = new QwtPlotCurve();
	_neighCurve->setRenderHint(QwtPlotItem::RenderAntialiased, true);
	_neighCurve->setPen(QPen(Qt::red, 1));
	_neighCurve->setZ(1);
	_neighCurve->attach(_realSpacePlot);
	_neighCurve->hide();

	// Axes.
	QGroupBox* axesBox = new QGroupBox(tr("Plot axes"), rollout);
	QVBoxLayout* axesSublayout = new QVBoxLayout(axesBox);
	axesSublayout->setContentsMargins(4,4,4,4);
	layout->addWidget(axesBox);
	// x-axis.
	{
		BooleanParameterUI* rangeUI = new BooleanParameterUI(this, PROPERTY_FIELD(SpatialCorrelationFunctionModifier::fixRealSpaceXAxisRange));
		axesSublayout->addWidget(rangeUI->checkBox());

		QHBoxLayout* hlayout = new QHBoxLayout();
		axesSublayout->addLayout(hlayout);
		FloatParameterUI* startPUI = new FloatParameterUI(this, PROPERTY_FIELD(SpatialCorrelationFunctionModifier::realSpaceXAxisRangeStart));
		FloatParameterUI* endPUI = new FloatParameterUI(this, PROPERTY_FIELD(SpatialCorrelationFunctionModifier::realSpaceXAxisRangeEnd));
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
		BooleanParameterUI* rangeUI = new BooleanParameterUI(this, PROPERTY_FIELD(SpatialCorrelationFunctionModifier::fixRealSpaceYAxisRange));
		axesSublayout->addWidget(rangeUI->checkBox());

		QHBoxLayout* hlayout = new QHBoxLayout();
		axesSublayout->addLayout(hlayout);
		FloatParameterUI* startPUI = new FloatParameterUI(this, PROPERTY_FIELD(SpatialCorrelationFunctionModifier::realSpaceYAxisRangeStart));
		FloatParameterUI* endPUI = new FloatParameterUI(this, PROPERTY_FIELD(SpatialCorrelationFunctionModifier::realSpaceYAxisRangeEnd));
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

	QVBoxLayout* realSpaceLayout = new QVBoxLayout(realSpaceGroupBox);
	realSpaceLayout->addWidget(doComputeNeighCorrelationUI->checkBox());
	realSpaceLayout->addLayout(realSpaceGridLayout);
	realSpaceLayout->addLayout(normalizeRealSpaceLayout);
	realSpaceLayout->addWidget(normalizeRealSpaceByRDFUI->checkBox());
	realSpaceLayout->addWidget(normalizeRealSpaceByCovarianceUI->checkBox());
	realSpaceLayout->addLayout(typeOfRealSpacePlotLayout);
	realSpaceLayout->addWidget(_realSpacePlot);
	realSpaceLayout->addWidget(axesBox);

	QGroupBox* reciprocalSpaceGroupBox = new QGroupBox(tr("Reciprocal-space correlation function"));
	layout->addWidget(reciprocalSpaceGroupBox);

	BooleanParameterUI* normalizeReciprocalSpaceUI = new BooleanParameterUI(this, PROPERTY_FIELD(SpatialCorrelationFunctionModifier::normalizeReciprocalSpace));

	QGridLayout* typeOfReciprocalSpacePlotLayout = new QGridLayout();
	IntegerRadioButtonParameterUI *typeOfReciprocalSpacePlotPUI = new IntegerRadioButtonParameterUI(this, PROPERTY_FIELD(SpatialCorrelationFunctionModifier::typeOfReciprocalSpacePlot));
	typeOfReciprocalSpacePlotLayout->addWidget(new QLabel(tr("Display as:")), 0, 0);
	typeOfReciprocalSpacePlotLayout->addWidget(typeOfReciprocalSpacePlotPUI->addRadioButton(0, tr("lin-lin")), 0, 1);
	typeOfReciprocalSpacePlotLayout->addWidget(typeOfReciprocalSpacePlotPUI->addRadioButton(1, tr("log-lin")), 0, 2);
	typeOfReciprocalSpacePlotLayout->addWidget(typeOfReciprocalSpacePlotPUI->addRadioButton(3, tr("log-log")), 0, 3);

	_reciprocalSpacePlot = new DataTablePlotWidget();
	_reciprocalSpacePlot->setMinimumHeight(200);
	_reciprocalSpacePlot->setMaximumHeight(200);

	// Axes.
	axesBox = new QGroupBox(tr("Plot axes"), rollout);
	axesSublayout = new QVBoxLayout(axesBox);
	axesSublayout->setContentsMargins(4,4,4,4);
	layout->addWidget(axesBox);
	// x-axis.
	{
		BooleanParameterUI* rangeUI = new BooleanParameterUI(this, PROPERTY_FIELD(SpatialCorrelationFunctionModifier::fixReciprocalSpaceXAxisRange));
		axesSublayout->addWidget(rangeUI->checkBox());

		QHBoxLayout* hlayout = new QHBoxLayout();
		axesSublayout->addLayout(hlayout);
		FloatParameterUI* startPUI = new FloatParameterUI(this, PROPERTY_FIELD(SpatialCorrelationFunctionModifier::reciprocalSpaceXAxisRangeStart));
		FloatParameterUI* endPUI = new FloatParameterUI(this, PROPERTY_FIELD(SpatialCorrelationFunctionModifier::reciprocalSpaceXAxisRangeEnd));
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
		BooleanParameterUI* rangeUI = new BooleanParameterUI(this, PROPERTY_FIELD(SpatialCorrelationFunctionModifier::fixReciprocalSpaceYAxisRange));
		axesSublayout->addWidget(rangeUI->checkBox());

		QHBoxLayout* hlayout = new QHBoxLayout();
		axesSublayout->addLayout(hlayout);
		FloatParameterUI* startPUI = new FloatParameterUI(this, PROPERTY_FIELD(SpatialCorrelationFunctionModifier::reciprocalSpaceYAxisRangeStart));
		FloatParameterUI* endPUI = new FloatParameterUI(this, PROPERTY_FIELD(SpatialCorrelationFunctionModifier::reciprocalSpaceYAxisRangeEnd));
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

	QVBoxLayout* reciprocalSpaceLayout = new QVBoxLayout(reciprocalSpaceGroupBox);
	reciprocalSpaceLayout->addWidget(normalizeReciprocalSpaceUI->checkBox());
	reciprocalSpaceLayout->addLayout(typeOfReciprocalSpacePlotLayout);
	reciprocalSpaceLayout->addWidget(_reciprocalSpacePlot);
	reciprocalSpaceLayout->addWidget(axesBox);

	connect(this, &SpatialCorrelationFunctionModifierEditor::contentsReplaced, this, &SpatialCorrelationFunctionModifierEditor::plotAllData);

	// Status label.
	layout->addSpacing(6);
	layout->addWidget(statusLabel());

	// Update data plot whenever the modifier has calculated new results.
	connect(this, &ModifierPropertiesEditor::contentsChanged, this, [this]() {
		plotAllDataLater(this);
	});
	connect(this, &ModifierPropertiesEditor::modifierEvaluated, this, [this]() {
		plotAllDataLater(this);
	});
}

/******************************************************************************
* Replots one of the correlation function computed by the modifier.
******************************************************************************/
std::pair<FloatType,FloatType> SpatialCorrelationFunctionModifierEditor::plotData(
												const DataTable* table,
												DataTablePlotWidget* plotWidget,
												FloatType offset,
												FloatType fac,
												ConstPropertyAccess<FloatType> normalization)
{
	// Duplicate the data table, then modify the stored values.
	UndoSuspender noUndo(table);
	CloneHelper cloneHelper;
	OORef<DataTable> clonedTable = cloneHelper.cloneObject(table, false);
	clonedTable->makePropertiesMutable();

	// Normalize function values.
	if(normalization) {
		OVITO_ASSERT(normalization.size() == clonedTable->elementCount());
		auto pf = normalization.cbegin();
		PropertyAccess<FloatType> valueArray = clonedTable->expectMutableProperty(DataTable::YProperty);
		for(FloatType& v : valueArray) {
			FloatType factor = *pf++;
			v = (factor > FloatType(1e-12)) ? (v / factor) : FloatType(0);
		}
	}

	// Scale and shift function values.
	if(fac != 1 || offset != 0) {
		PropertyAccess<FloatType> valueArray = clonedTable->expectMutableProperty(DataTable::YProperty);
		for(FloatType& v :valueArray)
			v = fac * (v - offset);
	}

	// Determine value range.
	ConstPropertyAccess<FloatType> yarray = clonedTable->getY();
	auto minmax = std::minmax_element(yarray.cbegin(), yarray.cend());

	// Hand data table over to plot widget.
	plotWidget->setTable(std::move(clonedTable));

	return { *minmax.first, *minmax.second };
}

/******************************************************************************
* Updates the plot of the RDF computed by the modifier.
******************************************************************************/
void SpatialCorrelationFunctionModifierEditor::plotAllData()
{
	SpatialCorrelationFunctionModifier* modifier = static_object_cast<SpatialCorrelationFunctionModifier>(editObject());

	// Set type of plot.
	if(modifier && modifier->typeOfRealSpacePlot() & 1)
		_realSpacePlot->setAxisScaleEngine(QwtPlot::yLeft, new QwtLogScaleEngine());
	else
		_realSpacePlot->setAxisScaleEngine(QwtPlot::yLeft, new QwtLinearScaleEngine());
	if(modifier && modifier->typeOfRealSpacePlot() & 2)
		_realSpacePlot->setAxisScaleEngine(QwtPlot::xBottom, new QwtLogScaleEngine());
	else
		_realSpacePlot->setAxisScaleEngine(QwtPlot::xBottom, new QwtLinearScaleEngine());

	if(modifier && modifier->typeOfReciprocalSpacePlot() & 1)
		_reciprocalSpacePlot->setAxisScaleEngine(QwtPlot::yLeft, new QwtLogScaleEngine());
	else
		_reciprocalSpacePlot->setAxisScaleEngine(QwtPlot::yLeft, new QwtLinearScaleEngine());
	if(modifier && modifier->typeOfReciprocalSpacePlot() & 2)
		_reciprocalSpacePlot->setAxisScaleEngine(QwtPlot::xBottom, new QwtLogScaleEngine());
	else
		_reciprocalSpacePlot->setAxisScaleEngine(QwtPlot::xBottom, new QwtLinearScaleEngine());

	// Set axis ranges.
	if(modifier && modifier->fixRealSpaceXAxisRange())
		_realSpacePlot->setAxisScale(QwtPlot::xBottom, modifier->realSpaceXAxisRangeStart(), modifier->realSpaceXAxisRangeEnd());
	else
		_realSpacePlot->setAxisAutoScale(QwtPlot::xBottom);
	if(modifier && modifier->fixRealSpaceYAxisRange())
		_realSpacePlot->setAxisScale(QwtPlot::yLeft, modifier->realSpaceYAxisRangeStart(), modifier->realSpaceYAxisRangeEnd());
	else
		_realSpacePlot->setAxisAutoScale(QwtPlot::yLeft);
	if(modifier && modifier->fixReciprocalSpaceXAxisRange())
		_reciprocalSpacePlot->setAxisScale(QwtPlot::xBottom, modifier->reciprocalSpaceXAxisRangeStart(), modifier->reciprocalSpaceXAxisRangeEnd());
	else
		_reciprocalSpacePlot->setAxisAutoScale(QwtPlot::xBottom);
	if(modifier && modifier->fixReciprocalSpaceYAxisRange())
		_reciprocalSpacePlot->setAxisScale(QwtPlot::yLeft, modifier->reciprocalSpaceYAxisRangeStart(), modifier->reciprocalSpaceYAxisRangeEnd());
	else
		_reciprocalSpacePlot->setAxisAutoScale(QwtPlot::yLeft);

	// Obtain the pipeline data produced by the modifier.
	const PipelineFlowState& state = getModifierOutput();

	// Retreive computed values from pipeline.
	const QVariant& mean1 = state.getAttributeValue(modifierApplication(), QStringLiteral("CorrelationFunction.mean1"));
	const QVariant& mean2 = state.getAttributeValue(modifierApplication(), QStringLiteral("CorrelationFunction.mean2"));
	const QVariant& variance1 = state.getAttributeValue(modifierApplication(), QStringLiteral("CorrelationFunction.variance1"));
	const QVariant& variance2 = state.getAttributeValue(modifierApplication(), QStringLiteral("CorrelationFunction.variance2"));
	const QVariant& covariance = state.getAttributeValue(modifierApplication(), QStringLiteral("CorrelationFunction.covariance"));

	// Determine scaling factor and offset.
	FloatType offset = 0.0;
	FloatType uniformFactor = 1;
	if(modifier && variance1.isValid() && variance2.isValid() && covariance.isValid()) {
		if(modifier->normalizeRealSpace() == SpatialCorrelationFunctionModifier::DIFFERENCE_CORRELATION) {
			offset = 0.5 * (variance1.toDouble() + variance2.toDouble());
			uniformFactor = -1;
		}
		if(modifier->normalizeRealSpaceByCovariance() && covariance.toDouble() != 0) {
			uniformFactor /= covariance.toDouble();
		}
	}

	// Display direct neighbor correlation function.
	const DataTable* neighCorrelation = state.getObjectBy<DataTable>(modifierApplication(), QStringLiteral("correlation-neighbor"));
	const DataTable* neighRDF = state.getObjectBy<DataTable>(modifierApplication(), QStringLiteral("correlation-neighbor-rdf"));
	if(modifier && modifierApplication() && modifier->doComputeNeighCorrelation() && neighCorrelation && neighRDF) {
		const auto& xStorage = neighCorrelation->getXStorage();
		ConstPropertyAccess<FloatType> xData(xStorage);
		const auto& yStorage = neighCorrelation->getYStorage();
		ConstPropertyAccess<FloatType> yData(yStorage);
		const auto& rdfStorage = neighRDF->getYStorage();
		ConstPropertyAccess<FloatType> rdfData(rdfStorage);
		size_t numberOfDataPoints = yData.size();
		QVector<QPointF> plotData(numberOfDataPoints);
		bool normByRDF = modifier->normalizeRealSpaceByRDF();
		for(size_t i = 0; i < numberOfDataPoints; i++) {
			FloatType xValue = xData[i];
			FloatType yValue = yData[i];
			if(normByRDF)
				yValue = rdfData[i] > 1e-12 ? (yValue / rdfData[i]) : 0.0;
			yValue = uniformFactor * (yValue - offset);
			plotData[i].rx() = xValue;
			plotData[i].ry() = yValue;
		}
		_neighCurve->setSamples(plotData);
		_neighCurve->show();
	}
	else {
		_neighCurve->hide();
	}

	// Plot real-space correlation function.
	const DataTable* realSpaceCorrelation = state.getObjectBy<DataTable>(modifierApplication(), QStringLiteral("correlation-real-space"));
	const DataTable* realSpaceRDF = state.getObjectBy<DataTable>(modifierApplication(), QStringLiteral("correlation-real-space-rdf"));
	if(modifier && modifierApplication() && realSpaceCorrelation) {
		auto realSpaceYRange = plotData(realSpaceCorrelation, _realSpacePlot, offset, uniformFactor,
			(realSpaceRDF && modifier->normalizeRealSpaceByRDF()) ? realSpaceRDF->getYStorage() : nullptr);

		UndoSuspender noUndo(modifier);
		if(!modifier->fixRealSpaceXAxisRange()) {
			modifier->setRealSpaceXAxisRangeStart(realSpaceCorrelation->intervalStart());
			modifier->setRealSpaceXAxisRangeEnd(realSpaceCorrelation->intervalEnd());
		}
		if(!modifier->fixRealSpaceYAxisRange()) {
			modifier->setRealSpaceYAxisRangeStart(realSpaceYRange.first);
			modifier->setRealSpaceYAxisRangeEnd(realSpaceYRange.second);
		}
	}
	else {
		_realSpacePlot->reset();
	}

	// Plot reciprocal-space correlation function.
	const DataTable* reciprocalSpaceCorrelation = state.getObjectBy<DataTable>(modifierApplication(), QStringLiteral("correlation-reciprocal-space"));
	if(modifier && modifierApplication() && reciprocalSpaceCorrelation) {
		FloatType rfac = 1;
		if(modifier->normalizeReciprocalSpace() && covariance.toDouble() != 0)
			rfac = 1.0 / covariance.toDouble();
		auto reciprocalSpaceYRange = plotData(reciprocalSpaceCorrelation, _reciprocalSpacePlot, 0.0, rfac, nullptr);

		UndoSuspender noUndo(modifier);
		if(!modifier->fixReciprocalSpaceXAxisRange()) {
			modifier->setReciprocalSpaceXAxisRangeStart(reciprocalSpaceCorrelation->intervalStart());
			modifier->setReciprocalSpaceXAxisRangeEnd(reciprocalSpaceCorrelation->intervalEnd());
		}
		if(!modifier->fixReciprocalSpaceYAxisRange()) {
			modifier->setReciprocalSpaceYAxisRangeStart(reciprocalSpaceYRange.first);
			modifier->setReciprocalSpaceYAxisRangeEnd(reciprocalSpaceYRange.second);
		}
	}
	else {
		_reciprocalSpacePlot->reset();
	}
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
