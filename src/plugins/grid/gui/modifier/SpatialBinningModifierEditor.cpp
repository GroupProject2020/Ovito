///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2018) Alexander Stukowski
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
#include <plugins/grid/modifier/SpatialBinningModifier.h>
#include <plugins/stdobj/gui/widgets/PropertyReferenceParameterUI.h>
#include <plugins/grid/objects/VoxelGrid.h>
#include <gui/properties/BooleanParameterUI.h>
#include <gui/properties/FloatParameterUI.h>
#include <gui/properties/IntegerParameterUI.h>
#include <gui/properties/IntegerRadioButtonParameterUI.h>
#include <gui/properties/VariantComboBoxParameterUI.h>
#include "SpatialBinningModifierEditor.h"

#include <qwt/qwt_plot.h>
#include <qwt/qwt_plot_curve.h>
#include <qwt/qwt_plot_spectrogram.h>
#include <qwt/qwt_plot_grid.h>
#include <qwt/qwt_scale_engine.h>
#include <qwt/qwt_matrix_raster_data.h>
#include <qwt/qwt_color_map.h>
#include <qwt/qwt_scale_widget.h>
#include <qwt/qwt_plot_layout.h>
#include <qwt/qwt_plot_textlabel.h>

namespace Ovito { namespace Grid { OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_CLASS(SpatialBinningModifierEditor);
SET_OVITO_OBJECT_EDITOR(SpatialBinningModifier, SpatialBinningModifierEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void SpatialBinningModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Spatial binning"), rolloutParams, "particles.modifiers.bin_and_reduce.html");

    // Create the rollout contents.
	QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);

	// Input group.
	QGroupBox* inputBox = new QGroupBox(tr("Input property"), rollout);
	QVBoxLayout* sublayout = new QVBoxLayout(inputBox);
	sublayout->setContentsMargins(4,4,4,4);
	sublayout->setSpacing(2);
	layout->addWidget(inputBox);
	PropertyReferenceParameterUI* sourcePropertyUI = new PropertyReferenceParameterUI(this, PROPERTY_FIELD(SpatialBinningModifier::sourceProperty), nullptr);
	sublayout->addWidget(sourcePropertyUI->comboBox());
	connect(this, &PropertiesEditor::contentsChanged, this, [sourcePropertyUI](RefTarget* editObject) {
		SpatialBinningModifier* modifier = static_object_cast<SpatialBinningModifier>(editObject);
        if(modifier && modifier->delegate())
    		sourcePropertyUI->setContainerRef(modifier->delegate()->subject());
        else
    		sourcePropertyUI->setContainerRef({});
	});
	BooleanParameterUI* onlySelectedUI = new BooleanParameterUI(this, PROPERTY_FIELD(SpatialBinningModifier::onlySelectedElements));
	sublayout->addWidget(onlySelectedUI->checkBox());

    // Binning grid group
	QGroupBox* gridBox = new QGroupBox(tr("Binning grid"), rollout);
	sublayout = new QVBoxLayout(gridBox);
	sublayout->setContentsMargins(4,4,4,4);
	sublayout->setSpacing(4);
	layout->addWidget(gridBox);
    QHBoxLayout* sublayout2 = new QHBoxLayout();
	sublayout->addLayout(sublayout2);
	sublayout2->setContentsMargins(0,0,0,0);
	sublayout2->setSpacing(4);
	sublayout2->addWidget(new QLabel(tr("Binning direction(s):")), 0);
	VariantComboBoxParameterUI* binDirectionPUI = new VariantComboBoxParameterUI(this, PROPERTY_FIELD(SpatialBinningModifier::binDirection));
    binDirectionPUI->comboBox()->addItem("1D: X", QVariant::fromValue(SpatialBinningModifier::CELL_VECTOR_1));
    binDirectionPUI->comboBox()->addItem("1D: Y", QVariant::fromValue(SpatialBinningModifier::CELL_VECTOR_2));
    binDirectionPUI->comboBox()->addItem("1D: Z", QVariant::fromValue(SpatialBinningModifier::CELL_VECTOR_3));
    binDirectionPUI->comboBox()->addItem("2D: X-Y", QVariant::fromValue(SpatialBinningModifier::CELL_VECTORS_1_2));
    binDirectionPUI->comboBox()->addItem("2D: X-Z", QVariant::fromValue(SpatialBinningModifier::CELL_VECTORS_1_3));
    binDirectionPUI->comboBox()->addItem("2D: Y-Z", QVariant::fromValue(SpatialBinningModifier::CELL_VECTORS_2_3));
    binDirectionPUI->comboBox()->addItem("3D: X-Y-Z", QVariant::fromValue(SpatialBinningModifier::CELL_VECTORS_1_2_3));
    sublayout2->addWidget(binDirectionPUI->comboBox(), 1);
    sublayout2 = new QHBoxLayout();
	sublayout->addLayout(sublayout2);
	sublayout2->setContentsMargins(0,0,0,0);
	sublayout2->setSpacing(2);
	// Number of bins parameters.
	IntegerParameterUI* numBinsXPUI = new IntegerParameterUI(this, PROPERTY_FIELD(SpatialBinningModifier::numberOfBinsX));
	sublayout2->addWidget(numBinsXPUI->label(), 0);
	sublayout2->addLayout(numBinsXPUI->createFieldLayout(), 1);
	_numBinsYPUI = new IntegerParameterUI(this, PROPERTY_FIELD(SpatialBinningModifier::numberOfBinsY));
	sublayout2->addLayout(_numBinsYPUI->createFieldLayout(), 1);
	_numBinsYPUI->setEnabled(false);
	_numBinsZPUI = new IntegerParameterUI(this, PROPERTY_FIELD(SpatialBinningModifier::numberOfBinsZ));
	sublayout2->addLayout(_numBinsZPUI->createFieldLayout(), 1);
	_numBinsZPUI->setEnabled(false);

