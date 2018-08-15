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
* Determines whether the given pipeline flow state contains data that can be 
* displayed by this applet.
******************************************************************************/
bool SeriesInspectionApplet::appliesTo(const PipelineFlowState& state)
{
	return state.findObject<DataSeriesObject>() != nullptr;
}

/******************************************************************************
* Lets the applet create the UI widget that is to be placed into the data 
* inspector panel. 
******************************************************************************/
QWidget* SeriesInspectionApplet::createWidget(MainWindow* mainWindow)
{
	QSplitter* splitter = new QSplitter();
	_seriesSelectionWidget = new QListWidget();
	splitter->addWidget(_seriesSelectionWidget);
	_plotWidget = new DataSeriesPlotWidget();
	splitter->addWidget(_plotWidget);
	splitter->setStretchFactor(0, 1);
	splitter->setStretchFactor(1, 4);
	connect(_seriesSelectionWidget, &QListWidget::currentItemChanged, this, &SeriesInspectionApplet::currentPlotChanged);
	return splitter;
}

/******************************************************************************
* Updates the contents displayed in the inspector.
******************************************************************************/
void SeriesInspectionApplet::updateDisplay(const PipelineFlowState& state, PipelineSceneNode* sceneNode)
{
	// Remember which series was previously selected.
	QString selectedSeriesTitle;
	if(seriesSelectionWidget()->currentItem())
		selectedSeriesTitle = seriesSelectionWidget()->currentItem()->text();

	// Rebuild list of data series.
	seriesSelectionWidget()->clear();
	for(DataObject* obj : state.objects()) {
		if(DataSeriesObject* seriesObj = dynamic_object_cast<DataSeriesObject>(obj)) {
			QListWidgetItem* item = new QListWidgetItem(seriesObj->title(), seriesSelectionWidget());
			item->setData(Qt::UserRole, QVariant::fromValue<OORef<OvitoObject>>(seriesObj));

			// Select again the previously selected series.
			if(item->text() == selectedSeriesTitle)
				seriesSelectionWidget()->setCurrentItem(item);
		}
	}

	if(!seriesSelectionWidget()->currentItem() && seriesSelectionWidget()->count() != 0)
		seriesSelectionWidget()->setCurrentRow(0);
}

/******************************************************************************
* Is called when the user selects a different plot item in the list.
******************************************************************************/
void SeriesInspectionApplet::currentPlotChanged(QListWidgetItem* current, QListWidgetItem* previous)
{
	DataSeriesObject* seriesObj = nullptr;
	if(current)
		seriesObj = static_object_cast<DataSeriesObject>(current->data(Qt::UserRole).value<OORef<OvitoObject>>());
	_plotWidget->setSeries(seriesObj);
}

}	// End of namespace
}	// End of namespace
