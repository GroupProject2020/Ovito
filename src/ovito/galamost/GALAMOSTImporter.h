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


#include <ovito/particles/Particles.h>
#include <ovito/particles/import/ParticleImporter.h>
#include <ovito/particles/import/ParticleFrameData.h>

#include <QXmlDefaultHandler>

namespace Ovito { namespace Particles {

/**
 * \brief File parser for data files of the GALAMOST MD code.
 */
class OVITO_GALAMOST_EXPORT GALAMOSTImporter : public ParticleImporter
{
	/// Defines a metaclass specialization for this importer type.
	class OOMetaClass : public ParticleImporter::OOMetaClass
	{
	public:
		/// Inherit standard constructor from base meta class.
		using ParticleImporter::OOMetaClass ::OOMetaClass;

		/// Returns the file filter that specifies the files that can be imported by this service.
		virtual QString fileFilter() const override { return QStringLiteral("*.xml"); }

		/// Returns the filter description that is displayed in the drop-down box of the file dialog.
		virtual QString fileFilterDescription() const override { return tr("GALAMOST Files"); }

		/// Checks if the given file has format that can be read by this importer.
		virtual bool checkFileFormat(const FileHandle& file) const override;
	};

	OVITO_CLASS_META(GALAMOSTImporter, OOMetaClass)
	Q_OBJECT

public:

	/// \brief Constructor.
	Q_INVOKABLE GALAMOSTImporter(DataSet *dataset) : ParticleImporter(dataset) {}

	/// Returns the title of this object.
	virtual QString objectTitle() const override { return tr("GALAMOST"); }

	/// Creates an asynchronous loader object that loads the data for the given frame from the external file.
	virtual std::shared_ptr<FileSourceImporter::FrameLoader> createFrameLoader(const Frame& frame, const FileHandle& file) override {
		return std::make_shared<FrameLoader>(frame, file);
	}

private:

	/// The format-specific task object that is responsible for reading an input file in a separate thread.
	class FrameLoader : public FileSourceImporter::FrameLoader, protected QXmlDefaultHandler
	{
	public:

		/// Constructor.
		FrameLoader(const FileSourceImporter::Frame& frame, const FileHandle& file)
			: FileSourceImporter::FrameLoader(frame, file) {}

	protected:

		/// Reads the frame data from the external file.
		virtual FrameDataPtr loadFile() override;

		/// Is called by the XML parser whenever a new XML element is read.
		virtual bool startElement(const QString& namespaceURI, const QString& localName, const QString& qName, const QXmlAttributes& atts) override;

		/// Is called by the XML parser whenever it has parsed an end element tag.
		virtual bool endElement(const QString& namespaceURI, const QString& localName, const QString& qName) override;

		/// Is called by the XML parser whenever it has parsed a chunk of character data.
		virtual bool characters(const QString& ch) override;

		/// Is called when a non-recoverable error is encountered during parsing of the XML file.
		virtual bool fatalError(const QXmlParseException& exception) override;

	private:

		/// Container for the parsed particle data.
		std::shared_ptr<ParticleFrameData> _frameData;

		/// The dimensionality of the dataset.
		int _dimensions = 3;

		/// The number of atoms.
		size_t _natoms = 0;

		/// The particle/bond property which is currently being parsed.
		PropertyPtr _currentProperty;

		/// Buffer for text data read from XML file.
		QString _characterData;

		/// The number of <configuration> elements that have been parsed so far.
		size_t _numConfigurationsRead = 0;
	};
};

}	// End of namespace
}	// End of namespace
