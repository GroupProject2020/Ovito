////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2016 Alexander Stukowski
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
#include <ovito/stdobj/simcell/SimulationCellVis.h>
#include <ovito/gui/properties/FloatParameterUI.h>
#include <ovito/gui/properties/ColorParameterUI.h>
#include <ovito/gui/properties/BooleanParameterUI.h>
#include "SimulationCellVisEditor.h"

namespace Ovito { namespace StdObj {

IMPLEMENT_OVITO_CLASS(SimulationCellVisEditor);
SET_OVITO_OBJECT_EDITOR(SimulationCellVis, SimulationCellVisEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void SimulationCellVisEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(QString(), rolloutParams, "visual_elements.simulation_cell.html");

    // Create the rollout contents.
	QGridLayout* layout = new QGridLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);
	layout->setColumnStretch(1, 1);

	// Render cell
	BooleanParameterUI* renderCellUI = new BooleanParameterUI(this, PROPERTY_FIELD(SimulationCellVis::renderCellEnabled));
	layout->addWidget(renderCellUI->checkBox(), 0, 0, 1, 2);

	// Line width
	FloatParameterUI* lineWidthUI = new FloatParameterUI(this, PROPERTY_FIELD(SimulationCellVis::cellLineWidth));
	layout->addWidget(lineWidthUI->label(), 1, 0);
	layout->addLayout(lineWidthUI->createFieldLayout(), 1, 1);

	// Line color
	ColorParameterUI* lineColorUI = new ColorParameterUI(this, PROPERTY_FIELD(SimulationCellVis::cellColor));
	layout->addWidget(lineColorUI->label(), 2, 0);
	layout->addWidget(lineColorUI->colorPicker(), 2, 1);
}

}	// End of namespace
}	// End of namespace
