///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2013) Alexander Stukowski
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
#include <core/app/Application.h>
#include <core/utilities/io/CompressedTextReader.h>
#include <core/utilities/io/FileManager.h>
#include <core/utilities/concurrent/TaskManager.h>
#include "LAMMPSDataImporter.h"

#include <QRegularExpression>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Import) OVITO_BEGIN_INLINE_NAMESPACE(Formats)

IMPLEMENT_OVITO_CLASS(LAMMPSDataImporter);	
DEFINE_PROPERTY_FIELD(LAMMPSDataImporter, atomStyle);
SET_PROPERTY_FIELD_LABEL(LAMMPSDataImporter, atomStyle, "LAMMPS atom style");

/******************************************************************************
* Checks if the given file has format that can be read by this importer.
******************************************************************************/
bool LAMMPSDataImporter::OOMetaClass::checkFileFormat(QFileDevice& input, const QUrl& sourceLocation) const
{
	// Open input file.
	CompressedTextReader stream(input, sourceLocation.path());

	// Read first comment line.
	stream.readLine(1024);

	// Read some lines until we encounter the "atoms" keyword.
	for(int i = 0; i < 20; i++) {
		if(stream.eof())
			return false;
		std::string line(stream.readLine(1024));
		// Trim anything from '#' onward.
		size_t commentStart = line.find('#');
		if(commentStart != std::string::npos) line.resize(commentStart);
		// If line is blank, continue.
		if(line.find_first_not_of(" \t\n\r") == std::string::npos) continue;
		if(line.find("atoms") != std::string::npos) {
			int natoms;
			if(sscanf(line.c_str(), "%u", &natoms) != 1 || natoms < 0)
				return false;
			return true;
		}
	}

	return false;
}

/******************************************************************************
* Inspects the header of the given file and returns the detected LAMMPS atom style.
******************************************************************************/
Future<LAMMPSDataImporter::LAMMPSAtomStyle> LAMMPSDataImporter::inspectFileHeader(const Frame& frame)
{
	// Retrieve file.
	return Application::instance()->fileManager()->fetchUrl(dataset()->container()->taskManager(), frame.sourceFile)
		.then(executor(), [this, frame](const QString& filename) {

			// Start task that inspects the file header to determine the LAMMPS atom style.
			FrameLoaderPtr inspectionTask = std::make_shared<FrameLoader>(frame, filename, atomStyle(), true);
			return dataset()->container()->taskManager().runTaskAsync(inspectionTask)
				.then([](const FileSourceImporter::FrameDataPtr& frameData) mutable {
					return static_cast<LAMMPSFrameData*>(frameData.get())->detectedAtomStyle();
				});
		});
}

