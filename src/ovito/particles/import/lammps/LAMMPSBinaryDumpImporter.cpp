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

#include <ovito/particles/Particles.h>
#include <ovito/particles/import/ParticleFrameData.h>
#include <ovito/core/app/Application.h>
#include <ovito/core/utilities/io/FileManager.h>
#include "LAMMPSBinaryDumpImporter.h"

#include <QtEndian>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Import) OVITO_BEGIN_INLINE_NAMESPACE(Formats)

IMPLEMENT_OVITO_CLASS(LAMMPSBinaryDumpImporter);

// Helper functions for converting binary floating-point representations used by
// different computing architectures. The Qt library provides similar conversion
// functions, but only for integer types.
#if Q_BYTE_ORDER == Q_BIG_ENDIAN
	inline double fromBigEndian(double source) { return source; }
	inline double fromLittleEndian(double source) {
		quint64 i = qFromLittleEndian(reinterpret_cast<const quint64&>(source));
		return reinterpret_cast<const double&>(i);
	}
#else
	inline double fromBigEndian(double source) {
		quint64 i = qFromBigEndian(reinterpret_cast<const quint64&>(source));
		return reinterpret_cast<const double&>(i);
	}
	inline double fromLittleEndian(double source) { return source; }
#endif


struct LAMMPSBinaryDumpHeader
{
	LAMMPSBinaryDumpHeader() :
		ntimestep(-1), natoms(-1),
		size_one(-1), nchunk(-1) {
			memset(boundaryFlags, 0, sizeof(boundaryFlags));
			memset(tiltFactors, 0, sizeof(tiltFactors));
		}

	int ntimestep;
	qlonglong natoms;
	int boundaryFlags[3][2];
	double bbox[3][2];
	double tiltFactors[3];
	int size_one;
	int nchunk;

	enum LAMMPSDataType {
		LAMMPS_SMALLBIG,
		LAMMPS_SMALLSMALL,
		LAMMPS_BIGBIG,
		// End of list marker:
		NUMBER_OF_LAMMPS_DATA_TYPES
	};
	LAMMPSDataType dataType;

	enum LAMMPSEndianess {
		LAMMPS_LITTLE_ENDIAN,
		LAMMPS_BIG_ENDIAN,
		// End of list marker:
		NUMBER_OF_LAMMPS_ENDIAN_TYPES
	};
	LAMMPSEndianess endianess;

	// Parses the LAMMPS dump file header.
	bool parse(QIODevice& input);

	// Parses a 32-bit integer with automatic conversion of endianess depending on current setting.
	int parseInt(QIODevice& input) {
		int val = -1;
		input.read(reinterpret_cast<char*>(&val), sizeof(val));
		if(endianess == LAMMPS_LITTLE_ENDIAN)
			return qFromLittleEndian(val);
		else
			return qFromBigEndian(val);
	}

	// Parses a "big" LAMMPS integer (may be 32 or 64 bit, depending on currently selected data type);
	// We downcast the result value to 32-bit int, because OVITO currently supports only 2^31 atoms anyway.
	// A return value of -1 indicates a number overflow.
	int readBigInt(QIODevice& input) {
		if(dataType == LAMMPS_SMALLSMALL) {
			return parseInt(input);
		}
		else {
			qint64 val;
			input.read(reinterpret_cast<char*>(&val), sizeof(val));
			if(endianess == LAMMPS_LITTLE_ENDIAN)
				val = qFromLittleEndian(val);
			else
				val = qFromBigEndian(val);
			if(val > (qint64)std::numeric_limits<int>::max())
				return -1;
			else
				return val;
		}
	}

	// Parses a floating-point with automatic conversion of endianess depending on current setting.
	double readDouble(QIODevice& input) {
		double val;
		input.read(reinterpret_cast<char*>(&val), sizeof(val));
		if(endianess == LAMMPS_LITTLE_ENDIAN)
			return fromLittleEndian(val);
		else
			return fromBigEndian(val);
	}
};

/******************************************************************************
 * Sets the user-defined mapping between data columns in the input file and
 * the internal particle properties.
 *****************************************************************************/
void LAMMPSBinaryDumpImporter::setColumnMapping(const InputColumnMapping& mapping)
{
	_columnMapping = mapping;

	if(Application::instance()->guiMode()) {
		// Remember the mapping for the next time.
		QSettings settings;
		settings.beginGroup("viz/importer/lammps_binary_dump/");
		settings.setValue("colmapping", mapping.toByteArray());
		settings.endGroup();
	}

	notifyTargetChanged();
}

