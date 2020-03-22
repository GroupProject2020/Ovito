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


#include <ovito/particles/Particles.h>
#include <ovito/particles/import/ParticleImporter.h>
#include <ovito/particles/import/ParticleFrameData.h>
#include <ovito/core/utilities/mesh/TriMesh.h>
#include <ovito/core/dataset/DataSetContainer.h>

#include <QReadWriteLock>

namespace Ovito { namespace Particles {

class GSDFile;	// Defined in GSDFile.h

/**
 * \brief File parser for GSD (General Simulation Data) files written by the HOOMD simulation code.
 */
class OVITO_PARTICLES_EXPORT GSDImporter : public ParticleImporter
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
		virtual QString fileFilterDescription() const override { return tr("GSD/HOOMD Files"); }

		/// Checks if the given file has format that can be read by this importer.
		virtual bool checkFileFormat(const FileHandle& file) const override;
	};

	OVITO_CLASS_META(GSDImporter, OOMetaClass)
	Q_OBJECT

public:

	/// \brief Constructs a new instance of this class.
	Q_INVOKABLE GSDImporter(DataSet* dataset) : ParticleImporter(dataset), _roundingResolution(4) {
		setMultiTimestepFile(true);
	}

	/// Returns the title of this object.
	virtual QString objectTitle() const override { return tr("GSD"); }

	/// Creates an asynchronous loader object that loads the data for the given frame from the external file.
	virtual std::shared_ptr<FileSourceImporter::FrameLoader> createFrameLoader(const Frame& frame, const FileHandle& file) override {
		return std::make_shared<FrameLoader>(frame, std::move(file), this, std::max(roundingResolution(), 1));
	}

	/// Creates an asynchronous frame discovery object that scans the input file for contained animation frames.
	virtual std::shared_ptr<FileSourceImporter::FrameFinder> createFrameFinder(const FileHandle& file) override {
		return std::make_shared<FrameFinder>(file);
	}

	/// Stores the particle shape geometry generated from a JSON string in the internal cache.
	void storeParticleShapeInCache(const QByteArray& jsonString, const TriMeshPtr& mesh);

	/// Looks up a particle shape geometry in the internal cache that was previously
	/// generated from a JSON string.
	TriMeshPtr lookupParticleShapeInCache(const QByteArray& jsonString) const;

protected:

	/// \brief Is called when the value of a property of this object has changed.
	virtual void propertyChanged(const PropertyFieldDescriptor& field) override;

private:

	/// A lookup map that holds geometries that have been generated from JSON strings.
	QHash<QByteArray, TriMeshPtr> _particleShapeCache;

	/// Synchronization object for multi-threaded access to the particle shape cache.
	mutable QReadWriteLock _cacheSynchronization;

	/// Controls the tessellation resolution for rounded corners and edges.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(int, roundingResolution, setRoundingResolution, PROPERTY_FIELD_MEMORIZE);

private:

	/// The format-specific task object that is responsible for reading an input file in the background.
	class FrameLoader : public FileSourceImporter::FrameLoader
	{
	public:

		/// Constructor.
		FrameLoader(const Frame& frame, const FileHandle& file, GSDImporter* importer, int roundingResolution)
			: FileSourceImporter::FrameLoader(frame, file), _importer(importer), _roundingResolution(roundingResolution) {}

	protected:

		/// Reads the frame data from the external file.
		virtual FrameDataPtr loadFile() override;

		/// Reads the values of a particle or bond property from the GSD file.
		PropertyStorage* readOptionalProperty(GSDFile& gsd, const char* chunkName, uint64_t frameNumber, uint32_t numElements, int propertyType, bool isBondProperty, const std::shared_ptr<ParticleFrameData>& frameData);

		/// Parse the JSON string containing a particle shape definition.
		void parseParticleShape(int typeId, ParticleFrameData::TypeList* typeList, size_t numParticles, ParticleFrameData* frameData, const QByteArray& shapeSpecString);

		/// Parsing routine for 'Sphere' particle shape definitions.
		void parseSphereShape(int typeId, ParticleFrameData::TypeList* typeList, QJsonObject definition);

		/// Parsing routine for 'Ellipsoid' particle shape definitions.
		void parseEllipsoidShape(int typeId, ParticleFrameData::TypeList* typeList, size_t numParticles, ParticleFrameData* frameData, QJsonObject definition);

		/// Parsing routine for 'Polygon' particle shape definitions.
		void parsePolygonShape(int typeId, ParticleFrameData::TypeList* typeList, QJsonObject definition, const QByteArray& shapeSpecString);

		/// Parsing routine for 'ConvexPolyhedron' particle shape definitions.
		void parseConvexPolyhedronShape(int typeId, ParticleFrameData::TypeList* typeList, QJsonObject definition, const QByteArray& shapeSpecString);

		/// Parsing routine for 'Mesh' particle shape definitions.
		void parseMeshShape(int typeId, ParticleFrameData::TypeList* typeList, QJsonObject definition, const QByteArray& shapeSpecString);

		/// Parsing routine for 'SphereUnion' particle shape definitions.
		void parseSphereUnionShape(int typeId, ParticleFrameData::TypeList* typeList, QJsonObject definition, const QByteArray& shapeSpecString);

	private:

		OORef<GSDImporter> _importer;
		int _roundingResolution;
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
};

}	// End of namespace
}	// End of namespace
