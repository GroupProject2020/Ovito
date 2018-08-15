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

#include <qwt/qwt_plot.h>
#include <qwt/qwt_plot_curve.h>
#include <qwt/qwt_plot_grid.h>
#include <qwt/qwt_plot_legenditem.h>

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
	connect(_plotSelectionWidget, &QListWidget::currentItemChanged, this, &SeriesInspectionApplet::currentPlotChanged);
	return splitter;
}

/******************************************************************************
* Updates the contents displayed in the inspector.
******************************************************************************/
void SeriesInspectionApplet::updateDisplay(const PipelineFlowState& state, PipelineSceneNode* sceneNode)
{
	// Remember which plot was previously selected.
	QString selectedPlotTitle;
	if(plotSelectionWidget()->currentItem())
		selectedPlotTitle = plotSelectionWidget()->currentItem()->text();

	// Rebuild list of plots.
	plotSelectionWidget()->clear();
	for(DataObject* obj : state.objects()) {
		if(DataSeriesObject* seriesObj = dynamic_object_cast<DataSeriesObject>(obj)) {
			QListWidgetItem* item = new QListWidgetItem(seriesObj->title(), plotSelectionWidget());
			item->setData(Qt::UserRole, QVariant::fromValue<OORef<OvitoObject>>(seriesObj));

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
void SeriesInspectionApplet::currentPlotChanged(QListWidgetItem* current, QListWidgetItem* previous)
{
	OORef<DataSeriesObject> seriesObj;
	if(current)
		seriesObj = static_object_cast<DataSeriesObject>(current->data(Qt::UserRole).value<OORef<OvitoObject>>());

	static const Qt::GlobalColor curveColors[] = {
		Qt::black, Qt::red, Qt::blue, Qt::green,
		Qt::cyan, Qt::magenta, Qt::gray, Qt::darkRed, 
		Qt::darkGreen, Qt::darkBlue, Qt::darkCyan, Qt::darkMagenta,
		Qt::darkYellow, Qt::darkGray
	};
	
	_plotWidget->setAxisTitle(QwtPlot::xBottom, QString{});
	_plotWidget->setAxisTitle(QwtPlot::yLeft, QString{});
	if(seriesObj && seriesObj->y()) {
		const auto& x = seriesObj->x();
		const auto& y = seriesObj->y();
		while(_plotCurves.size() < y->componentCount()) {
			QwtPlotCurve* curve = new QwtPlotCurve();
			curve->setRenderHint(QwtPlotItem::RenderAntialiased, true);
			curve->setPen(QPen(curveColors[_plotCurves.size() % (sizeof(curveColors)/sizeof(curveColors[0]))], 1));
			curve->attach(_plotWidget);
			_plotCurves.push_back(curve);
		}
		while(_plotCurves.size() > y->componentCount()) {
			delete _plotCurves.back();
			_plotCurves.pop_back();
		}
		if(_plotCurves.size() == 1 && y->componentNames().empty()) {
			_plotCurves[0]->setBrush(QColor(255, 160, 100));
		}
		else {
			for(QwtPlotCurve* curve : _plotCurves)
				curve->setBrush({});
		}
		if(y->componentNames().empty()) {
			if(_legendItem) {
				delete _legendItem;
				_legendItem = nullptr;
			}
		}
		else {
			if(!_legendItem) {
				_legendItem = new QwtPlotLegendItem();
				_legendItem->setAlignment(Qt::AlignRight | Qt::AlignTop);
				_legendItem->attach(_plotWidget);
			}
		}

		QVector<double> xcoords(y->size());
		QVector<double> ycoords(y->size());
		if(!x || x->size() != xcoords.size() || !x->copyTo(xcoords.begin())) {
			std::iota(xcoords.begin(), xcoords.end(), 0);
		}
		else _plotWidget->setAxisTitle(QwtPlot::xBottom, x->name());
		for(size_t cmpnt = 0; cmpnt < y->componentCount(); cmpnt++) {
			if(!y->copyTo(ycoords.begin(), cmpnt)) {
				std::fill(ycoords.begin(), ycoords.end(), 0.0);
			}
			else _plotWidget->setAxisTitle(QwtPlot::yLeft, y->name());
			_plotCurves[cmpnt]->setSamples(xcoords, ycoords);
			if(cmpnt < y->componentNames().size())
				_plotCurves[cmpnt]->setTitle(y->componentNames()[cmpnt]);
		}
	}
	else {
		for(QwtPlotCurve* curve : _plotCurves)
			delete curve;
		_plotCurves.clear();
		delete _legendItem;
		_legendItem = nullptr;
	}

	_plotWidget->replot();
}

}	// End of namespace
}	// End of namespace