/******************************************************************************
* Checks if the given file has format that can be read by this importer.
******************************************************************************/
bool LAMMPSBinaryDumpImporter::OOMetaClass::checkFileFormat(QFileDevice& input, const QUrl& sourceLocation) const
{
	// Open input file.
	if(!input.open(QIODevice::ReadOnly))
		return false;

	LAMMPSBinaryDumpHeader header;
	return header.parse(input);
}

/******************************************************************************
* Inspects the header of the given file and returns the number of file columns.
******************************************************************************/
Future<InputColumnMapping> LAMMPSBinaryDumpImporter::inspectFileHeader(const Frame& frame)
{
	// Retrieve file.
	return Application::instance()->fileManager()->fetchUrl(dataset()->container()->taskManager(), frame.sourceFile)
		.then(executor(), [this, frame](const QString& filename) {

			// Start task that inspects the file header to determine the contained data columns.
			FrameLoaderPtr inspectionTask = std::make_shared<FrameLoader>(frame, filename);
			return dataset()->container()->taskManager().runTaskAsync(inspectionTask)
				.then([](const FileSourceImporter::FrameDataPtr& frameData) {
					return static_cast<LAMMPSFrameData*>(frameData.get())->detectedColumnMapping();
				});
		});
}

/******************************************************************************
* Scans the given input file to find all contained simulation frames.
******************************************************************************/
void LAMMPSBinaryDumpImporter::FrameFinder::discoverFramesInFile(QFile& file, const QUrl& sourceUrl, QVector<FileSourceImporter::Frame>& frames)
{
	// Open input file in binary mode for reading.
	if(!file.open(QIODevice::ReadOnly))
		throw Exception(tr("Failed to open binary LAMMPS dump file: %1.").arg(file.errorString()));

	QFileInfo fileInfo(file.fileName());
	QString filename = fileInfo.fileName();
	QDateTime lastModified = fileInfo.lastModified();

	setProgressText(tr("Scanning binary LAMMPS dump file %1").arg(filename));
	setProgressMaximum(file.size());

	while(!file.atEnd() && !isCanceled()) {
		qint64 byteOffset = file.pos();

		// Parse file header.
		LAMMPSBinaryDumpHeader header;
		if(!header.parse(file))
			throw Exception(tr("Failed to read binary LAMMPS dump file: Invalid file header."));

		// Skip particle data.
		qint64 filePos = file.pos();
		for(int chunki = 0; chunki < header.nchunk; chunki++) {

			// Read chunk size.
			int n = header.parseInt(file);
			if(n < 0 || n > header.natoms * header.size_one)
				throw Exception(tr("Invalid data chunk size: %1").arg(n));

			// Skip chunk data.
			filePos += sizeof(n) + n * sizeof(double);
			if(!file.seek(filePos))
				throw Exception(tr("Unexpected end of file."));

			if(!setProgressValue(filePos))
				return;
		}

		// Create a new record for the time step.
		Frame frame;
		frame.sourceFile = sourceUrl;
		frame.byteOffset = byteOffset;
		frame.lastModificationTime = lastModified;
		frame.label = QString("Timestep %1").arg(header.ntimestep);
		frames.push_back(frame);
	}
}

