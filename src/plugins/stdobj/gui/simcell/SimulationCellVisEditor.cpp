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

#include <plugins/stdobj/gui/StdObjGui.h>
#include <plugins/stdobj/simcell/SimulationCellVis.h>
#include <gui/properties/FloatParameterUI.h>
#include <gui/properties/ColorParameterUI.h>
#include <gui/properties/BooleanParameterUI.h>
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
	QWidget* rollout = createRollout(QString(), rolloutParams, "display_objects.simulation_cell.html");

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
