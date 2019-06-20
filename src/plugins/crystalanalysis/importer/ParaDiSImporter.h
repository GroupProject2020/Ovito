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


#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include <plugins/crystalanalysis/objects/Microstructure.h>
#include <plugins/particles/import/ParticleImporter.h>
#include <plugins/particles/import/ParticleFrameData.h>

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

/**
 * \brief Importer for data files written by the ParaDiS discrete dislocation simulation code.
 */
class OVITO_CRYSTALANALYSIS_EXPORT ParaDiSImporter : public ParticleImporter
{
	/// Defines a metaclass specialization for this importer type.
	class OOMetaClass : public ParticleImporter::OOMetaClass
	{
	public:
		/// Inherit standard constructor from base meta class.
		using ParticleImporter::OOMetaClass ::OOMetaClass;

		/// Returns the file filter that specifies the files that can be imported by this service.
		virtual QString fileFilter() const override { return QStringLiteral("*"); }

		/// Returns the filter description that is displayed in the drop-down box of the file dialog.
		virtual QString fileFilterDescription() const override { return tr("ParaDiS data files"); }

		/// Checks if the given file has format that can be read by this importer.
		virtual bool checkFileFormat(QFileDevice& input, const QUrl& sourceLocation) const override;
	};

	OVITO_CLASS_META(ParaDiSImporter, OOMetaClass)
	Q_OBJECT

public:

	/// \brief Constructs a new instance of this class.
	Q_INVOKABLE ParaDiSImporter(DataSet* dataset) : ParticleImporter(dataset) {}

	/// Returns the title of this object.
	virtual QString objectTitle() const override { return tr("ParaDiS File"); }

	/// Creates an asynchronous loader object that loads the data for the given frame from the external file.
	virtual std::shared_ptr<FileSourceImporter::FrameLoader> createFrameLoader(const Frame& frame, const QString& localFilename) override {
		return std::make_shared<FrameLoader>(frame, localFilename);
	}

protected:

	/// The format-specific data holder.
	class DislocFrameData : public ParticleFrameData
	{
	public:

		/// Inherit constructor from base class.
		using ParticleFrameData::ParticleFrameData;

		/// Inserts the loaded data into the provided pipeline state structure. This function is
		/// called by the system from the main thread after the asynchronous loading task has finished.
		virtual OORef<DataCollection> handOver(const DataCollection* existing, bool isNewFile, FileSource* fileSource) override;

		/// Returns the loaded microstructure.
		const MicrostructureData& microstructure() const { return _microstructure; }

		/// Returns the microstructure being loaded.
		MicrostructureData& microstructure() { return _microstructure; }

		/// Returns the type of crystal structure.
		ParticleType::PredefinedStructureType latticeStructure() const { return _latticeStructure; }

		/// Sets the type of crystal ("fcc", "bcc", etc.)
		void setLatticeStructure(ParticleType::PredefinedStructureType latticeStructure) {
			_latticeStructure = latticeStructure;
		}

	protected:

		/// The loaded microstructure.
		MicrostructureData _microstructure;

		/// The type of crystal ("fcc", "bcc", etc.)
		ParticleType::PredefinedStructureType _latticeStructure = ParticleType::PredefinedStructureType::OTHER;
	};

	/// The format-specific task object that is responsible for reading an input file in a worker thread.
	class FrameLoader : public FileSourceImporter::FrameLoader
	{
	public:

		/// Inherit constructor from base class.
		using FileSourceImporter::FrameLoader::FrameLoader;

	protected:

		/// Loads the frame data from the given file.
		virtual FrameDataPtr loadFile(QFile& file) override;

    private:

        /// Parses a control parameter from the ParaDiS file.
        static std::pair<QString, QVariant> parseControlParameter(CompressedTextReader& stream);
	};
};

}	// End of namespace
}	// End of namespace
}	// End of namespace