/******************************************************************************
* Parses the file header of a binary LAMMPS dump file.
******************************************************************************/
bool LAMMPSBinaryDumpHeader::parse(QIODevice& input)
{
	// Auto-detection of the LAMMPS data type and architecture used by the dump file:
	// The computer architecture that wrote the file may have been based on little or big endian encoding.
	// Furthermore, LAMMPS may have been configured to use 32-bit or 64-bit integer numbers.
	// We repeatedly try to parse the LAMMPS dump file header with all possible combinations
	// of the data type and endianess settings until we find a combination that
	// leads to reasonable values. These settings will subsequently be used to parse the
	// rest of the dump file.
	qint64 headerPos = input.pos();
	for(int endianessIndex = 0; endianessIndex < NUMBER_OF_LAMMPS_ENDIAN_TYPES; endianessIndex++) {
		endianess = (LAMMPSEndianess)endianessIndex;
		for(int dataTypeIndex = 0; dataTypeIndex < NUMBER_OF_LAMMPS_DATA_TYPES; dataTypeIndex++) {
			dataType = (LAMMPSDataType)dataTypeIndex;
			input.seek(headerPos);

			ntimestep = readBigInt(input);
			if(ntimestep < 0 || input.atEnd()) continue;

			natoms = readBigInt(input);
			if(natoms < 0 || input.atEnd()) continue;

			qint64 startPos = input.pos();

			// Try parsing the new bounding box format first.
			// It starts with the boundary condition flags.
			int triclinic = -1;
			if(input.read(reinterpret_cast<char*>(&triclinic), sizeof(triclinic)) != sizeof(triclinic))
				continue;
			if(input.read(reinterpret_cast<char*>(boundaryFlags), sizeof(boundaryFlags)) != sizeof(boundaryFlags))
				continue;

			bool isValid = true;
			for(int i = 0; i < 3; i++) {
				for(int j = 0; j < 2; j++) {
					if(boundaryFlags[i][j] < 0 || boundaryFlags[i][j] > 3)
						isValid = false;
				}
			}

			if(!isValid) {
				// Try parsing the old bounding box format now.
				input.seek(startPos);
				isValid = true;
				triclinic = -1;
			}

			// Read bounding box.
			for(int i = 0; i < 3; i++) {
				bbox[i][0] = readDouble(input);
				bbox[i][1] = readDouble(input);
				if(bbox[i][0] > bbox[i][1])
					isValid = false;
				for(int j = 0; j < 2; j++) {
					if(!std::isfinite(bbox[i][j]) || bbox[i][j] < -1e9 || bbox[i][j] > 1e9)
						isValid = false;
				}
			}
			if(!isValid || input.atEnd())
				continue;

			// Try parsing shear parameters of triclinic cell.
			if(triclinic != 0) {
				startPos = input.pos();
				if(input.read(reinterpret_cast<char*>(tiltFactors), sizeof(tiltFactors)) != sizeof(tiltFactors))
					continue;
				for(int i = 0; i < 3; i++) {
					if(!std::isfinite(tiltFactors[i]) || tiltFactors[i] < bbox[i][0] - bbox[i][1] || tiltFactors[i] > bbox[i][1] - bbox[i][0])
						isValid = false;
				}
				if(!isValid) {
					input.seek(startPos);
					tiltFactors[0] = tiltFactors[1] = tiltFactors[2] = 0;
				}
			}

			size_one = parseInt(input);
			if(size_one <= 0 || size_one > 40) continue;

			nchunk = parseInt(input);
			if(nchunk <= 0 || nchunk > natoms) continue;

			if(!input.atEnd())
				return true;
		}
	}
	return false;
}

