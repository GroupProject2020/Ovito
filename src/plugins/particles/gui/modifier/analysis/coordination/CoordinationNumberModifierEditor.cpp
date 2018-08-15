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

#include <plugins/particles/gui/ParticlesGui.h>
#include <plugins/particles/modifier/analysis/coordination/CoordinationNumberModifier.h>
#include <gui/mainwin/MainWindow.h>
#include <gui/properties/IntegerParameterUI.h>
#include <gui/properties/FloatParameterUI.h>
#include <gui/properties/BooleanParameterUI.h>
#include "CoordinationNumberModifierEditor.h"

#include <qwt/qwt_plot.h>
#include <qwt/qwt_plot_curve.h>
#include <qwt/qwt_plot_grid.h>
#include <qwt/qwt_plot_legenditem.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_CLASS(CoordinationNumberModifierEditor);
SET_OVITO_OBJECT_EDITOR(CoordinationNumberModifier, CoordinationNumberModifierEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void CoordinationNumberModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Coordination analysis"), rolloutParams, "particles.modifiers.coordination_analysis.html");

    // Create the rollout contents.
	QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);

	QGridLayout* gridlayout = new QGridLayout();
	gridlayout->setContentsMargins(4,4,4,4);
	gridlayout->setColumnStretch(1, 1);

	// Cutoff parameter.
	FloatParameterUI* cutoffRadiusPUI = new FloatParameterUI(this, PROPERTY_FIELD(CoordinationNumberModifier::cutoff));
	gridlayout->addWidget(cutoffRadiusPUI->label(), 0, 0);
	gridlayout->addLayout(cutoffRadiusPUI->createFieldLayout(), 0, 1);

	// Number of bins parameter.
	IntegerParameterUI* numBinsPUI = new IntegerParameterUI(this, PROPERTY_FIELD(CoordinationNumberModifier::numberOfBins));
	gridlayout->addWidget(numBinsPUI->label(), 1, 0);
	gridlayout->addLayout(numBinsPUI->createFieldLayout(), 1, 1);
	layout->addLayout(gridlayout);

	// Partial RDFs option.
	BooleanParameterUI* partialRdfPUI = new BooleanParameterUI(this, PROPERTY_FIELD(CoordinationNumberModifier::computePartialRDF));
	layout->addWidget(partialRdfPUI->checkBox());

	_rdfPlot = new QwtPlot();
	_rdfPlot->setMinimumHeight(200);
	_rdfPlot->setMaximumHeight(200);
	_rdfPlot->setCanvasBackground(Qt::white);
	_rdfPlot->setAxisTitle(QwtPlot::xBottom, tr("Pair separation distance"));
	_rdfPlot->setAxisTitle(QwtPlot::yLeft, tr("g(r)"));
	QwtPlotGrid* plotGrid = new QwtPlotGrid();
	plotGrid->setPen(Qt::gray, 0, Qt::DotLine);
	plotGrid->attach(_rdfPlot);

	layout->addSpacing(12);
	layout->addWidget(new QLabel(tr("Radial distribution function:")));
	layout->addWidget(_rdfPlot);
	connect(this, &CoordinationNumberModifierEditor::contentsReplaced, this, &CoordinationNumberModifierEditor::plotRDF);

	layout->addSpacing(12);
	QPushButton* saveDataButton = new QPushButton(tr("Export data to text file"));
	layout->addWidget(saveDataButton);
	connect(saveDataButton, &QPushButton::clicked, this, &CoordinationNumberModifierEditor::onSaveData);

	// Status label.
	layout->addSpacing(6);
	layout->addWidget(statusLabel());
}

/******************************************************************************
* This method is called when a reference target changes.
******************************************************************************/
bool CoordinationNumberModifierEditor::referenceEvent(RefTarget* source, const ReferenceEvent& event)
{
	if(event.sender() == editObject() && (event.type() == ReferenceEvent::ObjectStatusChanged || event.type() == ReferenceEvent::TargetChanged)) {
		plotRDFLater(this);
	}
	return ModifierPropertiesEditor::referenceEvent(source, event);
}

