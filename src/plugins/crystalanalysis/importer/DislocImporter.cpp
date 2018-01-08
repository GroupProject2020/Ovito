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

#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include <plugins/crystalanalysis/objects/dislocations/DislocationNetworkObject.h>
#include <plugins/crystalanalysis/objects/dislocations/DislocationDisplay.h>
#include <plugins/crystalanalysis/objects/clusters/ClusterGraphObject.h>
#include <plugins/crystalanalysis/objects/patterns/PatternCatalog.h>
#include <plugins/crystalanalysis/objects/slip_surface/SlipSurface.h>
#include <plugins/crystalanalysis/objects/slip_surface/SlipSurfaceDisplay.h>
#include <plugins/crystalanalysis/data/Microstructure.h>
#include <plugins/mesh/surface/SurfaceMesh.h>
#include <plugins/mesh/surface/SurfaceMeshDisplay.h>
#include <core/utilities/io/CompressedTextReader.h>
#include "DislocImporter.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

IMPLEMENT_OVITO_CLASS(DislocImporter);

/******************************************************************************
* Checks if the given file has format that can be read by this importer.
******************************************************************************/
bool DislocImporter::OOMetaClass::checkFileFormat(QFileDevice& input, const QUrl& sourceLocation) const
{
	// Open input file.
	CompressedTextReader stream(input, sourceLocation.path());

	// Read first line.
	stream.readLine(20);

	// Files start with the marker string "# disloc file format".
	if(stream.lineStartsWith("# disloc file format"))
		return true;

	return false;
}

