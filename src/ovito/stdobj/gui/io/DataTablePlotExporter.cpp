////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2020 Alexander Stukowski
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

#include <ovito/stdobj/gui/StdObjGui.h>
#include <ovito/stdobj/gui/widgets/DataTablePlotWidget.h>
#include "DataTablePlotExporter.h"

#include <qwt/qwt_plot_renderer.h>

namespace Ovito { namespace StdObj {

IMPLEMENT_OVITO_CLASS(DataTablePlotExporter);
DEFINE_PROPERTY_FIELD(DataTablePlotExporter, plotWidth);
DEFINE_PROPERTY_FIELD(DataTablePlotExporter, plotHeight);
DEFINE_PROPERTY_FIELD(DataTablePlotExporter, plotDPI);
SET_PROPERTY_FIELD_LABEL(DataTablePlotExporter, plotWidth, "Width (mm)");
SET_PROPERTY_FIELD_LABEL(DataTablePlotExporter, plotHeight, "Height (mm)");
SET_PROPERTY_FIELD_LABEL(DataTablePlotExporter, plotDPI, "Resolution (DPI)");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(DataTablePlotExporter, plotWidth, FloatParameterUnit, 1);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(DataTablePlotExporter, plotHeight, FloatParameterUnit, 1);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(DataTablePlotExporter, plotDPI, IntegerParameterUnit, 1);

/******************************************************************************
* Constructs a new instance of the class.
******************************************************************************/
DataTablePlotExporter::DataTablePlotExporter(DataSet* dataset) : FileExporter(dataset),
	_plotWidth(150),
	_plotHeight(100),
	_plotDPI(200)
{
}

/******************************************************************************
 * This is called once for every output file to be written.
 *****************************************************************************/
bool DataTablePlotExporter::openOutputFile(const QString& filePath, int numberOfFrames, SynchronousOperation operation)
{
	OVITO_ASSERT(!_outputFile.isOpen());
	_outputFile.setFileName(filePath);

	return true;
}

/******************************************************************************
 * This is called once for every output file written .
 *****************************************************************************/
void DataTablePlotExporter::closeOutputFile(bool exportCompleted)
{
	if(!exportCompleted)
		_outputFile.remove();
}

/******************************************************************************
 * Exports a single animation frame to the current output file.
 *****************************************************************************/
bool DataTablePlotExporter::exportFrame(int frameNumber, TimePoint time, const QString& filePath, SynchronousOperation operation)
{
	// Evaluate pipeline.
	const PipelineFlowState& state = getPipelineDataToBeExported(time, operation.subOperation());
	if(operation.isCanceled())
		return false;

	// Look up the DataTable to be exported in the pipeline state.
	DataObjectReference objectRef(&DataTable::OOClass(), dataObjectToExport().dataPath());
	const DataTable* table = static_object_cast<DataTable>(state.getLeafObject(objectRef));
	if(!table) {
		throwException(tr("The pipeline output does not contain the data table to be exported (animation frame: %1; object key: %2). Available data tables: (%3)")
			.arg(frameNumber).arg(objectRef.dataPath()).arg(getAvailableDataObjectList(state, DataTable::OOClass())));
	}
	table->verifyIntegrity();

	operation.setProgressText(tr("Writing file %1").arg(filePath));

	DataTablePlotWidget plotWidget;
	plotWidget.setTable(table);
	plotWidget.axisScaleDraw(QwtPlot::yLeft)->setPenWidth(1);
	plotWidget.axisScaleDraw(QwtPlot::xBottom)->setPenWidth(1);
	QwtPlotRenderer plotRenderer;
	plotRenderer.setDiscardFlag(QwtPlotRenderer::DiscardFlag::DiscardBackground);
	plotRenderer.setDiscardFlag(QwtPlotRenderer::DiscardFlag::DiscardCanvasBackground);
	plotRenderer.setDiscardFlag(QwtPlotRenderer::DiscardFlag::DiscardCanvasFrame);
	plotRenderer.renderDocument(&plotWidget, outputFile().fileName(), QSizeF(plotWidth(), plotHeight()), plotDPI());

	return !operation.isCanceled();
}

}	// End of namespace
}	// End of namespace
