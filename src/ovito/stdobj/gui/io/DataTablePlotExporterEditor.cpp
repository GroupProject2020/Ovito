////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2020 Alexander Stukowski
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

#include <ovito/stdobj/gui/StdObjGui.h>
#include <ovito/stdobj/gui/io/DataTablePlotExporter.h>
#include <ovito/gui/properties/FloatParameterUI.h>
#include <ovito/gui/properties/IntegerParameterUI.h>
#include "DataTablePlotExporterEditor.h"

namespace Ovito { namespace StdObj {

IMPLEMENT_OVITO_CLASS(DataTablePlotExporterEditor);
SET_OVITO_OBJECT_EDITOR(DataTablePlotExporter, DataTablePlotExporterEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void DataTablePlotExporterEditor::createUI(const RolloutInsertionParameters& rolloutParams)
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

	FloatParameterUI* plotWidthUI = new FloatParameterUI(this, PROPERTY_FIELD(DataTablePlotExporter::plotWidth));
	layout->addWidget(plotWidthUI->label(), 0, 0);
	layout->addLayout(plotWidthUI->createFieldLayout(), 0, 1);

	FloatParameterUI* plotHeightUI = new FloatParameterUI(this, PROPERTY_FIELD(DataTablePlotExporter::plotHeight));
	layout->addWidget(plotHeightUI->label(), 1, 0);
	layout->addLayout(plotHeightUI->createFieldLayout(), 1, 1);

	IntegerParameterUI* dpiUI = new IntegerParameterUI(this, PROPERTY_FIELD(DataTablePlotExporter::plotDPI));
	layout->addWidget(dpiUI->label(), 0, 3);
	layout->addLayout(dpiUI->createFieldLayout(), 0, 4);
}

}	// End of namespace
}	// End of namespace
