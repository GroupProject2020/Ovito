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
#include <plugins/particles/import/ParticleFrameData.h>
#include <core/utilities/io/CompressedTextReader.h>
#include "QuantumEspressoImporter.h"

#include <boost/algorithm/string.hpp>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Import) OVITO_BEGIN_INLINE_NAMESPACE(Formats)

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
bool QuantumEspressoImporter::OOMetaClass::checkFileFormat(QFileDevice& input, const QUrl& sourceLocation) const
{
	// Open input file.
	CompressedTextReader stream(input, sourceLocation.path());

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
		else if(stream.lineStartsWith("ATOMIC_SPECIES")) {
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
FileSourceImporter::FrameDataPtr QuantumEspressoImporter::FrameLoader::loadFile(QFile& file)
{
	// Open file for reading.
	CompressedTextReader stream(file, frame().sourceFile.path());
	setProgressText(tr("Reading Quantum Espresso file %1").arg(frame().sourceFile.toString(QUrl::RemovePassword | QUrl::PreferLocalFile | QUrl::PrettyDecoded)));

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

		if(stream.lineStartsWith("ATOMIC_SPECIES")) {
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
		else if(stream.lineStartsWith("ATOMIC_POSITIONS")) {
			// Parse the unit specification.
			const char* units_start = stream.line() + 16;
			while(*units_start > 0 && *units_start <= ' ') ++units_start;
			const char* units_end = units_start;
			while(*units_end > ' ') ++units_end;
			std::string units(units_start, units_end);
			FloatType scaling = 1;
			if(units == "(alat)" || units.empty()) {
				scaling = alat;
			}
			else if(units == "(angstrom)") {
				// No scaling.
			}
			else if(units == "(bohr)") {
				// Convert from Bohr radii to Angstroms:
				scaling = bohr2angstrom;
			}
			else {
				throw Exception(tr("Unit type used in line %1 of QE file is not supported: %2").arg(stream.lineNumber()).arg(stream.lineString()));
			}

			// Create particle properties.
			PropertyPtr posProperty = ParticlesObject::OOClass().createStandardStorage(natoms, ParticlesObject::PositionProperty, false);
			frameData->addParticleProperty(posProperty);
			PropertyPtr typeProperty = ParticlesObject::OOClass().createStandardStorage(natoms, ParticlesObject::TypeProperty, false);
			frameData->addParticleProperty(typeProperty);
			PropertyPtr massProperty = ParticlesObject::OOClass().createStandardStorage(natoms, ParticlesObject::MassProperty, true);
			frameData->addParticleProperty(massProperty);

			// Parse atom definitions.
			for(int i = 0; i < natoms; i++) {
				const char* line = stream.readLineTrimLeft();

				// Parse atom type name.
				const char* token_end = line;
				while(*token_end > ' ') ++token_end;
				int typeId = typeList->addTypeName(line, token_end);
				typeProperty->setInt(i, typeId);
				if(typeId >= 1 && typeId <= type_masses.size())
					massProperty->setFloat(i, type_masses[typeId-1]);

				// Parse atomic coordinates.
				Point3 pos;
				if(sscanf(token_end, FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING, &pos.x(), &pos.y(), &pos.z()) != 3)
					throw Exception(tr("Invalid atom coordinates in line %1 of QE file: %2").arg(stream.lineNumber()).arg(stream.lineString()));
				posProperty->setPoint3(i, pos * scaling);
			}
			frameData->setPropertyTypesList(typeProperty, std::move(typeList));
		}
		else if(stream.lineStartsWith("CELL_PARAMETERS")) {
			// Parse the unit specification.
			const char* units_start = stream.line() + 16;
			while(*units_start > 0 && *units_start <= ' ') ++units_start;
			const char* units_end = units_start;
			while(*units_end > ' ') ++units_end;
			std::string units(units_start, units_end);
			FloatType scaling = 1;
			if(units == "{alat}" || units.empty()) {
				scaling = alat;
			}
			else if(units == "{angstrom}") {
				// No scaling.
			}
			else if(units == "{bohr}") {
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

	frameData->setStatus(tr("Number of particles: %1").arg(natoms));
	return frameData;
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
