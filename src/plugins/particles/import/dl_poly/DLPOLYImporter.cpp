///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2018) Alexander Stukowski
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

#include <plugins/particles/Particles.h>
#include <plugins/particles/import/InputColumnMapping.h>
#include <plugins/particles/import/ParticleFrameData.h>
#include <core/utilities/io/CompressedTextReader.h>
#include "DLPOLYImporter.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Import) OVITO_BEGIN_INLINE_NAMESPACE(Formats)

IMPLEMENT_OVITO_CLASS(DLPOLYImporter);
	
/******************************************************************************
* Checks if the given file has format that can be read by this importer.
******************************************************************************/
bool DLPOLYImporter::OOMetaClass::checkFileFormat(QFileDevice& input, const QUrl& sourceLocation) const
{
	// Open input file.
	CompressedTextReader stream(input, sourceLocation.path());

	// Ignore first comment line (record 1).
	stream.readLine(1024);

	// Parse second line (record 2).
	int levcfg;
	int imcon;
	if(stream.eof() || sscanf(stream.readLine(256), "%u %u", &levcfg, &imcon) != 2 || levcfg < 0 || levcfg > 2 || imcon < 0 || imcon > 6)
		return false;

	// Skip "timestep" record (if any).
	stream.readLine();
	if(stream.lineStartsWith("timestep"))
		stream.readLine();

	// Parse cell matrix (records 3-5, only when periodic boundary conditions are used)
	if(imcon != 0) {
		for(int i = 0; i < 3; i++) {
			char c;
			double x,y,z;
			if(sscanf(stream.line(), "%lg %lg %lg %c", &x, &y, &z, &c) != 3 || stream.eof())
				return false;
			stream.readLine();
		}
	}

	// Parse first atom record.
	double d;
	if(stream.eof() || sscanf(stream.line(), "%lg", &d) != 0) // Expect the line to start with a non-number!
		return false;
	char c;
	// Parse atomic coordinates.
	double x,y,z;
	if(sscanf(stream.readLine(), "%lg %lg %lg %c", &x, &y, &z, &c) != 3 || stream.eof())
		return false;
	// Parse atomic velocity vector.
	if(levcfg > 0) {
		if(sscanf(stream.readLine(), "%lg %lg %lg %c", &x, &y, &z, &c) != 3 || stream.eof())
			return false;
	}
	// Parse atomic force vector.
	if(levcfg > 1) {
		if(sscanf(stream.readLine(), "%lg %lg %lg %c", &x, &y, &z, &c) != 3 || stream.eof())
			return false;
	}

	return true;
}


