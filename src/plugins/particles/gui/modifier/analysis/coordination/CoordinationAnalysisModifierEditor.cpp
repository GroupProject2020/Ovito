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
#include <plugins/particles/modifier/analysis/coordination/CoordinationAnalysisModifier.h>
#include <gui/mainwin/MainWindow.h>
#include <gui/properties/IntegerParameterUI.h>
#include <gui/properties/FloatParameterUI.h>
#include <gui/properties/BooleanParameterUI.h>
#include "CoordinationAnalysisModifierEditor.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_CLASS(CoordinationAnalysisModifierEditor);
SET_OVITO_OBJECT_EDITOR(CoordinationAnalysisModifier, CoordinationAnalysisModifierEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void CoordinationAnalysisModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
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
	FloatParameterUI* cutoffRadiusPUI = new FloatParameterUI(this, PROPERTY_FIELD(CoordinationAnalysisModifier::cutoff));
	gridlayout->addWidget(cutoffRadiusPUI->label(), 0, 0);
	gridlayout->addLayout(cutoffRadiusPUI->createFieldLayout(), 0, 1);

	// Number of bins parameter.
	IntegerParameterUI* numBinsPUI = new IntegerParameterUI(this, PROPERTY_FIELD(CoordinationAnalysisModifier::numberOfBins));
	gridlayout->addWidget(numBinsPUI->label(), 1, 0);
	gridlayout->addLayout(numBinsPUI->createFieldLayout(), 1, 1);
	layout->addLayout(gridlayout);

	// Partial RDFs option.
	BooleanParameterUI* partialRdfPUI = new BooleanParameterUI(this, PROPERTY_FIELD(CoordinationAnalysisModifier::computePartialRDF));
	layout->addWidget(partialRdfPUI->checkBox());

	_rdfPlot = new DataSeriesPlotWidget();
	_rdfPlot->setMinimumHeight(200);
	_rdfPlot->setMaximumHeight(200);

	layout->addSpacing(12);
	layout->addWidget(new QLabel(tr("Radial distribution function:")));
	layout->addWidget(_rdfPlot);
	connect(this, &CoordinationAnalysisModifierEditor::contentsReplaced, this, &CoordinationAnalysisModifierEditor::plotRDF);

	layout->addSpacing(12);
	QPushButton* saveDataButton = new QPushButton(tr("Export data to text file"));
	layout->addWidget(saveDataButton);
	connect(saveDataButton, &QPushButton::clicked, this, &CoordinationAnalysisModifierEditor::onSaveData);

	// Status label.
	layout->addSpacing(6);
	layout->addWidget(statusLabel());
}

/******************************************************************************
* This method is called when a reference target changes.
******************************************************************************/
bool CoordinationAnalysisModifierEditor::referenceEvent(RefTarget* source, const ReferenceEvent& event)
{
	if(source == modifierApplication() && event.type() == ReferenceEvent::ObjectStatusChanged) {
		plotRDFLater(this);
	}
	return ModifierPropertiesEditor::referenceEvent(source, event);
}

/******************************************************************************
* Updates the plot of the RDF computed by the modifier.
******************************************************************************/
void CoordinationAnalysisModifierEditor::plotRDF()
{
	DataSeriesObject* series = nullptr;
	if(CoordinationAnalysisModifierApplication* modApp = dynamic_object_cast<CoordinationAnalysisModifierApplication>(modifierApplication()))
		series = modApp->rdf();

	// Determine X plotting range.
	if(series) {
		const auto& rdfY = series->y();
		double minX = 0;
		for(size_t i = 0; i < rdfY->size(); i++) {
			for(size_t cmpnt = 0; cmpnt < rdfY->componentCount(); cmpnt++) {
				if(rdfY->getFloatComponent(i, cmpnt) != 0) {
					minX = series->getXValue(i);
					break;
				}
			}
			if(minX) break;
		}
		_rdfPlot->setAxisScale(QwtPlot::xBottom, std::floor(minX * 9.0 / series->intervalEnd()) / 10.0 * series->intervalEnd(), series->intervalEnd());
	}
	_rdfPlot->setSeries(series);
}

/******************************************************************************
* This is called when the user has clicked the "Save Data" button.
******************************************************************************/
void CoordinationAnalysisModifierEditor::onSaveData()
{
	OORef<CoordinationAnalysisModifierApplication> modApp = dynamic_object_cast<CoordinationAnalysisModifierApplication>(modifierApplication());
	if(!modApp)
		return;

	QString fileName = QFileDialog::getSaveFileName(mainWindow(),
	    tr("Save RDF Data"), QString(), tr("Text files (*.txt);;All files (*)"));
	if(fileName.isEmpty())
		return;

	try {
		if(!modApp->rdf())
			modApp->throwException(tr("RDF has not been computed yet"));
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
		for(size_t i = 0; i < rdfY->size(); i++) {
			stream << i << "\t" << modApp->rdf()->getXValue(i);
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
