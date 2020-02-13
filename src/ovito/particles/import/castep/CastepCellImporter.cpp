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
#include <ovito/core/utilities/io/CompressedTextReader.h>
#include "CastepCellImporter.h"

#include <boost/algorithm/string.hpp>

namespace Ovito { namespace Particles {

IMPLEMENT_OVITO_CLASS(CastepCellImporter);

static const char* chemical_symbols[] = {
    // 0
    "X",
    // 1
    "H", "He",
    // 2
    "Li", "Be", "B", "C", "N", "O", "F", "Ne",
    // 3
    "Na", "Mg", "Al", "Si", "P", "S", "Cl", "Ar",
    // 4
    "K", "Ca", "Sc", "Ti", "V", "Cr", "Mn", "Fe", "Co", "Ni", "Cu", "Zn",
    "Ga", "Ge", "As", "Se", "Br", "Kr",
    // 5
    "Rb", "Sr", "Y", "Zr", "Nb", "Mo", "Tc", "Ru", "Rh", "Pd", "Ag", "Cd",
    "In", "Sn", "Sb", "Te", "I", "Xe",
    // 6
    "Cs", "Ba", "La", "Ce", "Pr", "Nd", "Pm", "Sm", "Eu", "Gd", "Tb", "Dy",
    "Ho", "Er", "Tm", "Yb", "Lu",
    "Hf", "Ta", "W", "Re", "Os", "Ir", "Pt", "Au", "Hg", "Tl", "Pb", "Bi",
    "Po", "At", "Rn",
    // 7
    "Fr", "Ra", "Ac", "Th", "Pa", "U", "Np", "Pu", "Am", "Cm", "Bk",
    "Cf", "Es", "Fm", "Md", "No", "Lr",
    "Rf", "Db", "Sg", "Bh", "Hs", "Mt", "Ds", "Rg", "Cn", "Nh", "Fl", "Mc",
    "Lv", "Ts", "Og"
};

/******************************************************************************
* Checks if the given file has format that can be read by this importer.
******************************************************************************/
bool CastepCellImporter::OOMetaClass::checkFileFormat(const FileHandle& file) const
{
	// Open input file.
	CompressedTextReader stream(file);

	// Look for string '%BLOCK POSITIONS' to occur within the first 100 lines of the .cell file.
	for(int i = 0; i < 100 && !stream.eof(); i++) {
		if(boost::algorithm::istarts_with(stream.readLineTrimLeft(1024), "%BLOCK POSITIONS"))
			return true;
	}

	return false;
}

/******************************************************************************
* Parses the given input file.
******************************************************************************/
FileSourceImporter::FrameDataPtr CastepCellImporter::FrameLoader::loadFile()
{
	// Open file for reading.
	CompressedTextReader stream(fileHandle());
	setProgressText(tr("Reading CASTEP file %1").arg(fileHandle().toString()));

	// Helper function that reads and returns the next line from the .cell file
	// that is not a comment line:
	auto readNonCommentLine = [&stream]() {
		while(!stream.eof()) {
			const char* line = stream.readLineTrimLeft();
			if(line[0] == '\0' || line[0] == '#' || line[0] == ';' || line[0] == '!') continue;
			if(boost::algorithm::istarts_with(line, "COMMENT")) continue;
			return line;
		}
		return "";
	};

	// Create the destination container for loaded data.
	auto frameData = std::make_shared<ParticleFrameData>();

	while(!isCanceled()) {

		// Parse line by line.
		const char* line = readNonCommentLine();
		if(line[0] == '\0') break;

		// Interpret only certain known keywords from the .cell file:

		if(boost::algorithm::istarts_with(line, "%BLOCK LATTICE_CART")) {
			line = readNonCommentLine();
			// Skip optional units.
			if((line[0] < '0' || line[0] > '9') && line[0] != '.')
				line = readNonCommentLine();
			// Parse cell vectors.
			AffineTransformation cell = AffineTransformation::Identity();
			for(int i = 0; i < 3; i++) {
				if(sscanf(line, FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING,
					&cell(0,i), &cell(1,i), &cell(2,i)) != 3)
					throw Exception(tr("Invalid simulation cell in CASTEP file at line %1").arg(stream.lineNumber()));
				line = readNonCommentLine();
			}
			frameData->simulationCell().setMatrix(cell);
		}
		else if(boost::algorithm::istarts_with(line, "%BLOCK LATTICE_ABC")) {
			line = readNonCommentLine();
			// Skip optional units..
			if((line[0] < '0' || line[0] > '9') && line[0] != '.')
				line = readNonCommentLine();
			// Parse cell side lengths and angles.
			FloatType a,b,c,alpha,beta,gamma;
			if(sscanf(line, FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING, &a, &b, &c) != 3)
				throw Exception(tr("Invalid simulation cell in CASTEP file at line %1").arg(stream.lineNumber()));
			line = readNonCommentLine();
			if(sscanf(line, FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING, &alpha, &beta, &gamma) != 3)
				throw Exception(tr("Invalid simulation cell in CASTEP file at line %1").arg(stream.lineNumber()));
			line = readNonCommentLine();
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
		}
		else if((boost::algorithm::istarts_with(line, "%BLOCK POSITIONS_FRAC") && !boost::algorithm::istarts_with(line, "%BLOCK POSITIONS_FRAC_"))
				|| (boost::algorithm::istarts_with(line, "%BLOCK POSITIONS_ABS") && !boost::algorithm::istarts_with(line, "%BLOCK POSITIONS_ABS_"))) {
			bool fractionalCoords = boost::algorithm::istarts_with(line, "%BLOCK POSITIONS_FRAC");
			line = readNonCommentLine();
			std::vector<Point3> coords;
			std::vector<int> types;
			std::unique_ptr<ParticleFrameData::TypeList> typeList = std::make_unique<ParticleFrameData::TypeList>();
			while(!boost::algorithm::istarts_with(line, "%ENDBLOCK") && !isCanceled() && !stream.eof()) {
				Point3 pos;
				int atomicNumber;
				if(sscanf(line, "%u " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING, &atomicNumber, &pos.x(), &pos.y(), &pos.z()) == 4) {
					coords.push_back(pos);
					if(atomicNumber < 0 || atomicNumber >= sizeof(chemical_symbols)/sizeof(chemical_symbols[0]))
						atomicNumber = 0;
					types.push_back(typeList->addTypeName(chemical_symbols[atomicNumber]));
				}
				else if(sscanf(line, "%*s " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING, &pos.x(), &pos.y(), &pos.z()) == 3) {
					coords.push_back(pos);
					const char* typeNameEnd = line;
					while(*typeNameEnd > ' ') typeNameEnd++;
					types.push_back(typeList->addTypeName(line, typeNameEnd));
				}
				else {
					// Ignore parsing error, skip optional units.
				}
				line = readNonCommentLine();
			}

			// Convert from fractional to cartesian coordinates.
			if(fractionalCoords) {
				const AffineTransformation& cell = frameData->simulationCell().matrix();
				for(Point3& p : coords)
					p = cell * p;
			}

			PropertyAccess<Point3> posProperty = frameData->addParticleProperty(ParticlesObject::OOClass().createStandardStorage(coords.size(), ParticlesObject::PositionProperty, false));
			boost::copy(coords, posProperty.begin());

			PropertyAccess<int> typeProperty = frameData->addParticleProperty(ParticlesObject::OOClass().createStandardStorage(types.size(), ParticlesObject::TypeProperty, false));
			boost::copy(types, typeProperty.begin());
			typeList->sortTypesByName(typeProperty);
			frameData->setPropertyTypesList(typeProperty, std::move(typeList));

			frameData->setStatus(tr("%1 atoms").arg(coords.size()));
		}
		else if(boost::algorithm::istarts_with(line, "%BLOCK IONIC_VELOCITIES")) {
			line = readNonCommentLine();
			std::vector<Vector3> velocities;
			while(!boost::algorithm::istarts_with(line, "%ENDBLOCK") && !isCanceled() && !stream.eof()) {
				Vector3 v;
				if(sscanf(line, FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING, &v.x(), &v.y(), &v.z()) == 3)
					velocities.push_back(v);
				// Ignore parsing error, skip optional units.
				line = readNonCommentLine();
			}

			PropertyAccess<Vector3> velocityProperty = frameData->addParticleProperty(ParticlesObject::OOClass().createStandardStorage(velocities.size(), ParticlesObject::VelocityProperty, false));
			boost::copy(velocities, velocityProperty.begin());
		}
	}
	return frameData;
}

}	// End of namespace
}	// End of namespace
