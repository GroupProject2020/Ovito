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

#include <ovito/stdobj/StdObj.h>
#include <ovito/stdobj/properties/PropertyAccess.h>
#include <ovito/core/utilities/concurrent/AsyncOperation.h>
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
	if(operation.isCanceled())
		return false;

	// Look up the DataSeries to be exported in the pipeline state.
	DataObjectReference objectRef(&DataSeriesObject::OOClass(), dataObjectToExport().dataPath());
	const DataSeriesObject* series = static_object_cast<DataSeriesObject>(state.getLeafObject(objectRef));
	if(!series) {
		throwException(tr("The pipeline output does not contain the data series to be exported (animation frame: %1; object key: %2). Available data series keys: (%3)")
			.arg(frameNumber).arg(objectRef.dataPath()).arg(getAvailableDataObjectList(state, DataSeriesObject::OOClass())));
	}
	series->verifyIntegrity();

	// Make sure the X property exists in the property container.
	// If not, create a temporary property for export.

	operation.setProgressText(tr("Writing file %1").arg(filePath));

	ConstPropertyPtr xstorage = series->getXStorage();
	ConstPropertyPtr ystorage = series->getYStorage();
	const PropertyObject* xprop = series->getX();
	const PropertyObject* yprop = series->getY();
	if(!ystorage || !yprop)
		throwException(tr("Data series to be exported contains no data points."));

	size_t row_count = series->elementCount();
	int xDataType = xstorage ? xstorage->dataType() : 0;
	int yDataType = ystorage->dataType();

	ConstPropertyAccess<int, true> xaccessInt(xDataType == PropertyStorage::Int ? xstorage : nullptr);
	ConstPropertyAccess<qlonglong, true> xaccessInt64(xDataType == PropertyStorage::Int64 ? xstorage : nullptr);
	ConstPropertyAccess<FloatType, true> xaccessFloat(xDataType == PropertyStorage::Float ? xstorage : nullptr);

	if(!series->title().isEmpty())
		textStream() << "# " << series->title() << " (" << row_count << " data points):\n";
	textStream() << "# ";
	auto formatColumnName = [](const QString& name) {
		return name.contains(QChar(' ')) ? (QChar('"') + name + QChar('"')) : name;
	};
	textStream() << formatColumnName((!xprop || !series->axisLabelX().isEmpty()) ? series->axisLabelX() : xprop->name());

	if(ystorage->componentNames().size() == ystorage->componentCount()) {
		for(size_t col = 0; col < ystorage->componentCount(); col++) {
			textStream() << " " << formatColumnName(ystorage->componentNames()[col]);
		}
	}
	else {
		textStream() << " " << formatColumnName(!series->axisLabelY().isEmpty() ? series->axisLabelY() : ystorage->name());
	}

	// Collect the extra properties that should be written to the file.
	std::vector<ConstPropertyAccess<void,true>> outputProperties;
	outputProperties.emplace_back(ystorage);
	for(const PropertyObject* propObj : series->properties()) {
		if(propObj->type() == DataSeriesObject::XProperty) continue;
		if(propObj->type() == DataSeriesObject::YProperty) continue;
		outputProperties.emplace_back(propObj->storage());
		if(propObj->componentNames().size() == propObj->componentCount()) {
			for(size_t col = 0; col < propObj->componentCount(); col++) {
				textStream() << " " << formatColumnName(QStringLiteral("%1.%2").arg(propObj->name()).arg(propObj->componentNames()[col]));
			}
		}
		else {
			textStream() << " " << formatColumnName(propObj->name());
		}
	}

	textStream() << "\n";

	for(size_t row = 0; row < row_count; row++) {
		// Write the X column.
		if(series->plotMode() == DataSeriesObject::BarChart) {
			ElementType* type = yprop->elementType(row);
			if(!type && xprop) type = xprop->elementType(row);
			if(type) {
				textStream() << formatColumnName(type->name()) << " ";
			}
			else continue;
		}
		else {
			if(xaccessInt)
				textStream() << xaccessInt.get(row, 0) << " ";
			else if(xaccessInt64)
				textStream() << xaccessInt64.get(row, 0) << " ";
			else if(xaccessFloat)
				textStream() << xaccessFloat.get(row, 0) << " ";
			else
				textStream() << "<?> ";
		}
		// Write the Y column(s).
		for(const ConstPropertyAccess<void,true>& array : outputProperties) {
			for(size_t col = 0; col < array.componentCount(); col++) {
				if(array.storage()->dataType() == PropertyStorage::Int)
					textStream() << *reinterpret_cast<const int*>(array.cdata(row, col)) << " ";
				else if(array.storage()->dataType() == PropertyStorage::Int64)
					textStream() << *reinterpret_cast<const qlonglong*>(array.cdata(row, col)) << " ";
				else if(array.storage()->dataType() == PropertyStorage::Float)
					textStream() << *reinterpret_cast<const FloatType*>(array.cdata(row, col)) << " ";
				else
					textStream() << "<?> ";
			}
		}
		textStream() << "\n";
	}

	return !operation.isCanceled();
}

}	// End of namespace
}	// End of namespace
