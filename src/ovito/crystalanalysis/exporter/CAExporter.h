///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2019) Alexander Stukowski
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


#include <ovito/crystalanalysis/CrystalAnalysis.h>
#include <ovito/crystalanalysis/objects/DislocationNetworkObject.h>
#include <ovito/crystalanalysis/objects/Microstructure.h>
#include <ovito/core/dataset/io/FileExporter.h>
#include <ovito/core/utilities/io/CompressedTextWriter.h>

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

/**
 * \brief Exporter that exports dislocation lines to a Crystal Analysis Tool (CA) file.
 */
class OVITO_CRYSTALANALYSIS_EXPORT CAExporter : public FileExporter
{
	/// Defines a metaclass specialization for this exporter type.
	class OOMetaClass : public FileExporter::OOMetaClass
	{
	public:

		/// Inherit standard constructor from base meta class.
		using FileExporter::OOMetaClass::OOMetaClass;

		/// Returns the file filter that specifies the extension of files written by this service.
		virtual QString fileFilter() const override { return QStringLiteral("*.ca"); }

		/// Returns the filter description that is displayed in the drop-down box of the file dialog.
		virtual QString fileFilterDescription() const override { return tr("Crystal Analysis File"); }
	};

	Q_OBJECT
	OVITO_CLASS_META(CAExporter, OOMetaClass)

public:

	/// \brief Constructs a new instance of this class.
	Q_INVOKABLE CAExporter(DataSet* dataset) : FileExporter(dataset) {}

	/// Returns whether the DXA defect mesh is exported (in addition to the dislocation lines).
	bool meshExportEnabled() const { return _meshExportEnabled; }

	/// Sets whether the DXA defect mesh is exported (in addition to the dislocation lines).
	void setMeshExportEnabled(bool enable) { _meshExportEnabled = enable; }

	/// \brief Returns the type(s) of data objects that this exporter service can export.
	virtual std::vector<DataObjectClassPtr> exportableDataObjectClass() const override {
		return { &DislocationNetworkObject::OOClass(), &Microstructure::OOClass() };
	}

protected:

	/// \brief This is called once for every output file to be written and before exportFrame() is called.
	virtual bool openOutputFile(const QString& filePath, int numberOfFrames, AsyncOperation& operation) override;

	/// \brief This is called once for every output file written after exportFrame() has been called.
	virtual void closeOutputFile(bool exportCompleted) override;

	/// \brief Exports a single animation frame to the current output file.
	virtual bool exportFrame(int frameNumber, TimePoint time, const QString& filePath, AsyncOperation&& operation) override;

	/// Returns the current file this exporter is writing to.
	QFile& outputFile() { return _outputFile; }

	/// Returns the text stream used to write into the current output file.
	CompressedTextWriter& textStream() { return *_outputStream; }

private:

	/// Controls whether the DXA defect mesh is exported (in addition to the dislocation lines).
	bool _meshExportEnabled = true;

	/// The output file stream.
	QFile _outputFile;

	/// The stream object used to write into the output file.
	std::unique_ptr<CompressedTextWriter> _outputStream;
};

}	// End of namespace
}	// End of namespace
}	// End of namespace
