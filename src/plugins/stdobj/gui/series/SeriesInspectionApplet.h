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
#include <plugins/stdobj/gui/widgets/DataSeriesPlotWidget.h>
#include <plugins/stdobj/gui/properties/PropertyInspectionApplet.h>

namespace Ovito { namespace StdObj {

/**
 * \brief Data inspector page for 2d plots.
 */
class OVITO_STDOBJGUI_EXPORT SeriesInspectionApplet : public PropertyInspectionApplet
{
	Q_OBJECT
	OVITO_CLASS(SeriesInspectionApplet)
	Q_CLASSINFO("DisplayName", "Data Series");

public:

	/// Constructor.
	Q_INVOKABLE SeriesInspectionApplet() : PropertyInspectionApplet(DataSeriesObject::OOClass()) {}

	/// Returns the key value for this applet that is used for ordering the applet tabs.
	virtual int orderingKey() const override { return 200; }

	/// Lets the applet create the UI widget that is to be placed into the data inspector panel. 
	virtual QWidget* createWidget(MainWindow* mainWindow) override;

	/// Returns the plotting widget.
	DataSeriesPlotWidget* plotWidget() const { return _plotWidget; }

protected:

	/// Creates the evaluator object for filter expressions.
	virtual std::unique_ptr<PropertyExpressionEvaluator> createExpressionEvaluator() override {
		return std::make_unique<PropertyExpressionEvaluator>();
	}

	/// Is called when the user selects a different property container object in the list.
	virtual void currentContainerChanged() override;

private:

	/// The plotting widget.
	DataSeriesPlotWidget* _plotWidget;
};

}	// End of namespace
}	// End of namespace