/******************************************************************************
* Scans the given input file to find all contained simulation frames.
******************************************************************************/
void DLPOLYImporter::FrameFinder::discoverFramesInFile(QFile& file, const QUrl& sourceUrl, QVector<FileSourceImporter::Frame>& frames)
{
	CompressedTextReader stream(file, sourceUrl.path());
	setProgressText(tr("Scanning DL_POLY file %1").arg(stream.filename()));
	setProgressMaximum(stream.underlyingSize());

	// Skip first comment line (record 1).
	stream.readLine();

	// Parse second line (record 2).
	int levcfg;
	int imcon;
	qlonglong expectedAtomCount = -1;
	int frame_count = -1;
	if(stream.eof() || sscanf(stream.readLine(), "%u %u %llu %u", &levcfg, &imcon, &expectedAtomCount, &frame_count) < 2 || levcfg < 0 || levcfg > 2 || imcon < 0 || imcon > 6)
		throw Exception(tr("Invalid record line %1 in DL_POLY file: %2").arg(stream.lineNumber()).arg(stream.lineString()));

	QFileInfo fileInfo(stream.device().fileName());
	QString filename = fileInfo.fileName();
	QDateTime lastModified = fileInfo.lastModified();
	auto byteOffset = stream.byteOffset();
	auto lineNumber = stream.lineNumber();

	// Look for "timestep" record.
	stream.readLine();
	if(stream.lineStartsWith("timestep")) {
		if(expectedAtomCount <= 0)
			throw Exception(tr("Invalid number of atoms in line %1 of DL_POLY file.").arg(stream.lineNumber()-1));
		if(frame_count <= 0)
			throw Exception(tr("Invalid frame count in line %1 of DL_POLY file.").arg(stream.lineNumber()-1));

		for(int frameIndex = 0; frameIndex < frame_count; frameIndex++) {
			if(frameIndex != 0) {
				byteOffset = stream.byteOffset();
				lineNumber = stream.lineNumber();
				stream.readLine();
			}
			Frame frame;
			frame.sourceFile = sourceUrl;
			frame.byteOffset = byteOffset;
			frame.lineNumber = lineNumber;
			frame.lastModificationTime = lastModified;
			int nstep;
			qlonglong megatm;
			int keytrj;
			double tstep;
			double ttime;
			if(sscanf(stream.line(), "timestep %u %llu %i %i %lg %lg", &nstep, &megatm, &keytrj, &imcon, &tstep, &ttime) != 6 || megatm != expectedAtomCount)
				throw Exception(tr("Invalid timestep record in line %1 of DL_POLY file: %2").arg(stream.lineNumber()).arg(stream.lineString()));
			frame.label = QStringLiteral("Time: %1 ps").arg(ttime);
			frames.push_back(frame);

			// Skip simulation cell.
			if(imcon != 0) {
				for(int j = 0; j < 3; j++) {
					stream.readLine();
				}
			}

			// Skip the right number of atom lines.
			int nLinesPerAtom = 2;
			if(keytrj > 0) nLinesPerAtom++;
			if(keytrj > 1) nLinesPerAtom++;
			for(qlonglong i = 0; i < expectedAtomCount; i++) {
				for(int j = 0; j < nLinesPerAtom; j++) {
					stream.readLine();
				}
				if((i % 1024) == 0 && !setProgressValue(stream.underlyingByteOffset()))
					return;
			}
		}
	}
	else {
		// It's not a trajectory file. Report just a single frame.
		Frame frame;
		frame.sourceFile = sourceUrl;
		frame.byteOffset = 0;
		frame.lineNumber = 0;
		frame.lastModificationTime = lastModified;
		frames.push_back(frame);
	}
}

