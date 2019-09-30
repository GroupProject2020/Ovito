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


#include <ovito/particles/Particles.h>
#include <ovito/particles/import/ParticleImporter.h>
#include <ovito/core/dataset/DataSetContainer.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Import) OVITO_BEGIN_INLINE_NAMESPACE(Formats)

/**
 * \brief File parser for Crystallographic Information Files (CIF).
 */
class OVITO_PARTICLES_EXPORT CIFImporter : public ParticleImporter
{
	/// Defines a metaclass specialization for this importer type.
	class OOMetaClass : public ParticleImporter::OOMetaClass
	{
	public:
		/// Inherit standard constructor from base meta class.
		using ParticleImporter::OOMetaClass ::OOMetaClass;

		/// Returns the file filter that specifies the files that can be imported by this service.
		virtual QString fileFilter() const override { return QStringLiteral("*.cif"); }

		/// Returns the filter description that is displayed in the drop-down box of the file dialog.
		virtual QString fileFilterDescription() const override { return tr("Crystallographic Information Files"); }

		/// Checks if the given file has format that can be read by this importer.
		virtual bool checkFileFormat(QFileDevice& input, const QUrl& sourceLocation) const override;
	};

	OVITO_CLASS_META(CIFImporter, OOMetaClass)
	Q_OBJECT

public:

	/// \brief Constructs a new instance of this class.
	Q_INVOKABLE CIFImporter(DataSet* dataset) : ParticleImporter(dataset) {}

	/// Returns the title of this object.
	virtual QString objectTitle() const override { return tr("CIF"); }

	/// Creates an asynchronous loader object that loads the data for the given frame from the external file.
	virtual std::shared_ptr<FileSourceImporter::FrameLoader> createFrameLoader(const Frame& frame, const QString& localFilename) override {
		activateCLocale();
		return std::make_shared<FrameLoader>(frame, localFilename);
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
		virtual FrameDataPtr loadFile(QFile& file) override;
	};
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
