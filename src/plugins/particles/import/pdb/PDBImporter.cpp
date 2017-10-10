///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2015) Alexander Stukowski
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
#include <plugins/particles/import/ParticleFrameData.h>
#include <core/utilities/io/CompressedTextReader.h>
#include "PDBImporter.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Import) OVITO_BEGIN_INLINE_NAMESPACE(Formats)

IMPLEMENT_OVITO_CLASS(PDBImporter);	

/******************************************************************************
* Checks if the given file has format that can be read by this importer.
******************************************************************************/
bool PDBImporter::OOMetaClass::checkFileFormat(QFileDevice& input, const QUrl& sourceLocation) const
{
	// Open input file.
	CompressedTextReader stream(input, sourceLocation.path());

	// Read the first N lines.
	for(int i = 0; i < 20 && !stream.eof(); i++) {
		stream.readLine(86);
		if(qstrlen(stream.line()) > 83 && !stream.lineStartsWith("TITLE "))
			return false;
		if(qstrlen(stream.line()) >= 7 && stream.line()[6] != ' ')
			return false;
		if(stream.lineStartsWith("HEADER ") || stream.lineStartsWith("ATOM   ") || stream.lineStartsWith("HETATM "))
			return true;
	}
	return false;
}

/******************************************************************************
* Parses the given input file.
******************************************************************************/
FileSourceImporter::FrameDataPtr PDBImporter::FrameLoader::loadFile(QFile& file)
{
	// Open file for reading.
	CompressedTextReader stream(file, frame().sourceFile.path());
	setProgressText(tr("Reading PDB file %1").arg(frame().sourceFile.toString(QUrl::RemovePassword | QUrl::PreferLocalFile | QUrl::PrettyDecoded)));

	// Jump to byte offset.
	if(frame().byteOffset != 0)
		stream.seek(frame().byteOffset);

	// Create the destination container for loaded data.
	std::shared_ptr<ParticleFrameData> frameData = std::make_shared<ParticleFrameData>();

	// Parse metadata records.
	int numAtoms = 0;
	bool hasSimulationCell = false;
	while(!stream.eof()) {

		if(isCanceled())
			return {};

		stream.readLine();
		int lineLength = qstrlen(stream.line());
		if(lineLength < 3 || (lineLength > 83 && !stream.lineStartsWith("TITLE ")))
			throw Exception(tr("Invalid line length detected in Protein Data Bank (PDB) file at line %1").arg(stream.lineNumber()));

		// Parse simulation cell.
		if(stream.lineStartsWith("CRYST1")) {
			FloatType a,b,c,alpha,beta,gamma;
			if(sscanf(stream.line(), "CRYST1 " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " "
					FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING, &a, &b, &c, &alpha, &beta, &gamma) != 6)
				throw Exception(tr("Invalid simulation cell in Protein Data Bank (PDB) file at line %1").arg(stream.lineNumber()));
			AffineTransformation cell = AffineTransformation::Identity();
			if(alpha == 90 && beta == 90 && gamma == 90) {
				cell(0,0) = a;
				cell(1,1) = b;
				cell(2,2) = c;
			}
			else if(alpha == 90 && beta == 90) {
				gamma *= FLOATTYPE_PI / 180;
				cell(0,0) = a;
				cell(0,1) = b * cos(gamma);
				cell(1,1) = b * sin(gamma);
				cell(2,2) = c;
			}
			else {
				alpha *= FLOATTYPE_PI / 180;
				beta *= FLOATTYPE_PI / 180;
				gamma *= FLOATTYPE_PI / 180;
				FloatType v = a*b*c*sqrt(1.0 - cos(alpha)*cos(alpha) - cos(beta)*cos(beta) - cos(gamma)*cos(gamma) + 2.0 * cos(alpha) * cos(beta) * cos(gamma));
				cell(0,0) = a;
				cell(0,1) = b * cos(gamma);
				cell(1,1) = b * sin(gamma);
				cell(0,2) = c * cos(beta);
				cell(1,2) = c * (cos(alpha) - cos(beta)*cos(gamma)) / sin(gamma);
				cell(2,2) = v / (a*b*sin(gamma));
			}
			frameData->simulationCell().setMatrix(cell);
			hasSimulationCell = true;
		}
		// Count atoms.
		else if(stream.lineStartsWith("ATOM  ") || stream.lineStartsWith("HETATM")) {
			numAtoms++;
		}
	}

	setProgressMaximum(numAtoms);

	// Jump back to beginning of file.
	stream.seek(0);

	// Create the particle properties.
	PropertyPtr posProperty = ParticleProperty::createStandardStorage(numAtoms, ParticleProperty::PositionProperty, true);
	frameData->addParticleProperty(posProperty);
	PropertyPtr typeProperty = ParticleProperty::createStandardStorage(numAtoms, ParticleProperty::TypeProperty, true);
	frameData->addParticleProperty(typeProperty);
	ParticleFrameData::TypeList* typeList = frameData->propertyTypesList(typeProperty);

	// Parse atoms.
	size_t atomIndex = 0;
	Point3* p = posProperty->dataPoint3();
	int* a = typeProperty->dataInt();
	PropertyPtr particleIdentifierProperty;
	PropertyPtr moleculeIdentifierProperty;
	PropertyPtr moleculeTypeProperty;
	ParticleFrameData::TypeList* moleculeTypeList = nullptr;
	while(!stream.eof() && atomIndex < numAtoms) {
		if(!setProgressValueIntermittent(atomIndex)) return {};

		stream.readLine();
		int lineLength = qstrlen(stream.line());
		if(lineLength < 3 || (lineLength > 83 && !stream.lineStartsWith("TITLE ")))
			throw Exception(tr("Invalid line length detected in Protein Data Bank (PDB) file at line %1").arg(stream.lineNumber()));

		// Parse atom definition.
		if(stream.lineStartsWith("ATOM  ") || stream.lineStartsWith("HETATM")) {
			char atomType[4];
			int atomTypeLength = 0;
			for(const char* c = stream.line() + 76; c <= stream.line() + std::min(77, lineLength); ++c)
				if(*c != ' ') atomType[atomTypeLength++] = *c;
			if(atomTypeLength == 0) {
				for(const char* c = stream.line() + 12; c <= stream.line() + std::min(15, lineLength); ++c)
					if(*c != ' ') atomType[atomTypeLength++] = *c;
			}
			*a = typeList->addTypeName(atomType, atomType + atomTypeLength);
#ifdef FLOATTYPE_FLOAT
			if(lineLength <= 30 || sscanf(stream.line() + 30, "%8g%8g%8g", &p->x(), &p->y(), &p->z()) != 3)
#else
			if(lineLength <= 30 || sscanf(stream.line() + 30, "%8lg%8lg%8lg", &p->x(), &p->y(), &p->z()) != 3)
#endif
				throw Exception(tr("Invalid atom coordinates (line %1): %2").arg(stream.lineNumber()).arg(stream.lineString()));

			// Parse atom ID (serial number).
			unsigned int atomSerialNumber;
			if(sscanf(stream.line() + 6, "%5u", &atomSerialNumber) == 1) {
				if(!particleIdentifierProperty) {
					particleIdentifierProperty = ParticleProperty::createStandardStorage(numAtoms, ParticleProperty::IdentifierProperty, true);
					frameData->addParticleProperty(particleIdentifierProperty);
				}
				particleIdentifierProperty->setInt64(atomIndex, atomSerialNumber);
			}

			// Parse molecule ID (residue sequence number).
			unsigned int residueSequenceNumber;
			if(sscanf(stream.line() + 22, "%4u", &residueSequenceNumber) == 1) {
				if(!moleculeIdentifierProperty) {
					moleculeIdentifierProperty = ParticleProperty::createStandardStorage(numAtoms, ParticleProperty::MoleculeProperty, true);
					frameData->addParticleProperty(moleculeIdentifierProperty);
				}
				moleculeIdentifierProperty->setInt64(atomIndex, residueSequenceNumber);
			}

			// Parse molecule type.
			char moleculeType[3];
			int moleculeTypeLength = 0;
			for(const char* c = stream.line() + 17; c <= stream.line() + std::min(19, lineLength); ++c)
				if(*c != ' ') moleculeType[moleculeTypeLength++] = *c;
			if(moleculeTypeLength != 0) {
				if(!moleculeTypeProperty) {
					moleculeTypeProperty = ParticleProperty::createStandardStorage(numAtoms, ParticleProperty::MoleculeTypeProperty, true);
					frameData->addParticleProperty(moleculeTypeProperty);
					moleculeTypeList = frameData->propertyTypesList(moleculeTypeProperty);
				}
				moleculeTypeProperty->setInt(atomIndex, moleculeTypeList->addTypeName(moleculeType, moleculeType + moleculeTypeLength));
			}

			atomIndex++;
			++p;
			++a;
		}
	}

	// Parse bonds.
	while(!stream.eof()) {
		stream.readLine();
		int lineLength = qstrlen(stream.line());
		if(lineLength < 3 || (lineLength > 83 && !stream.lineStartsWith("TITLE ")))
			throw Exception(tr("Invalid line length detected in Protein Data Bank (PDB) file at line %1").arg(stream.lineNumber()));

		// Parse bonds.
		if(stream.lineStartsWith("CONECT")) {
			// Parse first atom index.
			unsigned int atomSerialNumber1;
			if(lineLength <= 11 || sscanf(stream.line() + 6, "%5u", &atomSerialNumber1) != 1 || particleIdentifierProperty == nullptr)
				throw Exception(tr("Invalid CONECT record (line %1): %2").arg(stream.lineNumber()).arg(stream.lineString()));
			size_t atomIndex1 = std::find(particleIdentifierProperty->constDataInt64(), particleIdentifierProperty->constDataInt64() + particleIdentifierProperty->size(), atomSerialNumber1) - particleIdentifierProperty->constDataInt64();
			for(int i = 0; i < 10; i++) {
				unsigned int atomSerialNumber2;
				if(lineLength >= 16+5*i && sscanf(stream.line() + 11+5*i, "%5u", &atomSerialNumber2) == 1) {
					size_t atomIndex2 = std::find(particleIdentifierProperty->constDataInt64(), particleIdentifierProperty->constDataInt64() + particleIdentifierProperty->size(), atomSerialNumber2) - particleIdentifierProperty->constDataInt64();
					if(atomIndex1 >= particleIdentifierProperty->size() || atomIndex2 >= particleIdentifierProperty->size())
						throw Exception(tr("Nonexistent atom ID encountered in line %1 of PDB file.").arg(stream.lineNumber()));
					if(!frameData->bonds())
						frameData->setBonds(std::make_shared<BondsStorage>());
					frameData->bonds()->push_back({ atomIndex1, atomIndex2, Vector_3<int8_t>::Zero() });
				}
			}
		}
		else if(stream.lineStartsWith("END")) {
			break;
		}
	}

	// If file does not contains simulation cell info,
	// compute bounding box of atoms and use it as an adhoc simulation cell.
	if(!hasSimulationCell && numAtoms > 0) {
		Box3 boundingBox;
		boundingBox.addPoints(posProperty->constDataPoint3(), posProperty->size());
		frameData->simulationCell().setPbcFlags(false, false, false);
		frameData->simulationCell().setMatrix(AffineTransformation(
				Vector3(boundingBox.sizeX(), 0, 0),
				Vector3(0, boundingBox.sizeY(), 0),
				Vector3(0, 0, boundingBox.sizeZ()),
				boundingBox.minc - Point3::Origin()));
	}

	frameData->setStatus(tr("Number of particles: %1").arg(numAtoms));
	return frameData;
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
