///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2017) Alexander Stukowski
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
#include "CastepMDImporter.h"

#include <boost/algorithm/string.hpp>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Import) OVITO_BEGIN_INLINE_NAMESPACE(Formats)

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(CastepMDImporter, ParticleImporter);

/******************************************************************************
* Checks if the given file has format that can be read by this importer.
******************************************************************************/
bool CastepMDImporter::checkFileFormat(QFileDevice& input, const QUrl& sourceLocation)
{
	// Open input file.
	CompressedTextReader stream(input, sourceLocation.path());

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
* Scans the given input file to find all contained simulation frames.
******************************************************************************/
void CastepMDImporter::scanFileForTimesteps(PromiseBase& promise, QVector<FileSourceImporter::Frame>& frames, const QUrl& sourceUrl, CompressedTextReader& stream)
{
	promise.setProgressText(tr("Scanning CASTEP file %1").arg(stream.filename()));
	promise.setProgressMaximum(stream.underlyingSize() / 1000);

	// Look for string 'BEGIN header' to occur on first line.
	if(!boost::algorithm::istarts_with(stream.readLineTrimLeft(32), "BEGIN header"))
		throw Exception(tr("Invalid CASTEP md/geom file header"));

	// Fast forward to line 'END header'.
	for(;;) {
		if(promise.isCanceled())
			return;
		if(stream.eof())
			throw Exception(tr("Invalid CASTEP md/geom file. Unexpected end of file."));
		if(boost::algorithm::istarts_with(stream.readLineTrimLeft(), "END header"))
			break;
		promise.setProgressValueIntermittent(stream.underlyingByteOffset() / 1000);
	}

	QFileInfo fileInfo(stream.device().fileName());
	QString filename = fileInfo.fileName();
	QDateTime lastModified = fileInfo.lastModified();
	int frameNumber = 0;

	while(!stream.eof()) {
		auto byteOffset = stream.byteOffset();
		const char* line = stream.readLineTrimLeft();
		if(boost::algorithm::icontains(line, "<-- h")) {
			Frame frame;
			frame.sourceFile = sourceUrl;
			frame.byteOffset = byteOffset;
			frame.lineNumber = stream.lineNumber();
			frame.lastModificationTime = lastModified;
			frame.label = QString("%1 (Frame %2)").arg(filename).arg(frameNumber++);
			frames.push_back(frame);
			// Skip the two other lines of the cell matrix
			stream.readLine();
			stream.readLine();
		}

		promise.setProgressValueIntermittent(stream.underlyingByteOffset() / 1000);
		if(promise.isCanceled())
			return;
	}
}

/******************************************************************************
* Parses the given input file and stores the data in the given container object.
******************************************************************************/
void CastepMDImporter::ImportTask::parseFile(CompressedTextReader& stream)
{
	setProgressText(tr("Reading CASTEP file %1").arg(frame().sourceFile.toString(QUrl::RemovePassword | QUrl::PreferLocalFile | QUrl::PrettyDecoded)));

	std::vector<Point3> coords;
	std::vector<int> types;
	std::vector<Vector3> velocities;
	std::vector<Vector3> forces;
	std::unique_ptr<ParticleFrameLoader::ParticleTypeList> typeList(new ParticleFrameLoader::ParticleTypeList());

	AffineTransformation cell = AffineTransformation::Identity();
	int numCellVectors = 0;

	while(!stream.eof()) {
		const char* line = stream.readLineTrimLeft();

		if(boost::algorithm::icontains(line, "<-- h")) {
			if(numCellVectors == 3) break;
			if(sscanf(line, FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING, 
					&cell(0,numCellVectors), &cell(1,numCellVectors), &cell(2,numCellVectors)) != 3)
				throw Exception(tr("Invalid simulation cell in CASTEP file at line %1").arg(stream.lineNumber()));
			numCellVectors++;
		}
		else if(boost::algorithm::icontains(line, "<-- r")) {			
			Point3 pos;
			if(sscanf(line, "%*s %*u " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING, 
					&pos.x(), &pos.y(), &pos.z()) != 3)
				throw Exception(tr("Invalid coordinates in CASTEP file at line %1").arg(stream.lineNumber()));			
			coords.push_back(pos);
			const char* typeNameEnd = line;
			while(*typeNameEnd > ' ') typeNameEnd++;
			types.push_back(typeList->addParticleTypeName(line, typeNameEnd));
		}
		else if(boost::algorithm::icontains(line, "<-- v")) {
			Vector3 v;
			if(sscanf(line, "%*s %*u " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING, 
					&v.x(), &v.y(), &v.z()) != 3)
				throw Exception(tr("Invalid velocity in CASTEP file at line %1").arg(stream.lineNumber()));			
			velocities.push_back(v);
		}
		else if(boost::algorithm::icontains(line, "<-- f")) {
			Vector3 f;
			if(sscanf(line, "%*s %*u " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING, 
					&f.x(), &f.y(), &f.z()) != 3)
				throw Exception(tr("Invalid force in CASTEP file at line %1").arg(stream.lineNumber()));			
			forces.push_back(f);
		}

		if(isCanceled())
			return;
	}
	simulationCell().setMatrix(cell);

	// Create the particle properties.
	ParticleProperty* posProperty = new ParticleProperty(coords.size(), ParticleProperty::PositionProperty, 0, false);
	addParticleProperty(posProperty);
	std::copy(coords.begin(), coords.end(), posProperty->dataPoint3());
	
	ParticleProperty* typeProperty = new ParticleProperty(types.size(), ParticleProperty::ParticleTypeProperty, 0, false);
	addParticleProperty(typeProperty, typeList.release());
	std::copy(types.begin(), types.end(), typeProperty->dataInt());

	// Since we created particle types on the go while reading the particles, the assigned particle type IDs
	// depend on the storage order of particles in the file. We rather want a well-defined particle type ordering, that's
	// why we sort them now.
	getTypeListOfParticleProperty(typeProperty)->sortParticleTypesByName(typeProperty);

	if(velocities.size() == coords.size()) {
		ParticleProperty* velocityProperty = new ParticleProperty(velocities.size(), ParticleProperty::VelocityProperty, 0, false);
		addParticleProperty(velocityProperty);
		std::copy(velocities.begin(), velocities.end(), velocityProperty->dataVector3());
	}
	if(forces.size() == coords.size()) {
		ParticleProperty* forceProperty = new ParticleProperty(forces.size(), ParticleProperty::ForceProperty, 0, false);
		addParticleProperty(forceProperty);
		std::copy(forces.begin(), forces.end(), forceProperty->dataVector3());
	}

	setStatus(tr("%1 atoms").arg(coords.size()));
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
