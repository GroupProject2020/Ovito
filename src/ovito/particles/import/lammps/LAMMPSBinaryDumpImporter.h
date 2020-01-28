////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2017 Alexander Stukowski
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


#include <ovito/particles/Particles.h>
#include <ovito/particles/import/InputColumnMapping.h>
#include <ovito/particles/import/ParticleImporter.h>
#include <ovito/particles/import/ParticleFrameData.h>
#include <ovito/core/dataset/DataSetContainer.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Import) OVITO_BEGIN_INLINE_NAMESPACE(Formats)

/**
 * \brief File parser for binary LAMMPS dump files.
 */
class OVITO_PARTICLES_EXPORT LAMMPSBinaryDumpImporter : public ParticleImporter
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
		virtual QString fileFilterDescription() const override { return tr("LAMMPS Binary Dump Files"); }

		/// Checks if the given file has format that can be read by this importer.
		virtual bool checkFileFormat(const FileHandle& file) const override;
	};

	OVITO_CLASS_META(LAMMPSBinaryDumpImporter, OOMetaClass)
	Q_OBJECT

public:

	/// \brief Constructs a new instance of this class.
	Q_INVOKABLE LAMMPSBinaryDumpImporter(DataSet* dataset) : ParticleImporter(dataset) {}

	/// Returns the title of this object.
	virtual QString objectTitle() const override { return tr("LAMMPS Dump File"); }

	/// \brief Returns the user-defined mapping between data columns in the input file and
	///        the internal particle properties.
	const InputColumnMapping& columnMapping() const { return _columnMapping; }

	/// \brief Sets the user-defined mapping between data columns in the input file and
	///        the internal particle properties.
	void setColumnMapping(const InputColumnMapping& mapping);

	/// Creates an asynchronous loader object that loads the data for the given frame from the external file.
	virtual std::shared_ptr<FileSourceImporter::FrameLoader> createFrameLoader(const Frame& frame, const FileHandle& file) override {
		return std::make_shared<FrameLoader>(frame, std::move(file), sortParticles(), _columnMapping);
	}

	/// Creates an asynchronous frame discovery object that scans the input file for contained animation frames.
	virtual std::shared_ptr<FileSourceImporter::FrameFinder> createFrameFinder(const FileHandle& file) override {
		return std::make_shared<FrameFinder>(sourceUrl, std::move(file));
	}

	/// Inspects the header of the given file and returns the number of file columns.
	Future<InputColumnMapping> inspectFileHeader(const Frame& frame);

public:

	Q_PROPERTY(Ovito::Particles::InputColumnMapping columnMapping READ columnMapping WRITE setColumnMapping);

private:

	class LAMMPSFrameData : public ParticleFrameData
	{
	public:

		/// Inherit constructor from base class.
		using ParticleFrameData::ParticleFrameData;

		/// Returns the file column mapping generated from the information in the file header.
		InputColumnMapping& detectedColumnMapping() { return _detectedColumnMapping; }

	private:
		InputColumnMapping _detectedColumnMapping;
	};

	/// The format-specific task object that is responsible for reading an input file in the background.
	class FrameLoader : public FileSourceImporter::FrameLoader
	{
	public:

		/// Normal constructor.
		FrameLoader(const FileSourceImporter::Frame& frame, const QString& filename, bool sortParticles, const InputColumnMapping& columnMapping)
			: FileSourceImporter::FrameLoader(frame, filename), _sortParticles(sortParticles), _parseFileHeaderOnly(false), _columnMapping(columnMapping) {}

		/// Constructor used when reading only the file header information.
		FrameLoader(const FileSourceImporter::Frame& frame, const QString& filename)
			: FileSourceImporter::FrameLoader(frame, filename), _parseFileHeaderOnly(true) {}

		/// Returns the file column mapping used to load the file.
		const InputColumnMapping& columnMapping() const { return _columnMapping; }

	protected:

		/// Loads the frame data from the given file.
		virtual FrameDataPtr loadFile(QIODevice& file) override;

	private:

		bool _sortParticles;
		bool _parseFileHeaderOnly;
		InputColumnMapping _columnMapping;
	};

	/// The format-specific task object that is responsible for scanning the input file for animation frames.
	class FrameFinder : public FileSourceImporter::FrameFinder
	{
	public:

		/// Inherit constructor from base class.
		using FileSourceImporter::FrameFinder::FrameFinder;

	protected:

		/// Scans the data file and builds a list of source frames.
		virtual void discoverFramesInFile(QVector<FileSourceImporter::Frame>& frames) override;
	};

protected:

	/// \brief Saves the class' contents to the given stream.
	virtual void saveToStream(ObjectSaveStream& stream, bool excludeRecomputableData) override;

	/// \brief Loads the class' contents from the given stream.
	virtual void loadFromStream(ObjectLoadStream& stream) override;

	/// \brief Creates a copy of this object.
	virtual OORef<RefTarget> clone(bool deepCopy, CloneHelper& cloneHelper) const override;

private:

	/// Stores the user-defined mapping between data columns in the input file and
	/// the internal particle properties.
	InputColumnMapping _columnMapping;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace


