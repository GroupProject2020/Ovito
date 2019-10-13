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

#include <ovito/crystalanalysis/CrystalAnalysis.h>
#include <ovito/crystalanalysis/objects/DislocationVis.h>
#include <ovito/core/dataset/io/FileSource.h>
#include <ovito/core/app/Application.h>
#include <ovito/core/utilities/io/CompressedTextReader.h>
#include <ovito/core/utilities/io/NumberParsing.h>
#include "ParaDiSImporter.h"

namespace Ovito { namespace CrystalAnalysis {

IMPLEMENT_OVITO_CLASS(ParaDiSImporter);

/******************************************************************************
* Checks if the given file has format that can be read by this importer.
******************************************************************************/
bool ParaDiSImporter::OOMetaClass::checkFileFormat(QFileDevice& input, const QUrl& sourceLocation) const
{
	// Open input file.
	CompressedTextReader stream(input, sourceLocation.path());

	// Read just the first line of the file.
	stream.readLine(20);

	// ParaDiS files start with the string "dataFileVersion".
	if(stream.lineStartsWithToken("dataFileVersion", true))
		return true;

	return false;
}

/******************************************************************************
* Parses the given input file.
******************************************************************************/
FileSourceImporter::FrameDataPtr ParaDiSImporter::FrameLoader::loadFile(QFile& file)
{
	// Open file for reading.
	CompressedTextReader stream(file, frame().sourceFile.path());
	setProgressText(tr("Reading ParaDiS file %1").arg(frame().sourceFile.toString(QUrl::RemovePassword | QUrl::PreferLocalFile | QUrl::PrettyDecoded)));
    setProgressMaximum(stream.underlyingSize());

	// Create the container structures for holding the loaded data.
	auto frameData = std::make_shared<DislocFrameData>();
	MicrostructureData& microstructure = frameData->microstructure();

    Vector3 minCoordinates = Vector3::Zero();
    Vector3 maxCoordinates = Vector3::Zero();
	for(;;) {
        // Report progress.
		if(isCanceled()) return {};
		setProgressValueIntermittent(stream.underlyingByteOffset());

        // Parse the next control parameter from the file.
		std::pair<QString, QVariant> keyValuePair = parseControlParameter(stream);
        if(keyValuePair.first.isEmpty()) break; // Reached end of file.
        if(keyValuePair.second.isValid() == false) break; // Reached end of first data file section.

        // Parse simulation cell geometry.
        if(keyValuePair.first == "minCoordinates") {
            QVariantList valueList = keyValuePair.second.value<QVariantList>();
            if(valueList.size() != 3)
                throw Exception(tr("Invalid 'minCoordinates' parameter value in line %1 of ParaDiS file.").arg(stream.lineNumber()));
            for(int i = 0; i < 3; i++)
                minCoordinates[i] = valueList[i].value<FloatType>();
        }
        else if(keyValuePair.first == "maxCoordinates") {
            QVariantList valueList = keyValuePair.second.value<QVariantList>();
            if(valueList.size() != 3)
                throw Exception(tr("Invalid 'maxCoordinates' parameter value in line %1 of ParaDiS file.").arg(stream.lineNumber()));
            for(int i = 0; i < 3; i++)
                maxCoordinates[i] = valueList[i].value<FloatType>();
        }
        else if(keyValuePair.first == "numFileSegments") {
            if(keyValuePair.second.toInt() != 1)
                throw Exception(tr("Invalid 'numFileSegments' parameter value in line %1 of ParaDiS file: %2. OVITO supports only single-segment ParaDiS files.").arg(stream.lineNumber()).arg(keyValuePair.second.toString()));
        }
	}

	frameData->simulationCell().setMatrix(AffineTransformation(
        Vector3(maxCoordinates.x() - minCoordinates.x(), 0, 0),
        Vector3(0, maxCoordinates.y() - minCoordinates.y(), 0),
        Vector3(0, 0, maxCoordinates.z() - minCoordinates.z()),
        minCoordinates
    ));
	frameData->simulationCell().setPbcFlags(true, true, true);

    // Skip to third section of data file.
    for(;;) {
        // Report progress.
		if(isCanceled()) return {};
		setProgressValueIntermittent(stream.underlyingByteOffset());
        if(stream.lineStartsWithToken("nodalData")) break;
        stream.readLine();
    }

    // Mapping from unique node IDs (domain ID + local index) to global node index.
    std::map<std::pair<int,int>, MicrostructureData::vertex_index> nodeMap;

    // Absolute Burgers vector component of 1/2<111>-type dislocations.
    FloatType bmag111 = 0;
    // Absolute Burgers vector component of 1/2<110>-type dislocations.
    FloatType bmag110 = 0;

    // Parse nodes list.
    while(!stream.eof()) {
        // Report progress.
		if(isCanceled()) return {};
        if((microstructure.vertexCount() % 1024) == 0)
		    setProgressValueIntermittent(stream.underlyingByteOffset());

		const char* line = stream.readLineTrimLeft();
        if(line[0] <= ' ') continue; // Skip empty lines
        if(line[0] == '#') continue; // Skip comment lines

        // Parse node specific data.
        std::pair<int,int> node_tag;
        Point3 coords;
        int num_arms, constraint;
        if(sscanf(stream.line(), "%i,%i " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " %i %i",
                &node_tag.first, &node_tag.second, &coords.x(), &coords.y(), &coords.z(), &num_arms, &constraint) != 7)
            throw Exception(tr("Invalid node record in line %1 of ParaDiS file: %2").arg(stream.lineNumber()).arg(stream.lineString()));

        // Create node (if it hasn't been defined before) and assign parsed coordinates.
        auto insertionResult = nodeMap.emplace(node_tag, HalfEdgeMesh::InvalidIndex);
        if(insertionResult.first->second == HalfEdgeMesh::InvalidIndex)
            insertionResult.first->second = microstructure.createVertex(coords);
        else
            microstructure.setVertexPosition(insertionResult.first->second, coords);

        // Parse segment specific data.
        for(int seg = 0; seg < num_arms; seg++) {
            // Read next non-empty line.
            for(;;) {
                const char* line = stream.readLineTrimLeft();
                if(line[0] <= ' ') continue; // Skip empty lines
                if(line[0] == '#') continue; // Skip comment lines
                break;
            }

            // Parse first line of segment specific data.
            std::pair<int,int> nbr_tag;
            Vector3 b;
            if(sscanf(stream.line(), "%i,%i " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING,
                    &nbr_tag.first, &nbr_tag.second, &b.x(), &b.y(), &b.z()) != 5)
                throw Exception(tr("Invalid segment record in line %1 of ParaDiS file: %2").arg(stream.lineNumber()).arg(stream.lineString()));

            // Read next non-empty line.
            for(;;) {
                const char* line = stream.readLineTrimLeft();
                if(line[0] <= ' ') continue; // Skip empty lines
                if(line[0] == '#') continue; // Skip comment lines
                break;
            }

            // Parse second line of segment specific data.
            Vector3 norm;
            if(sscanf(stream.line(), FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING,
                    &norm.x(), &norm.y(), &norm.z()) != 3)
                throw Exception(tr("Invalid segment record in line %1 of ParaDiS file: %2").arg(stream.lineNumber()).arg(stream.lineString()));

            // Look up or create the second node connected by the segment.
            auto insertionResult2 = nodeMap.emplace(nbr_tag, HalfEdgeMesh::InvalidIndex);
            if(insertionResult2.first->second == HalfEdgeMesh::InvalidIndex)
                insertionResult2.first->second = microstructure.createVertex(Point3::Origin());

            // Create the line segment connecting the two nodes.
            // Skip every other segment, because the ParaDiS defines each segment twice.
            if(insertionResult.first->second < insertionResult2.first->second)
    			microstructure.createDislocationSegment(insertionResult.first->second, insertionResult2.first->second, b, 0);

            // Look out for <111> and <110> type dislocations.
            if(bmag111 == 0) {
                if(qFuzzyCompare(std::abs(b.x()), std::abs(b.y())) && qFuzzyCompare(std::abs(b.y()), std::abs(b.z())))
                    bmag111 = std::abs(b.x());
            }
            if(bmag110 == 0) {
                if(qFuzzyCompare(std::abs(b.x()), std::abs(b.y())) && qFuzzyIsNull(b.z()))
                    bmag110 = std::abs(b.x());
                else if(qFuzzyCompare(std::abs(b.x()), std::abs(b.z())) && qFuzzyIsNull(b.y()))
                    bmag110 = std::abs(b.x());
                else if(qFuzzyCompare(std::abs(b.z()), std::abs(b.y())) && qFuzzyIsNull(b.x()))
                    bmag110 = std::abs(b.y());
            }
        }
    }

    // Heuristic to determine the likely crystal structure:
    // If there exists at least one <110>-type dislocation, it's like the FCC crystal structure.
    // Otherwise, if there exists at least one <111>-type dislocation, it's like the BCC crystal structure.
    // Note: This will only work for the default [100] crystal orientation.
    if(std::abs(bmag110 - sqrt(1.0/2.0)) < 1e-4) {
        frameData->setLatticeStructure(ParticleType::PredefinedStructureType::FCC);
        FloatType scaleFactor = 0.5 / sqrt(1.0/2.0);
        for(MicrostructureData::face_index face = 0; face < microstructure.faceCount(); face++) {
            microstructure.setBurgersVector(face, microstructure.burgersVector(face) * scaleFactor);
        }
    }
    else if(std::abs(bmag111 - sqrt(1.0/3.0)) < 1e-4) {
        frameData->setLatticeStructure(ParticleType::PredefinedStructureType::BCC);
        FloatType scaleFactor = 0.5 / sqrt(1.0/3.0);
        for(MicrostructureData::face_index face = 0; face < microstructure.faceCount(); face++) {
            microstructure.setBurgersVector(face, microstructure.burgersVector(face) * scaleFactor);
        }
    }
    microstructure.createRegion(frameData->latticeStructure());

    // Form continuous dislocation lines from the segments having the same Burgers vector.
    microstructure.makeContinuousDislocationLines();

	frameData->setStatus(tr("Number of nodes: %1\nNumber of dislocations: %2")
		.arg(microstructure.vertexCount())
		.arg(microstructure.faceCount()));

	return frameData;
}

/******************************************************************************
* Parses a control parameter from the ParaDiS file.
******************************************************************************/
std::pair<QString, QVariant> ParaDiSImporter::FrameLoader::parseControlParameter(CompressedTextReader& stream)
{
	while(!stream.eof()) {
		const char* line = stream.readLineTrimLeft();
        if(line[0] <= ' ') continue; // Skip empty lines
        if(line[0] == '#') continue; // Skip comment lines

        // Parse parameter identifier.
		const char* token_end = line;
		while(*token_end > ' ' && *token_end != '=') ++token_end;
        QString identifier = QString::fromLatin1(line, token_end - line);

        // Parse parameter value.
		while(*token_end <= ' ' && *token_end != '\0') ++token_end;
        if(*token_end != '=') return { identifier, QVariant() }; // A parameter without a value.

        const char* value_start = token_end + 1;
		while(*value_start <= ' ' && *value_start != '\0') ++value_start;
        if(*value_start == '\0') return { identifier, QVariant() }; // A parameter without a value.

        if(*value_start != '[') {
            return { identifier, QString::fromLatin1(value_start).trimmed() };
        }
        else {
            // Parse array of values.
            QVariantList values;
            const char* s = value_start + 1;
            for(;;) {
                const char* token;
                for(;;) {
                    while(*s <= ' ' && *s != '\0') ++s;
                    token = s;
                    while(*s > ' ' || *s < 0) ++s;
                    if(s != token) break;
                    s = stream.readLine();
                }
                if(token[0] == ']') break;
                FloatType val;
                if(!parseFloatType(token, s, val))
                    throw Exception(tr("Invalid value encountered in ParaDiS file (line %1): \"%2\"").arg(stream.lineNumber()).arg(QString::fromLocal8Bit(token, s - token)));
                values.push_back(val);
                if(*s != '\0')
                    s++;
            }
            return { identifier, std::move(values) };
        }
	}
    return {};
}

/******************************************************************************
* Inserts the data loaded by the FrameLoader into the provided data collection.
* This function is called by the system from the main thread after the
* asynchronous loading operation has finished.
******************************************************************************/
OORef<DataCollection> ParaDiSImporter::DislocFrameData::handOver(const DataCollection* existing, bool isNewFile, FileSource* fileSource)
{
	// Insert simulation cell.
	OORef<DataCollection> output = ParticleFrameData::handOver(existing, isNewFile, fileSource);

	// Insert microstructure.
	Microstructure* microstructureObj = const_cast<Microstructure*>(existing ? existing->getObject<Microstructure>() : nullptr);
	if(!microstructureObj) {
		microstructureObj = output->createObject<Microstructure>(fileSource);

		// Create a visual element for the dislocation lines.
		OORef<DislocationVis> dislocationVis = new DislocationVis(fileSource->dataset());
		if(Application::instance()->executionContext() == Application::ExecutionContext::Interactive)
			dislocationVis->loadUserDefaults();
		microstructureObj->setVisElement(dislocationVis);
	}
	else {
		output->addObject(microstructureObj);
	}
	microstructureObj->setDomain(output->getObject<SimulationCellObject>());
	microstructure().transferTo(microstructureObj);

	// Define crystal phase.
	PropertyObject* phaseProperty = microstructureObj->regions()->expectMutableProperty(SurfaceMeshRegions::PhaseProperty);
	OORef<MicrostructurePhase> phase = dynamic_object_cast<MicrostructurePhase>(phaseProperty->elementType(latticeStructure()));
	if(!phase) {
		phase = new MicrostructurePhase(phaseProperty->dataset());
		phase->setNumericId(latticeStructure());
		phase->setName(ParticleType::getPredefinedStructureTypeName(latticeStructure()));
		phaseProperty->addElementType(phase);
	}
	if(latticeStructure() == ParticleType::PredefinedStructureType::BCC) {
		phase->setCrystalSymmetryClass(MicrostructurePhase::CrystalSymmetryClass::CubicSymmetry);
		phase->setColor(ParticleType::getDefaultParticleColor(ParticlesObject::StructureTypeProperty, phase->name(), ParticleType::PredefinedStructureType::BCC));
		if(phase->burgersVectorFamilies().empty()) {
			phase->addBurgersVectorFamily(new BurgersVectorFamily(phase->dataset()));
			phase->addBurgersVectorFamily(new BurgersVectorFamily(phase->dataset(), 11, tr("1/2<111>"), Vector3(1.0f/2.0f, 1.0f/2.0f, 1.0f/2.0f), Color(0,1,0)));
			phase->addBurgersVectorFamily(new BurgersVectorFamily(phase->dataset(), 12, tr("<100>"), Vector3(1.0f, 0.0f, 0.0f), Color(1, 0.3f, 0.8f)));
			phase->addBurgersVectorFamily(new BurgersVectorFamily(phase->dataset(), 13, tr("<110>"), Vector3(1.0f, 1.0f, 0.0f), Color(0.2f, 0.5f, 1.0f)));
		}
	}
	else if(latticeStructure() == ParticleType::PredefinedStructureType::FCC) {
		phase->setCrystalSymmetryClass(MicrostructurePhase::CrystalSymmetryClass::CubicSymmetry);
		phase->setColor(ParticleType::getDefaultParticleColor(ParticlesObject::StructureTypeProperty, phase->name(), ParticleType::PredefinedStructureType::FCC));
		if(phase->burgersVectorFamilies().empty()) {
			phase->addBurgersVectorFamily(new BurgersVectorFamily(phase->dataset()));
			phase->addBurgersVectorFamily(new BurgersVectorFamily(phase->dataset(), 1, tr("1/2<110> (Perfect)"), Vector3(1.0f/2.0f, 1.0f/2.0f, 0.0f), Color(0.2f,0.2f,1)));
			phase->addBurgersVectorFamily(new BurgersVectorFamily(phase->dataset(), 2, tr("1/6<112> (Shockley)"), Vector3(1.0f/6.0f, 1.0f/6.0f, 2.0f/6.0f), Color(0,1,0)));
			phase->addBurgersVectorFamily(new BurgersVectorFamily(phase->dataset(), 3, tr("1/6<110> (Stair-rod)"), Vector3(1.0f/6.0f, 1.0f/6.0f, 0.0f/6.0f), Color(1,0,1)));
			phase->addBurgersVectorFamily(new BurgersVectorFamily(phase->dataset(), 4, tr("1/3<001> (Hirth)"), Vector3(1.0f/3.0f, 0.0f, 0.0f), Color(1,1,0)));
			phase->addBurgersVectorFamily(new BurgersVectorFamily(phase->dataset(), 5, tr("1/3<111> (Frank)"), Vector3(1.0f/3.0f, 1.0f/3.0f, 1.0f/3.0f), Color(0,1,1)));
		}
	}
	else {
		phase->setCrystalSymmetryClass(MicrostructurePhase::CrystalSymmetryClass::NoSymmetry);
		if(phase->burgersVectorFamilies().empty()) {
			phase->addBurgersVectorFamily(new BurgersVectorFamily(phase->dataset()));
            phase->defaultBurgersVectorFamily()->setColor(Color(0.7,0.7,0.7));
		}
	}

	return output;
}

}	// End of namespace
}	// End of namespace
