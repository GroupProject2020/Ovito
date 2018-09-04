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

#include <plugins/stdobj/gui/StdObjGui.h>
#include <gui/mainwin/MainWindow.h>
#include "SeriesInspectionApplet.h"

namespace Ovito { namespace StdObj {

IMPLEMENT_OVITO_CLASS(SeriesInspectionApplet);

/******************************************************************************
* Lets the applet create the UI widget that is to be placed into the data 
* inspector panel. 
******************************************************************************/
QWidget* SeriesInspectionApplet::createWidget(MainWindow* mainWindow)
{
	createBaseWidgets();
	
	QSplitter* splitter = new QSplitter();
	splitter->addWidget(containerSelectionWidget());

	QStackedWidget* stackedWidget = new QStackedWidget();
	splitter->addWidget(stackedWidget);
	splitter->setStretchFactor(0, 1);
	splitter->setStretchFactor(1, 4);

	_plotWidget = new DataSeriesPlotWidget();
	stackedWidget->addWidget(_plotWidget);
	stackedWidget->addWidget(tableView());

	return splitter;
}

/******************************************************************************
* Is called when the user selects a different container object from the list.
******************************************************************************/
void SeriesInspectionApplet::currentContainerChanged()
{
	PropertyInspectionApplet::currentContainerChanged();

	// Update the displayed plot.
	plotWidget()->setSeries(static_object_cast<DataSeriesObject>(selectedContainerObject()));
}

}	// End of namespace
}	// End of namespace
