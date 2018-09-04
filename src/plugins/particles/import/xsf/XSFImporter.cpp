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
#include <plugins/particles/import/ParticleFrameData.h>
#include <plugins/particles/import/InputColumnMapping.h>
#include <core/utilities/io/NumberParsing.h>
#include <core/utilities/io/CompressedTextReader.h>
#include "XSFImporter.h"

#include <boost/algorithm/string.hpp>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Import) OVITO_BEGIN_INLINE_NAMESPACE(Formats)

IMPLEMENT_OVITO_CLASS(XSFImporter);	

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
bool XSFImporter::OOMetaClass::checkFileFormat(QFileDevice& input, const QUrl& sourceLocation) const
{
	// Open input file.
	CompressedTextReader stream(input, sourceLocation.path());

	// Look for 'ATOMS', 'BEGIN_BLOCK_DATAGRID' or other XSF-specific keywords.
	// One of them must appear within the first 40 lines of the file.
	for(int i = 0; i < 40 && !stream.eof(); i++) {
		const char* line = stream.readLineTrimLeft(1024);

		if(boost::algorithm::starts_with(line, "ATOMS")) {
			// Make sure the line following the keyword has the right format.
			return (sscanf(stream.readLineTrimLeft(1024), "%*s %*g %*g %*g") == 0);
		}
		else if(boost::algorithm::starts_with(line, "PRIMCOORD") || boost::algorithm::starts_with(line, "CONVCOORD")) {
			// Make sure the line following the keyword has the right format.
			return (sscanf(stream.readLineTrimLeft(1024), "%*ull %*i") == 0);
		}
		else if(boost::algorithm::starts_with(line, "BEGIN_BLOCK_DATAGRID")) {
			return true;
		}
	}
	return false;
}

/******************************************************************************
* Scans the given input file to find all contained simulation frames.
******************************************************************************/
void XSFImporter::FrameFinder::discoverFramesInFile(QFile& file, const QUrl& sourceUrl, QVector<FileSourceImporter::Frame>& frames)
{
	CompressedTextReader stream(file, sourceUrl.path());
	setProgressText(tr("Scanning XSF file %1").arg(stream.filename()));
	setProgressMaximum(stream.underlyingSize());

	int nFrames = 1;
	while(!stream.eof() && !isCanceled()) {
		const char* line = stream.readLineTrimLeft(1024);
		if(boost::algorithm::starts_with(line, "ANIMSTEPS")) {
			if(sscanf(line, "ANIMSTEPS %i", &nFrames) != 1 || nFrames < 1)
				throw Exception(tr("XSF file parsing error. Invalid ANIMSTEPS in line %1:\n%2").arg(stream.lineNumber()).arg(stream.lineString()));			
			break;
		}
		else if(line[0] != '#') {
			break;
		}
		setProgressValueIntermittent(stream.underlyingByteOffset());
	}

	QFileInfo fileInfo(stream.device().fileName());
	QDateTime lastModified = fileInfo.lastModified();
	for(int i = 0; i < nFrames; i++) {
		Frame frame;
		frame.sourceFile = sourceUrl;
		frame.lineNumber = i;
		frame.lastModificationTime = lastModified;
		frame.label = tr("Frame %1").arg(i);
		frames.push_back(frame);
	}
}

