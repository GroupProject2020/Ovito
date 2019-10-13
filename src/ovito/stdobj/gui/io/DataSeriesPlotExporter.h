////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2019 Alexander Stukowski
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
#include <ovito/stdobj/series/DataSeriesObject.h>
#include <ovito/core/dataset/io/FileExporter.h>

namespace Ovito { namespace StdObj {

/**
 * \brief Exporter that writes the graphical plot of a data series to an image file.
 */
class OVITO_STDOBJGUI_EXPORT DataSeriesPlotExporter : public FileExporter
{
	/// Defines a metaclass specialization for this exporter type.
	class OOMetaClass : public FileExporter::OOMetaClass
	{
	public:
		/// Inherit standard constructor from base meta class.
		using FileExporter::OOMetaClass::OOMetaClass;

		/// Returns the file filter that specifies the files that can be exported by this service.
		virtual QString fileFilter() const override { return QStringLiteral("*.pdf *.png"); }

		/// Returns the filter description that is displayed in the drop-down box of the file dialog.
		virtual QString fileFilterDescription() const override { return tr("Data Plot File"); }
	};

	Q_OBJECT
	OVITO_CLASS_META(DataSeriesPlotExporter, OOMetaClass)

public:

	/// \brief Constructs a new instance of this class.
	Q_INVOKABLE DataSeriesPlotExporter(DataSet* dataset);

	/// \brief Returns the type(s) of data objects that this exporter service can export.
	virtual std::vector<DataObjectClassPtr> exportableDataObjectClass() const override {
		return { &DataSeriesObject::OOClass() };
	}

protected:

	/// \brief This is called once for every output file to be written and before exportData() is called.
	virtual bool openOutputFile(const QString& filePath, int numberOfFrames, AsyncOperation& operation) override;

	/// \brief This is called once for every output file written after exportData() has been called.
	virtual void closeOutputFile(bool exportCompleted) override;

	/// \brief Exports a single animation frame to the current output file.
	virtual bool exportFrame(int frameNumber, TimePoint time, const QString& filePath, AsyncOperation&& operation) override;

	/// Returns the current file this exporter is writing to.
	QFile& outputFile() { return _outputFile; }

private:

	/// The output file stream.
	QFile _outputFile;

	/// The width of the plot in milimeters.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, plotWidth, setPlotWidth, PROPERTY_FIELD_MEMORIZE);

	/// The height of the plot in milimeters.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, plotHeight, setPlotHeight, PROPERTY_FIELD_MEMORIZE);

	/// The resolution of the plot in DPI.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(int, plotDPI, setPlotDPI, PROPERTY_FIELD_MEMORIZE);
};

}	// End of namespace
}	// End of namespace