/******************************************************************************
* Parses the given input file.
******************************************************************************/
FileSourceImporter::FrameDataPtr LAMMPSDataImporter::FrameLoader::loadFile(QFile& file)
{
	using namespace std;

	// Open file for reading.
	CompressedTextReader stream(file, frame().sourceFile.path());
	setProgressText(tr("Reading LAMMPS data file %1").arg(frame().sourceFile.toString(QUrl::RemovePassword | QUrl::PreferLocalFile | QUrl::PrettyDecoded)));

	// Jump to byte offset.
	if(frame().byteOffset != 0)
		stream.seek(frame().byteOffset);

	// Read comment line
	stream.readLine();

	int natoms = 0;
	int natomtypes = 0;
	int nbonds = 0;
	int nangles = 0;
	int ndihedrals = 0;
	int nimpropers = 0;
	int nbondtypes = 0;
	int nangletypes = 0;
	int ndihedraltypes = 0;
	int nimpropertypes = 0;
	FloatType xlo = 0, xhi = 0;
	FloatType ylo = 0, yhi = 0;
	FloatType zlo = 0, zhi = 0;
	FloatType xy = 0, xz = 0, yz = 0;

	// Read header
	while(true) {

		string line(stream.readLine());

		// Trim anything from '#' onward.
		size_t commentStart = line.find('#');
		if(commentStart != string::npos) line.resize(commentStart);

    	// If line is blank, continue.
		if(line.find_first_not_of(" \t\n\r") == string::npos) continue;

    	if(line.find("atoms") != string::npos) {
    		if(sscanf(line.c_str(), "%u", &natoms) != 1)
    			throw Exception(tr("Invalid number of atoms (line %1): %2").arg(stream.lineNumber()).arg(line.c_str()));

			setProgressMaximum(natoms);
		}
    	else if(line.find("atom types") != string::npos) {
    		if(sscanf(line.c_str(), "%u", &natomtypes) != 1)
    			throw Exception(tr("Invalid number of atom types (line %1): %2").arg(stream.lineNumber()).arg(line.c_str()));
    	}
    	else if(line.find("xlo xhi") != string::npos) {
    		if(sscanf(line.c_str(), FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING, &xlo, &xhi) != 2)
    			throw Exception(tr("Invalid xlo/xhi values (line %1): %2").arg(stream.lineNumber()).arg(line.c_str()));
    	}
    	else if(line.find("ylo yhi") != string::npos) {
    		if(sscanf(line.c_str(), FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING, &ylo, &yhi) != 2)
    			throw Exception(tr("Invalid ylo/yhi values (line %1): %2").arg(stream.lineNumber()).arg(line.c_str()));
    	}
    	else if(line.find("zlo zhi") != string::npos) {
    		if(sscanf(line.c_str(), FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING, &zlo, &zhi) != 2)
    			throw Exception(tr("Invalid zlo/zhi values (line %1): %2").arg(stream.lineNumber()).arg(line.c_str()));
    	}
    	else if(line.find("xy xz yz") != string::npos) {
    		if(sscanf(line.c_str(), FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING, &xy, &xz, &yz) != 3)
    			throw Exception(tr("Invalid xy/xz/yz values (line %1): %2").arg(stream.lineNumber()).arg(line.c_str()));
    	}
    	else if(line.find("bonds") != string::npos) {
    		if(sscanf(line.c_str(), "%u", &nbonds) != 1)
    			throw Exception(tr("Invalid number of bonds (line %1): %2").arg(stream.lineNumber()).arg(line.c_str()));
    	}
    	else if(line.find("bond types") != string::npos) {
    		if(sscanf(line.c_str(), "%u", &nbondtypes) != 1)
    			throw Exception(tr("Invalid number of bond types (line %1): %2").arg(stream.lineNumber()).arg(line.c_str()));
    	}
    	else if(line.find("angle types") != string::npos) {
    		if(sscanf(line.c_str(), "%u", &nangletypes) != 1)
    			throw Exception(tr("Invalid number of angle types (line %1): %2").arg(stream.lineNumber()).arg(line.c_str()));
    	}
    	else if(line.find("dihedral types") != string::npos) {
    		if(sscanf(line.c_str(), "%u", &ndihedraltypes) != 1)
    			throw Exception(tr("Invalid number of dihedral types (line %1): %2").arg(stream.lineNumber()).arg(line.c_str()));
    	}
    	else if(line.find("improper types") != string::npos) {
    		if(sscanf(line.c_str(), "%u", &nimpropertypes) != 1)
    			throw Exception(tr("Invalid number of improper types (line %1): %2").arg(stream.lineNumber()).arg(line.c_str()));
    	}
    	else if(line.find("angles") != string::npos) {
    		if(sscanf(line.c_str(), "%u", &nangles) != 1)
    			throw Exception(tr("Invalid number of angles (line %1): %2").arg(stream.lineNumber()).arg(line.c_str()));
    	}
    	else if(line.find("dihedrals") != string::npos) {
    		if(sscanf(line.c_str(), "%u", &ndihedrals) != 1)
    			throw Exception(tr("Invalid number of dihedrals (line %1): %2").arg(stream.lineNumber()).arg(line.c_str()));
    	}
    	else if(line.find("impropers") != string::npos) {
    		if(sscanf(line.c_str(), "%u", &nimpropers) != 1)
    			throw Exception(tr("Invalid number of impropers (line %1): %2").arg(stream.lineNumber()).arg(line.c_str()));
    	}
    	else if(line.find("extra bond per atom") != string::npos) {}
    	else if(line.find("extra angle per atom") != string::npos) {}
    	else if(line.find("extra dihedral per atom") != string::npos) {}
    	else if(line.find("extra improper per atom") != string::npos) {}
    	else if(line.find("extra special per atom") != string::npos) {}
    	else if(line.find("triangles") != string::npos) {}
    	else if(line.find("ellipsoids") != string::npos) {}
    	else if(line.find("lines") != string::npos) {}
    	else if(line.find("bodies") != string::npos) {}
    	else break;
	}

	if(xhi < xlo || yhi < ylo || zhi < zlo)
		throw Exception(tr("Invalid simulation cell size in header of LAMMPS data file."));

	// Create the destination container for loaded data.
	auto frameData = std::make_shared<LAMMPSFrameData>();

	// Define the simulation cell geometry.
	frameData->simulationCell().setMatrix(AffineTransformation(
			Vector3(xhi - xlo, 0, 0),
			Vector3(xy, yhi - ylo, 0),
			Vector3(xz, yz, zhi - zlo),
			Vector3(xlo, ylo, zlo)));

	// Skip to following line after first non-blank line.
	while(!stream.eof() && string(stream.line()).find_first_not_of(" \t\n\r") == string::npos) {
		stream.readLine();
	}

	// This flag is set to true if the atomic coordinates have been parsed.
	bool foundAtomsSection = false;
	if(natoms == 0)
		foundAtomsSection = true;

	// Create standard particle properties.
	PropertyPtr posProperty = ParticleProperty::createStandardStorage(natoms, ParticleProperty::PositionProperty, true);
	frameData->addParticleProperty(posProperty);
	Point3* pos = posProperty->dataPoint3();
	PropertyPtr typeProperty = ParticleProperty::createStandardStorage(natoms, ParticleProperty::TypeProperty, true);
	frameData->addParticleProperty(typeProperty);
	ParticleFrameData::TypeList* typeList = frameData->propertyTypesList(typeProperty);
	int* atomType = typeProperty->dataInt();
	PropertyPtr identifierProperty = ParticleProperty::createStandardStorage(natoms, ParticleProperty::IdentifierProperty, true);
	frameData->addParticleProperty(identifierProperty);
	int* atomId = identifierProperty->dataInt();

	// Create atom types.
	for(int i = 1; i <= natomtypes; i++)
		typeList->addTypeId(i);

	/// Maps atom IDs to indices.
	std::unordered_map<int,int> atomIdMap;
	atomIdMap.reserve(natoms);

	// Read identifier strings one by one in free-form part of data file.
	QByteArray keyword = QByteArray(stream.line()).trimmed();
	for(;;) {
	    // Skip blank line after keyword.
		if(stream.eof()) break;
		stream.readLine();

		if(keyword.startsWith("Atoms")) {
			if(natoms != 0) {
				stream.readLine();
				bool withPBCImageFlags;
				std::tie(_atomStyle, withPBCImageFlags) = detectAtomStyle(stream.line(), keyword, _atomStyle);
				frameData->setDetectedAtomStyle(_atomStyle);
				if(_detectAtomStyle) {
					// We are done at this point if we are only supposed to 
					// detect the atom style used in the file.
					return frameData;
				}

				Point3I* pbcImage = nullptr;
				if(withPBCImageFlags) {
					PropertyPtr pbcProperty = ParticleProperty::createStandardStorage(natoms, ParticleProperty::PeriodicImageProperty, true);
					frameData->addParticleProperty(pbcProperty);
					pbcImage = pbcProperty->dataPoint3I();
				}

				if(_atomStyle == AtomStyle_Atomic || _atomStyle == AtomStyle_Hybrid) {
					for(int i = 0; i < natoms; i++, ++pos, ++atomType, ++atomId) {
						if(!setProgressValueIntermittent(i)) return {};
						if(i != 0) stream.readLine();
						bool invalidLine;
						if(!pbcImage)
							invalidLine = (sscanf(stream.line(), "%u %u " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING, atomId, atomType, &pos->x(), &pos->y(), &pos->z()) != 5);
						else {
							invalidLine = (sscanf(stream.line(), "%u %u " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " %i %i %i", atomId, atomType, &pos->x(), &pos->y(), &pos->z(), &pbcImage->x(), &pbcImage->y(), &pbcImage->z()) != 5+3);
							++pbcImage;
						}
						if(invalidLine)
							throw Exception(tr("Invalid data in Atoms section of LAMMPS data file at line %1: %2").arg(stream.lineNumber()).arg(stream.lineString()));
						if(*atomType < 1 || *atomType > natomtypes)
							throw Exception(tr("Atom type out of range in Atoms section of LAMMPS data file at line %1.").arg(stream.lineNumber()));
						atomIdMap.insert(std::make_pair(*atomId, i));
					}
				}
				else if(_atomStyle == AtomStyle_Charge || _atomStyle == AtomStyle_Dipole) {
					PropertyPtr chargeProperty = ParticleProperty::createStandardStorage(natoms, ParticleProperty::ChargeProperty, true);
					frameData->addParticleProperty(chargeProperty);
					FloatType* charge = chargeProperty->dataFloat();
					for(int i = 0; i < natoms; i++, ++pos, ++atomType, ++atomId, ++charge) {
						if(!setProgressValueIntermittent(i)) return {};
						if(i != 0) stream.readLine();
						bool invalidLine;
						if(!pbcImage)
							invalidLine = (sscanf(stream.line(), "%u %u " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING, atomId, atomType, charge, &pos->x(), &pos->y(), &pos->z()) != 6);
						else {
							invalidLine = (sscanf(stream.line(), "%u %u " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " %i %i %i", atomId, atomType, charge, &pos->x(), &pos->y(), &pos->z(), &pbcImage->x(), &pbcImage->y(), &pbcImage->z()) != 6+3);
							++pbcImage;
						}
						if(invalidLine)
							throw Exception(tr("Invalid data in Atoms section of LAMMPS data file at line %1: %2").arg(stream.lineNumber()).arg(stream.lineString()));
						if(*atomType < 1 || *atomType > natomtypes)
							throw Exception(tr("Atom type out of range in Atoms section of LAMMPS data file at line %1.").arg(stream.lineNumber()));
						atomIdMap.insert(std::make_pair(*atomId, i));
					}
				}
				else if(_atomStyle == AtomStyle_Angle || _atomStyle == AtomStyle_Bond || _atomStyle == AtomStyle_Molecular) {
					PropertyPtr moleculeProperty = ParticleProperty::createStandardStorage(natoms, ParticleProperty::MoleculeProperty, true);
					frameData->addParticleProperty(moleculeProperty);
					int* molecule = moleculeProperty->dataInt();
					for(int i = 0; i < natoms; i++, ++pos, ++atomType, ++atomId, ++molecule) {
						if(!setProgressValueIntermittent(i)) return {};
						if(i != 0) stream.readLine();
						bool invalidLine;
						if(!pbcImage)
							invalidLine = (sscanf(stream.line(), "%u %u %u " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING, atomId, molecule, atomType, &pos->x(), &pos->y(), &pos->z()) != 6);
						else {
							invalidLine = (sscanf(stream.line(), "%u %u %u " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " %i %i %i", atomId, molecule, atomType, &pos->x(), &pos->y(), &pos->z(), &pbcImage->x(), &pbcImage->y(), &pbcImage->z()) != 6+3);
							++pbcImage;
						}
						if(invalidLine)
							throw Exception(tr("Invalid data in Atoms section of LAMMPS data file at line %1: %2").arg(stream.lineNumber()).arg(stream.lineString()));
						if(*atomType < 1 || *atomType > natomtypes)
							throw Exception(tr("Atom type out of range in Atoms section of LAMMPS data file at line %1.").arg(stream.lineNumber()));
						atomIdMap.insert(std::make_pair(*atomId, i));
					}
				}
				else if(_atomStyle == AtomStyle_Full) {
					PropertyPtr chargeProperty = ParticleProperty::createStandardStorage(natoms, ParticleProperty::ChargeProperty, true);
					frameData->addParticleProperty(chargeProperty);
					FloatType* charge = chargeProperty->dataFloat();
					PropertyPtr moleculeProperty = ParticleProperty::createStandardStorage(natoms, ParticleProperty::MoleculeProperty, true);
					frameData->addParticleProperty(moleculeProperty);
					int* molecule = moleculeProperty->dataInt();
					for(int i = 0; i < natoms; i++, ++pos, ++atomType, ++atomId, ++charge, ++molecule) {
						if(!setProgressValueIntermittent(i)) return {};
						if(i != 0) stream.readLine();
						bool invalidLine;
						if(!pbcImage)
							invalidLine = (sscanf(stream.line(), "%u %u %u " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING, atomId, molecule, atomType, charge, &pos->x(), &pos->y(), &pos->z()) != 7);
						else {
							invalidLine = (sscanf(stream.line(), "%u %u %u " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " %i %i %i", atomId, molecule, atomType, charge, &pos->x(), &pos->y(), &pos->z(), &pbcImage->x(), &pbcImage->y(), &pbcImage->z()) != 7+3);
							++pbcImage;
						}
						if(invalidLine)
							throw Exception(tr("Invalid data in Atoms section of LAMMPS data file at line %1: %2").arg(stream.lineNumber()).arg(stream.lineString()));
						if(*atomType < 1 || *atomType > natomtypes)
							throw Exception(tr("Atom type out of range in Atoms section of LAMMPS data file at line %1.").arg(stream.lineNumber()));
						atomIdMap.insert(std::make_pair(*atomId, i));
					}
				}
				else if(_atomStyle == AtomStyle_Sphere) {
					PropertyPtr radiusProperty = ParticleProperty::createStandardStorage(natoms, ParticleProperty::RadiusProperty, true);
					frameData->addParticleProperty(radiusProperty);
					FloatType* radius = radiusProperty->dataFloat();
					PropertyPtr massProperty = ParticleProperty::createStandardStorage(natoms, ParticleProperty::MassProperty, true);
					frameData->addParticleProperty(massProperty);
					FloatType* mass = massProperty->dataFloat();
					for(int i = 0; i < natoms; i++, ++pos, ++atomType, ++atomId, ++radius, ++mass) {
						if(!setProgressValueIntermittent(i)) return {};
						if(i != 0) stream.readLine();
						bool invalidLine;
						if(!pbcImage)
							invalidLine = (sscanf(stream.line(), "%u %u " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING, atomId, atomType, radius, mass, &pos->x(), &pos->y(), &pos->z()) != 7);
						else {
							invalidLine = (sscanf(stream.line(), "%u %u " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " %i %i %i", atomId, atomType, radius, mass, &pos->x(), &pos->y(), &pos->z(), &pbcImage->x(), &pbcImage->y(), &pbcImage->z()) != 7+3);
							++pbcImage;
						}
						if(invalidLine)
							throw Exception(tr("Invalid data in Atoms section of LAMMPS data file at line %1: %2").arg(stream.lineNumber()).arg(stream.lineString()));
						if(*atomType < 1 || *atomType > natomtypes)
							throw Exception(tr("Atom type out of range in Atoms section of LAMMPS data file at line %1.").arg(stream.lineNumber()));
						atomIdMap.insert(std::make_pair(*atomId, i));
						
						*radius /= 2; // Convert diameter to radius.
						if(*radius != 0) *mass *= pow(*radius, 3) * (FLOATTYPE_PI * FloatType(4) / FloatType(3)); // Convert density to mass.
					}
				}
				else if(_atomStyle == AtomStyle_Unknown) {
					throw Exception(tr("Number of columns in Atoms section of data file (line %1) does not match to selected LAMMPS atom style.").arg(stream.lineNumber()));
				}
				else {
					throw Exception(tr("Selected LAMMPS atom style is not supported by the file parser."));
				}
			}
			foundAtomsSection = true;
		}
		else if(keyword.startsWith("Velocities")) {

			// Get the atomic IDs.
			PropertyPtr identifierProperty = frameData->findStandardParticleProperty(ParticleProperty::IdentifierProperty);
			if(!identifierProperty)
				throw Exception(tr("Atoms section must precede Velocities section in data file (error in line %1).").arg(stream.lineNumber()));

			// Create the velocity property.
			PropertyPtr velocityProperty = ParticleProperty::createStandardStorage(natoms, ParticleProperty::VelocityProperty, true);
			frameData->addParticleProperty(velocityProperty);

			for(int i = 0; i < natoms; i++) {
				if(!setProgressValueIntermittent(i)) return {};
				stream.readLine();

				Vector3 v;
				int atomId;

    			if(sscanf(stream.line(), "%u " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING, &atomId, &v.x(), &v.y(), &v.z()) != 4)
					throw Exception(tr("Invalid velocity specification (line %1): %2").arg(stream.lineNumber()).arg(stream.lineString()));

    			int atomIndex = i;
    			if(atomId != identifierProperty->getInt(i)) {
					auto iter = atomIdMap.find(atomId);
					if(iter == atomIdMap.end())
    					throw Exception(tr("Nonexistent atom ID encountered in line %1 of data file.").arg(stream.lineNumber()));
					atomIndex = iter->second;
    			}

				velocityProperty->setVector3(atomIndex, v);
			}
		}
		else if(keyword.startsWith("Masses")) {
			for(int i = 1; i <= natomtypes; i++) {
				// Try to parse atom types names, which some data files list as comments in the Masses section.
				const char* start = stream.readLine();
				while(*start && *start != '#') start++;
				if(*start) {
					QStringList words = QString::fromLocal8Bit(start).split(QRegularExpression("\\s+"), QString::SkipEmptyParts);
					if(words.size() == 2)
						typeList->setTypeName(i, words[1]);
				}
			}
		}
		else if(keyword.startsWith("Pair Coeffs")) {
			for(int i = 0; i < natomtypes; i++) stream.readLine();
		}
		else if(keyword.startsWith("PairIJ Coeffs")) {
			for(int i = 0; i < natomtypes*(natomtypes+1)/2; i++) stream.readLine();
		}
		else if(keyword.startsWith("Bond Coeffs")) {
			for(int i = 0; i < nbondtypes; i++) stream.readLine();
		}
		else if(keyword.startsWith("Angle Coeffs") || keyword.startsWith("BondAngle Coeffs") || keyword.startsWith("BondBond Coeffs")) {
			for(int i = 0; i < nangletypes; i++) stream.readLine();
		}
		else if(keyword.startsWith("Dihedral Coeffs") || keyword.startsWith("EndBondTorsion Coeffs") || keyword.startsWith("BondBond13 Coeffs") || keyword.startsWith("MiddleBondTorsion Coeffs") || keyword.startsWith("AngleAngleTorsion Coeffs") || keyword.startsWith("AngleTorsion Coeffs")) {
			for(int i = 0; i < ndihedraltypes; i++) stream.readLine();
		}
		else if(keyword.startsWith("Improper Coeffs") || keyword.startsWith("AngleAngle Coeffs")) {
			for(int i = 0; i < nimpropertypes; i++) stream.readLine();
		}
		else if(keyword.startsWith("Angles")) {
			for(int i = 0; i < nangles; i++) stream.readLine();
		}
		else if(keyword.startsWith("Dihedrals")) {
			for(int i = 0; i < ndihedrals; i++) stream.readLine();
		}
		else if(keyword.startsWith("Impropers")) {
			for(int i = 0; i < nimpropers; i++) stream.readLine();
		}
		else if(keyword.startsWith("Bonds")) {

			// Get the atomic IDs and positions.
			PropertyPtr identifierProperty = frameData->findStandardParticleProperty(ParticleProperty::IdentifierProperty);
			PropertyPtr posProperty = frameData->findStandardParticleProperty(ParticleProperty::PositionProperty);
			if(!identifierProperty || !posProperty)
				throw Exception(tr("Atoms section must precede Bonds section in data file (error in line %1).").arg(stream.lineNumber()));

			// Create bonds storage.
			frameData->setBonds(std::make_shared<BondsStorage>());
			frameData->bonds()->reserve(nbonds);

			// Create bond type property.
			PropertyPtr typeProperty = BondProperty::createStandardStorage(nbonds, BondProperty::TypeProperty, true);
			frameData->addBondProperty(typeProperty);
			ParticleFrameData::TypeList* bondTypeList = frameData->propertyTypesList(typeProperty);
			int* bondType = typeProperty->dataInt();

			// Create bond types.
			for(int i = 1; i <= nbondtypes; i++)
				bondTypeList->addTypeId(i);

			setProgressMaximum(nbonds);
			for(int i = 0; i < nbonds; i++) {
				if(!setProgressValueIntermittent(i)) return {};
				stream.readLine();

				int bondId, atomId1, atomId2;
    			if(sscanf(stream.line(), "%u %u %u %u", &bondId, bondType, &atomId1, &atomId2) != 4)
					throw Exception(tr("Invalid bond specification (line %1): %2").arg(stream.lineNumber()).arg(stream.lineString()));

				unsigned int atomIndex1 = atomId1;
    			if(atomIndex1 >= identifierProperty->size() || atomId1 != identifierProperty->getInt(atomIndex1)) {
					auto iter = atomIdMap.find(atomId1);
					if(iter == atomIdMap.end())
    					throw Exception(tr("Nonexistent atom ID encountered in line %1 of data file.").arg(stream.lineNumber()));
					atomIndex1 = iter->second;
    			}

				unsigned int atomIndex2 = atomId2;
    			if(atomIndex2 >= identifierProperty->size() || atomId2 != identifierProperty->getInt(atomIndex2)) {
					auto iter = atomIdMap.find(atomId2);
					if(iter == atomIdMap.end())
    					throw Exception(tr("Nonexistent atom ID encountered in line %1 of data file.").arg(stream.lineNumber()));
					atomIndex2 = iter->second;
    			}

				if(*bondType < 1 || *bondType > nbondtypes)
					throw Exception(tr("Bond type out of range in Bonds section of LAMMPS data file at line %1.").arg(stream.lineNumber()));
    			bondType++;

				// Use minimum image convention to determine PBC shift vector of the bond.
				Vector3 delta = frameData->simulationCell().absoluteToReduced(posProperty->getPoint3(atomIndex2) - posProperty->getPoint3(atomIndex1));
				Vector_3<int8_t> shift = Vector_3<int8_t>::Zero();
				for(size_t dim = 0; dim < 3; dim++) {
					if(frameData->simulationCell().pbcFlags()[dim])
						shift[dim] -= (int8_t)floor(delta[dim] + FloatType(0.5));
				}

				// Create a bonds.
				frameData->bonds()->push_back({ atomIndex1, atomIndex2,  shift });
			}
		}
		else if(keyword.isEmpty() == false) {
			throw Exception(tr("Unknown or unsupported keyword in line %1 of LAMMPS data file: %2.").arg(stream.lineNumber()-1).arg(QString::fromLocal8Bit(keyword)));
		}
		else break;

		// Read up to non-blank line plus one subsequent line.
		while(!stream.eof() && string(stream.readLine()).find_first_not_of(" \t\n\r") == string::npos);

		// Read identifier strings one by one in free-form part of data file.
		keyword = QByteArray(stream.line()).trimmed();
	}

	if(!foundAtomsSection)
		throw Exception("LAMMPS data file does not contain atomic coordinates.");

	QString statusString = tr("Number of particles: %1").arg(natoms);
	if(nbondtypes > 0 || nbonds > 0)
		statusString += tr("\nNumber of bonds: %1").arg(nbonds);
	frameData->setStatus(statusString);
	return frameData;
}