/******************************************************************************
* Parses the given input file.
******************************************************************************/
FileSourceImporter::FrameDataPtr DislocImporter::FrameLoader::loadFile(QFile& file)
{
	// Open file for reading.
	CompressedTextReader stream(file, frame().sourceFile.path());
	setProgressText(tr("Reading disloc file %1").arg(frame().sourceFile.toString(QUrl::RemovePassword | QUrl::PreferLocalFile | QUrl::PrettyDecoded)));
	qint64 progressUpdateInterval = std::max(stream.underlyingSize() / 1000, (qint64)1);
	setProgressMaximum(stream.underlyingSize() / progressUpdateInterval);

	// Read file header.
	stream.readLine();
	if(!stream.lineStartsWith("# disloc file format"))
		throw Exception(tr("File parsing error. This is not a proper disloc file."));

	// Create the data structures for holding the loaded data.
	auto clusterGraph = std::make_shared<ClusterGraph>();
	Microstructure microstructure(clusterGraph);
	Cluster* defaultCluster = clusterGraph->createCluster(1);
	auto frameData = std::make_shared<DislocFrameData>();
	frameData->setClusterGraph(clusterGraph);
	
	// Meta information.
	Vector3I processorGrid = Vector3I::Zero();
	int numProcessorPiecesLoaded = 0;
	std::vector<Vector3> latticeVectors;
	std::vector<Vector3> transformedLatticeVectors;
	size_t segmentCount = 0;

	// Working data structure.
	std::map<std::array<unsigned int,4>, Microstructure::Vertex*> vertexMap;

	auto createVertexFromCode = [&vertexMap,&microstructure](std::array<unsigned int,4>&& code) -> Microstructure::Vertex* {
		std::sort(std::begin(code), std::end(code));
		auto iter = vertexMap.find(code);
		if(iter != vertexMap.end())
			return iter->second;
		else
			return vertexMap.emplace(code, microstructure.createVertex(Point3::Origin())).first->second;
	};

	while(!stream.eof()) {
		stream.readLineTrimLeft();
		if(!setProgressValueIntermittent(stream.underlyingByteOffset() / progressUpdateInterval))
			return {};

		if(stream.lineStartsWith("simulation cell:")) {
			AffineTransformation cell;
			std::array<bool,3> pbcFlags;
			for(size_t dim = 0; dim < 3; dim++) {
				char bcString[3];
				if(sscanf(stream.readLine(), FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " %2s", &cell(0,dim), &cell(1,dim), &cell(2,dim), bcString) != 4)
					throw Exception(tr("File parsing error. Invalid cell vector in line %1.").arg(stream.lineNumber()));
				pbcFlags[dim] = (strcmp(bcString, "pp") == 0);
			}
			if(sscanf(stream.readLine(), FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING, &cell(0,3), &cell(1,3), &cell(2,3)) != 3)
				throw Exception(tr("File parsing error. Invalid cell origin in line %1.").arg(stream.lineNumber()));
			frameData->simulationCell().setMatrix(cell);
			frameData->simulationCell().setPbcFlags(pbcFlags[0], pbcFlags[1], pbcFlags[2]);
		}
		else if(stream.lineStartsWith("timestep number:")) {
			int timestep;
			if(sscanf(stream.readLine(), "%i", &timestep) != 1)
				throw Exception(tr("File parsing error. Invalid timestep number (line %1):\n%2").arg(stream.lineNumber()).arg(stream.lineString()));
			frameData->attributes().insert(QStringLiteral("Timestep"), QVariant::fromValue(timestep));
		}
		else if(stream.lineStartsWith("processor grid:")) {
			Vector3I pg;
			if(sscanf(stream.readLine(), "%i %i %i", &pg.x(), &pg.y(), &pg.z()) != 3)
				throw Exception(tr("File parsing error. Invalid processor grid specification (line %1):\n%2").arg(stream.lineNumber()).arg(stream.lineString()));
			if(pg != processorGrid && processorGrid != Vector3I::Zero())
				throw Exception(tr("File parsing error. Inconsistent processor grid specification in line %1:\n%2").arg(stream.lineNumber()).arg(stream.lineString()));
			processorGrid = pg;
			numProcessorPiecesLoaded++;
		}
		else if(stream.lineStartsWith("lattice structure:")) {
			stream.readLineTrimLeft();
			if(stream.lineStartsWith("bcc")) defaultCluster->structure = StructureAnalysis::LATTICE_BCC;
			else if(stream.lineStartsWith("fcc")) defaultCluster->structure = StructureAnalysis::LATTICE_FCC;
			else if(stream.lineStartsWith("fcc_perfect")) defaultCluster->structure = StructureAnalysis::LATTICE_FCC;
			else throw Exception(tr("File parsing error. Unknown lattice structure type in line %1:\n%2").arg(stream.lineNumber()).arg(stream.lineString()));
		}
		else if(stream.lineStartsWith("lattice vectors:")) {
			int nvectors;
			if(sscanf(stream.readLine(), "%i", &nvectors) != 1 || nvectors <= 1)
				throw Exception(tr("File parsing error. Invalid number of lattice vectors (line %1):\n%2").arg(stream.lineNumber()).arg(stream.lineString()));
			if(latticeVectors.empty()) {
				latticeVectors.resize(nvectors);
				transformedLatticeVectors.resize(nvectors);
			}
			else if(latticeVectors.size() != nvectors) {
				throw Exception(tr("File parsing error. Inconsistent number of lattice vectors (line %1):\n%2").arg(stream.lineNumber()).arg(stream.lineString()));
			}
			for(int i = 0; i < nvectors; i++) {
				Vector3& lv = latticeVectors[i];
				Vector3& sv = transformedLatticeVectors[i];
				if(sscanf(stream.readLine(), FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " -> " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING, 
						&lv.x(), &lv.y(), &lv.z(), &sv.x(), &sv.y(), &sv.z()) != 6)
					throw Exception(tr("File parsing error. Invalid lattice vector specification in line %1: %2").arg(stream.lineNumber()).arg(stream.lineString()));
			}
		}
		else if(stream.lineStartsWith("line segments:")) {
			for(;;) {
				stream.readLine();
				if(stream.lineStartsWith("end of line segments:"))
					break;
				if(!setProgressValueIntermittent(stream.underlyingByteOffset() / progressUpdateInterval))
					return {};

				std::array<unsigned int,5> vertexCodes;
				int burgersVectorCode;

				if(sscanf(stream.line(), "%x %x %x %x %x %i", &vertexCodes[0], &vertexCodes[1], &vertexCodes[2], &vertexCodes[3], &vertexCodes[4], &burgersVectorCode) != 6)
					throw Exception(tr("File parsing error. Invalid line segment specification in line %1: %2").arg(stream.lineNumber()).arg(stream.lineString()));
				OVITO_ASSERT(vertexCodes[0] != vertexCodes[1]);
				OVITO_ASSERT(vertexCodes[1] != vertexCodes[2]);
				OVITO_ASSERT(vertexCodes[2] != vertexCodes[3]);
				Vector3 burgersVector;
				if(burgersVectorCode == -1) {
					if(sscanf(stream.line(), FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING, &burgersVector.x(), &burgersVector.y(), &burgersVector.z()) != 3)
						throw Exception(tr("File parsing error. Invalid Burgers vector specification in line %1: %2").arg(stream.lineNumber()).arg(stream.lineString()));
				}
				else {
					if(burgersVectorCode < 0 || burgersVectorCode > latticeVectors.size())
						throw Exception(tr("File parsing error. Invalid Burgers vector code in line %1: %2").arg(stream.lineNumber()).arg(stream.lineString()));
					burgersVector = latticeVectors[burgersVectorCode];
				}
				Microstructure::Vertex* vertex1 = createVertexFromCode({vertexCodes[0], vertexCodes[1], vertexCodes[2], vertexCodes[3]});
				Microstructure::Vertex* vertex2 = createVertexFromCode({vertexCodes[0], vertexCodes[1], vertexCodes[2], vertexCodes[4]});
				if(vertex1 < vertex2) {
					Microstructure::Face* face1 = microstructure.createFace({vertex1, vertex2});
					Microstructure::Face* face2 = microstructure.createFace({vertex2, vertex1});
					face1->edges()->linkToOppositeEdge(face2->edges());
					face1->edges()->nextFaceEdge()->linkToOppositeEdge(face2->edges()->nextFaceEdge());
					face1->setOppositeFace(face2);
					face2->setOppositeFace(face1);
					face1->setBurgersVector(burgersVector);
					face2->setBurgersVector(-burgersVector);
					face1->setCluster(defaultCluster);
					face2->setCluster(defaultCluster);
					face1->setFlag(Microstructure::FACE_IS_DISLOCATION);
					face2->setFlag(Microstructure::FACE_IS_DISLOCATION);
					OVITO_ASSERT(face1->edges()->vertex1() == vertex1);
					OVITO_ASSERT(face1->edges()->vertex2() == vertex2);
					OVITO_ASSERT(face2->edges()->vertex1() == vertex2);
					OVITO_ASSERT(face2->edges()->vertex2() == vertex1);
					segmentCount++;
				}
			}
		}	
		else if(stream.lineStartsWith("nodes:")) {
			int nnodes;
			if(sscanf(stream.readLine(), "%i", &nnodes) != 1)
				throw Exception(tr("File parsing error. Invalid number of nodes (line %1):\n%2").arg(stream.lineNumber()).arg(stream.lineString()));
			for(int i = 0; i < nnodes; i++) {
				if(!setProgressValueIntermittent(stream.underlyingByteOffset() / progressUpdateInterval))
					return {};
				
				std::array<unsigned int, 4> vertexCodes;
				Point3 position;
				if(sscanf(stream.readLine(), "%x %x %x %x " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING, 
						&vertexCodes[0], &vertexCodes[1], &vertexCodes[2], &vertexCodes[3], &position.x(), &position.y(), &position.z()) != 7)
					throw Exception(tr("File parsing error. Invalid node specification in line %1: %2").arg(stream.lineNumber()).arg(stream.lineString()));
				OVITO_ASSERT(std::is_sorted(std::begin(vertexCodes), std::end(vertexCodes)));

				auto vertex = vertexMap.find(vertexCodes);
				if(vertex != vertexMap.end())
					vertex->second->setPos(position);
			}
		}
	}

	// Consistency check.
	if(numProcessorPiecesLoaded != processorGrid.x() * processorGrid.y() * processorGrid.z())
		throw Exception(tr("File parsing error. Number of read data pieces %i is not consistent with processor grid size %i x %i x %i.")
			.arg(numProcessorPiecesLoaded).arg(processorGrid.x()).arg(processorGrid.y()).arg(processorGrid.z()));

	// Convert dislocation network from nodal to line representation.
	frameData->setDislocations(std::make_shared<DislocationNetwork>(microstructure, frameData->simulationCell()));

	frameData->setStatus(tr("Number of nodes: %1\nNumber of segments: %2")
		.arg(microstructure.vertices().size())
		.arg(segmentCount));

	return frameData;
}