/******************************************************************************
* Parses the given input file.
******************************************************************************/
FileSourceImporter::FrameDataPtr LAMMPSBinaryDumpImporter::FrameLoader::loadFile(QFile& file)
{
	setProgressText(tr("Reading binary LAMMPS dump file %1").arg(frame().sourceFile.toString(QUrl::RemovePassword | QUrl::PreferLocalFile | QUrl::PrettyDecoded)));

	// Open input file for reading.
	if(!file.open(QIODevice::ReadOnly))
		throw Exception(tr("Failed to open binary LAMMPS dump file: %1.").arg(file.errorString()));

	// Seek to byte offset.
	if(frame().byteOffset && !file.seek(frame().byteOffset))
		throw Exception(tr("Failed to read binary LAMMPS dump file: Could not jump to start byte offset."));

	// Parse file header.
	LAMMPSBinaryDumpHeader header;
	if(!header.parse(file))
		throw Exception(tr("Failed to read binary LAMMPS dump file: Invalid file header."));

	// Create the destination container for loaded data.
	auto frameData = std::make_shared<LAMMPSFrameData>();

	if(_parseFileHeaderOnly) {
		// We are done at this point if we are only supposed to detect the
		// number of file columns.
		frameData->detectedColumnMapping().resize(header.size_one);
		return frameData;
	}

	frameData->attributes().insert(QStringLiteral("Timestep"), QVariant::fromValue(header.ntimestep));

	setProgressMaximum(header.natoms);

	// LAMMPS only stores the outer bounding box of the simulation cell in the dump file.
	// We have to determine the size of the actual triclinic cell.
	Box3 simBox;
	simBox.minc = Point3(header.bbox[0][0], header.bbox[1][0], header.bbox[2][0]);
	simBox.maxc = Point3(header.bbox[0][1], header.bbox[1][1], header.bbox[2][1]);
	simBox.minc.x() -= std::min(std::min(std::min(header.tiltFactors[0], header.tiltFactors[1]), header.tiltFactors[0]+header.tiltFactors[1]), 0.0);
	simBox.maxc.x() -= std::max(std::max(std::max(header.tiltFactors[0], header.tiltFactors[1]), header.tiltFactors[0]+header.tiltFactors[1]), 0.0);
	simBox.minc.y() -= std::min(header.tiltFactors[2], 0.0);
	simBox.maxc.y() -= std::max(header.tiltFactors[2], 0.0);
	frameData->simulationCell().setMatrix(AffineTransformation(
			Vector3(simBox.sizeX(), 0, 0),
			Vector3(header.tiltFactors[0], simBox.sizeY(), 0),
			Vector3(header.tiltFactors[1], header.tiltFactors[2], simBox.sizeZ()),
			simBox.minc - Point3::Origin()));
	frameData->simulationCell().setPbcFlags(header.boundaryFlags[0][0] == 0, header.boundaryFlags[1][0] == 0, header.boundaryFlags[2][0] == 0);

	// Parse particle data.
	InputColumnReader columnParser(_columnMapping, *frameData, header.natoms);
	try {
		QVector<double> chunkData;
		int i = 0;
		for(int chunki = 0; chunki < header.nchunk; chunki++) {

			// Read chunk size.
			int n = header.parseInt(file);
			if(n < 0 || n > header.natoms * header.size_one)
				throw Exception(tr("Invalid data chunk size: %1").arg(n));
			if(n == 0)
				continue;

			// Read chunk data.
			chunkData.resize(n);
			if(file.read(reinterpret_cast<char*>(chunkData.data()), n * sizeof(double)) != n * sizeof(double))
				throw Exception(tr("Unexpected end of file."));

			// If necessary, convert endianess of floating-point values.
			if(Q_BYTE_ORDER != (header.endianess == LAMMPSBinaryDumpHeader::LAMMPS_BIG_ENDIAN ? Q_BIG_ENDIAN : Q_LITTLE_ENDIAN)) {
				for(double& d : chunkData) {
#if Q_BYTE_ORDER == Q_BIG_ENDIAN
					d = fromLittleEndian(d);
#else
					d = fromBigEndian(d);
#endif
				}
			}

			const double* iter = chunkData.constData();
			for(int nChunkAtoms = n / header.size_one; nChunkAtoms--; ++i, iter += header.size_one) {

				// Update progress indicator.
				if(!setProgressValueIntermittent(i)) return {};

				try {
					columnParser.readParticle(i, iter, header.size_one);
				}
				catch(Exception& ex) {
					throw ex.prependGeneralMessage(tr("Parsing error in LAMMPS binary dump file."));
				}
			}
		}
	}
	catch(Exception& ex) {
		throw ex.prependGeneralMessage(tr("Parsing error at byte offset %1 of binary LAMMPS dump file.").arg(file.pos()));
	}

	// Sort the particle type list since we created particles on the go and their order depends on the occurrence of types in the file.
	columnParser.sortParticleTypes();

	PropertyPtr posProperty = frameData->findStandardParticleProperty(ParticlesObject::PositionProperty);
	if(posProperty && posProperty->size() > 0) {
		Box3 boundingBox;
		boundingBox.addPoints(posProperty->constDataPoint3(), posProperty->size());

		// Find out if coordinates are given in reduced format and need to be rescaled to absolute format.
		// Check if all atom coordinates are within the [0,1] interval.
		// If yes, we assume reduced coordinate format.

		if(Box3(Point3(-0.01f), Point3(1.01f)).containsBox(boundingBox)) {
			// Convert all atom coordinates from reduced to absolute (Cartesian) format.
			const AffineTransformation simCell = frameData->simulationCell().matrix();
			for(Point3& p : posProperty->point3Range())
				p = simCell * p;
		}
	}

	// Detect if there are more simulation frames following in the file.
	if(!file.atEnd())
		frameData->signalAdditionalFrames();

	// Sort particles by ID.
	if(_sortParticles)
		frameData->sortParticlesById();

	frameData->setStatus(tr("%1 particles at timestep %2").arg(header.natoms).arg(header.ntimestep));
	return frameData;
}

/******************************************************************************
 * Saves the class' contents to the given stream.
 *****************************************************************************/
void LAMMPSBinaryDumpImporter::saveToStream(ObjectSaveStream& stream, bool excludeRecomputableData)
{
	ParticleImporter::saveToStream(stream, excludeRecomputableData);

	stream.beginChunk(0x01);
	_columnMapping.saveToStream(stream);
	stream.endChunk();
}

/******************************************************************************
 * Loads the class' contents from the given stream.
 *****************************************************************************/
void LAMMPSBinaryDumpImporter::loadFromStream(ObjectLoadStream& stream)
{
	ParticleImporter::loadFromStream(stream);

	stream.expectChunk(0x01);
	_columnMapping.loadFromStream(stream);
	stream.closeChunk();
}

/******************************************************************************
 * Creates a copy of this object.
 *****************************************************************************/
OORef<RefTarget> LAMMPSBinaryDumpImporter::clone(bool deepCopy, CloneHelper& cloneHelper) const
{
	// Let the base class create an instance of this class.
	OORef<LAMMPSBinaryDumpImporter> clone = static_object_cast<LAMMPSBinaryDumpImporter>(ParticleImporter::clone(deepCopy, cloneHelper));
	clone->_columnMapping = this->_columnMapping;
	return clone;
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
