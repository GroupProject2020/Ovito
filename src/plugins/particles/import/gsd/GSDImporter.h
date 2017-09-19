///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2016) Alexander Stukowski
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


#include <plugins/particles/Particles.h>
#include <plugins/particles/import/ParticleImporter.h>
#include <core/dataset/DataSetContainer.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Import) OVITO_BEGIN_INLINE_NAMESPACE(Formats)

class GSDFile;	// Defined in GSDFile.h

/**
 * \brief File parser for GSD (General Simulation Data) files.
 */
class OVITO_PARTICLES_EXPORT GSDImporter : public ParticleImporter
{
	Q_OBJECT
	OVITO_CLASS(GSDImporter)
	
public:

	/// \brief Constructs a new instance of this class.
	Q_INVOKABLE GSDImporter(DataSet* dataset) : ParticleImporter(dataset) {
		setMultiTimestepFile(true);
	}

	/// \brief Returns the file filter that specifies the files that can be imported by this service.
	/// \return A wild-card pattern that specifies the file types that can be handled by this import class.
	virtual QString fileFilter() override { return QStringLiteral("*"); }

	/// \brief Returns the filter description that is displayed in the drop-down box of the file dialog.
	/// \return A string that describes the file format.
	virtual QString fileFilterDescription() override { return tr("GSD/HOOMD Files"); }

	/// \brief Checks if the given file has format that can be read by this importer.
	virtual bool checkFileFormat(QFileDevice& input, const QUrl& sourceLocation) override;

	/// Returns the title of this object.
	virtual QString objectTitle() override { return tr("GSD"); }

	/// Creates an asynchronous loader object that loads the data for the given frame from the external file.
	virtual std::shared_ptr<FileSourceImporter::FrameLoader> createFrameLoader(const Frame& frame, const QString& localFilename) override {
		return std::make_shared<FrameLoader>(frame, localFilename);
	}

	/// Creates an asynchronous frame discovery object that scans the input file for contained animation frames.
	virtual std::shared_ptr<FileSourceImporter::FrameFinder> createFrameFinder(const QUrl& sourceUrl, const QString& localFilename) override {
		return std::make_shared<FrameFinder>(sourceUrl, localFilename);
	}

private:

	/// The format-specific task object that is responsible for reading an input file in the background.
	class FrameLoader : public FileSourceImporter::FrameLoader
	{
	public:

		/// Inherit constructor from base class.
		using FileSourceImporter::FrameLoader::FrameLoader;

	protected:

		/// Loads the frame data from the given file.
		virtual void loadFile(QFile& file) override;

		/// Reads the values of a particle property from the GSD file.
		PropertyStorage* readOptionalParticleProperty(GSDFile& gsd, const char* chunkName, uint64_t frameNumber, uint32_t numParticles, ParticleProperty::Type propertyType, const std::shared_ptr<ParticleFrameData>& frameData);
	};

	/// The format-specific task object that is responsible for scanning the input file for animation frames. 
	class FrameFinder : public FileSourceImporter::FrameFinder
	{
	public:

		/// Inherit constructor from base class.
		using FileSourceImporter::FrameFinder::FrameFinder;

	protected:

		/// Scans the given file for source frames.
		virtual void discoverFramesInFile(QFile& file, const QUrl& sourceUrl, QVector<FileSourceImporter::Frame>& frames) override;	
	};
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace


