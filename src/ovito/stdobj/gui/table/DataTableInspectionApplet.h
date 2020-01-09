////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2018 Alexander Stukowski
//
//  This file is part of OVITO (Open Visualization Tool).
//
//  OVITO is free software; you can redistribute it and/or modify it either under the
//  terms of the GNU General Public License version 3 as published by the Free Software
//  Foundation (the "GPL") or, at your option, under the terms of the MIT License.
//  If you do not alter this notice, a recipient may use your version of this
//  file under either the GPL or the MIT License.
//
//  You should have received a copy of the GPL along with this program in a
//  file LICENSE.GPL.txt.  You should have received a copy of the MIT License along
//  with this program in a file LICENSE.MIT.txt
//
//  This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND,
//  either express or implied. See the GPL or the MIT License for the specific language
//  governing rights and limitations.
//
////////////////////////////////////////////////////////////////////////////////////////

#pragma once


#include <ovito/stdobj/gui/StdObjGui.h>
#include <ovito/stdobj/table/DataTable.h>
#include <ovito/stdobj/gui/widgets/DataTablePlotWidget.h>
#include <ovito/stdobj/gui/properties/PropertyInspectionApplet.h>

namespace Ovito { namespace StdObj {

/**
 * \brief Data inspector page for data tables and 2d data plots.
 */
class OVITO_STDOBJGUI_EXPORT DataTableInspectionApplet : public PropertyInspectionApplet
{
	Q_OBJECT
	OVITO_CLASS(DataTableInspectionApplet)
	Q_CLASSINFO("DisplayName", "Data Tables");

public:

	/// Constructor.
	Q_INVOKABLE DataTableInspectionApplet() : PropertyInspectionApplet(DataTable::OOClass()) {}

	/// Returns the key value for this applet that is used for ordering the applet tabs.
	virtual int orderingKey() const override { return 200; }

	/// Lets the applet create the UI widget that is to be placed into the data inspector panel.
	virtual QWidget* createWidget(MainWindow* mainWindow) override;

	/// Returns the plotting widget.
	DataTablePlotWidget* plotWidget() const { return _plotWidget; }

	/// Selects a specific data object in this applet.
	virtual bool selectDataObject(PipelineObject* dataSource, const QString& objectIdentifierHint, const QVariant& modeHint) override;

protected:

	/// Creates the evaluator object for filter expressions.
	virtual std::unique_ptr<PropertyExpressionEvaluator> createExpressionEvaluator() override {
		return std::make_unique<PropertyExpressionEvaluator>();
	}

	/// Is called when the user selects a different property container object in the list.
	virtual void currentContainerChanged() override;

private Q_SLOTS:

	/// Action handler.
	void exportDataToFile();

private:

	/// The plotting widget.
	DataTablePlotWidget* _plotWidget;

	MainWindow* _mainWindow;
	QStackedWidget* _stackedWidget;
	QAction* _switchToPlotAction;
	QAction* _switchToTableAction;
	QAction* _exportTableToFileAction;
};

}	// End of namespace
}	// End of namespace
