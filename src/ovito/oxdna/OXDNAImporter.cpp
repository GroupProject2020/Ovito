////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2020 Alexander Stukowski
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
#include <ovito/core/utilities/io/CompressedTextReader.h>
#include <ovito/core/utilities/io/FileManager.h>
#include <ovito/core/app/Application.h>
#include "OXDNAImporter.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Import) OVITO_BEGIN_INLINE_NAMESPACE(Formats)

IMPLEMENT_OVITO_CLASS(OXDNAImporter);
DEFINE_PROPERTY_FIELD(OXDNAImporter, topologyFileUrl);
SET_PROPERTY_FIELD_LABEL(OXDNAImporter, topologyFileUrl, "Topology file");

/******************************************************************************
* Checks if the given file has format that can be read by this importer.
******************************************************************************/
bool OXDNAImporter::OOMetaClass::checkFileFormat(QFileDevice& input, const QUrl& sourceLocation) const
{
	// Open input file for reading.
	CompressedTextReader stream(input, sourceLocation.path());

	// Check for a valid "t = ..." line.
	FloatType t;
	if(sscanf(stream.readLineTrimLeft(128), "t = " FLOATTYPE_SCANF_STRING, &t) != 1) 
		return false; 

	// Check for a valid "b = ..." line.
	Vector3 b;
	if(sscanf(stream.readLineTrimLeft(128), "b = " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING, &b.x(), &b.y(), &b.z()) != 3) 
		return false; 

	// Check for a valid "E = ..." line.
	FloatType Etot, U, K;
	if(sscanf(stream.readLineTrimLeft(128), "E = " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING, &Etot, &U, &K) != 3) 
		return false; 

	return true;
}

