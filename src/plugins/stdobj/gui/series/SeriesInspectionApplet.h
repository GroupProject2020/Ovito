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

#pragma once


#include <plugins/stdobj/gui/StdObjGui.h>
#include <plugins/stdobj/series/DataSeriesObject.h>
#include <gui/mainwin/data_inspector/DataInspectionApplet.h>

class QwtPlot;
class QwtPlotCurve;
class QwtPlotLegendItem;

namespace Ovito { namespace StdObj {

/**
 * \brief Data inspector page for 2d plots.
 */
class OVITO_STDOBJGUI_EXPORT SeriesInspectionApplet : public DataInspectionApplet
{
	Q_OBJECT
	OVITO_CLASS(SeriesInspectionApplet)
	Q_CLASSINFO("DisplayName", "Data Series");

public:

	/// Constructor.
	Q_INVOKABLE SeriesInspectionApplet() {}

	/// Returns the key value for this applet that is used for ordering the applet tabs.
	virtual int orderingKey() const override { return 200; }

	/// Determines whether the given pipeline flow state contains data that can be displayed by this applet.
	virtual bool appliesTo(const PipelineFlowState& state) override;

	/// Lets the applet create the UI widget that is to be placed into the data inspector panel. 
	virtual QWidget* createWidget(MainWindow* mainWindow) override;

	/// Lets the applet update the contents displayed in the inspector.
	virtual void updateDisplay(const PipelineFlowState& state, PipelineSceneNode* sceneNode) override;

	/// Returns the widget for selecting the current data plot.
	QListWidget* plotSelectionWidget() const { return _plotSelectionWidget; }

	/// Returns the plotting widget.
	QwtPlot* plotWidget() const { return _plotWidget; }

private Q_SLOTS:

	/// Is called when the user selects a different plot item in the list.
	void currentPlotChanged(QListWidgetItem* current, QListWidgetItem* previous);

private:

	/// The widget for selecting the current data plot.
	QListWidget* _plotSelectionWidget = nullptr;

	/// The plotting widget.
	QwtPlot* _plotWidget;

	/// The plot item(s).
    std::vector<QwtPlotCurve*> _plotCurves;	

	/// The plot legend.
	QwtPlotLegendItem* _legendItem = nullptr;
};

}	// End of namespace
}	// End of namespace
