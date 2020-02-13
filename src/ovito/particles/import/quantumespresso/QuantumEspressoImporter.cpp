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
#include "QuantumEspressoImporter.h"

#include <boost/algorithm/string.hpp>

namespace Ovito { namespace Particles {

IMPLEMENT_OVITO_CLASS(QuantumEspressoImporter);

/******************************************************************************
* Determines if a character is a normal letter.
******************************************************************************/
static bool isalpha_ascii(char c)
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

/******************************************************************************
* Checks if the given file has format that can be read by this importer.
******************************************************************************/
bool QuantumEspressoImporter::OOMetaClass::checkFileFormat(const FileHandle& file) const
{
	// Open input file.
	CompressedTextReader stream(file);

	// Maximum number of lines we are going to read from the input file before giving up.
	int numLinesToRead = 20;

	while(!stream.eof() && numLinesToRead > 0) {
		numLinesToRead--;
		const char* line = stream.readLineTrimLeft(256);
		// Skip parameter blocks.
		if(line[0] == '&' && isalpha_ascii(line[1])) {
			while(!stream.eof()) {
				const char* line = stream.readLineTrimLeft();
				if(line[0] == '/') {
					numLinesToRead = 20;
					break;
				}
			}
			continue;
		}
		else if(stream.lineStartsWithToken("ATOMIC_SPECIES")) {
			return true;
		}
		else if(line[0] != '\0') {
			return false;
		}
	}

	return false;
}

/******************************************************************************
* Parses the given input file.
******************************************************************************/
FileSourceImporter::FrameDataPtr QuantumEspressoImporter::FrameLoader::loadFile()
{
	// Open file for reading.
	CompressedTextReader stream(fileHandle());
	setProgressText(tr("Reading Quantum Espresso file %1").arg(fileHandle().toString()));

	// Create the storage container for the data being loaded.
	std::shared_ptr<ParticleFrameData> frameData = std::make_shared<ParticleFrameData>();

	// For converting Bohr radii to Angstrom units:
	constexpr FloatType bohr2angstrom = 0.529177;

	// Parsed parameters:
	FloatType alat = 1;
	int natoms = 0;
	int ntypes = 0;
	int ibrav = 0;
	std::vector<FloatType> type_masses;
	auto typeList = std::make_unique<ParticleFrameData::TypeList>();
	bool hasCellVectors = false;
	bool convertToAbsoluteCoordinates = false;
	PropertyAccess<Point3> posProperty;

	while(!stream.eof() && !isCanceled()) {
		const char* line = stream.readLineTrimLeft();

		// Skip comment lines, which start with a '!' or a '#'.
		if(line[0] == '!' || line[0] == '#') {
			continue;
		}

		// Read parameter blocks, which start with a '&'.
		if(line[0] == '&' && isalpha_ascii(line[1])) {
			while(!stream.eof() && !isCanceled()) {
				line = stream.readLineTrimLeft();
				if(line[0] == '/') {
					break;
				}
				else if(boost::algorithm::starts_with(line, "celldm(1)")) {
					// Extract 'alat' parameter value.
					line += 9;
					if(*line == '=' || *line <= ' ') {
						if(sscanf(line, "= " FLOATTYPE_SCANF_STRING, &alat) != 1)
							throw Exception(tr("Invalid celldm(1) value in line %1 of QE file: %2").arg(stream.lineNumber()).arg(stream.lineString()));
						alat *= bohr2angstrom;
					}
				}
				else if(boost::algorithm::starts_with(line, "A")) {
					// Extract 'alat' parameter value.
					line += 1;
					if(*line == '=' || *line <= ' ') {
						if(sscanf(line, "= " FLOATTYPE_SCANF_STRING, &alat) != 1)
							throw Exception(tr("Invalid A value in line %1 of QE file: %2").arg(stream.lineNumber()).arg(stream.lineString()));
					}
				}
				else if(boost::algorithm::starts_with(line, "nat")) {
					// Extract 'nat' parameter value.
					line += 3;
					if(*line == '=' || *line <= ' ') {
						if(sscanf(line, "=%i" , &natoms) != 1 || natoms <= 0)
							throw Exception(tr("Invalid 'nat' value in line %1 of QE file: %2").arg(stream.lineNumber()).arg(stream.lineString()));
					}
				}
				else if(boost::algorithm::starts_with(line, "ntyp")) {
					// Extract 'ntyp' parameter value.
					line += 4;
					if(*line == '=' || *line <= ' ') {
						if(sscanf(line, "=%i" , &ntypes) != 1 || ntypes <= 0)
							throw Exception(tr("Invalid 'ntyp' value in line %1 of QE file: %2").arg(stream.lineNumber()).arg(stream.lineString()));
					}
				}
				else if(boost::algorithm::starts_with(line, "ibrav")) {
					// Extract 'ibrav' parameter value.
					line += 5;
					if(*line == '=' || *line <= ' ') {
						if(sscanf(line, "=%i" , &ibrav) != 1)
							throw Exception(tr("Invalid 'ibrav' value in line %1 of QE file: %2").arg(stream.lineNumber()).arg(stream.lineString()));
					}
				}
			}
			continue;
		}

		if(stream.lineStartsWithToken("ATOMIC_SPECIES")) {
			type_masses.resize(ntypes);
			for(int i = 0; i < ntypes; i++) {
				const char* line = stream.readLineTrimLeft();

				// Parse atom type name.
				const char* token_end = line;
				while(*token_end > ' ') ++token_end;
				typeList->addTypeName(line, token_end);

				// Parse atomic mass.
				if(sscanf(token_end, FLOATTYPE_SCANF_STRING, &type_masses[i]) != 1)
					throw Exception(tr("Invalid atom type definition in line %1 of QE file: %2").arg(stream.lineNumber()).arg(stream.lineString()));
			}
		}
		else if(stream.lineStartsWithToken("ATOMIC_POSITIONS")) {
			// Parse the unit specification.
			const char* units_start = stream.line() + 16;
			while(*units_start > 0 && (*units_start <= ' ' || *units_start == '(' || *units_start == '{')) ++units_start;
			const char* units_end = units_start;
			while(*units_end > ' ' && *units_end != ')' && *units_end != '}') ++units_end;
			std::string units(units_start, units_end);
			FloatType scaling = 1;
			if(units == "alat" || units.empty()) {
				scaling = alat;
			}
			else if(units == "angstrom") {
				// No scaling.
			}
			else if(units == "crystal") {
				// Conversion from reduced to absolute coordinates will be done later.
				convertToAbsoluteCoordinates = true;
			}
			else if(units == "bohr") {
				// Convert from Bohr radii to Angstroms:
				scaling = bohr2angstrom;
			}
			else {
				throw Exception(tr("Unit type used in line %1 of QE file is not supported: %2").arg(stream.lineNumber()).arg(stream.lineString()));
			}

			// Create particle properties.
			posProperty = frameData->addParticleProperty(ParticlesObject::OOClass().createStandardStorage(natoms, ParticlesObject::PositionProperty, false));
			PropertyAccess<int> typeProperty = frameData->addParticleProperty(ParticlesObject::OOClass().createStandardStorage(natoms, ParticlesObject::TypeProperty, false));
			PropertyAccess<FloatType> massProperty = frameData->addParticleProperty(ParticlesObject::OOClass().createStandardStorage(natoms, ParticlesObject::MassProperty, true));

			// Parse atom definitions.
			for(int i = 0; i < natoms; i++) {
				const char* line = stream.readLineTrimLeft();

				// Parse atom type name.
				const char* token_end = line;
				while(*token_end > ' ') ++token_end;
				int typeId = typeList->addTypeName(line, token_end);
				typeProperty[i] = typeId;
				if(typeId >= 1 && typeId <= type_masses.size())
					massProperty[i] = type_masses[typeId-1];

				// Parse atomic coordinates.
				Point3 pos;
				if(sscanf(token_end, FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING, &pos.x(), &pos.y(), &pos.z()) != 3)
					throw Exception(tr("Invalid atomic coordinates in line %1 of QE file: %2").arg(stream.lineNumber()).arg(stream.lineString()));
				posProperty[i] = pos * scaling;
			}
			frameData->setPropertyTypesList(typeProperty, std::move(typeList));
		}
		else if(stream.lineStartsWithToken("CELL_PARAMETERS")) {
			// Parse the unit specification.
			const char* units_start = stream.line() + 16;
			while(*units_start > 0 && (*units_start <= ' ' || *units_start == '(' || *units_start == '{')) ++units_start;
			const char* units_end = units_start;
			while(*units_end > ' ' && *units_end != ')' && *units_end != '}') ++units_end;
			std::string units(units_start, units_end);
			FloatType scaling = 1;
			if(units == "alat" || units.empty()) {
				scaling = alat;
			}
			else if(units == "angstrom") {
				// No scaling.
			}
			else if(units == "bohr") {
				// Convert from Bohr radii to Angstroms:
				scaling = bohr2angstrom;
			}
			else {
				throw Exception(tr("Unit type used in line %1 of QE file is not supported: %2").arg(stream.lineNumber()).arg(stream.lineString()));
			}
			// Read cell matrix.
			AffineTransformation cell = AffineTransformation::Identity();
			for(size_t i = 0; i < 3; i++) {
				std::string line = stream.readLine();
				// Convert Fortran number format to C format:
				for(char& c : line)
					if(c == 'd' || c == 'D') c = 'e';
				if(sscanf(line.c_str(),
						FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING,
						&cell(0,i), &cell(1,i), &cell(2,i)) != 3 || cell.column(i) == Vector3::Zero())
					throw Exception(tr("Invalid cell vector in line %1 of QE file: %2").arg(stream.lineNumber()).arg(stream.lineString()));
			}
			frameData->simulationCell().setMatrix(cell * scaling);
			hasCellVectors = true;
		}
	}
	if(isCanceled())
		return {};

	// Make sure some atoms have been defined in the file.
	if(natoms <= 0 || ntypes <= 0)
		throw Exception(tr("Invalid Quantum Espresso file. No atoms defined."));

	if(!hasCellVectors) {
		Matrix3 cell;
		switch(ibrav) {
			case 0: throw Exception(tr("Invalid 'ibrav' value in QE file: ibrav==0 requires a CELL_PARAMETERS card."));
			case 1: // SC:
				cell = Matrix3(Vector3(alat, 0, 0), Vector3(0, alat, 0), Vector3(0, 0, alat));
				break;
			case 2: // FCC:
				cell = Matrix3(Vector3(-alat/2, 0, alat/2), Vector3(0, alat/2, alat/2), Vector3(-alat/2, alat/2, 0));
				break;
			case 3: // BCC:
				cell = Matrix3(Vector3(alat/2, alat/2, alat/2), Vector3(-alat/2, alat/2, alat/2), Vector3(-alat/2, -alat/2, alat/2));
				break;
			case -3: // BCC, more symmetric axis:
				cell = Matrix3(Vector3(-alat/2, alat/2, alat/2), Vector3(alat/2, -alat/2, alat/2), Vector3(alat/2, alat/2, -alat/2));
				break;
			default: throw Exception(tr("Unsupported 'ibrav' value in QE file: %1").arg(ibrav));
		}
		frameData->simulationCell().setMatrix(AffineTransformation(cell));
	}

	if(convertToAbsoluteCoordinates) {
		// Convert all atom coordinates from reduced to absolute (Cartesian) format.
		const AffineTransformation simCell = frameData->simulationCell().matrix();
		for(Point3& p : posProperty)
			p = simCell * p;
	}

	frameData->setStatus(tr("Number of particles: %1").arg(natoms));
	return frameData;
}

}	// End of namespace
}	// End of namespace