/******************************************************************************
* Parses the given input file.
******************************************************************************/
FileSourceImporter::FrameDataPtr OXDNAImporter::FrameLoader::loadFile(QFile& file)
{
	// Locate the topology file. 
	QUrl topoFileUrl = _userSpecifiedTopologyUrl;
	if(!topoFileUrl.isValid()) {
		// If no explicit path for the topology file was specified by the user, try to infer it from the
		// base name of the configuration file. Replace original suffix of configuration file name with ".top".
		topoFileUrl = frame().sourceFile;
		QFileInfo filepath(topoFileUrl.path());
		topoFileUrl.setPath(filepath.path() + QStringLiteral("/") + filepath.completeBaseName() + QStringLiteral(".top"));

		// Check if the topology file exists.
		if(!topoFileUrl.isValid() || (topoFileUrl.isLocalFile() && !QFileInfo::exists(topoFileUrl.toLocalFile())))
			throw Exception(tr("Could not locate corresponding topology file for oxDNA configuration file '%1'. "
				"Tried inferred path '%2', but the file does not exist. Please specify the path of the topology file explicitly.")
					.arg(frame().sourceFile.toString(QUrl::RemovePassword | QUrl::PreferLocalFile | QUrl::PrettyDecoded))
					.arg(topoFileUrl.toLocalFile()));
	}

	// Fetch the oxDNA topology file if it is stored on a remote location.
	SharedFuture<QString> localTopologyFileFuture = Application::instance()->fileManager()->fetchUrl(taskManager(), topoFileUrl);
	if(!waitForFuture(localTopologyFileFuture))
		return {};

	// Open oxDNA topology file for reading.
	QFile localTopologyFile(localTopologyFileFuture.result());
	CompressedTextReader topoStream(localTopologyFile, topoFileUrl.path());
	setProgressText(tr("Reading oxDNA topology file %1").arg(topoFileUrl.toString(QUrl::RemovePassword | QUrl::PreferLocalFile | QUrl::PrettyDecoded)));

	// Create the container for the particle data to be loaded.
	auto frameData = std::make_shared<ParticleFrameData>();

	// Parse number of nucleotides and number of strands.
	unsigned long long numNucleotidesLong;
	int numStrandsLong;
	if(sscanf(topoStream.readLine(), "%llu %i", &numNucleotidesLong, &numStrandsLong) != 2)
		throw Exception(tr("Invalid number of nucleotides or strands in line %1 of oxDNA topology file: %2").arg(topoStream.lineNumber()).arg(topoStream.lineString().trimmed()));

	// Define nucleotide types.
	PropertyAccess<int> typeProperty = frameData->addParticleProperty(ParticlesObject::OOClass().createStandardStorage(numNucleotidesLong, ParticlesObject::TypeProperty, false));
	ParticleFrameData::TypeList* typeList = frameData->propertyTypesList(typeProperty);
	typeList->addTypeId(1, QStringLiteral("A"));
	typeList->addTypeId(2, QStringLiteral("C"));
	typeList->addTypeId(3, QStringLiteral("G"));
	typeList->addTypeId(4, QStringLiteral("T"));

	// The list of bonds between nucleotides.
	std::vector<ParticleIndexPair> bonds;
	bonds.reserve(numNucleotidesLong);

	// Parse the nucleotides list in the topology file.
	setProgressMaximum(numNucleotidesLong);
	int* particleTypeIter = typeProperty.begin();
	for(size_t i = 0; i < numNucleotidesLong; i++) {
		if(!setProgressValueIntermittent(i)) return {};

		int strandId;
		char base;
		qlonglong neighbor1, neighbor2;
		if(sscanf(topoStream.readLine(), "%u %c %lld %lld", &strandId, &base, &neighbor1, &neighbor2) != 4)
			throw Exception(tr("Invalid nucleotide specification in line %1 of oxDNA topology file: %2").arg(topoStream.lineNumber()).arg(topoStream.lineString()));

		if(strandId < 1 || strandId > numStrandsLong)
			throw Exception(tr("Strand ID %2 in line %1 of oxDNA topology file is out of range.").arg(topoStream.lineNumber()).arg(strandId));

		if(neighbor1 < -1 || neighbor1 >= (qlonglong)numNucleotidesLong)
			throw Exception(tr("3' neighbor %2 in line %1 of oxDNA topology file is out of range.").arg(topoStream.lineNumber()).arg(neighbor1));

		if(neighbor2 < -1 || neighbor2 >= (qlonglong)numNucleotidesLong)
			throw Exception(tr("5' neighbor %2 in line %1 of oxDNA topology file is out of range.").arg(topoStream.lineNumber()).arg(neighbor2));

		if(neighbor2 != -1)
			bonds.push_back({(qlonglong)i, neighbor2});

		switch(base) {
		case 'A': *particleTypeIter++ = 1; break;
		case 'C': *particleTypeIter++ = 2; break;
		case 'G': *particleTypeIter++ = 3; break;
		case 'T': *particleTypeIter++ = 4; break;
		default:
			throw Exception(tr("Base in line %1 of oxDNA topology file is invalid. Must be one of {A,C,G,T}.").arg(topoStream.lineNumber()));			
		}

	}

	// Create and fill bonds topology storage.
	PropertyAccess<ParticleIndexPair> bondTopologyProperty = frameData->addBondProperty(BondsObject::OOClass().createStandardStorage(bonds.size(), BondsObject::TopologyProperty, false));
	boost::copy(bonds, bondTopologyProperty.begin());

	// Define strands list.
	PropertyAccess<int> strandsProperty = frameData->addParticleProperty(ParticlesObject::OOClass().createStandardStorage(numNucleotidesLong, ParticlesObject::StrandProperty, false));
	ParticleFrameData::TypeList* strandsList = frameData->propertyTypesList(strandsProperty);
	for(int i = 1; i <= numStrandsLong; i++)
		strandsList->addTypeId(i);

	// Open oxDNA configuration file for reading.
	CompressedTextReader stream(file, frame().sourceFile.path());
	setProgressText(tr("Reading oxDNA file %1").arg(frame().sourceFile.toString(QUrl::RemovePassword | QUrl::PreferLocalFile | QUrl::PrettyDecoded)));

	// Jump to byte offset.
	if(frame().byteOffset != 0)
		stream.seek(frame().byteOffset, frame().lineNumber);

	// Parse the 1st line: "t = T".
	FloatType simulationTime;
	if(sscanf(stream.readLineTrimLeft(), "t = " FLOATTYPE_SCANF_STRING, &simulationTime) != 1) 
		throw Exception(tr("Invalid header format encountered in line %1 of oxDNA configuration file: %2").arg(stream.lineNumber()).arg(stream.lineString()));
	frameData->attributes().insert(QStringLiteral("Time"), QVariant::fromValue(simulationTime));

	// Parse the 2nd line: "b = Lx Ly Lz".
	Vector3 boxSize;
	if(sscanf(stream.readLineTrimLeft(), "b = " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING, &boxSize.x(), &boxSize.y(), &boxSize.z()) != 3) 
		throw Exception(tr("Invalid header format encountered in line %1 of oxDNA configuration file: %2").arg(stream.lineNumber()).arg(stream.lineString()));

	// Parse the 3rd line: "E = Etot U K".
	FloatType Etot, U, K;
	if(sscanf(stream.readLineTrimLeft(), "E = " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING, &Etot, &U, &K) != 3) 
		throw Exception(tr("Invalid header format encountered in line %1 of oxDNA configuration file: %2").arg(stream.lineNumber()).arg(stream.lineString()));
	frameData->attributes().insert(QStringLiteral("Etot"), QVariant::fromValue(Etot));
	frameData->attributes().insert(QStringLiteral("U"), QVariant::fromValue(U));
	frameData->attributes().insert(QStringLiteral("K"), QVariant::fromValue(K));

	InputColumnMapping columnMapping(15);
	columnMapping[0].mapStandardColumn(ParticlesObject::PositionProperty, 0);
	columnMapping[1].mapStandardColumn(ParticlesObject::PositionProperty, 1);
	columnMapping[2].mapStandardColumn(ParticlesObject::PositionProperty, 2);
	columnMapping[3].mapCustomColumn(QStringLiteral("Backbone Base"), PropertyStorage::Float, 0);
	columnMapping[4].mapCustomColumn(QStringLiteral("Backbone Base"), PropertyStorage::Float, 1);
	columnMapping[5].mapCustomColumn(QStringLiteral("Backbone Base"), PropertyStorage::Float, 2);
	columnMapping[6].mapCustomColumn(QStringLiteral("Normal Vector"), PropertyStorage::Float, 0);
	columnMapping[7].mapCustomColumn(QStringLiteral("Normal Vector"), PropertyStorage::Float, 1);
	columnMapping[8].mapCustomColumn(QStringLiteral("Normal Vector"), PropertyStorage::Float, 2);
	columnMapping[9].mapStandardColumn(ParticlesObject::VelocityProperty, 0);
	columnMapping[10].mapStandardColumn(ParticlesObject::VelocityProperty, 1);
	columnMapping[11].mapStandardColumn(ParticlesObject::VelocityProperty, 2);
	columnMapping[12].mapStandardColumn(ParticlesObject::AngularVelocityProperty, 0);
	columnMapping[13].mapStandardColumn(ParticlesObject::AngularVelocityProperty, 1);
	columnMapping[14].mapStandardColumn(ParticlesObject::AngularVelocityProperty, 2);

	// Parse data columns.
	InputColumnReader columnParser(columnMapping, *frameData, numNucleotidesLong);
	for(size_t i = 0; i < numNucleotidesLong; i++) {
		if(!setProgressValueIntermittent(i)) return {};
		try {
			columnParser.readParticle(i, stream.readLine());
		}
		catch(Exception& ex) {
			throw ex.prependGeneralMessage(tr("Parsing error in line %1 of oxDNA configuration file (nucleotide index %2).").arg(stream.lineNumber()).arg(i));
		}
	}

	return std::move(frameData);
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