/******************************************************************************
* Parses the given input file.
******************************************************************************/
FileSourceImporter::FrameDataPtr DLPOLYImporter::FrameLoader::loadFile(QFile& file)
{
	// Open file for reading.
	CompressedTextReader stream(file, frame().sourceFile.path());
	setProgressText(tr("Reading DL_POLY file %1").arg(frame().sourceFile.toString(QUrl::RemovePassword | QUrl::PreferLocalFile | QUrl::PrettyDecoded)));
	setProgressMaximum(stream.underlyingSize());

	// Create the destination container for loaded data.
	auto frameData = std::make_shared<ParticleFrameData>();

	// Read first comment line (record 1).
	stream.readLine(1024);
	QString trimmedComment = stream.lineString().trimmed();
	if(!trimmedComment.isEmpty())
		frameData->attributes().insert(QStringLiteral("Comment"), QVariant::fromValue(trimmedComment));

	// Parse second line (record 2).
	int levcfg;
	int imcon;
	qlonglong expectedAtomCount = -1;
	if(stream.eof() || sscanf(stream.readLine(256), "%u %u %llu", &levcfg, &imcon, &expectedAtomCount) < 2 || levcfg < 0 || levcfg > 2 || imcon < 0 || imcon > 6)
		throw Exception(tr("Invalid record line %1 in DL_POLY file: %2").arg(stream.lineNumber()).arg(stream.lineString()));

	if(imcon == 0) frameData->simulationCell().setPbcFlags(false, false, false);
	else if(imcon == 1 || imcon == 2 || imcon == 3) frameData->simulationCell().setPbcFlags(true, true, true);
	else if(imcon == 6) frameData->simulationCell().setPbcFlags(true, true, false);
	else throw Exception(tr("Invalid boundary condition type in line %1 of DL_POLY file: %2").arg(stream.lineNumber()).arg(stream.lineString()));

	// Jump to byte offset.
	if(frame().byteOffset != 0)
		stream.seek(frame().byteOffset, frame().lineNumber);

	// Parse "timestep" record (if any).
	stream.readLine();
	if(stream.lineStartsWith("timestep")) {
		int nstep;
		qlonglong megatm;
		int keytrj;
		double tstep;
		double ttime;
		if(sscanf(stream.line(), "timestep %u %llu %i %i %lg %lg", &nstep, &megatm, &keytrj, &imcon, &tstep, &ttime) != 6 || megatm != expectedAtomCount)
			throw Exception(tr("Invalid timestep record in line %1 of DL_POLY file: %2").arg(stream.lineNumber()).arg(stream.lineString()));
		frameData->attributes().insert(QStringLiteral("IntegrationTimestep"), QVariant::fromValue(tstep));
		frameData->attributes().insert(QStringLiteral("Time"), QVariant::fromValue(ttime));
		stream.readLine();
	}

	// Parse cell matrix (records 3-5, only when periodic boundary conditions are used)
	if(imcon != 0) {
		AffineTransformation cell = AffineTransformation::Identity();
		for(size_t i = 0; i < 3; i++) {
			if(sscanf(stream.line(),
					FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING,
					&cell(0,i), &cell(1,i), &cell(2,i)) != 3 || cell.column(i) == Vector3::Zero())
				throw Exception(tr("Invalid cell vector in line %1 of DL_POLY file: %2").arg(stream.lineNumber()).arg(stream.lineString()));
			stream.readLine();
		}
		cell.column(3) = cell * Vector3(-0.5, -0.5, -0.5);
		frameData->simulationCell().setMatrix(cell);
	}

	// The temporary buffers for the atom records.
	std::vector<qlonglong> identifiers;
	std::vector<int> atom_types;
	std::vector<Point3> positions;
	std::vector<Vector3> velocities;
	std::vector<Vector3> forces;
	std::vector<FloatType> masses;
	std::vector<FloatType> charges;
	std::vector<FloatType> displacementMagnitudes;

	// Create particle type property now, because we need to populate the type list while parsing.
	PropertyPtr typeProperty = ParticlesObject::OOClass().createStandardStorage(0, ParticlesObject::TypeProperty, false);
	frameData->addParticleProperty(typeProperty);
	ParticleFrameData::TypeList* typeList = frameData->propertyTypesList(typeProperty);

	// Parse atoms.
	do {
		// Report progress. 
		if(isCanceled()) return {};
		if((positions.size() % 1024) == 0)
			setProgressValueIntermittent(stream.underlyingByteOffset());

		// Parse first line of atom record.
		if(!positions.empty()) stream.readLine();
		const char* line = stream.line();
		while(*line != '\0' && *line <= ' ') ++line;
		double d;
		if(sscanf(line, "%lg", &d) != 0) // Expect the line to start with a non-number!
			throw Exception(tr("Invalid atom type specification in line %1 of DL_POLY file: %2").arg(stream.lineNumber()).arg(stream.lineString()));

		// Parse atom type name.
		const char* line_end = line;
		while(*line_end != '\0' && *line_end > ' ') ++line_end;
		atom_types.push_back(typeList->addTypeName(line, line_end));

		// Parse atom identifier and other info (optional).
		if(*line_end != '\0') {
			identifiers.emplace_back();
			FloatType mass, charge, displ;
			int fieldCount;
			if((fieldCount = sscanf(line_end, "%llu " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING, &identifiers.back(), &mass, &charge, &displ)) < 1)
				throw Exception(tr("Invalid atom identifier field in line %1 of DL_POLY file: %2").arg(stream.lineNumber()).arg(stream.lineString()));
			if(fieldCount == 4) {
				masses.push_back(mass);
				charges.push_back(charge);
				displacementMagnitudes.push_back(displ);
			}
		}

		// Parse atomic coordinates.
		char c;
		positions.emplace_back();
		Point3& pos = positions.back();
		if(sscanf(stream.readLine(), FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " %c", &pos.x(), &pos.y(), &pos.z(), &c) != 3)
			throw Exception(tr("Invalid atom coordinate triplet in line %1 of DL_POLY file: %2").arg(stream.lineNumber()).arg(stream.lineString()));

		if(levcfg > 0) {
			// Parse atomic velocity vector.
			velocities.emplace_back();
			Vector3& vel = velocities.back();
			if(sscanf(stream.readLine(), FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " %c", &vel.x(), &vel.y(), &vel.z(), &c) != 3)
				throw Exception(tr("Invalid atomic velocity vector in line %1 of DL_POLY file: %2").arg(stream.lineNumber()).arg(stream.lineString()));
		}

		if(levcfg > 1) {
			// Parse atomic force vector.
			forces.emplace_back();
			Vector3& force = forces.back();
			if(sscanf(stream.readLine(), FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " %c", &force.x(), &force.y(), &force.z(), &c) != 3)
				throw Exception(tr("Invalid atomic force vector in line %1 of DL_POLY file: %2").arg(stream.lineNumber()).arg(stream.lineString()));
		}

		if(positions.size() == expectedAtomCount)
			break;
	}
	while(!stream.eof());

	// Make sure the number of atoms specified in the header was correct.
	if(expectedAtomCount > 0 && positions.size() < expectedAtomCount)
		throw Exception(tr("Unexpected end of DL_POLY file. Expected %1 atom records but found only %2.").arg(expectedAtomCount).arg(positions.size()));

	// Create particle properties.
	PropertyPtr posProperty = ParticlesObject::OOClass().createStandardStorage(positions.size(), ParticlesObject::PositionProperty, false);
	frameData->addParticleProperty(posProperty);
	std::copy(positions.cbegin(), positions.cend(), posProperty->dataPoint3());
	typeProperty->resize(atom_types.size(), false);
	std::copy(atom_types.cbegin(), atom_types.cend(), typeProperty->dataInt());
	if(identifiers.size() == positions.size()) {
		PropertyPtr identifierProperty = ParticlesObject::OOClass().createStandardStorage(identifiers.size(), ParticlesObject::IdentifierProperty, false);
		frameData->addParticleProperty(identifierProperty);
		std::copy(identifiers.cbegin(), identifiers.cend(), identifierProperty->dataInt64());
	}
	if(levcfg > 0) {
		PropertyPtr velocityProperty = ParticlesObject::OOClass().createStandardStorage(velocities.size(), ParticlesObject::VelocityProperty, false);
		frameData->addParticleProperty(velocityProperty);
		std::copy(velocities.cbegin(), velocities.cend(), velocityProperty->dataVector3());
	}
	if(levcfg > 1) {
		PropertyPtr forceProperty = ParticlesObject::OOClass().createStandardStorage(forces.size(), ParticlesObject::ForceProperty, false);
		frameData->addParticleProperty(forceProperty);
		std::copy(forces.cbegin(), forces.cend(), forceProperty->dataVector3());
	}
	if(masses.size() == positions.size()) {
		PropertyPtr massProperty = ParticlesObject::OOClass().createStandardStorage(masses.size(), ParticlesObject::MassProperty, false);
		frameData->addParticleProperty(massProperty);
		std::copy(masses.cbegin(), masses.cend(), massProperty->dataFloat());
	}
	if(charges.size() == positions.size()) {
		PropertyPtr chargeProperty = ParticlesObject::OOClass().createStandardStorage(charges.size(), ParticlesObject::ChargeProperty, false);
		frameData->addParticleProperty(chargeProperty);
		std::copy(charges.cbegin(), charges.cend(), chargeProperty->dataFloat());
	}
	if(displacementMagnitudes.size() == positions.size()) {
		PropertyPtr displProperty = ParticlesObject::OOClass().createStandardStorage(displacementMagnitudes.size(), ParticlesObject::DisplacementMagnitudeProperty, false);
		frameData->addParticleProperty(displProperty);
		std::copy(displacementMagnitudes.cbegin(), displacementMagnitudes.cend(), displProperty->dataFloat());
	}

	// Since we created particle types on the go while reading the particles, the assigned particle type IDs
	// depend on the storage order of particles in the file. We rather want a well-defined particle type ordering, that's
	// why we sort them now.
	typeList->sortTypesByName(typeProperty);	

	// Sort particles by ID if requested.
	if(_sortParticles)
		frameData->sortParticlesById();

	frameData->setStatus(tr("Number of particles: %1").arg(positions.size()));
	return frameData;
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
