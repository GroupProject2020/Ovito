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

#include <plugins/stdobj/StdObj.h>
#include "DataSeriesExporter.h"

namespace Ovito { namespace StdObj {

IMPLEMENT_OVITO_CLASS(DataSeriesExporter);

/******************************************************************************
 * This is called once for every output file to be written and before
 * exportData() is called.
 *****************************************************************************/
bool DataSeriesExporter::openOutputFile(const QString& filePath, int numberOfFrames, AsyncOperation& operation)
{
	OVITO_ASSERT(!_outputFile.isOpen());
	OVITO_ASSERT(!_outputStream);

	_outputFile.setFileName(filePath);
	_outputStream.reset(new CompressedTextWriter(_outputFile, dataset()));

	return true;
}

/******************************************************************************
 * This is called once for every output file written after exportData()
 * has been called.
 *****************************************************************************/
void DataSeriesExporter::closeOutputFile(bool exportCompleted)
{
	_outputStream.reset();
	if(_outputFile.isOpen())
		_outputFile.close();

	if(!exportCompleted)
		_outputFile.remove();
}

/******************************************************************************
 * Exports a single animation frame to the current output file.
 *****************************************************************************/
bool DataSeriesExporter::exportFrame(int frameNumber, TimePoint time, const QString& filePath, AsyncOperation&& operation)
{
	// Evaluate pipeline.
	const PipelineFlowState& state = getPipelineDataToBeExported(time, operation);

	// Look up the DataSeries to be exported in the pipeline state.
	DataObjectReference objectRef(&DataSeriesObject::OOClass(), dataObjectToExport().dataPath());
	const DataSeriesObject* series = static_object_cast<DataSeriesObject>(state.getLeafObject(objectRef));
	if(!series) {
		throwException(tr("The pipeline output does not contain the data series to be exported (animation frame: %1; object key: %2). Available data series keys: (%3)")
			.arg(frameNumber).arg(objectRef.dataPath()).arg(getAvailableDataObjectList(state, DataSeriesObject::OOClass())));
	}

	operation.setProgressText(tr("Writing file %1").arg(filePath));	

	ConstPropertyPtr x = series->getXStorage();
	ConstPropertyPtr y = series->getYStorage();
	const PropertyObject* yprop = series->getY();
	if(!y || !yprop)
		throwException(tr("Data series to be exported contains no data points."));

	size_t row_count = series->elementCount();
	size_t col_count = y->componentCount();
	int xDataType = x ? x->dataType() : 0;
	int yDataType = y->dataType();

	if(!series->title().isEmpty())
		textStream() << "# " << series->title() << ":\n";
	textStream() << "# ";
	auto formatColumnName = [](const QString& name) {
		return name.contains(QChar(' ')) ? (QChar('"') + name + QChar('"')) : name;
	};
	textStream() << formatColumnName((!series->getX() || !series->axisLabelX().isEmpty()) ? series->axisLabelX() : series->getX()->name());
	if(y->componentNames().size() == y->componentCount()) {
		for(size_t col = 0; col < col_count; col++) {
			textStream() << " " << formatColumnName(y->componentNames()[col]);
		}
	}
	else {
		textStream() << " " << formatColumnName(!series->axisLabelY().isEmpty() ? series->axisLabelY() : y->name());
	}
	textStream() << "\n";
	for(size_t row = 0; row < row_count; row++) {
		if(series->plotMode() == DataSeriesObject::BarChart) {
			if(ElementType* type = yprop->elementType(row)) {
				textStream() << formatColumnName(type->name()) << " ";
			}
			else continue;
		}
		else {
			if(xDataType == PropertyStorage::Int)
				textStream() << x->getIntComponent(row, 0) << " ";
			else if(xDataType == PropertyStorage::Int64)
				textStream() << x->getInt64Component(row, 0) << " ";
			else if(xDataType == PropertyStorage::Float)
				textStream() << x->getFloatComponent(row, 0) << " ";
		}
		for(size_t col = 0; col < col_count; col++) {
			if(yDataType == PropertyStorage::Int)
				textStream() << y->getIntComponent(row, col) << " ";
			else if(yDataType == PropertyStorage::Int64)
				textStream() << y->getInt64Component(row, col) << " ";
			else if(yDataType == PropertyStorage::Float)
				textStream() << y->getFloatComponent(row, col) << " ";
		}
		textStream() << "\n";
	}

	return !operation.isCanceled();
}

}	// End of namespace
}	// End of namespace
