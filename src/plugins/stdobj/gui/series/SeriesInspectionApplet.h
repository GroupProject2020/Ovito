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
	Q_INVOKABLE SeriesInspectionApplet() : PropertyInspectionApplet(DataSeriesProperty::OOClass()) {}

	/// Returns the key value for this applet that is used for ordering the applet tabs.
	virtual int orderingKey() const override { return 200; }

	/// Lets the applet create the UI widget that is to be placed into the data inspector panel. 
	virtual QWidget* createWidget(MainWindow* mainWindow) override;

	/// Lets the applet update the contents displayed in the inspector.
	virtual void updateDisplay(const PipelineFlowState& state, PipelineSceneNode* sceneNode) override;

	/// Returns the plotting widget.
	QwtPlot* plotWidget() const { return _plotWidget; }

protected:

	/// Creates the evaluator object for filter expressions.
	virtual std::unique_ptr<PropertyExpressionEvaluator> createExpressionEvaluator() override {
		return std::make_unique<PropertyExpressionEvaluator>();
	}

	/// Returns the data object that represents the given data bundle.
	virtual DataObject* lookupBundleObject(const PipelineFlowState& state, const QString& bundleId) const override {
		for(DataObject* obj : state.objects()) {
			if(DataSeriesObject* series = dynamic_object_cast<DataSeriesObject>(obj)) {
				if(series->identifier() == bundleId) return series;
			}
		}
		return nullptr;
	}

private:

	/// The plotting widget.
	DataSeriesPlotWidget* _plotWidget;
};

}	// End of namespace
}	// End of namespace
