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

#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include <plugins/crystalanalysis/objects/dislocations/DislocationNetworkObject.h>
#include <core/dataset/pipeline/PipelineFlowState.h>
#include "DislocationInspectionApplet.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

IMPLEMENT_OVITO_CLASS(DislocationInspectionApplet);

/******************************************************************************
* Determines whether the given pipeline flow state contains data that can be 
* displayed by this applet.
******************************************************************************/
bool DislocationInspectionApplet::appliesTo(const PipelineFlowState& state)
{
	return state.findObject<DislocationNetworkObject>() != nullptr;
}

/******************************************************************************
* Lets the applet create the UI widget that is to be placed into the data 
* inspector panel. 
******************************************************************************/
QWidget* DislocationInspectionApplet::createWidget(MainWindow* mainWindow)
{
	_tableView = new QTableView();
	_tableModel = new DislocationTableModel(_tableView);
	_tableView->setModel(_tableModel);
	_tableView->horizontalHeader()->resizeSection(0, 60);
	_tableView->horizontalHeader()->resizeSection(1, 140);
	_tableView->horizontalHeader()->resizeSection(2, 200);
	_tableView->horizontalHeader()->resizeSection(4, 60);
	_tableView->verticalHeader()->hide();
	return _tableView;
}

/******************************************************************************
* Updates the contents displayed in the inspector.
******************************************************************************/
void DislocationInspectionApplet::updateDisplay(const PipelineFlowState& state, PipelineSceneNode* sceneNode)
{
	_tableModel->setContents(state);
}

}	// End of namespace
}	// End of namespace
}	// End of namespace
