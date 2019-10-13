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
#include <ovito/particles/import/InputColumnMapping.h>
#include <ovito/core/app/Application.h>
#include <ovito/core/utilities/io/CompressedTextReader.h>
#include <ovito/core/utilities/io/FileManager.h>
#include <ovito/core/utilities/concurrent/TaskManager.h>
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
			activateCLocale();
			FrameLoaderPtr inspectionTask = std::make_shared<FrameLoader>(frame, filename, sortParticles(), atomStyle(), true);
			return dataset()->container()->taskManager().runTaskAsync(inspectionTask)
				.then([](const FileSourceImporter::FrameDataPtr& frameData) {
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
		stream.seek(frame().byteOffset, frame().lineNumber);

	// Read comment line
	stream.readLine();

	qlonglong natoms = 0;
	int natomtypes = 0;
	qlonglong nbonds = 0;
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
    		if(sscanf(line.c_str(), "%llu", &natoms) != 1)
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
    		if(sscanf(line.c_str(), "%llu", &nbonds) != 1)
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
	PropertyPtr posProperty = ParticlesObject::OOClass().createStandardStorage(natoms, ParticlesObject::PositionProperty, true);
	frameData->addParticleProperty(posProperty);
	PropertyPtr typeProperty = ParticlesObject::OOClass().createStandardStorage(natoms, ParticlesObject::TypeProperty, true);
	frameData->addParticleProperty(typeProperty);
	ParticleFrameData::TypeList* typeList = frameData->propertyTypesList(typeProperty);
	PropertyPtr identifierProperty = ParticlesObject::OOClass().createStandardStorage(natoms, ParticlesObject::IdentifierProperty, true);
	frameData->addParticleProperty(identifierProperty);

	// Create atom types.
	for(int i = 1; i <= natomtypes; i++)
		typeList->addTypeId(i);

	// Atom type mass table.
	std::vector<FloatType> massTable;

	/// Maps atom IDs to indices.
	std::unordered_map<qlonglong,size_t> atomIdMap;
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
				bool withPBCImageFlags = detectAtomStyle(stream.line(), keyword);
				frameData->setDetectedAtomStyle(_atomStyle, _atomSubStyles);
				if(_detectAtomStyle) {
					// We are done at this point if we are only supposed to
					// detect the atom style used in the file.
					return frameData;
				}
				if(_atomStyle == AtomStyle_Unknown)
					throw Exception(tr("Atom style of the LAMMPS data file could not be detected, or the number of file columns is not as expected for the selected atom style."));

				// Set up mapping of file columns to internal particle properties.
				// The number and order of file columns in a LAMMPS data file depends
				// on the atom style detected above.
				InputColumnMapping columnMapping = createColumnMapping(_atomStyle, withPBCImageFlags);

				// Append also the data columns of the sub-styles if main atom style is "hybrid".
				if(_atomStyle == AtomStyle_Hybrid) {
					for(LAMMPSAtomStyle subStyle : _atomSubStyles) {
						InputColumnMapping subStyleMapping = createColumnMapping(subStyle, false);
						for(const InputColumnInfo& column : subStyleMapping) {
							if(column.isMapped() && (
									column.property.type() == ParticlesObject::IdentifierProperty ||
									column.property.type() == ParticlesObject::TypeProperty ||
									column.property.type() == ParticlesObject::PositionProperty))
								continue;
							columnMapping.push_back(column);
						}
					}
				}

				// Parse data in the Atoms section line by line:
				InputColumnReader columnParser(columnMapping, *frameData, natoms);
				try {
					const int* atomType = typeProperty->constDataInt();
					const qlonglong* atomId = identifierProperty->constDataInt64();
					for(size_t i = 0; i < (size_t)natoms; i++, ++atomType, ++atomId) {
						if(!setProgressValueIntermittent(i)) return {};
						if(i != 0) stream.readLine();
						columnParser.readParticle(i, stream.line());
						if(*atomType < 1 || *atomType > natomtypes)
							throw Exception(tr("Atom type out of range in Atoms section of LAMMPS data file at line %1.").arg(stream.lineNumber()));
						atomIdMap.insert(std::make_pair(*atomId, i));
					}
				}
				catch(Exception& ex) {
					throw ex.prependGeneralMessage(tr("Parsing error in line %1 of LAMMPS data file.").arg(stream.lineNumber()));
				}

				// Some LAMMPS data files contain per-particle diameter information.
				// OVITO only knows the "Radius" particle property, which is means we have to divide by 2.
				if(PropertyPtr radiusProperty = frameData->findStandardParticleProperty(ParticlesObject::RadiusProperty)) {
					for(FloatType& r : radiusProperty->floatRange())
						r /= FloatType(2);
				}
			}
			foundAtomsSection = true;
		}
		else if(keyword.startsWith("Velocities")) {

			// Get the atomic IDs.
			PropertyPtr identifierProperty = frameData->findStandardParticleProperty(ParticlesObject::IdentifierProperty);
			if(!identifierProperty)
				throw Exception(tr("Atoms section must precede Velocities section in data file (error in line %1).").arg(stream.lineNumber()));

			// Create the velocity property.
			PropertyPtr velocityProperty = ParticlesObject::OOClass().createStandardStorage(natoms, ParticlesObject::VelocityProperty, true);
			frameData->addParticleProperty(velocityProperty);

			for(size_t i = 0; i < (size_t)natoms; i++) {
				if(!setProgressValueIntermittent(i)) return {};
				stream.readLine();

				Vector3 v;
				qlonglong atomId;

    			if(sscanf(stream.line(), "%llu " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING, &atomId, &v.x(), &v.y(), &v.z()) != 4)
					throw Exception(tr("Invalid velocity specification (line %1): %2").arg(stream.lineNumber()).arg(stream.lineString()));

    			size_t atomIndex = i;
    			if(atomId != identifierProperty->getInt64(i)) {
					auto iter = atomIdMap.find(atomId);
					if(iter == atomIdMap.end())
    					throw Exception(tr("Nonexistent atom ID encountered in line %1 of data file.").arg(stream.lineNumber()));
					atomIndex = iter->second;
    			}

				velocityProperty->setVector3(atomIndex, v);
			}
		}
		else if(keyword.startsWith("Masses")) {
			massTable.resize(natomtypes+1, 0);
			for(int i = 1; i <= natomtypes; i++) {
				// Try to parse atom types names, which some data files list as comments in the Masses section.
				const char* start = stream.readLine();

				// Parse mass information.
				int atomType;
				FloatType mass;
    			if(sscanf(start, "%i " FLOATTYPE_SCANF_STRING, &atomType, &mass) != 2 || atomType < 1 || atomType > natomtypes)
					throw Exception(tr("Invalid mass specification (line %1): %2").arg(stream.lineNumber()).arg(stream.lineString()));
				massTable[atomType] = mass;
				typeList->setTypeMass(i, mass);

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
			PropertyPtr identifierProperty = frameData->findStandardParticleProperty(ParticlesObject::IdentifierProperty);
			PropertyPtr posProperty = frameData->findStandardParticleProperty(ParticlesObject::PositionProperty);
			if(!identifierProperty || !posProperty)
				throw Exception(tr("Atoms section must precede Bonds section in data file (error in line %1).").arg(stream.lineNumber()));

			// Create bonds storage.
			PropertyPtr bondTopologyProperty = BondsObject::OOClass().createStandardStorage(nbonds, BondsObject::TopologyProperty, false);
			frameData->addBondProperty(bondTopologyProperty);
			qlonglong* atomIndex = bondTopologyProperty->dataInt64();

			// Create bond type property.
			PropertyPtr typeProperty = BondsObject::OOClass().createStandardStorage(nbonds, BondsObject::TypeProperty, true);
			frameData->addBondProperty(typeProperty);
			ParticleFrameData::TypeList* bondTypeList = frameData->propertyTypesList(typeProperty);
			int* bondType = typeProperty->dataInt();

			// Create bond types.
			for(int i = 1; i <= nbondtypes; i++)
				bondTypeList->addTypeId(i);

			setProgressMaximum(nbonds);
			for(size_t i = 0; i < (size_t)nbonds; i++) {
				if(!setProgressValueIntermittent(i)) return {};
				stream.readLine();

				qlonglong bondId, atomId1, atomId2;
    			if(sscanf(stream.line(), "%llu %u %llu %llu", &bondId, bondType, &atomId1, &atomId2) != 4)
					throw Exception(tr("Invalid bond specification (line %1): %2").arg(stream.lineNumber()).arg(stream.lineString()));

    			if(atomId1 < 0 || atomId1 >= (qlonglong)identifierProperty->size() || atomId1 != identifierProperty->getInt64(atomId1)) {
					auto iter = atomIdMap.find(atomId1);
					if(iter == atomIdMap.end())
    					throw Exception(tr("Nonexistent atom ID encountered in line %1 of data file.").arg(stream.lineNumber()));
					*atomIndex = iter->second;
				}
				else *atomIndex = atomId1;
				++atomIndex;

				if(atomId2 < 0 || atomId2 >= (qlonglong)identifierProperty->size() || atomId2 != identifierProperty->getInt64(atomId2)) {
					auto iter = atomIdMap.find(atomId2);
					if(iter == atomIdMap.end())
    					throw Exception(tr("Nonexistent atom ID encountered in line %1 of data file.").arg(stream.lineNumber()));
					*atomIndex = iter->second;
				}
				else *atomIndex = atomId2;
				++atomIndex;

				if(*bondType < 1 || *bondType > nbondtypes)
					throw Exception(tr("Bond type out of range in Bonds section of LAMMPS data file at line %1.").arg(stream.lineNumber()));
    			bondType++;
			}
			frameData->generateBondPeriodicImageProperty();
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

	// Assign masses to particles based on their type.
	if(!massTable.empty() && !frameData->findStandardParticleProperty(ParticlesObject::MassProperty)) {
		PropertyPtr massProperty = ParticlesObject::OOClass().createStandardStorage(natoms, ParticlesObject::MassProperty, false);
		frameData->addParticleProperty(massProperty);
		const int* atomType = typeProperty->constDataInt();
		for(FloatType& mass : massProperty->floatRange()) {
			mass = massTable[*atomType++];
		}
	}

	// Sort particles by ID.
	if(_sortParticles)
		frameData->sortParticlesById();

	QString statusString = tr("Number of particles: %1").arg(natoms);
	if(nbondtypes > 0 || nbonds > 0)
		statusString += tr("\nNumber of bonds: %1").arg(nbonds);
	frameData->setStatus(statusString);
	return frameData;
}

/******************************************************************************
* Detects or verifies the LAMMPS atom style used by the data file.
******************************************************************************/
bool LAMMPSDataImporter::FrameLoader::detectAtomStyle(const char* firstLine, const QByteArray& keywordLine)
{
	QRegularExpression ws_re(QStringLiteral("\\s+"));

	// Data files may contain a comment string after the 'Atoms' keyword indicating the LAMMPS atom style.
	QString atomStyleHint;
	QStringList atomSubStyleHints;
	int commentStart = keywordLine.indexOf('#');
	if(commentStart != -1) {
		QStringList tokens = QString::fromLatin1(keywordLine.data() + commentStart).split(ws_re, QString::SkipEmptyParts);
		if(tokens.size() >= 2) {
			atomStyleHint = tokens[1];
			atomSubStyleHints = tokens.mid(2);
		}
	}

	// Count number of columns in first data line of the Atoms section.
	QString str = QString::fromLatin1(firstLine);
	commentStart = str.indexOf(QChar('#'));
	if(commentStart >= 0) str.truncate(commentStart);
	QStringList tokens = str.split(ws_re, QString::SkipEmptyParts);
	int count = tokens.size();

	if((_atomStyle == AtomStyle_Unknown || _atomStyle == AtomStyle_Hybrid) && !atomStyleHint.isEmpty()) {
		_atomStyle = parseAtomStyleHint(atomStyleHint);
		if(_atomStyle == AtomStyle_Hybrid) {
			if(!atomSubStyleHints.empty()) {
				_atomSubStyles.clear();
				for(const QString& subStyleHint : atomSubStyleHints) {
					_atomSubStyles.push_back(parseAtomStyleHint(subStyleHint));
					if(_atomSubStyles.back() == AtomStyle_Unknown || _atomSubStyles.back() == AtomStyle_Hybrid) {
						_atomSubStyles.clear();
						qWarning() << "This atom sub-style in LAMMPS data file is not supported by OVITO:" << subStyleHint;
						break;
					}
				}
			}
		}
	}

	// If no style hint is given in the data file, and if the number of
	// columns is 5 (or 5+3 including image flags), assume atom style is "atomic".
	if(_atomStyle == AtomStyle_Unknown) {
		if(count == 5) {
			_atomStyle = AtomStyle_Atomic;
			return false;
		}
		else if(count == 5+3) {
			if(!tokens[5].contains(QChar('.')) && !tokens[6].contains(QChar('.')) && !tokens[7].contains(QChar('.'))) {
				_atomStyle = AtomStyle_Atomic;
				return true;
			}
		}
	}
	else if(_atomStyle == AtomStyle_Hybrid) {
		if(count >= 5)
			return false;
	}
	else {
		// Check if the number of columns present in the data file matches the expectated count for the selected atom style.
		InputColumnMapping columnMapping = createColumnMapping(_atomStyle, false);
		if(columnMapping.size() == count) return false;
		else if(columnMapping.size()+3 == count) return true;
	}
	// Invalid or unexpected column count:
	_atomStyle = AtomStyle_Unknown;
	return false;
}

/******************************************************************************
* Parses a hint string for the LAMMPS atom style.
******************************************************************************/
LAMMPSDataImporter::LAMMPSAtomStyle LAMMPSDataImporter::FrameLoader::parseAtomStyleHint(const QString& atomStyleHint)
{
	if(atomStyleHint == QStringLiteral("angle")) return AtomStyle_Angle;
	else if(atomStyleHint == QStringLiteral("atomic")) return AtomStyle_Atomic;
	else if(atomStyleHint == QStringLiteral("body")) return AtomStyle_Body;
	else if(atomStyleHint == QStringLiteral("bond")) return AtomStyle_Bond;
	else if(atomStyleHint == QStringLiteral("charge")) return AtomStyle_Charge;
	else if(atomStyleHint == QStringLiteral("dipole")) return AtomStyle_Dipole;
	else if(atomStyleHint == QStringLiteral("dpd")) return AtomStyle_DPD;
	else if(atomStyleHint == QStringLiteral("edpd")) return AtomStyle_EDPD;
	else if(atomStyleHint == QStringLiteral("mdpd")) return AtomStyle_MDPD;
	else if(atomStyleHint == QStringLiteral("electron")) return AtomStyle_Electron;
	else if(atomStyleHint == QStringLiteral("ellipsoid")) return AtomStyle_Ellipsoid;
	else if(atomStyleHint == QStringLiteral("full")) return AtomStyle_Full;
	else if(atomStyleHint == QStringLiteral("line")) return AtomStyle_Line;
	else if(atomStyleHint == QStringLiteral("meso")) return AtomStyle_Meso;
	else if(atomStyleHint == QStringLiteral("molecular")) return AtomStyle_Molecular;
	else if(atomStyleHint == QStringLiteral("peri")) return AtomStyle_Peri;
	else if(atomStyleHint == QStringLiteral("smd")) return AtomStyle_SMD;
	else if(atomStyleHint == QStringLiteral("sphere")) return AtomStyle_Sphere;
	else if(atomStyleHint == QStringLiteral("template")) return AtomStyle_Template;
	else if(atomStyleHint == QStringLiteral("tri")) return AtomStyle_Tri;
	else if(atomStyleHint == QStringLiteral("wavepacket")) return AtomStyle_Wavepacket;
	else if(atomStyleHint == QStringLiteral("hybrid")) return AtomStyle_Hybrid;
	return AtomStyle_Unknown;
}

/******************************************************************************
* Sets up the mapping of data file columns to internal particle properties based on the selected LAMMPS atom style.
******************************************************************************/
InputColumnMapping LAMMPSDataImporter::FrameLoader::createColumnMapping(LAMMPSAtomStyle atomStyle, bool includeImageFlags)
{
	InputColumnMapping columnMapping;
	switch(atomStyle) {
	case AtomStyle_Angle:
		columnMapping.resize(6);
		columnMapping[0].mapStandardColumn(ParticlesObject::IdentifierProperty);
		columnMapping[1].mapStandardColumn(ParticlesObject::MoleculeProperty);
		columnMapping[2].mapStandardColumn(ParticlesObject::TypeProperty);
		columnMapping[3].mapStandardColumn(ParticlesObject::PositionProperty, 0);
		columnMapping[4].mapStandardColumn(ParticlesObject::PositionProperty, 1);
		columnMapping[5].mapStandardColumn(ParticlesObject::PositionProperty, 2);
		break;
	case AtomStyle_Atomic:
		columnMapping.resize(5);
		columnMapping[0].mapStandardColumn(ParticlesObject::IdentifierProperty);
		columnMapping[1].mapStandardColumn(ParticlesObject::TypeProperty);
		columnMapping[2].mapStandardColumn(ParticlesObject::PositionProperty, 0);
		columnMapping[3].mapStandardColumn(ParticlesObject::PositionProperty, 1);
		columnMapping[4].mapStandardColumn(ParticlesObject::PositionProperty, 2);
		break;
	case AtomStyle_Body:
		columnMapping.resize(7);
		columnMapping[0].mapStandardColumn(ParticlesObject::IdentifierProperty);
		columnMapping[1].mapStandardColumn(ParticlesObject::TypeProperty);
		// Ignore third column (bodyflag).
		columnMapping[3].mapStandardColumn(ParticlesObject::MassProperty);
		columnMapping[4].mapStandardColumn(ParticlesObject::PositionProperty, 0);
		columnMapping[5].mapStandardColumn(ParticlesObject::PositionProperty, 1);
		columnMapping[6].mapStandardColumn(ParticlesObject::PositionProperty, 2);
		break;
	case AtomStyle_Bond:
		columnMapping.resize(6);
		columnMapping[0].mapStandardColumn(ParticlesObject::IdentifierProperty);
		columnMapping[1].mapStandardColumn(ParticlesObject::MoleculeProperty);
		columnMapping[2].mapStandardColumn(ParticlesObject::TypeProperty);
		columnMapping[3].mapStandardColumn(ParticlesObject::PositionProperty, 0);
		columnMapping[4].mapStandardColumn(ParticlesObject::PositionProperty, 1);
		columnMapping[5].mapStandardColumn(ParticlesObject::PositionProperty, 2);
		break;
	case AtomStyle_Charge:
		columnMapping.resize(6);
		columnMapping[0].mapStandardColumn(ParticlesObject::IdentifierProperty);
		columnMapping[1].mapStandardColumn(ParticlesObject::TypeProperty);
		columnMapping[2].mapStandardColumn(ParticlesObject::ChargeProperty);
		columnMapping[3].mapStandardColumn(ParticlesObject::PositionProperty, 0);
		columnMapping[4].mapStandardColumn(ParticlesObject::PositionProperty, 1);
		columnMapping[5].mapStandardColumn(ParticlesObject::PositionProperty, 2);
		break;
	case AtomStyle_Dipole:
		columnMapping.resize(9);
		columnMapping[0].mapStandardColumn(ParticlesObject::IdentifierProperty);
		columnMapping[1].mapStandardColumn(ParticlesObject::TypeProperty);
		columnMapping[2].mapStandardColumn(ParticlesObject::ChargeProperty);
		columnMapping[3].mapStandardColumn(ParticlesObject::PositionProperty, 0);
		columnMapping[4].mapStandardColumn(ParticlesObject::PositionProperty, 1);
		columnMapping[5].mapStandardColumn(ParticlesObject::PositionProperty, 2);
		columnMapping[6].mapStandardColumn(ParticlesObject::DipoleOrientationProperty, 0);
		columnMapping[7].mapStandardColumn(ParticlesObject::DipoleOrientationProperty, 1);
		columnMapping[8].mapStandardColumn(ParticlesObject::DipoleOrientationProperty, 2);
		break;
	case AtomStyle_DPD:
		columnMapping.resize(6);
		columnMapping[0].mapStandardColumn(ParticlesObject::IdentifierProperty);
		columnMapping[1].mapStandardColumn(ParticlesObject::TypeProperty);
		columnMapping[2].mapCustomColumn(QStringLiteral("theta"), PropertyStorage::Float);
		columnMapping[3].mapStandardColumn(ParticlesObject::PositionProperty, 0);
		columnMapping[4].mapStandardColumn(ParticlesObject::PositionProperty, 1);
		columnMapping[5].mapStandardColumn(ParticlesObject::PositionProperty, 2);
		break;
	case AtomStyle_EDPD:
		columnMapping.resize(7);
		columnMapping[0].mapStandardColumn(ParticlesObject::IdentifierProperty);
		columnMapping[1].mapStandardColumn(ParticlesObject::TypeProperty);
		columnMapping[2].mapCustomColumn(QStringLiteral("edpd_temp"), PropertyStorage::Float);
		columnMapping[3].mapCustomColumn(QStringLiteral("edpd_cv"), PropertyStorage::Float);
		columnMapping[4].mapStandardColumn(ParticlesObject::PositionProperty, 0);
		columnMapping[5].mapStandardColumn(ParticlesObject::PositionProperty, 1);
		columnMapping[6].mapStandardColumn(ParticlesObject::PositionProperty, 2);
		break;
	case AtomStyle_MDPD:
		columnMapping.resize(6);
		columnMapping[0].mapStandardColumn(ParticlesObject::IdentifierProperty);
		columnMapping[1].mapStandardColumn(ParticlesObject::TypeProperty);
		columnMapping[2].mapCustomColumn(QStringLiteral("rho"), PropertyStorage::Float);
		columnMapping[3].mapStandardColumn(ParticlesObject::PositionProperty, 0);
		columnMapping[4].mapStandardColumn(ParticlesObject::PositionProperty, 1);
		columnMapping[5].mapStandardColumn(ParticlesObject::PositionProperty, 2);
		break;
	case AtomStyle_Electron:
		columnMapping.resize(8);
		columnMapping[0].mapStandardColumn(ParticlesObject::IdentifierProperty);
		columnMapping[1].mapStandardColumn(ParticlesObject::TypeProperty);
		columnMapping[2].mapStandardColumn(ParticlesObject::ChargeProperty);
		columnMapping[3].mapStandardColumn(ParticlesObject::SpinProperty);
		columnMapping[4].mapCustomColumn(QStringLiteral("eradius"), PropertyStorage::Float);
		columnMapping[5].mapStandardColumn(ParticlesObject::PositionProperty, 0);
		columnMapping[6].mapStandardColumn(ParticlesObject::PositionProperty, 1);
		columnMapping[7].mapStandardColumn(ParticlesObject::PositionProperty, 2);
		break;
	case AtomStyle_Ellipsoid:
		columnMapping.resize(7);
		columnMapping[0].mapStandardColumn(ParticlesObject::IdentifierProperty);
		columnMapping[1].mapStandardColumn(ParticlesObject::TypeProperty);
		columnMapping[2].mapCustomColumn(QStringLiteral("ellipsoidflag"), PropertyStorage::Int);
		columnMapping[3].mapCustomColumn(QStringLiteral("Density"), PropertyStorage::Float);
		columnMapping[4].mapStandardColumn(ParticlesObject::PositionProperty, 0);
		columnMapping[5].mapStandardColumn(ParticlesObject::PositionProperty, 1);
		columnMapping[6].mapStandardColumn(ParticlesObject::PositionProperty, 2);
		break;
	case AtomStyle_Full:
		columnMapping.resize(7);
		columnMapping[0].mapStandardColumn(ParticlesObject::IdentifierProperty);
		columnMapping[1].mapStandardColumn(ParticlesObject::MoleculeProperty);
		columnMapping[2].mapStandardColumn(ParticlesObject::TypeProperty);
		columnMapping[3].mapStandardColumn(ParticlesObject::ChargeProperty);
		columnMapping[4].mapStandardColumn(ParticlesObject::PositionProperty, 0);
		columnMapping[5].mapStandardColumn(ParticlesObject::PositionProperty, 1);
		columnMapping[6].mapStandardColumn(ParticlesObject::PositionProperty, 2);
		break;
	case AtomStyle_Line:
		columnMapping.resize(8);
		columnMapping[0].mapStandardColumn(ParticlesObject::IdentifierProperty);
		columnMapping[1].mapStandardColumn(ParticlesObject::MoleculeProperty);
		columnMapping[2].mapStandardColumn(ParticlesObject::TypeProperty);
		columnMapping[3].mapCustomColumn(QStringLiteral("lineflag"), PropertyStorage::Int);
		columnMapping[4].mapCustomColumn(QStringLiteral("Density"), PropertyStorage::Float);
		columnMapping[5].mapStandardColumn(ParticlesObject::PositionProperty, 0);
		columnMapping[6].mapStandardColumn(ParticlesObject::PositionProperty, 1);
		columnMapping[7].mapStandardColumn(ParticlesObject::PositionProperty, 2);
		break;
	case AtomStyle_Meso:
		columnMapping.resize(8);
		columnMapping[0].mapStandardColumn(ParticlesObject::IdentifierProperty);
		columnMapping[1].mapStandardColumn(ParticlesObject::TypeProperty);
		columnMapping[2].mapCustomColumn(QStringLiteral("rho"), PropertyStorage::Float);
		columnMapping[3].mapCustomColumn(QStringLiteral("e"), PropertyStorage::Float);
		columnMapping[4].mapCustomColumn(QStringLiteral("cv"), PropertyStorage::Float);
		columnMapping[5].mapStandardColumn(ParticlesObject::PositionProperty, 0);
		columnMapping[6].mapStandardColumn(ParticlesObject::PositionProperty, 1);
		columnMapping[7].mapStandardColumn(ParticlesObject::PositionProperty, 2);
		break;
	case AtomStyle_Molecular:
		columnMapping.resize(6);
		columnMapping[0].mapStandardColumn(ParticlesObject::IdentifierProperty);
		columnMapping[1].mapStandardColumn(ParticlesObject::MoleculeProperty);
		columnMapping[2].mapStandardColumn(ParticlesObject::TypeProperty);
		columnMapping[3].mapStandardColumn(ParticlesObject::PositionProperty, 0);
		columnMapping[4].mapStandardColumn(ParticlesObject::PositionProperty, 1);
		columnMapping[5].mapStandardColumn(ParticlesObject::PositionProperty, 2);
		break;
	case AtomStyle_Peri:
		columnMapping.resize(7);
		columnMapping[0].mapStandardColumn(ParticlesObject::IdentifierProperty);
		columnMapping[1].mapStandardColumn(ParticlesObject::TypeProperty);
		columnMapping[2].mapCustomColumn(QStringLiteral("Volume"), PropertyStorage::Float);
		columnMapping[3].mapCustomColumn(QStringLiteral("Density"), PropertyStorage::Float);
		columnMapping[4].mapStandardColumn(ParticlesObject::PositionProperty, 0);
		columnMapping[5].mapStandardColumn(ParticlesObject::PositionProperty, 1);
		columnMapping[6].mapStandardColumn(ParticlesObject::PositionProperty, 2);
		break;
	case AtomStyle_SMD:
		columnMapping.resize(10);
		columnMapping[0].mapStandardColumn(ParticlesObject::IdentifierProperty);
		columnMapping[1].mapStandardColumn(ParticlesObject::TypeProperty);
		columnMapping[2].mapCustomColumn(QStringLiteral("molecule"), PropertyStorage::Float);
		columnMapping[3].mapCustomColumn(QStringLiteral("Volume"), PropertyStorage::Float);
		columnMapping[4].mapStandardColumn(ParticlesObject::MassProperty);
		columnMapping[5].mapCustomColumn(QStringLiteral("kernelradius"), PropertyStorage::Float);
		columnMapping[6].mapCustomColumn(QStringLiteral("contactradius"), PropertyStorage::Float);
		columnMapping[7].mapStandardColumn(ParticlesObject::PositionProperty, 0);
		columnMapping[8].mapStandardColumn(ParticlesObject::PositionProperty, 1);
		columnMapping[9].mapStandardColumn(ParticlesObject::PositionProperty, 2);
		break;
	case AtomStyle_Sphere:
		columnMapping.resize(7);
		columnMapping[0].mapStandardColumn(ParticlesObject::IdentifierProperty);
		columnMapping[1].mapStandardColumn(ParticlesObject::TypeProperty);
		columnMapping[2].mapStandardColumn(ParticlesObject::RadiusProperty);
		columnMapping[3].mapCustomColumn(QStringLiteral("Density"), PropertyStorage::Float);
		columnMapping[4].mapStandardColumn(ParticlesObject::PositionProperty, 0);
		columnMapping[5].mapStandardColumn(ParticlesObject::PositionProperty, 1);
		columnMapping[6].mapStandardColumn(ParticlesObject::PositionProperty, 2);
		break;
	case AtomStyle_Template:
		columnMapping.resize(8);
		columnMapping[0].mapStandardColumn(ParticlesObject::IdentifierProperty);
		columnMapping[1].mapStandardColumn(ParticlesObject::MoleculeProperty);
		columnMapping[2].mapCustomColumn(QStringLiteral("templateindex"), PropertyStorage::Int);
		columnMapping[3].mapCustomColumn(QStringLiteral("templateatom"), PropertyStorage::Int64);
		columnMapping[4].mapStandardColumn(ParticlesObject::TypeProperty);
		columnMapping[5].mapStandardColumn(ParticlesObject::PositionProperty, 0);
		columnMapping[6].mapStandardColumn(ParticlesObject::PositionProperty, 1);
		columnMapping[7].mapStandardColumn(ParticlesObject::PositionProperty, 2);
		break;
	case AtomStyle_Tri:
		columnMapping.resize(8);
		columnMapping[0].mapStandardColumn(ParticlesObject::IdentifierProperty);
		columnMapping[1].mapStandardColumn(ParticlesObject::MoleculeProperty);
		columnMapping[2].mapStandardColumn(ParticlesObject::TypeProperty);
		columnMapping[3].mapCustomColumn(QStringLiteral("triangleflag"), PropertyStorage::Int);
		columnMapping[4].mapCustomColumn(QStringLiteral("Density"), PropertyStorage::Float);
		columnMapping[5].mapStandardColumn(ParticlesObject::PositionProperty, 0);
		columnMapping[6].mapStandardColumn(ParticlesObject::PositionProperty, 1);
		columnMapping[7].mapStandardColumn(ParticlesObject::PositionProperty, 2);
		break;
	case AtomStyle_Wavepacket:
		columnMapping.resize(11);
		columnMapping[0].mapStandardColumn(ParticlesObject::IdentifierProperty);
		columnMapping[1].mapStandardColumn(ParticlesObject::TypeProperty);
		columnMapping[2].mapStandardColumn(ParticlesObject::ChargeProperty);
		columnMapping[3].mapStandardColumn(ParticlesObject::SpinProperty);
		columnMapping[4].mapCustomColumn(QStringLiteral("eradius"), PropertyStorage::Float);
		columnMapping[5].mapCustomColumn(QStringLiteral("etag"), PropertyStorage::Float);
		columnMapping[6].mapCustomColumn(QStringLiteral("cs_re"), PropertyStorage::Float);
		columnMapping[7].mapCustomColumn(QStringLiteral("cs_im"), PropertyStorage::Float);
		columnMapping[8].mapStandardColumn(ParticlesObject::PositionProperty, 0);
		columnMapping[9].mapStandardColumn(ParticlesObject::PositionProperty, 1);
		columnMapping[10].mapStandardColumn(ParticlesObject::PositionProperty, 2);
		break;
	case AtomStyle_Hybrid:
		columnMapping.resize(5);
		columnMapping[0].mapStandardColumn(ParticlesObject::IdentifierProperty);
		columnMapping[1].mapStandardColumn(ParticlesObject::TypeProperty);
		columnMapping[2].mapStandardColumn(ParticlesObject::PositionProperty, 0);
		columnMapping[3].mapStandardColumn(ParticlesObject::PositionProperty, 1);
		columnMapping[4].mapStandardColumn(ParticlesObject::PositionProperty, 2);
		break;
	case AtomStyle_Unknown:
		break;
	}
	if(includeImageFlags) {
		columnMapping.push_back({ParticlesObject::PeriodicImageProperty, 0});
		columnMapping.push_back({ParticlesObject::PeriodicImageProperty, 1});
		columnMapping.push_back({ParticlesObject::PeriodicImageProperty, 2});
	}
	return columnMapping;
}


OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
