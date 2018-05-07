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
#include "PlotInspectionApplet.h"

#include <qwt/qwt_plot.h>
#include <qwt/qwt_plot_curve.h>
#include <qwt/qwt_plot_grid.h>

namespace Ovito { namespace StdObj {

IMPLEMENT_OVITO_CLASS(PlotInspectionApplet);

/******************************************************************************
* Determines whether the given pipeline flow state contains data that can be 
* displayed by this applet.
******************************************************************************/
bool PlotInspectionApplet::appliesTo(const PipelineFlowState& state)
{
	return state.findObject<PlotObject>() != nullptr;
}

/******************************************************************************
* Lets the applet create the UI widget that is to be placed into the data 
* inspector panel. 
******************************************************************************/
QWidget* PlotInspectionApplet::createWidget(MainWindow* mainWindow)
{
	QSplitter* splitter = new QSplitter();
	_plotSelectionWidget = new QListWidget();
	splitter->addWidget(_plotSelectionWidget);
	_plotWidget = new QwtPlot();
	_plotWidget->setCanvasBackground(Qt::white);
	QwtPlotGrid* plotGrid = new QwtPlotGrid();
	plotGrid->setPen(Qt::gray, 0, Qt::DotLine);
	plotGrid->attach(_plotWidget);
	splitter->addWidget(_plotWidget);
	splitter->setStretchFactor(0, 1);
	splitter->setStretchFactor(1, 4);
	connect(_plotSelectionWidget, &QListWidget::currentItemChanged, this, &PlotInspectionApplet::currentPlotChanged);
	return splitter;
}

/******************************************************************************
* Updates the contents displayed in the inspector.
******************************************************************************/
void PlotInspectionApplet::updateDisplay(const PipelineFlowState& state, PipelineSceneNode* sceneNode)
{
	// Remember which plot was previously selected.
	QString selectedPlotTitle;
	if(plotSelectionWidget()->currentItem())
		selectedPlotTitle = plotSelectionWidget()->currentItem()->text();

	// Rebuild list of plots.
	plotSelectionWidget()->clear();
	for(DataObject* obj : state.objects()) {
		if(PlotObject* plotObj = dynamic_object_cast<PlotObject>(obj)) {
			QListWidgetItem* item = new QListWidgetItem(plotObj->title(), plotSelectionWidget());
			item->setData(Qt::UserRole, QVariant::fromValue<OORef<OvitoObject>>(plotObj));

			// Select again the previously selected plot.
			if(item->text() == selectedPlotTitle)
				plotSelectionWidget()->setCurrentItem(item);
		}
	}

	if(!plotSelectionWidget()->currentItem() && plotSelectionWidget()->count() != 0)
		plotSelectionWidget()->setCurrentRow(0);
}

/******************************************************************************
* Is called when the user selects a different plot item in the list.
******************************************************************************/
void PlotInspectionApplet::currentPlotChanged(QListWidgetItem* current, QListWidgetItem* previous)
{
	OORef<PlotObject> plotObj;
	if(current)
		plotObj = static_object_cast<PlotObject>(current->data(Qt::UserRole).value<OORef<OvitoObject>>());

	_plotWidget->setAxisTitle(QwtPlot::xBottom, QString());
	_plotWidget->setAxisTitle(QwtPlot::yLeft, QString());
	if(plotObj && plotObj->y()) {
		if(!_plotCurve) {
			_plotCurve = new QwtPlotCurve();
			_plotCurve->setRenderHint(QwtPlotItem::RenderAntialiased, true);
			_plotCurve->setBrush(QColor(255, 160, 100));
			_plotCurve->attach(_plotWidget);
		}

		QVector<double> xcoords(plotObj->y()->size());
		QVector<double> ycoords(plotObj->y()->size());
		if(!plotObj->x() || plotObj->x()->size() != xcoords.size() || !plotObj->x()->copyTo(xcoords.begin())) {
			std::iota(xcoords.begin(), xcoords.end(), 0);
		}
		else _plotWidget->setAxisTitle(QwtPlot::xBottom, plotObj->x()->name());
		if(!plotObj->y() || plotObj->y()->size() != ycoords.size() || !plotObj->y()->copyTo(ycoords.begin())) {
			std::fill(ycoords.begin(), ycoords.end(), 0.0);
		}
		else _plotWidget->setAxisTitle(QwtPlot::yLeft, plotObj->y()->name());
		_plotCurve->setSamples(xcoords, ycoords);
	}
	else if(_plotCurve) {
		_plotCurve->detach();
		delete _plotCurve;
		_plotCurve = nullptr;
	}

	_plotWidget->replot();
}

}	// End of namespace
}	// End of namespace
