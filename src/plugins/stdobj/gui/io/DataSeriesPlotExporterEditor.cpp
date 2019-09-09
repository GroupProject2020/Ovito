///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2019) Alexander Stukowski
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

#include <plugins/stdobj/gui/StdObjGui.h>
#include <plugins/stdobj/gui/io/DataSeriesPlotExporter.h>
#include <gui/properties/FloatParameterUI.h>
#include <gui/properties/IntegerParameterUI.h>
#include "DataSeriesPlotExporterEditor.h"

namespace Ovito { namespace StdObj {

IMPLEMENT_OVITO_CLASS(DataSeriesPlotExporterEditor);
SET_OVITO_OBJECT_EDITOR(DataSeriesPlotExporter, DataSeriesPlotExporterEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void DataSeriesPlotExporterEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Plot options"), rolloutParams);

    // Create the rollout contents.
	QGridLayout* layout = new QGridLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);
	layout->setColumnStretch(1,1);
	layout->setColumnStretch(4,1);
	layout->setColumnMinimumWidth(2,10);

	FloatParameterUI* plotWidthUI = new FloatParameterUI(this, PROPERTY_FIELD(DataSeriesPlotExporter::plotWidth));
	layout->addWidget(plotWidthUI->label(), 0, 0);
	layout->addLayout(plotWidthUI->createFieldLayout(), 0, 1);

	FloatParameterUI* plotHeightUI = new FloatParameterUI(this, PROPERTY_FIELD(DataSeriesPlotExporter::plotHeight));
	layout->addWidget(plotHeightUI->label(), 1, 0);
	layout->addLayout(plotHeightUI->createFieldLayout(), 1, 1);

	IntegerParameterUI* dpiUI = new IntegerParameterUI(this, PROPERTY_FIELD(DataSeriesPlotExporter::plotDPI));
	layout->addWidget(dpiUI->label(), 0, 3);
	layout->addLayout(dpiUI->createFieldLayout(), 0, 4);
}

}	// End of namespace
}	// End of namespace
