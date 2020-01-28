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

#include <ovito/particles/Particles.h>
#include <ovito/particles/import/ParticleFrameData.h>
#include <ovito/core/app/Application.h>
#include <ovito/core/utilities/io/CompressedTextReader.h>
#include <ovito/core/utilities/io/FileManager.h>
#include "LAMMPSTextDumpImporter.h"

#include <QRegularExpression>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Import) OVITO_BEGIN_INLINE_NAMESPACE(Formats)

IMPLEMENT_OVITO_CLASS(LAMMPSTextDumpImporter);
DEFINE_PROPERTY_FIELD(LAMMPSTextDumpImporter, useCustomColumnMapping);
SET_PROPERTY_FIELD_LABEL(LAMMPSTextDumpImporter, useCustomColumnMapping, "Custom file column mapping");

/******************************************************************************
 * Sets the user-defined mapping between data columns in the input file and
 * the internal particle properties.
 *****************************************************************************/
void LAMMPSTextDumpImporter::setCustomColumnMapping(const InputColumnMapping& mapping)
{
	_customColumnMapping = mapping;
	notifyTargetChanged();
}

/******************************************************************************
* Checks if the given file has format that can be read by this importer.
******************************************************************************/
bool LAMMPSTextDumpImporter::OOMetaClass::checkFileFormat(const FileHandle& file) const
{
	// Open input file.
	CompressedTextReader stream(input, sourceLocation.path());

	// Read first line.
	stream.readLine(15);
	if(stream.lineStartsWith("ITEM: TIMESTEP"))
		return true;

	return false;
}

/******************************************************************************
* Inspects the header of the given file and returns the number of file columns.
******************************************************************************/
Future<InputColumnMapping> LAMMPSTextDumpImporter::inspectFileHeader(const Frame& frame)
{
	// Retrieve file.
	return Application::instance()->fileManager()->fetchUrl(dataset()->container()->taskManager(), frame.sourceFile)
		.then(executor(), [this, frame](const QString& filename) {

			// Start task that inspects the file header to determine the contained data columns.
			activateCLocale();
			FrameLoaderPtr inspectionTask = std::make_shared<FrameLoader>(frame, filename);
			return dataset()->container()->taskManager().runTaskAsync(inspectionTask)
				.then([](const FileSourceImporter::FrameDataPtr& frameData) {
					return static_cast<LAMMPSFrameData*>(frameData.get())->detectedColumnMapping();
				});
		});
}