    // Reduction group.
	QGroupBox* reductionBox = new QGroupBox(tr("Reduction"), rollout);
	QGridLayout* gridlayout = new QGridLayout(reductionBox);
	gridlayout->setContentsMargins(4,4,4,4);
	gridlayout->setSpacing(2);
	layout->addWidget(reductionBox);
	gridlayout->addWidget(new QLabel(tr("Operation:"), rollout), 0, 0);
	VariantComboBoxParameterUI* reductionOperationPUI = new VariantComboBoxParameterUI(this, PROPERTY_FIELD(SpatialBinningModifier::reductionOperation));
    reductionOperationPUI->comboBox()->addItem(tr("mean"), QVariant::fromValue(SpatialBinningModifier::RED_MEAN));
    reductionOperationPUI->comboBox()->addItem(tr("sum"), QVariant::fromValue(SpatialBinningModifier::RED_SUM));
    reductionOperationPUI->comboBox()->addItem(tr("sum divided by bin volume"), QVariant::fromValue(SpatialBinningModifier::RED_SUM_VOL));
    reductionOperationPUI->comboBox()->addItem(tr("min"), QVariant::fromValue(SpatialBinningModifier::RED_MIN));
    reductionOperationPUI->comboBox()->addItem(tr("max"), QVariant::fromValue(SpatialBinningModifier::RED_MAX));
    gridlayout->addWidget(reductionOperationPUI->comboBox(), 0, 1);
    _firstDerivativePUI = new BooleanParameterUI(this, PROPERTY_FIELD(SpatialBinningModifier::firstDerivative));
	_firstDerivativePUI->setEnabled(false);
	gridlayout->addWidget(_firstDerivativePUI->checkBox(), 1, 0, 1, 2);

	_plotWidget1d = new DataSeriesPlotWidget();
	_plotWidget1d->setMinimumHeight(240);
	_plotWidget1d->setMaximumHeight(240);
    _mode3dLabel = new QwtPlotTextLabel();
    QwtText labelText(tr("Three-dimensional binning grids\nare not displayed in this plot area."));
    labelText.setRenderFlags(Qt::AlignHCenter | Qt::AlignVCenter);
    _mode3dLabel->setText(labelText);
    _mode3dLabel->attach(_plotWidget1d);
    _mode3dLabel->hide();

    _plotWidget2d = new QwtPlot();
    _plotWidget2d->hide();
	_plotWidget2d->setCanvasBackground(Qt::white);
	_plotWidget2d->setMinimumHeight(240);
	_plotWidget2d->setMaximumHeight(240);
    _plotRaster = new QwtPlotSpectrogram();
    _plotRaster->attach(_plotWidget2d);
    _rasterData = new QwtMatrixRasterData();
    _plotRaster->setData(_rasterData);
    _plotWidget2d->enableAxis(QwtPlot::yRight);
    _plotWidget2d->axisWidget(QwtPlot::yRight)->setColorBarEnabled(true);
    _plotWidget2d->axisWidget(QwtPlot::yRight)->setColorBarWidth(20);
    _plotWidget2d->plotLayout()->setAlignCanvasToScales(true);
    _plotWidget2d->axisScaleEngine(QwtPlot::xBottom)->setAttribute(QwtScaleEngine::Floating);

	layout->addSpacing(8);
	layout->addWidget(_plotWidget1d);
    layout->addWidget(_plotWidget2d);
	connect(this, &SpatialBinningModifierEditor::contentsReplaced, this, &SpatialBinningModifierEditor::plotData);

