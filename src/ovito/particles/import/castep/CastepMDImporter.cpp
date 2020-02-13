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
#include <ovito/core/utilities/io/CompressedTextReader.h>
#include "CastepMDImporter.h"

#include <boost/algorithm/string.hpp>

namespace Ovito { namespace Particles {

IMPLEMENT_OVITO_CLASS(CastepMDImporter);

/******************************************************************************
* Checks if the given file has format that can be read by this importer.
******************************************************************************/
bool CastepMDImporter::OOMetaClass::checkFileFormat(const FileHandle& file) const
{
	// Open input file.
	CompressedTextReader stream(file);

	// Look for string 'BEGIN header' to occur on first line.
	if(!boost::algorithm::istarts_with(stream.readLineTrimLeft(32), "BEGIN header"))
		return false;

	// Look for string 'END header' to occur within the first 50 lines of the file.
	for(int i = 0; i < 50 && !stream.eof(); i++) {
		if(boost::algorithm::istarts_with(stream.readLineTrimLeft(1024), "END header"))
			return true;
	}

	return false;
}

/******************************************************************************
* Scans the data file and builds a list of source frames.
******************************************************************************/
void CastepMDImporter::FrameFinder::discoverFramesInFile(QVector<FileSourceImporter::Frame>& frames)
{
	CompressedTextReader stream(fileHandle());
	setProgressText(tr("Scanning CASTEP file %1").arg(stream.filename()));
	setProgressMaximum(stream.underlyingSize());

	// Look for string 'BEGIN header' to occur on first line.
	if(!boost::algorithm::istarts_with(stream.readLineTrimLeft(32), "BEGIN header"))
		throw Exception(tr("Invalid CASTEP md/geom file header"));

	// Fast forward to line 'END header'.
	for(;;) {
		if(stream.eof())
			throw Exception(tr("Invalid CASTEP md/geom file. Unexpected end of file."));
		if(boost::algorithm::istarts_with(stream.readLineTrimLeft(), "END header"))
			break;
		if(!setProgressValueIntermittent(stream.underlyingByteOffset()))
			return;
	}

	Frame frame(fileHandle());
	QString filename = fileHandle().sourceUrl().fileName();
	int frameNumber = 0;

	while(!stream.eof()) {
		frame.byteOffset = stream.byteOffset();
		frame.lineNumber = stream.lineNumber();
		stream.readLine();
		if(stream.lineEndsWith("<-- h")) {
			frame.label = tr("%1 (Frame %2)").arg(filename).arg(frameNumber++);
			frames.push_back(frame);
			// Skip the two other lines of the cell matrix
			stream.readLine();
			stream.readLine();
		}

		if(!setProgressValueIntermittent(stream.underlyingByteOffset()))
			return;
	}
}

/******************************************************************************
* Parses the given input file.
******************************************************************************/
FileSourceImporter::FrameDataPtr CastepMDImporter::FrameLoader::loadFile()
{
	// Open file for reading.
	CompressedTextReader stream(fileHandle());
	setProgressText(tr("Reading CASTEP file %1").arg(fileHandle().toString()));

	// Jump to byte offset.
	if(frame().byteOffset != 0)
		stream.seek(frame().byteOffset, frame().lineNumber);

	std::vector<Point3> coords;
	std::vector<int> types;
	std::vector<Vector3> velocities;
	std::vector<Vector3> forces;
	std::unique_ptr<ParticleFrameData::TypeList> typeList = std::make_unique<ParticleFrameData::TypeList>();

	// Create the destination container for loaded data.
	auto frameData = std::make_shared<ParticleFrameData>();

	AffineTransformation cell = AffineTransformation::Identity();
	int numCellVectors = 0;

	while(!stream.eof()) {
		const char* line = stream.readLineTrimLeft();

		if(stream.lineEndsWith("<-- h")) {
			if(numCellVectors == 3) break;
			if(sscanf(line, FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING,
					&cell(0,numCellVectors), &cell(1,numCellVectors), &cell(2,numCellVectors)) != 3)
				throw Exception(tr("Invalid simulation cell in CASTEP file at line %1").arg(stream.lineNumber()));
			// Convert units from Bohr to Angstrom.
			cell.column(numCellVectors) *= 0.529177210903;
			numCellVectors++;
		}
		else if(stream.lineEndsWith("<-- R")) {
			Point3 pos;
			if(sscanf(line, "%*s %*u " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING,
					&pos.x(), &pos.y(), &pos.z()) != 3)
				throw Exception(tr("Invalid coordinates in CASTEP file at line %1").arg(stream.lineNumber()));
			// Convert units from Bohr to Angstrom.
			pos *= 0.529177210903;
			coords.push_back(pos);
			const char* typeNameEnd = line;
			while(*typeNameEnd > ' ') typeNameEnd++;
			types.push_back(typeList->addTypeName(line, typeNameEnd));
		}
		else if(stream.lineEndsWith("<-- V")) {
			Vector3 v;
			if(sscanf(line, "%*s %*u " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING,
					&v.x(), &v.y(), &v.z()) != 3)
				throw Exception(tr("Invalid velocity in CASTEP file at line %1").arg(stream.lineNumber()));
			velocities.push_back(v);
		}
		else if(stream.lineEndsWith("<-- F")) {
			Vector3 f;
			if(sscanf(line, "%*s %*u " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING,
					&f.x(), &f.y(), &f.z()) != 3)
				throw Exception(tr("Invalid force in CASTEP file at line %1").arg(stream.lineNumber()));
			forces.push_back(f);
		}

		if(isCanceled())
			return {};
	}
	frameData->simulationCell().setMatrix(cell);

	// Create the particle properties.
	PropertyAccess<Point3> posProperty = frameData->addParticleProperty(ParticlesObject::OOClass().createStandardStorage(coords.size(), ParticlesObject::PositionProperty, false));
	boost::copy(coords, posProperty.begin());

	PropertyAccess<int> typeProperty = frameData->addParticleProperty(ParticlesObject::OOClass().createStandardStorage(types.size(), ParticlesObject::TypeProperty, false));
	boost::copy(types, typeProperty.begin());

	// Since we created particle types on the go while reading the particles, the assigned particle type IDs
	// depend on the storage order of particles in the file. We rather want a well-defined particle type ordering, that's
	// why we sort them now.
	typeList->sortTypesByName(typeProperty);
	frameData->setPropertyTypesList(typeProperty, std::move(typeList));

	if(velocities.size() == coords.size()) {
		PropertyAccess<Vector3> velocityProperty = frameData->addParticleProperty(ParticlesObject::OOClass().createStandardStorage(velocities.size(), ParticlesObject::VelocityProperty, false));
		boost::copy(velocities, velocityProperty.begin());
	}
	if(forces.size() == coords.size()) {
		PropertyAccess<Vector3> forceProperty = frameData->addParticleProperty(ParticlesObject::OOClass().createStandardStorage(forces.size(), ParticlesObject::ForceProperty, false));
		boost::copy(forces, forceProperty.begin());
	}

	frameData->setStatus(tr("%1 atoms").arg(coords.size()));
	return frameData;
}

}	// End of namespace
}	// End of namespace