/******************************************************************************
* Parses the given input file.
******************************************************************************/
FileSourceImporter::FrameDataPtr XSFImporter::FrameLoader::loadFile(QFile& file)
{
	// Open file for reading.
	CompressedTextReader stream(file, frame().sourceFile.path());
	setProgressText(tr("Reading XSF file %1").arg(frame().sourceFile.toString(QUrl::RemovePassword | QUrl::PreferLocalFile | QUrl::PrettyDecoded)));

	// Create the destination container for the loaded data.
	auto frameData = std::make_shared<ParticleFrameData>();

	// The animation frame number to load from the XSF file.
	int frameNumber = frame().lineNumber + 1;

	while(!stream.eof()) {
		if(isCanceled()) return {};
		const char* line = stream.readLineTrimLeft(1024);
		if(boost::algorithm::starts_with(line, "ATOMS")) {

			int anim;
			if(sscanf(line, "ATOMS %i", &anim) == 1 && anim != frameNumber)
				continue;
			
			std::unique_ptr<ParticleFrameData::TypeList> typeList = std::make_unique<ParticleFrameData::TypeList>();
			std::vector<Point3> coords;
			std::vector<int> types;
			std::vector<Vector3> forces;
			while(!stream.eof()) {
				Point3 pos;
				Vector3 f;
				char atomTypeName[16];
				int nfields = sscanf(stream.readLine(), "%15s " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING,  
						atomTypeName, &pos.x(), &pos.y(), &pos.z(), &f.x(), &f.y(), &f.z());
				if(nfields != 4 && nfields != 7) break;
				coords.push_back(pos);
				int atomTypeId;
				if(sscanf(atomTypeName, "%i", &atomTypeId) == 1) {
					typeList->addTypeId(atomTypeId);
					types.push_back(atomTypeId);
				}
				else {
					types.push_back(typeList->addTypeName(atomTypeName));
				}
				if(nfields == 7) {
					forces.resize(coords.size(), Vector3::Zero());
					forces.back() = f;
				}
				if(isCanceled()) return {};
			}
			if(coords.empty())
				throw Exception(tr("Invalid ATOMS section in line %1 of XSF file.").arg(stream.lineNumber()));

			PropertyPtr posProperty = ParticlesObject::OOClass().createStandardStorage(coords.size(), ParticlesObject::PositionProperty, false);
			frameData->addParticleProperty(posProperty);
			std::copy(coords.begin(), coords.end(), posProperty->dataPoint3());
			
			PropertyPtr typeProperty = ParticlesObject::OOClass().createStandardStorage(types.size(), ParticlesObject::TypeProperty, false);
			frameData->addParticleProperty(typeProperty);
			std::copy(types.begin(), types.end(), typeProperty->dataInt());
			frameData->setPropertyTypesList(typeProperty, std::move(typeList));

			if(forces.size() != 0) {
				PropertyPtr forceProperty = ParticlesObject::OOClass().createStandardStorage(coords.size(), ParticlesObject::ForceProperty, false);
				frameData->addParticleProperty(forceProperty);
				std::copy(forces.begin(), forces.end(), forceProperty->dataVector3());
			}

			frameData->setStatus(tr("%1 atoms").arg(coords.size()));

			// If the input file does not contain simulation cell info,
			// Use bounding box of particles as simulation cell.
			Box3 boundingBox;
			boundingBox.addPoints(posProperty->constDataPoint3(), posProperty->size());
			frameData->simulationCell().setMatrix(AffineTransformation(
					Vector3(boundingBox.sizeX(), 0, 0),
					Vector3(0, boundingBox.sizeY(), 0),
					Vector3(0, 0, boundingBox.sizeZ()),
					boundingBox.minc - Point3::Origin()));
			frameData->simulationCell().setPbcFlags(false, false, false);
		}

		if(boost::algorithm::starts_with(line, "CRYSTAL")) {
			frameData->simulationCell().setPbcFlags(true, true, true);
		}
		else if(boost::algorithm::starts_with(line, "SLAB")) {
			frameData->simulationCell().setPbcFlags(true, true, false);
		}
		else if(boost::algorithm::starts_with(line, "POLYMER")) {
			frameData->simulationCell().setPbcFlags(true, false, false);
		}
		else if(boost::algorithm::starts_with(line, "MOLECULE")) {
			frameData->simulationCell().setPbcFlags(false, false, false);
		}
		else if(boost::algorithm::starts_with(line, "PRIMVEC")) {
			int anim;
			if(sscanf(line, "PRIMVEC %i", &anim) == 1 && anim != frameNumber)
				continue;
			AffineTransformation cell = AffineTransformation::Identity();
			for(size_t i = 0; i < 3; i++) {
				if(sscanf(stream.readLine(), FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING, 
						&cell.column(i).x(), &cell.column(i).y(), &cell.column(i).z()) != 3)
					throw Exception(tr("Invalid cell vector in XSF file at line %1").arg(stream.lineNumber()));
			}
			frameData->simulationCell().setMatrix(cell);
		}
		else if(boost::algorithm::starts_with(line, "PRIMCOORD")) {
			int anim;
			if(sscanf(line, "PRIMCOORD %i", &anim) == 1 && anim != frameNumber)
				continue;
			
			// Parse number of atoms.
			unsigned long long u;
			int ii;
			if(sscanf(stream.readLine(), "%llu %i", &u, &ii) != 2)
				throw Exception(tr("XSF file parsing error. Invalid number of atoms in line %1:\n%2").arg(stream.lineNumber()).arg(stream.lineString()));
			size_t natoms = (size_t)u;

			qint64 atomsListOffset = stream.byteOffset();
			int atomsLineNumber = stream.lineNumber();

			// Detect number of columns.
			Point3 pos;
			Vector3 f;
			int nfields = sscanf(stream.readLine(), "%*s " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING,  
					&pos.x(), &pos.y(), &pos.z(), &f.x(), &f.y(), &f.z());
			if(nfields != 3 && nfields != 6)
				throw Exception(tr("XSF file parsing error. Invalid number of data columns in line %1.").arg(stream.lineNumber()));
		
			// Prepare the file column to particle property mapping.
			InputColumnMapping columnMapping;
			columnMapping.resize(nfields + 1);
			columnMapping[0].mapStandardColumn(ParticlesObject::TypeProperty);
			columnMapping[1].mapStandardColumn(ParticlesObject::PositionProperty, 0);
			columnMapping[2].mapStandardColumn(ParticlesObject::PositionProperty, 1);
			columnMapping[3].mapStandardColumn(ParticlesObject::PositionProperty, 2);
			if(nfields == 6) {
				columnMapping[4].mapStandardColumn(ParticlesObject::ForceProperty, 0);
				columnMapping[5].mapStandardColumn(ParticlesObject::ForceProperty, 1);
				columnMapping[6].mapStandardColumn(ParticlesObject::ForceProperty, 2);
			}

			// Jump back to start of atoms list.
			stream.seek(atomsListOffset, atomsLineNumber);

			// Parse atoms data.
			InputColumnReader columnParser(columnMapping, *frameData, natoms);
			setProgressMaximum(natoms);
			for(size_t i = 0; i < natoms; i++) {
				if(!setProgressValueIntermittent(i)) return {};
				try {
					columnParser.readParticle(i, stream.readLine());
				}
				catch(Exception& ex) {
					throw ex.prependGeneralMessage(tr("Parsing error in line %1 of XSF file.").arg(atomsLineNumber + i));
				}
			}
		}
		else if(boost::algorithm::starts_with(line, "BEGIN_DATAGRID_3D_") || boost::algorithm::starts_with(line, "DATAGRID_3D_")) {
			QString name = QString::fromLatin1(line + (boost::algorithm::starts_with(line, "BEGIN_DATAGRID_3D_") ? 18 : 12)).trimmed();
			for(const PropertyPtr& p : frameData->voxelProperties()) {
				if(p->name() == name)
					throw Exception(tr("XSF file parsing error. Duplicate data grid identifier in line %1: %2").arg(stream.lineNumber()).arg(name));
			}

			size_t nx, ny, nz;
			if(sscanf(stream.readLine(), "%zu %zu %zu", &nx, &ny, &nz) != 3)
				throw Exception(tr("XSF file parsing error. Invalid data grid specification in line %1: %2").arg(stream.lineNumber()).arg(stream.lineString()));
			if(frameData->voxelGridShape().empty())
				frameData->setVoxelGridShape({nx, ny, nz});
			else if(frameData->voxelGridShape() != std::vector<size_t>({nx, ny, nz}))
				throw Exception(tr("XSF file parsing error. Data grid specification in line %1 is incompatible with preceding grid dimensions found in the same file.").arg(stream.lineNumber()));			

			AffineTransformation cell = AffineTransformation::Identity();
			if(sscanf(stream.readLine(), FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING, 
					&cell.column(3).x(), &cell.column(3).y(), &cell.column(3).z()) != 3)
				throw Exception(tr("Invalid cell origin in XSF file at line %1").arg(stream.lineNumber()));
			for(size_t i = 0; i < 3; i++) {
				if(sscanf(stream.readLine(), FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING, 
						&cell.column(i).x(), &cell.column(i).y(), &cell.column(i).z()) != 3)
					throw Exception(tr("Invalid cell vector in XSF file at line %1").arg(stream.lineNumber()));
			}
			frameData->simulationCell().setMatrix(cell);
				
			PropertyPtr fieldQuantity = std::make_shared<PropertyStorage>(nx*ny*nz, PropertyStorage::Float, 1, 0, name, false);
			FloatType* data = fieldQuantity->dataFloat();
			setProgressMaximum(fieldQuantity->size());
			const char* s = "";
			for(size_t i = 0; i < fieldQuantity->size(); i++, ++data) {
				const char* token;
				for(;;) {
					while(*s == ' ' || *s == '\t') ++s;
					token = s;
					while(*s > ' ' || *s < 0) ++s;
					if(s != token) break;
					s = stream.readLine();
				}
				if(!parseFloatType(token, s, *data))
					throw Exception(tr("Invalid numeric value in data grid section in line %1: \"%2\"").arg(stream.lineNumber()).arg(QString::fromLocal8Bit(token, s - token)));
				if(*s != '\0')
					s++;

				if(!setProgressValueIntermittent(i)) return {};						
			}
			frameData->addVoxelProperty(fieldQuantity);
		}
	}

	// Translate atomic numbers into element names.
	if(PropertyPtr typeProperty = frameData->findStandardParticleProperty(ParticlesObject::TypeProperty)) {
		if(ParticleFrameData::TypeList* typeList = frameData->propertyTypesList(typeProperty)) {
			for(const auto& t : typeList->types()) {
				if(t.name.isEmpty() && t.id >= 1 && t.id < sizeof(chemical_symbols)/sizeof(chemical_symbols[0])) {
					typeList->setTypeName(t.id, chemical_symbols[t.id]);
				}
			}
		}
	}
	
	return frameData;
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