	// Axes.
	QGroupBox* axesBox = new QGroupBox(tr("Plot axes"), rollout);
	QVBoxLayout* axesSublayout = new QVBoxLayout(axesBox);
	axesSublayout->setContentsMargins(4,4,4,4);
	layout->addWidget(axesBox);
    BooleanParameterUI* rangeUI = new BooleanParameterUI(this, PROPERTY_FIELD(SpatialBinningModifier::fixPropertyAxisRange));
    axesSublayout->addWidget(rangeUI->checkBox());

    QHBoxLayout* hlayout = new QHBoxLayout();
    axesSublayout->addLayout(hlayout);
    FloatParameterUI* startPUI = new FloatParameterUI(this, PROPERTY_FIELD(SpatialBinningModifier::propertyAxisRangeStart));
    FloatParameterUI* endPUI = new FloatParameterUI(this, PROPERTY_FIELD(SpatialBinningModifier::propertyAxisRangeEnd));
    hlayout->addWidget(new QLabel(tr("From:")));
    hlayout->addLayout(startPUI->createFieldLayout());
    hlayout->addSpacing(12);
    hlayout->addWidget(new QLabel(tr("To:")));
    hlayout->addLayout(endPUI->createFieldLayout());
    startPUI->setEnabled(false);
    endPUI->setEnabled(false);
    connect(rangeUI->checkBox(), &QCheckBox::toggled, startPUI, &FloatParameterUI::setEnabled);
    connect(rangeUI->checkBox(), &QCheckBox::toggled, endPUI, &FloatParameterUI::setEnabled);

	// Status label.
	layout->addSpacing(6);
	layout->addWidget(statusLabel());

	connect(this, &SpatialBinningModifierEditor::contentsChanged, this, &SpatialBinningModifierEditor::updateWidgets);