/******************************************************************************
* Detects or verifies the LAMMPS atom style used by the data file.
******************************************************************************/
std::tuple<LAMMPSDataImporter::LAMMPSAtomStyle,bool> LAMMPSDataImporter::FrameLoader::detectAtomStyle(const char* firstLine, const QByteArray& keywordLine, LAMMPSAtomStyle style)
{
	QRegularExpression ws_re(QStringLiteral("\\s+"));

	// Some data files contain a comment after the 'Atoms' keyword that indicates the atom type.
	QString atomTypeHint;
	int commentStart = keywordLine.indexOf('#');
	if(commentStart != -1) {
		QStringList words = QString::fromLatin1(keywordLine.data() + commentStart).split(ws_re, QString::SkipEmptyParts);
		if(words.size() == 2)
			atomTypeHint = words[1];
	}

	// Count fields in first line of Atoms section.
	QString str = QString::fromLatin1(firstLine);
	commentStart = str.indexOf(QChar('#'));
	if(commentStart >= 0) str.truncate(commentStart);
	QStringList tokens = str.split(ws_re, QString::SkipEmptyParts);
	int count = tokens.size();

	if(style == AtomStyle_Unknown && !atomTypeHint.isEmpty()) {
		if(atomTypeHint == QStringLiteral("atomic")) style = AtomStyle_Atomic;
		else if(atomTypeHint == QStringLiteral("full")) style = AtomStyle_Full;
		else if(atomTypeHint == QStringLiteral("angle")) style = AtomStyle_Angle;
		else if(atomTypeHint == QStringLiteral("bond")) style = AtomStyle_Bond;
		else if(atomTypeHint == QStringLiteral("charge")) style = AtomStyle_Charge;
		else if(atomTypeHint == QStringLiteral("molecular")) style = AtomStyle_Molecular;
		else if(atomTypeHint == QStringLiteral("sphere")) style = AtomStyle_Sphere;
	}

	if(style == AtomStyle_Unknown) {
		if(count == 5) {
			return std::make_tuple(AtomStyle_Atomic, false);
		}
		else if(count == 5+3) {
			if(!tokens[5].contains(QChar('.')) && !tokens[6].contains(QChar('.')) && !tokens[7].contains(QChar('.'))) {
				return std::make_tuple(AtomStyle_Atomic, true);
			}
		}
	}

	if(style == AtomStyle_Atomic && (count == 5 || count == 5+3)) return std::make_tuple(style, count == 5+3);
	if(style == AtomStyle_Hybrid && count >= 5) return std::make_tuple(style, false);
	if((style == AtomStyle_Angle || style == AtomStyle_Bond || style == AtomStyle_Charge || style == AtomStyle_Molecular) && (count == 6 || count == 6+3)) return std::make_tuple(style, count == 6+3);
	if((style == AtomStyle_Body || style == AtomStyle_Ellipsoid || style == AtomStyle_Full || style == AtomStyle_Peri || style == AtomStyle_Sphere) && (count == 7 || count == 7+3)) return std::make_tuple(style, count == 7+3);
	if((style == AtomStyle_Electron || style == AtomStyle_Line || style == AtomStyle_Meso || style == AtomStyle_Template || style == AtomStyle_Tri) && (count == 8 || count == 8+3)) return std::make_tuple(style, count == 8+3);
	if(style == AtomStyle_Dipole && (count == 9 || count == 9+3)) return std::make_tuple(style, count == 9+3);
	if(style == AtomStyle_Wavepacket && (count == 11 || count == 11+3)) return std::make_tuple(style, count == 11+3);
	return std::make_tuple(AtomStyle_Unknown, false);
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