/******************************************************************************
* Updates the plot of the RDF computed by the modifier.
******************************************************************************/
void CoordinationNumberModifierEditor::plotRDF()
{
	CoordinationNumberModifier* modifier = static_object_cast<CoordinationNumberModifier>(editObject());
	CoordinationNumberModifierApplication* modApp = dynamic_object_cast<CoordinationNumberModifierApplication>(someModifierApplication());
	if(!modApp || !modifier || !modApp->rdf() || modApp->rdf()->x()->size() != modApp->rdf()->y()->size())
		return;

	static const Qt::GlobalColor curveColors[] = {
		Qt::black, Qt::red, Qt::blue, Qt::green,
		Qt::cyan, Qt::magenta, Qt::gray, Qt::darkRed, 
		Qt::darkGreen, Qt::darkBlue, Qt::darkCyan, Qt::darkMagenta,
		Qt::darkYellow, Qt::darkGray
	};
	const auto& rdfX = modApp->rdf()->x();
	const auto& rdfY = modApp->rdf()->y();

	// Create plot curve object.
	while(_plotCurves.size() < rdfY->componentCount()) {
		QwtPlotCurve* curve = new QwtPlotCurve();
	    curve->setRenderHint(QwtPlotItem::RenderAntialiased, true);
		curve->attach(_rdfPlot);
		curve->setPen(QPen(curveColors[_plotCurves.size() % (sizeof(curveColors)/sizeof(curveColors[0]))], 1));
		_plotCurves.push_back(curve);
	}
	while(_plotCurves.size() > rdfY->componentCount()) {
		delete _plotCurves.back();
		_plotCurves.pop_back();
	}

	// Configure plot curve style.
	if(_plotCurves.size() == 1 && rdfY->componentNames().empty()) {
		_plotCurves[0]->setBrush(Qt::lightGray);
		if(_legendItem) {
			delete _legendItem;
			_legendItem = nullptr;
		}
	}
	else {
		for(QwtPlotCurve* curve : _plotCurves)
			curve->setBrush({});
		if(!_legendItem) {
			_legendItem = new QwtPlotLegendItem();
			_legendItem->setAlignment(Qt::AlignRight | Qt::AlignTop);
			_legendItem->attach(_rdfPlot);
		}
	}

	// Plotting X range:
	double maxx = modifier->cutoff();
	double minx = maxx;

	for(size_t cmpnt = 0; cmpnt < _plotCurves.size(); cmpnt++) {
		QVector<double> x(rdfX->size());
		QVector<double> y(rdfY->size());
		for(size_t j = 0; j < x.size(); j++) {
			x[j] = rdfX->getFloat(j);
			y[j] = rdfY->getFloatComponent(j, cmpnt);
		}
		_plotCurves[cmpnt]->setSamples(x, y);
		if(cmpnt < rdfY->componentNames().size())
			_plotCurves[cmpnt]->setTitle(rdfY->componentNames()[cmpnt]);

		// Determine lower X coordinate at which the RDF histogram becomes non-zero.
		auto ycoord = y.cbegin();
		for(auto xcoord : x) {
			if(*ycoord++ != 0) {
				minx = std::min(minx, xcoord);
				break;
			}
		}
	}
	// Set plotting range.
	if(minx < maxx)
		_rdfPlot->setAxisScale(QwtPlot::xBottom, std::floor(minx * 9.0 / maxx) / 10.0 * maxx, maxx);
	else
		_rdfPlot->setAxisAutoScale(QwtPlot::xBottom);

	_rdfPlot->replot();
}

/******************************************************************************
* This is called when the user has clicked the "Save Data" button.
******************************************************************************/
void CoordinationNumberModifierEditor::onSaveData()
{
	OORef<CoordinationNumberModifierApplication> modApp = dynamic_object_cast<CoordinationNumberModifierApplication>(someModifierApplication());
	if(!modApp)
		return;

	QString fileName = QFileDialog::getSaveFileName(mainWindow(),
	    tr("Save RDF Data"), QString(), tr("Text files (*.txt);;All files (*)"));
	if(fileName.isEmpty())
		return;

	try {
		if(!modApp->rdf())
			modApp->throwException(tr("RDF has not been computed yet"));
		const auto& rdfX = modApp->rdf()->x();
		const auto& rdfY = modApp->rdf()->y();

		QFile file(fileName);
		if(!file.open(QIODevice::WriteOnly | QIODevice::Text))
			modApp->throwException(tr("Could not open file for writing: %1").arg(file.errorString()));

		QTextStream stream(&file);

		stream << "# bin r";
		if(rdfY->componentNames().empty())
			stream << " g(r)";
		else {
			for(const QString& name : rdfY->componentNames())
				stream << " g[" << name << "](r)";
		}
		stream << endl;
		for(size_t i = 0; i < rdfX->size(); i++) {
			stream << i << "\t" << rdfX->getFloat(i);
			for(size_t j = 0; j < rdfY->componentCount(); j++)
				stream << "\t" << rdfY->getFloatComponent(i, j);
			stream << endl;
		}
	}
	catch(const Exception& ex) {
		ex.reportError();
	}
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