/******************************************************************************
* Inserts the data loaded by perform() into the provided pipeline state.
* This function is called by the system from the main thread after the
* asynchronous loading task has finished.
******************************************************************************/
PipelineFlowState DislocImporter::DislocFrameData::handOver(DataSet* dataset, const PipelineFlowState& existing, bool isNewFile)
{
	// Insert simulation cell.
	PipelineFlowState output = ParticleFrameData::handOver(dataset, existing, isNewFile);

	// Insert pattern catalog.
	OORef<PatternCatalog> patternCatalog = existing.findObject<PatternCatalog>();
	if(!patternCatalog) {
		patternCatalog = new PatternCatalog(dataset);

		// Create the structure types.
		ParticleType::PredefinedStructureType predefTypes[] = {
				ParticleType::PredefinedStructureType::OTHER,
				ParticleType::PredefinedStructureType::FCC,
				ParticleType::PredefinedStructureType::HCP,
				ParticleType::PredefinedStructureType::BCC,
				ParticleType::PredefinedStructureType::CUBIC_DIAMOND,
				ParticleType::PredefinedStructureType::HEX_DIAMOND
		};
		OVITO_STATIC_ASSERT(sizeof(predefTypes)/sizeof(predefTypes[0]) == StructureAnalysis::NUM_LATTICE_TYPES);
		for(int id = 0; id < StructureAnalysis::NUM_LATTICE_TYPES; id++) {
			OORef<StructurePattern> stype = patternCatalog->structureById(id);
			if(!stype) {
				stype = new StructurePattern(dataset);
				stype->setId(id);
				stype->setStructureType(StructurePattern::Lattice);
				patternCatalog->addPattern(stype);
			}
			stype->setName(ParticleType::getPredefinedStructureTypeName(predefTypes[id]));
			stype->setColor(ParticleType::getDefaultParticleColor(ParticleProperty::StructureTypeProperty, stype->name(), id));
		}

		// Create Burgers vector families.

		StructurePattern* fccPattern = patternCatalog->structureById(StructureAnalysis::LATTICE_FCC);
		fccPattern->setSymmetryType(StructurePattern::CubicSymmetry);
		fccPattern->setShortName(QStringLiteral("fcc"));
		fccPattern->addBurgersVectorFamily(new BurgersVectorFamily(dataset, tr("1/2<110> (Perfect)"), Vector3(1.0f/2.0f, 1.0f/2.0f, 0.0f), Color(0.2f,0.2f,1)));
		fccPattern->addBurgersVectorFamily(new BurgersVectorFamily(dataset, tr("1/6<112> (Shockley)"), Vector3(1.0f/6.0f, 1.0f/6.0f, 2.0f/6.0f), Color(0,1,0)));
		fccPattern->addBurgersVectorFamily(new BurgersVectorFamily(dataset, tr("1/6<110> (Stair-rod)"), Vector3(1.0f/6.0f, 1.0f/6.0f, 0.0f/6.0f), Color(1,0,1)));
		fccPattern->addBurgersVectorFamily(new BurgersVectorFamily(dataset, tr("1/3<001> (Hirth)"), Vector3(1.0f/3.0f, 0.0f, 0.0f), Color(1,1,0)));
		fccPattern->addBurgersVectorFamily(new BurgersVectorFamily(dataset, tr("1/3<111> (Frank)"), Vector3(1.0f/3.0f, 1.0f/3.0f, 1.0f/3.0f), Color(0,1,1)));

		StructurePattern* bccPattern = patternCatalog->structureById(StructureAnalysis::LATTICE_BCC);
		bccPattern->setSymmetryType(StructurePattern::CubicSymmetry);
		bccPattern->setShortName(QStringLiteral("bcc"));
		bccPattern->addBurgersVectorFamily(new BurgersVectorFamily(dataset, tr("1/2<111>"), Vector3(1.0f/2.0f, 1.0f/2.0f, 1.0f/2.0f), Color(0,1,0)));
		bccPattern->addBurgersVectorFamily(new BurgersVectorFamily(dataset, tr("<100>"), Vector3(1.0f, 0.0f, 0.0f), Color(1, 0.3f, 0.8f)));
		bccPattern->addBurgersVectorFamily(new BurgersVectorFamily(dataset, tr("<110>"), Vector3(1.0f, 1.0f, 0.0f), Color(0.2f, 0.5f, 1.0f)));

		StructurePattern* hcpPattern = patternCatalog->structureById(StructureAnalysis::LATTICE_HCP);
		hcpPattern->setShortName(QStringLiteral("hcp"));
		hcpPattern->setSymmetryType(StructurePattern::HexagonalSymmetry);
		hcpPattern->addBurgersVectorFamily(new BurgersVectorFamily(dataset, tr("1/3<1-210>"), Vector3(sqrt(0.5f), 0.0f, 0.0f), Color(0,1,0)));
		hcpPattern->addBurgersVectorFamily(new BurgersVectorFamily(dataset, tr("<0001>"), Vector3(0.0f, 0.0f, sqrt(4.0f/3.0f)), Color(0.2f,0.2f,1)));
		hcpPattern->addBurgersVectorFamily(new BurgersVectorFamily(dataset, tr("<1-100>"), Vector3(0.0f, sqrt(3.0f/2.0f), 0.0f), Color(1,0,1)));
		hcpPattern->addBurgersVectorFamily(new BurgersVectorFamily(dataset, tr("1/3<1-100>"), Vector3(0.0f, sqrt(3.0f/2.0f)/3.0f, 0.0f), Color(1,0.5f,0)));
		hcpPattern->addBurgersVectorFamily(new BurgersVectorFamily(dataset, tr("1/3<1-213>"), Vector3(sqrt(0.5f), 0.0f, sqrt(4.0f/3.0f)), Color(1,1,0)));

		StructurePattern* cubicDiaPattern = patternCatalog->structureById(StructureAnalysis::LATTICE_CUBIC_DIAMOND);
		cubicDiaPattern->setShortName(QStringLiteral("diamond"));
		cubicDiaPattern->setSymmetryType(StructurePattern::CubicSymmetry);
		cubicDiaPattern->addBurgersVectorFamily(new BurgersVectorFamily(dataset, tr("1/2<110>"), Vector3(1.0f/2.0f, 1.0f/2.0f, 0.0f), Color(0.2f,0.2f,1)));
		cubicDiaPattern->addBurgersVectorFamily(new BurgersVectorFamily(dataset, tr("1/6<112>"), Vector3(1.0f/6.0f, 1.0f/6.0f, 2.0f/6.0f), Color(0,1,0)));
		cubicDiaPattern->addBurgersVectorFamily(new BurgersVectorFamily(dataset, tr("1/6<110>"), Vector3(1.0f/6.0f, 1.0f/6.0f, 0.0f), Color(1,0,1)));
		cubicDiaPattern->addBurgersVectorFamily(new BurgersVectorFamily(dataset, tr("1/3<111>"), Vector3(1.0f/3.0f, 1.0f/3.0f, 1.0f/3.0f), Color(0,1,1)));

		StructurePattern* hexDiaPattern = patternCatalog->structureById(StructureAnalysis::LATTICE_HEX_DIAMOND);
		hexDiaPattern->setShortName(QStringLiteral("hex_diamond"));
		hexDiaPattern->setSymmetryType(StructurePattern::HexagonalSymmetry);
		hexDiaPattern->addBurgersVectorFamily(new BurgersVectorFamily(dataset, tr("1/3<1-210>"), Vector3(sqrt(0.5f), 0.0f, 0.0f), Color(0,1,0)));
		hexDiaPattern->addBurgersVectorFamily(new BurgersVectorFamily(dataset, tr("<0001>"), Vector3(0.0f, 0.0f, sqrt(4.0f/3.0f)), Color(0.2f,0.2f,1)));
		hexDiaPattern->addBurgersVectorFamily(new BurgersVectorFamily(dataset, tr("<1-100>"), Vector3(0.0f, sqrt(3.0f/2.0f), 0.0f), Color(1,0,1)));
		hexDiaPattern->addBurgersVectorFamily(new BurgersVectorFamily(dataset, tr("1/3<1-100>"), Vector3(0.0f, sqrt(3.0f/2.0f)/3.0f, 0.0f), Color(1,0.5f,0)));		
	}
	output.addObject(patternCatalog);
		
	// Insert cluster graph.
	if(_clusterGraph) {
		OORef<ClusterGraphObject> clusterGraphObj;
		clusterGraphObj = existing.findObject<ClusterGraphObject>();
		if(!clusterGraphObj) {
			clusterGraphObj = new ClusterGraphObject(dataset);
		}
		clusterGraphObj->setStorage(clusterGraph());
		output.addObject(clusterGraphObj);
	}

	// Insert dislocations.
	if(_dislocations) {
		OORef<DislocationNetworkObject> dislocationNetwork;
		dislocationNetwork = existing.findObject<DislocationNetworkObject>();
		if(!dislocationNetwork) {
			dislocationNetwork = new DislocationNetworkObject(dataset);
			OORef<DislocationDisplay> displayObj = new DislocationDisplay(dataset);
			displayObj->loadUserDefaults();
			dislocationNetwork->setDisplayObject(displayObj);
		}
		dislocationNetwork->setDomain(output.findObject<SimulationCellObject>());
		dislocationNetwork->setStorage(dislocations());
		output.addObject(dislocationNetwork);
	}

	return output;
}

}	// End of namespace
}	// End of namespace
}	// End of namespace