/******************************************************************************
* Scans the data file and builds a list of source frames.
******************************************************************************/
void LAMMPSTextDumpImporter::FrameFinder::discoverFramesInFile(QVector<FileSourceImporter::Frame>& frames)
{
	CompressedTextReader stream(file, sourceUrl.path());
	setProgressText(tr("Scanning LAMMPS dump file %1").arg(stream.filename()));
	setProgressMaximum(stream.underlyingSize());

	// Regular expression for whitespace characters.
	QRegularExpression ws_re(QStringLiteral("\\s+"));

	int timestep = 0;
	size_t numParticles = 0;
	Frame frame(sourceUrl, file);

	while(!stream.eof() && !isCanceled()) {
		qint64 byteOffset = stream.byteOffset();
		int lineNumber = stream.lineNumber();

		// Parse next line.
		stream.readLine();

		do {
			if(stream.lineStartsWith("ITEM: TIMESTEP")) {
				if(sscanf(stream.readLine(), "%i", &timestep) != 1)
					throw Exception(tr("LAMMPS dump file parsing error. Invalid timestep number (line %1):\n%2").arg(stream.lineNumber()).arg(QString::fromLocal8Bit(stream.line())));
				frame.byteOffset = byteOffset;
				frame.lineNumber = lineNumber;
				frame.label = QString("Timestep %1").arg(timestep);
				frames.push_back(frame);
				break;
			}
			else if(stream.lineStartsWith("ITEM: NUMBER OF ATOMS")) {
				// Parse number of atoms.
				unsigned long long u;
				if(sscanf(stream.readLine(), "%llu", &u) != 1)
					throw Exception(tr("LAMMPS dump file parsing error. Invalid number of atoms in line %1:\n%2").arg(stream.lineNumber()).arg(stream.lineString()));
				if(u > 100'000'000'000ll)
					throw Exception(tr("LAMMPS dump file parsing error. Number of atoms in line %1 is too large. The LAMMPS dump file reader doesn't accept files with more than 100 billion atoms.").arg(stream.lineNumber()));
				numParticles = (size_t)u;
				break;
			}
			else if(stream.lineStartsWith("ITEM: ATOMS")) {
				for(size_t i = 0; i < numParticles; i++) {
					stream.readLine();
					if(!setProgressValueIntermittent(stream.underlyingByteOffset()))
						return;
				}
				break;
			}
			else if(stream.lineStartsWith("ITEM:")) {
				// Skip lines up to next ITEM:
				while(!stream.eof()) {
					byteOffset = stream.byteOffset();
					stream.readLine();
					if(stream.lineStartsWith("ITEM:"))
						break;
				}
			}
			else {
				throw Exception(tr("LAMMPS dump file parsing error. Line %1 of file %2 is invalid.").arg(stream.lineNumber()).arg(stream.filename()));
			}
		}
		while(!stream.eof());
	}
}

/******************************************************************************
* Parses the given input file.
******************************************************************************/
FileSourceImporter::FrameDataPtr LAMMPSTextDumpImporter::FrameLoader::loadFile(QIODevice& file)
{
	// Open file for reading.
	CompressedTextReader stream(file, frame().sourceFile.path());
	setProgressText(tr("Reading LAMMPS dump file %1").arg(frame().sourceFile.toString(QUrl::RemovePassword | QUrl::PreferLocalFile | QUrl::PrettyDecoded)));

	// Jump to byte offset.
	if(frame().byteOffset != 0)
		stream.seek(frame().byteOffset, frame().lineNumber);

	// Create the destination container for loaded data.
	auto frameData = std::make_shared<LAMMPSFrameData>();

	// Regular expression for whitespace characters.
	QRegularExpression ws_re(QStringLiteral("\\s+"));

	int timestep;
	size_t numParticles = 0;

	while(!stream.eof()) {

		// Parse next line.
		stream.readLine();

		do {
			if(stream.lineStartsWith("ITEM: TIMESTEP")) {
				if(sscanf(stream.readLine(), "%i", &timestep) != 1)
					throw Exception(tr("LAMMPS dump file parsing error. Invalid timestep number (line %1):\n%2").arg(stream.lineNumber()).arg(stream.lineString()));
				frameData->attributes().insert(QStringLiteral("Timestep"), QVariant::fromValue(timestep));
				break;
			}
			else if(stream.lineStartsWith("ITEM: NUMBER OF ATOMS")) {
				// Parse number of atoms.
				unsigned long long u;
				if(sscanf(stream.readLine(), "%llu", &u) != 1)
					throw Exception(tr("LAMMPS dump file parsing error. Invalid number of atoms in line %1:\n%2").arg(stream.lineNumber()).arg(stream.lineString()));
				if(u >= 2147483648ull)
					throw Exception(tr("LAMMPS dump file parsing error. Number of atoms in line %1 exceeds internal limit of 2^31 atoms:\n%2").arg(stream.lineNumber()).arg(stream.lineString()));

				numParticles = (size_t)u;
				setProgressMaximum(u);
				break;
			}
			else if(stream.lineStartsWith("ITEM: BOX BOUNDS xy xz yz")) {

				// Parse optional boundary condition flags.
				QStringList tokens = stream.lineString().mid(qstrlen("ITEM: BOX BOUNDS xy xz yz")).split(ws_re, QString::SkipEmptyParts);
				if(tokens.size() >= 3)
					frameData->simulationCell().setPbcFlags(tokens[0] == "pp", tokens[1] == "pp", tokens[2] == "pp");

				// Parse triclinic simulation box.
				FloatType tiltFactors[3];
				Box3 simBox;
				for(int k = 0; k < 3; k++) {
					if(sscanf(stream.readLine(), FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING, &simBox.minc[k], &simBox.maxc[k], &tiltFactors[k]) != 3)
						throw Exception(tr("Invalid box size in line %1 of LAMMPS dump file: %2").arg(stream.lineNumber()).arg(stream.lineString()));
				}

				// LAMMPS only stores the outer bounding box of the simulation cell in the dump file.
				// We have to determine the size of the actual triclinic cell.
				simBox.minc.x() -= std::min(std::min(std::min(tiltFactors[0], tiltFactors[1]), tiltFactors[0]+tiltFactors[1]), (FloatType)0);
				simBox.maxc.x() -= std::max(std::max(std::max(tiltFactors[0], tiltFactors[1]), tiltFactors[0]+tiltFactors[1]), (FloatType)0);
				simBox.minc.y() -= std::min(tiltFactors[2], (FloatType)0);
				simBox.maxc.y() -= std::max(tiltFactors[2], (FloatType)0);
				frameData->simulationCell().setMatrix(AffineTransformation(
						Vector3(simBox.sizeX(), 0, 0),
						Vector3(tiltFactors[0], simBox.sizeY(), 0),
						Vector3(tiltFactors[1], tiltFactors[2], simBox.sizeZ()),
						simBox.minc - Point3::Origin()));
				break;
			}
			else if(stream.lineStartsWith("ITEM: BOX BOUNDS")) {
				// Parse optional boundary condition flags.
				QStringList tokens = stream.lineString().mid(qstrlen("ITEM: BOX BOUNDS")).split(ws_re, QString::SkipEmptyParts);
				if(tokens.size() >= 3)
					frameData->simulationCell().setPbcFlags(tokens[0] == "pp", tokens[1] == "pp", tokens[2] == "pp");

				// Parse orthogonal simulation box size.
				Box3 simBox;
				for(int k = 0; k < 3; k++) {
					if(sscanf(stream.readLine(), FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING, &simBox.minc[k], &simBox.maxc[k]) != 2)
						throw Exception(tr("Invalid box size in line %1 of dump file: %2").arg(stream.lineNumber()).arg(stream.lineString()));
				}

				frameData->simulationCell().setMatrix(AffineTransformation(
						Vector3(simBox.sizeX(), 0, 0),
						Vector3(0, simBox.sizeY(), 0),
						Vector3(0, 0, simBox.sizeZ()),
						simBox.minc - Point3::Origin()));
				break;
			}
			else if(stream.lineStartsWith("ITEM: ATOMS")) {

				// Read the column names list.
				QStringList tokens = stream.lineString().split(ws_re, QString::SkipEmptyParts);
				OVITO_ASSERT(tokens[0] == "ITEM:" && tokens[1] == "ATOMS");
				QStringList fileColumnNames = tokens.mid(2);

				// Stop here if we are only inspecting the file's header.
				if(_parseFileHeaderOnly) {
					if(fileColumnNames.isEmpty()) {
						// If no file columns names are available, count at least the number
						// of data columns.
						stream.readLine();
						int columnCount = stream.lineString().split(ws_re, QString::SkipEmptyParts).size();
						frameData->detectedColumnMapping().resize(columnCount);
					}
					else {
						frameData->detectedColumnMapping() = generateAutomaticColumnMapping(fileColumnNames);
					}
					return frameData;
				}

				// Set up column-to-property mapping.
				InputColumnMapping columnMapping;
				if(_useCustomColumnMapping)
					columnMapping = _customColumnMapping;
				else
					columnMapping = generateAutomaticColumnMapping(fileColumnNames);

				// Parse data columns.
				InputColumnReader columnParser(columnMapping, *frameData, numParticles);

				// If possible, use memory-mapped file access for best performance.
				const char* s_start;
				const char* s_end;
				std::tie(s_start, s_end) = stream.mmap();
				auto s = s_start;
				int lineNumber = stream.lineNumber() + 1;
				try {
					for(size_t i = 0; i < numParticles; i++, lineNumber++) {
						if(!setProgressValueIntermittent(i)) return {};
						if(!s)
							columnParser.readParticle(i, stream.readLine());
						else
							s = columnParser.readParticle(i, s, s_end);
					}
				}
				catch(Exception& ex) {
					throw ex.prependGeneralMessage(tr("Parsing error in line %1 of LAMMPS dump file.").arg(lineNumber));
				}
				if(s) {
					stream.munmap();
					stream.seek(stream.byteOffset() + (s - s_start));
				}

				// Sort the particle type list since we created particles on the go and their order depends on the occurrence of types in the file.
				columnParser.sortParticleTypes();

				// Find out if coordinates are given in reduced format and need to be rescaled to absolute format.
				bool reducedCoordinates = false;
				if(!fileColumnNames.empty()) {
					for(int i = 0; i < (int)columnMapping.size() && i < fileColumnNames.size(); i++) {
						if(columnMapping[i].property.type() == ParticlesObject::PositionProperty) {
							reducedCoordinates = (
									fileColumnNames[i] == "xs" || fileColumnNames[i] == "xsu" ||
									fileColumnNames[i] == "ys" || fileColumnNames[i] == "ysu" ||
									fileColumnNames[i] == "zs" || fileColumnNames[i] == "zsu");
							// break; Note: Do not stop the loop here, because the 'Position' particle 
							// property may be associated with several file columns, and it's the last column that 
							// ends up getting imported into OVITO. 
						}
					}
				}
				else {
					// Check if all atom coordinates are within the [0,1] interval.
					// If yes, we assume reduced coordinate format.
					if(ConstPropertyAccess<Point3> posProperty = frameData->findStandardParticleProperty(ParticlesObject::PositionProperty)) {
						Box3 boundingBox;
						boundingBox.addPoints(posProperty);
						if(Box3(Point3(FloatType(-0.02)), Point3(FloatType(1.02))).containsBox(boundingBox))
							reducedCoordinates = true;
					}
				}

				if(reducedCoordinates) {
					// Convert all atom coordinates from reduced to absolute (Cartesian) format.
					if(PropertyAccess<Point3> posProperty = frameData->findStandardParticleProperty(ParticlesObject::PositionProperty)) {
						const AffineTransformation simCell = frameData->simulationCell().matrix();
						for(Point3& p : posProperty)
							p = simCell * p;
					}
				}

				// Detect dimensionality of system.
				frameData->simulationCell().set2D(!columnMapping.hasZCoordinates());

				// Detect if there are more simulation frames following in the file.
				if(!stream.eof()) {
					stream.readLine();
					if(stream.lineStartsWith("ITEM: TIMESTEP"))
						frameData->signalAdditionalFrames();
				}

				// Sort particles by ID.
				if(_sortParticles)
					frameData->sortParticlesById();

				frameData->setStatus(tr("%1 particles at timestep %2").arg(numParticles).arg(timestep));
				return frameData; // Done!
			}
			else if(stream.lineStartsWith("ITEM:")) {
				// For the sake of forward compatibility, we ignore unknown ITEM sections.
				// Skip lines until the next "ITEM:" is reached.
				while(!stream.eof() && !isCanceled()) {
					stream.readLine();
					if(stream.lineStartsWith("ITEM:"))
						break;
				}
			}
			else {
				throw Exception(tr("LAMMPS dump file parsing error. Line %1 of file %2 is invalid.").arg(stream.lineNumber()).arg(stream.filename()));
			}
		}
		while(!stream.eof());
	}

	throw Exception(tr("LAMMPS dump file parsing error. Unexpected end of file at line %1 or \"ITEM: ATOMS\" section is not present in dump file.").arg(stream.lineNumber()));
}

/******************************************************************************
 * Guesses the mapping of input file columns to internal particle properties.
 *****************************************************************************/
InputColumnMapping LAMMPSTextDumpImporter::generateAutomaticColumnMapping(const QStringList& columnNames)
{
	InputColumnMapping columnMapping;
	columnMapping.resize(columnNames.size());
	for(int i = 0; i < columnNames.size(); i++) {
		QString name = columnNames[i].toLower();
		columnMapping[i].columnName = columnNames[i];
		if(name == "x" || name == "xu" || name == "coordinates") columnMapping[i].mapStandardColumn(ParticlesObject::PositionProperty, 0);
		else if(name == "y" || name == "yu") columnMapping[i].mapStandardColumn(ParticlesObject::PositionProperty, 1);
		else if(name == "z" || name == "zu") columnMapping[i].mapStandardColumn(ParticlesObject::PositionProperty, 2);
		else if(name == "xs" || name == "xsu") { columnMapping[i].mapStandardColumn(ParticlesObject::PositionProperty, 0); }
		else if(name == "ys" || name == "ysu") { columnMapping[i].mapStandardColumn(ParticlesObject::PositionProperty, 1); }
		else if(name == "zs" || name == "zsu") { columnMapping[i].mapStandardColumn(ParticlesObject::PositionProperty, 2); }
		else if(name == "vx" || name == "velocities") columnMapping[i].mapStandardColumn(ParticlesObject::VelocityProperty, 0);
		else if(name == "vy") columnMapping[i].mapStandardColumn(ParticlesObject::VelocityProperty, 1);
		else if(name == "vz") columnMapping[i].mapStandardColumn(ParticlesObject::VelocityProperty, 2);
		else if(name == "id") columnMapping[i].mapStandardColumn(ParticlesObject::IdentifierProperty);
		else if(name == "type" || name == "element" || name == "atom_types") columnMapping[i].mapStandardColumn(ParticlesObject::TypeProperty);
		else if(name == "mass") columnMapping[i].mapStandardColumn(ParticlesObject::MassProperty);
		else if(name == "radius") columnMapping[i].mapStandardColumn(ParticlesObject::RadiusProperty);
		else if(name == "mol") columnMapping[i].mapStandardColumn(ParticlesObject::MoleculeProperty);
		else if(name == "q") columnMapping[i].mapStandardColumn(ParticlesObject::ChargeProperty);
		else if(name == "ix") columnMapping[i].mapStandardColumn(ParticlesObject::PeriodicImageProperty, 0);
		else if(name == "iy") columnMapping[i].mapStandardColumn(ParticlesObject::PeriodicImageProperty, 1);
		else if(name == "iz") columnMapping[i].mapStandardColumn(ParticlesObject::PeriodicImageProperty, 2);
		else if(name == "fx" || name == "forces") columnMapping[i].mapStandardColumn(ParticlesObject::ForceProperty, 0);
		else if(name == "fy") columnMapping[i].mapStandardColumn(ParticlesObject::ForceProperty, 1);
		else if(name == "fz") columnMapping[i].mapStandardColumn(ParticlesObject::ForceProperty, 2);
		else if(name == "mux") columnMapping[i].mapStandardColumn(ParticlesObject::DipoleOrientationProperty, 0);
		else if(name == "muy") columnMapping[i].mapStandardColumn(ParticlesObject::DipoleOrientationProperty, 1);
		else if(name == "muz") columnMapping[i].mapStandardColumn(ParticlesObject::DipoleOrientationProperty, 2);
		else if(name == "mu") columnMapping[i].mapStandardColumn(ParticlesObject::DipoleMagnitudeProperty);
		else if(name == "omegax") columnMapping[i].mapStandardColumn(ParticlesObject::AngularVelocityProperty, 0);
		else if(name == "omegay") columnMapping[i].mapStandardColumn(ParticlesObject::AngularVelocityProperty, 1);
		else if(name == "omegaz") columnMapping[i].mapStandardColumn(ParticlesObject::AngularVelocityProperty, 2);
		else if(name == "angmomx") columnMapping[i].mapStandardColumn(ParticlesObject::AngularMomentumProperty, 0);
		else if(name == "angmomy") columnMapping[i].mapStandardColumn(ParticlesObject::AngularMomentumProperty, 1);
		else if(name == "angmomz") columnMapping[i].mapStandardColumn(ParticlesObject::AngularMomentumProperty, 2);
		else if(name == "tqx") columnMapping[i].mapStandardColumn(ParticlesObject::TorqueProperty, 0);
		else if(name == "tqy") columnMapping[i].mapStandardColumn(ParticlesObject::TorqueProperty, 1);
		else if(name == "tqz") columnMapping[i].mapStandardColumn(ParticlesObject::TorqueProperty, 2);
		else if(name == "spin") columnMapping[i].mapStandardColumn(ParticlesObject::SpinProperty);
		else if(name == "c_cna" || name == "pattern") columnMapping[i].mapStandardColumn(ParticlesObject::StructureTypeProperty);
		else if(name == "c_epot") columnMapping[i].mapStandardColumn(ParticlesObject::PotentialEnergyProperty);
		else if(name == "c_kpot") columnMapping[i].mapStandardColumn(ParticlesObject::KineticEnergyProperty);
		else if(name == "c_stress[1]") columnMapping[i].mapStandardColumn(ParticlesObject::StressTensorProperty, 0);
		else if(name == "c_stress[2]") columnMapping[i].mapStandardColumn(ParticlesObject::StressTensorProperty, 1);
		else if(name == "c_stress[3]") columnMapping[i].mapStandardColumn(ParticlesObject::StressTensorProperty, 2);
		else if(name == "c_stress[4]") columnMapping[i].mapStandardColumn(ParticlesObject::StressTensorProperty, 3);
		else if(name == "c_stress[5]") columnMapping[i].mapStandardColumn(ParticlesObject::StressTensorProperty, 4);
		else if(name == "c_stress[6]") columnMapping[i].mapStandardColumn(ParticlesObject::StressTensorProperty, 5);
		else if(name == "c_orient[1]") columnMapping[i].mapStandardColumn(ParticlesObject::OrientationProperty, 0);
		else if(name == "c_orient[2]") columnMapping[i].mapStandardColumn(ParticlesObject::OrientationProperty, 1);
		else if(name == "c_orient[3]") columnMapping[i].mapStandardColumn(ParticlesObject::OrientationProperty, 2);
		else if(name == "c_orient[4]") columnMapping[i].mapStandardColumn(ParticlesObject::OrientationProperty, 3);
		else if(name == "c_shape[1]") columnMapping[i].mapStandardColumn(ParticlesObject::AsphericalShapeProperty, 0);
		else if(name == "c_shape[2]") columnMapping[i].mapStandardColumn(ParticlesObject::AsphericalShapeProperty, 1);
		else if(name == "c_shape[3]") columnMapping[i].mapStandardColumn(ParticlesObject::AsphericalShapeProperty, 2);
		else if(name == "selection") columnMapping[i].mapStandardColumn(ParticlesObject::SelectionProperty, 0);
		else {
			columnMapping[i].mapCustomColumn(name, PropertyStorage::Float);
		}
	}
	return columnMapping;
}

/******************************************************************************
 * Saves the class' contents to the given stream.
 *****************************************************************************/
void LAMMPSTextDumpImporter::saveToStream(ObjectSaveStream& stream, bool excludeRecomputableData)
{
	ParticleImporter::saveToStream(stream, excludeRecomputableData);

	stream.beginChunk(0x01);
	_customColumnMapping.saveToStream(stream);
	stream.endChunk();
}

/******************************************************************************
 * Loads the class' contents from the given stream.
 *****************************************************************************/
void LAMMPSTextDumpImporter::loadFromStream(ObjectLoadStream& stream)
{
	ParticleImporter::loadFromStream(stream);

	stream.expectChunk(0x01);
	_customColumnMapping.loadFromStream(stream);
	stream.closeChunk();
}

/******************************************************************************
 * Creates a copy of this object.
 *****************************************************************************/
OORef<RefTarget> LAMMPSTextDumpImporter::clone(bool deepCopy, CloneHelper& cloneHelper) const
{
	// Let the base class create an instance of this class.
	OORef<LAMMPSTextDumpImporter> clone = static_object_cast<LAMMPSTextDumpImporter>(ParticleImporter::clone(deepCopy, cloneHelper));
	clone->_customColumnMapping = this->_customColumnMapping;
	return clone;
}


OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
