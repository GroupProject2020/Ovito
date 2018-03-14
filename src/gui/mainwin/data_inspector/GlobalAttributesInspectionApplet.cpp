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
#include <core/dataset/pipeline/PipelineFlowState.h>
#include "GlobalAttributesInspectionApplet.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui)

IMPLEMENT_OVITO_CLASS(GlobalAttributesInspectionApplet);

/******************************************************************************
* Determines whether the given pipeline flow state contains data that can be 
* displayed by this applet.
******************************************************************************/
bool GlobalAttributesInspectionApplet::appliesTo(const PipelineFlowState& state)
{
	return state.attributes().empty() == false;
}

/******************************************************************************
* Lets the applet create the UI widget that is to be placed into the data 
* inspector panel. 
******************************************************************************/
QWidget* GlobalAttributesInspectionApplet::createWidget(MainWindow* mainWindow)
{
	_tableView = new QTableView();	
	_tableModel = new AttributeTableModel(_tableView);
	_tableView->setModel(_tableModel);
	_tableView->setWordWrap(false);
	_tableView->verticalHeader()->hide();
	_tableView->horizontalHeader()->resizeSection(0, 180);
	_tableView->horizontalHeader()->setStretchLastSection(true);
	return _tableView;
}

/******************************************************************************
* Updates the contents displayed in the inspector.
******************************************************************************/
void GlobalAttributesInspectionApplet::updateDisplay(const PipelineFlowState& state, PipelineSceneNode* sceneNode)
{
	_tableModel->setContents(state);
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
