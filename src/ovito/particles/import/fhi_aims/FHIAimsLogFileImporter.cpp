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
#include "FHIAimsLogFileImporter.h"

#include <boost/algorithm/string.hpp>
#include <QRegularExpression>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Import) OVITO_BEGIN_INLINE_NAMESPACE(Formats)

IMPLEMENT_OVITO_CLASS(FHIAimsLogFileImporter);

/******************************************************************************
* Checks if the given file has format that can be read by this importer.
******************************************************************************/
bool FHIAimsLogFileImporter::OOMetaClass::checkFileFormat(const FileHandle& file) const
{
	// Open input file.
	CompressedTextReader stream(input, sourceLocation.path());

	// Look for 'Invoking FHI-aims' message.
	// It must appear within the first 20 lines of the file.
	for(int i = 0; i < 20 && !stream.eof(); i++) {
		const char* line = stream.readLineTrimLeft(128);
		if(boost::algorithm::starts_with(line, "Invoking FHI-aims")) {
			return true;
		}
	}
	return false;
}

/******************************************************************************
* Scans the data file and builds a list of source frames.
******************************************************************************/
void FHIAimsLogFileImporter::FrameFinder::discoverFramesInFile(QVector<FileSourceImporter::Frame>& frames)
{
	CompressedTextReader stream(file, sourceUrl.path());
	setProgressText(tr("Scanning file %1").arg(sourceUrl.toString(QUrl::RemovePassword | QUrl::PreferLocalFile | QUrl::PrettyDecoded)));
	setProgressMaximum(stream.underlyingSize());

	// Regular expression for whitespace characters.
	QRegularExpression ws_re(QStringLiteral("\\s+"));

	Frame frame(sourceUrl, file);
	QString filename = sourceUrl.fileName();
	int frameNumber = 0;

	while(!stream.eof() && !isCanceled()) {
		const char* line = stream.readLineTrimLeft();
		if(boost::algorithm::starts_with(line, "Updated atomic structure:")) {
			stream.readLine();
			frame.byteOffset = stream.byteOffset();
			frame.lineNumber = stream.lineNumber();
			frame.label = tr("%1 (Frame %2)").arg(filename).arg(frameNumber++);
			frames.push_back(frame);
		}

		setProgressValueIntermittent(stream.underlyingByteOffset());
	}
}

/******************************************************************************
* Parses the given input file.
******************************************************************************/
FileSourceImporter::FrameDataPtr FHIAimsLogFileImporter::FrameLoader::loadFile(QIODevice& file)
{
	// Open file for reading.
	CompressedTextReader stream(file, frame().sourceFile.path());
	setProgressText(tr("Reading FHI-aims log file %1").arg(frame().sourceFile.toString(QUrl::RemovePassword | QUrl::PreferLocalFile | QUrl::PrettyDecoded)));

	// Jump to byte offset.
	if(frame().byteOffset != 0)
		stream.seek(frame().byteOffset, frame().lineNumber);

	// Create the destination container for loaded data.
	auto frameData = std::make_shared<ParticleFrameData>();

	// First pass: determine the cell geometry and number of atoms.
	AffineTransformation cell = AffineTransformation::Identity();
	int lattVecCount = 0;
	int totalAtomCount = 0;
	while(!stream.eof()) {
		const char* line = stream.readLineTrimLeft();
		if(boost::algorithm::starts_with(line, "lattice_vector")) {
			if(lattVecCount >= 3)
				throw Exception(tr("FHI-aims file contains more than three lattice vectors (line %1): %2").arg(stream.lineNumber()).arg(stream.lineString()));
			if(sscanf(line, "lattice_vector " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING, &cell(0,lattVecCount), &cell(1,lattVecCount), &cell(2,lattVecCount)) != 3 || cell.column(lattVecCount) == Vector3::Zero())
				throw Exception(tr("Invalid cell vector in FHI-aims (line %1): %2").arg(stream.lineNumber()).arg(stream.lineString()));
			lattVecCount++;
		}
		else if(boost::algorithm::starts_with(line, "atom")) {
			totalAtomCount++;
		}
		else if(line[0] > ' ') {
			break;
		}
	}
	if(totalAtomCount == 0)
		throw Exception(tr("Invalid FHI-aims log file: No atoms found."));

	// Create the particle properties.
	PropertyAccess<Point3> posProperty = frameData->addParticleProperty(ParticlesObject::OOClass().createStandardStorage(totalAtomCount, ParticlesObject::PositionProperty, false));
	PropertyAccess<int> typeProperty = frameData->addParticleProperty(ParticlesObject::OOClass().createStandardStorage(totalAtomCount, ParticlesObject::TypeProperty, false));
	ParticleFrameData::TypeList* typeList = frameData->createPropertyTypesList(typeProperty);

	// Return to beginning of frame.
	stream.seek(frame().byteOffset, frame().lineNumber);

	// Second pass: read atom coordinates and types.
	for(int i = 0; i < totalAtomCount; i++) {
		while(true) {
			const char* line = stream.readLineTrimLeft();
			if(boost::algorithm::starts_with(line, "atom")) {
				bool isFractional = boost::algorithm::starts_with(line, "atom_frac");
				Point3& pos = posProperty[i];
				char atomTypeName[16];
				if(sscanf(line + (isFractional ? 9 : 4), FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " %15s", &pos.x(), &pos.y(), &pos.z(), atomTypeName) != 4)
					throw Exception(tr("Invalid atom specification (line %1): %2").arg(stream.lineNumber()).arg(stream.lineString()));
				if(isFractional) {
					if(lattVecCount != 3)
						throw Exception(tr("Invalid fractional atom coordinates (in line %1). Cell vectors have not been specified: %2").arg(stream.lineNumber()).arg(stream.lineString()));
					pos = cell * pos;
				}
				typeProperty[i] = typeList->addTypeName(atomTypeName);
				break;
			}
		}
	}

	// Since we created particle types on the go while reading the particles, the assigned particle type IDs
	// depend on the storage order of particles in the file. We rather want a well-defined particle type ordering, that's
	// why we sort them now.
	typeList->sortTypesByName(typeProperty);

	// Set simulation cell.
	if(lattVecCount == 3) {
		frameData->simulationCell().setMatrix(cell);
		frameData->simulationCell().setPbcFlags(true, true, true);
	}
	else {
		// If the input file does not contain simulation cell info,
		// Use bounding box of particles as simulation cell.
		Box3 boundingBox;
		boundingBox.addPoints(posProperty);
		frameData->simulationCell().setMatrix(AffineTransformation(
				Vector3(boundingBox.sizeX(), 0, 0),
				Vector3(0, boundingBox.sizeY(), 0),
				Vector3(0, 0, boundingBox.sizeZ()),
				boundingBox.minc - Point3::Origin()));
		frameData->simulationCell().setPbcFlags(false, false, false);
	}

	frameData->setStatus(tr("%1 atoms").arg(totalAtomCount));
	return frameData;
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