	// Update data plot whenever the modifier has calculated new results.
	connect(this, &ModifierPropertiesEditor::modifierEvaluated, this, [this]() {
		plotLater(this);
	});
}

/******************************************************************************
* Plots the data computed by the modifier.
******************************************************************************/
void SpatialBinningModifierEditor::plotData()
{
    SpatialBinningModifier* modifier = static_object_cast<SpatialBinningModifier>(editObject());
    if(modifierApplication()) {
        // Request the modifier's pipeline output.
        const PipelineFlowState& state = getModifierOutput();

        // Look up the data series in the modifier's pipeline output.
        QString seriesName = QStringLiteral("binning[%1]").arg(modifier->sourceProperty().nameWithComponent());
        const DataSeriesObject* series = state.getObjectBy<DataSeriesObject>(modifierApplication(), seriesName);

        // Hand plot data over to plotting widget.
        if(series) {

            if(modifier->fixPropertyAxisRange()) {
                _plotWidget1d->setAxisScale(QwtPlot::yLeft, modifier->propertyAxisRangeStart(), modifier->propertyAxisRangeEnd());
            }
            else {
                _plotWidget1d->setAxisAutoScale(QwtPlot::yLeft);
            }

            _mode3dLabel->hide();
            _plotWidget2d->hide();
            _plotWidget1d->setSeries(series);
            _plotWidget1d->show();
            return;
        }
        else {

            // It's not a 1d plot. Check if the modifier has generated a two-dimensional voxel grid.
            const VoxelGrid* grid = state.getObjectBy<VoxelGrid>(modifierApplication(), seriesName);
            if(grid && grid->domain()->is2D() && grid->properties().size() == 1) {
                _plotWidget1d->hide();
                _plotWidget2d->show();
                const PropertyObject* property = grid->properties().front();
                QVector<double> values(property->size());
                if(property->storage()->copyTo(values.begin())) {
                    QwtInterval zInterval;
                    if(!modifier->fixPropertyAxisRange()) {
                        auto minmax = std::minmax_element(values.cbegin(), values.cend());
                        if(*minmax.first != *minmax.second)
                            zInterval = QwtInterval(*minmax.first, *minmax.second);
                        else
                            zInterval = QwtInterval(*minmax.first, *minmax.second + 1.0);
                    }
                    else {
                        zInterval = QwtInterval(modifier->propertyAxisRangeStart(), modifier->propertyAxisRangeEnd());
                    }
                    _rasterData->setValueMatrix(values, grid->shape()[0]);
                    _rasterData->setInterval(Qt::XAxis, QwtInterval(0, grid->domain()->cellVector1().length()));
                    _rasterData->setInterval(Qt::YAxis, QwtInterval(0, grid->domain()->cellVector2().length()));
                    _rasterData->setInterval(Qt::ZAxis, zInterval.normalized());
                    class ColorMap: public QwtLinearColorMap
                    {
                    public:
                        ColorMap() : QwtLinearColorMap(Qt::darkBlue, Qt::darkRed) {
                            addColorStop(0.2, Qt::blue);
                            addColorStop(0.4, Qt::cyan);
                            addColorStop(0.6, Qt::yellow);
                            addColorStop(0.8, Qt::red);
                        }
                    };
                    _plotRaster->show();
                    _plotWidget2d->axisScaleEngine(QwtPlot::yRight)->setAttribute(QwtScaleEngine::Inverted, zInterval.minValue() > zInterval.maxValue());
                    _plotWidget2d->setAxisScale(QwtPlot::xBottom, 0, grid->domain()->cellVector1().length());
                    _plotWidget2d->setAxisScale(QwtPlot::yLeft, 0, grid->domain()->cellVector2().length());
                    _plotWidget2d->axisWidget(QwtPlot::yRight)->setColorMap(zInterval.normalized(), new ColorMap());
                    _plotRaster->setColorMap(new ColorMap());
                    _plotWidget2d->setAxisScale(QwtPlot::yRight, zInterval.minValue(), zInterval.maxValue());
                    _plotWidget2d->setAxisTitle(QwtPlot::yRight, property->name());
                    _plotWidget2d->setAxisTitle(QwtPlot::xBottom, tr("Position along axis 1"));
                    _plotWidget2d->setAxisTitle(QwtPlot::yLeft, tr("Position along axis 2"));
                    _plotWidget2d->replot();
                    return;
                }
            }
            else {
                _mode3dLabel->show();
            }
        }
    }

    // Reset plot widgets.
    _plotWidget2d->hide();
    _plotWidget1d->reset();
    _plotWidget1d->show();
}

/******************************************************************************
* Enable/disable the editor for number of y-bins and the first derivative
* button
******************************************************************************/
void SpatialBinningModifierEditor::updateWidgets()
{
	SpatialBinningModifier* modifier = static_object_cast<SpatialBinningModifier>(editObject());
    _numBinsYPUI->setEnabled(modifier && !modifier->is1D());
    _numBinsZPUI->setEnabled(modifier && modifier->is3D());
    _firstDerivativePUI->setEnabled(modifier && modifier->is1D());
}

#if 0
/******************************************************************************
* This is called when the user has clicked the "Save Data" button.
******************************************************************************/
void SpatialBinningModifierEditor::onSaveData()
{
	SpatialBinningModifier* modifier = static_object_cast<SpatialBinningModifier>(editObject());
	BinningModifierApplication* modApp = dynamic_object_cast<BinningModifierApplication>(modifierApplication());
	if(!modifier || !modApp)
		return;

	QString fileName = QFileDialog::getSaveFileName(mainWindow(),
	    tr("Save Data"), QString(),
        tr("Text files (*.txt);;All files (*)"));
	if(fileName.isEmpty())
		return;

	try {

		QFile file(fileName);
		if(!file.open(QIODevice::WriteOnly | QIODevice::Text))
			throwException(tr("Could not open file for writing: %1").arg(file.errorString()));

		int binDataSizeX = std::max(1, modifier->numberOfBinsX());
		int binDataSizeY = std::max(1, modifier->numberOfBinsY());
        if(modifier->is1D()) binDataSizeY = 1;
		FloatType binSizeX = (modApp->range1().second - modApp->range1().first) / binDataSizeX;
		FloatType binSizeY = (modApp->range2().second - modApp->range2().first) / binDataSizeY;

        if(!modifier->isEnabled() || !modApp->binData() || modApp->binData()->size() != (size_t)binDataSizeX * (size_t)binDataSizeY)
			throwException(tr("Modifier has not been evaluated as part of the data pipeline yet."));

		QTextStream stream(&file);
        if(binDataSizeY == 1) {
            stream << "# " << modifier->sourceProperty().nameWithComponent() << ", bin size: " << binSizeX << ", bin count: " << binDataSizeX << endl;
			for(int i = 0; i < binDataSizeX; i++) {
                stream << (binSizeX * (FloatType(i) + FloatType(0.5)) + modApp->range1().first) << " " << modApp->binData()->getFloat(i) << "\n";
            }
        }
        else {
            stream << "# " << modifier->sourceProperty().nameWithComponent() << ", bin size X: " << binDataSizeX << ", bin count X: " << binDataSizeX << ", bin size Y: " << binDataSizeY << ", bin count Y: " << binDataSizeY << endl;
            auto d = modApp->binData()->constDataFloat();
            for(int i = 0; i < binDataSizeY; i++) {
                for(int j = 0; j < binDataSizeX; j++) {
                    stream << *d++ << " ";
                }
                stream << "\n";
            }
        }
	}
	catch(const Exception& ex) {
		ex.reportError();
	}
}
#endif

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
