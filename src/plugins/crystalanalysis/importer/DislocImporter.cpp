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
#include <plugins/crystalanalysis/objects/microstructure/MicrostructureObject.h>
#include <plugins/crystalanalysis/objects/dislocations/DislocationDisplay.h>
#include <plugins/crystalanalysis/objects/slip_surface/SlipSurfaceDisplay.h>
#include <plugins/crystalanalysis/objects/clusters/ClusterGraphObject.h>
#include <plugins/crystalanalysis/objects/patterns/PatternCatalog.h>
#include <plugins/crystalanalysis/modifier/microstructure/SimplifyMicrostructureModifier.h>
#include <plugins/crystalanalysis/data/Microstructure.h>
#include <core/utilities/io/CompressedTextReader.h>
#include "DislocImporter.h"

#include <netcdf.h>

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
* This method is called when the scene node for the FileSource is created.
******************************************************************************/
void DislocImporter::prepareSceneNode(ObjectNode* node, FileSource* importObj)
{
	ParticleImporter::prepareSceneNode(node, importObj);

	// Insert a SimplyMicrostructureModifier into the data pipeline by default.
	OORef<SimplifyMicrostructureModifier> modifier = new SimplifyMicrostructureModifier(node->dataset());
	modifier->loadUserDefaults();
	node->applyModifier(modifier);
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
	auto frameData = std::make_shared<DislocFrameData>();
	auto microstructure = frameData->microstructure();
	Cluster* defaultCluster = microstructure->clusterGraph()->createCluster(1);
	
	// Meta information.
	Vector3I processorGrid = Vector3I::Zero();
	int numProcessorPiecesLoaded = 0;
	std::vector<Vector3> latticeVectors;
	std::vector<Vector3> transformedLatticeVectors;
	size_t segmentCount = 0;

	// Working data structures.
	std::map<std::array<qulonglong,4>, Microstructure::Vertex*> vertexMap;
	std::map<Microstructure::Face*, std::pair<qulonglong,qulonglong>> slipSurfaceMap;

	auto createVertexFromCode = [&vertexMap,&microstructure](std::array<qulonglong,4>&& code) -> Microstructure::Vertex* {
		std::sort(std::begin(code), std::end(code));
		auto iter = vertexMap.find(code);
		if(iter != vertexMap.end())
			return iter->second;
		else
			return vertexMap.emplace(code, microstructure->createVertex(Point3::Origin())).first->second;
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

				std::array<qulonglong,5> vertexCodes;
				int burgersVectorCode;

				if(sscanf(stream.line(), "%llx %llx %llx %llx %llx %i", &vertexCodes[0], &vertexCodes[1], &vertexCodes[2], &vertexCodes[3], &vertexCodes[4], &burgersVectorCode) != 6)
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
					microstructure->createDislocationSegment(vertex1, vertex2, burgersVector, defaultCluster);
					segmentCount++;
				}
			}
		}	
		else if(stream.lineStartsWith("slip surfaces:")) {
			for(;;) {
				stream.readLine();
				if(stream.lineStartsWith("end of slip surfaces:"))
					break;
				if(!setProgressValueIntermittent(stream.underlyingByteOffset() / progressUpdateInterval))
					return {};

				int slipVectorCode;
				int charCount;
				const char* line = stream.line();
				if(sscanf(line, "%i%n", &slipVectorCode, &charCount) != 1)
					throw Exception(tr("File parsing error. Invalid slip surface specification in line %1: %2").arg(stream.lineNumber()).arg(stream.lineString()));
				Vector3 slipVector;
				line += charCount;
				if(slipVectorCode == -1) {
					if(sscanf(line, FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING "%n", &slipVector.x(), &slipVector.y(), &slipVector.z(), &charCount) != 3)
						throw Exception(tr("File parsing error. Invalid slip vector specification in line %1: %2").arg(stream.lineNumber()).arg(stream.lineString()));
					line += charCount;
				}
				else {
					if(slipVectorCode < 0 || slipVectorCode > latticeVectors.size())
						throw Exception(tr("File parsing error. Invalid slip vector code in line %1: %2").arg(stream.lineNumber()).arg(stream.lineString()));
					slipVector = latticeVectors[slipVectorCode];
				}

				std::array<qulonglong,2> edgeVertexCodes;
				int numVertices;
				if(sscanf(line, "%llx %llx %i%n", &edgeVertexCodes[0], &edgeVertexCodes[1], &numVertices, &charCount) != 3)
					throw Exception(tr("File parsing error. Invalid slip edge specification in line %1: %2").arg(stream.lineNumber()).arg(stream.lineString()));				
				line += charCount;
				Microstructure::Face* face = microstructure->createFace();
				face->setBurgersVector(slipVector);
				face->setCluster(defaultCluster);
				face->setSlipSurfaceFace(true);
				slipSurfaceMap.emplace(face, std::make_pair(edgeVertexCodes[0], edgeVertexCodes[1]));
				Microstructure::Vertex* node0;
				Microstructure::Vertex* node1;
				qulonglong cellVertexCode0;
				qulonglong cellVertexCode1;
				for(int i = 0; i < numVertices; i++) {
					qulonglong cellVertexCode2;
					if(sscanf(line, "%llx%n", &cellVertexCode2, &charCount) != 1)
						throw Exception(tr("File parsing error. Invalid slip edge specification in line %1: %2").arg(stream.lineNumber()).arg(stream.lineString()));
					line += charCount;
					OVITO_ASSERT(cellVertexCode2 != edgeVertexCodes[0] && cellVertexCode2 != edgeVertexCodes[1]);
					if(i == 0) {
						cellVertexCode0 = cellVertexCode1 = cellVertexCode2;
					}
					else {
						Microstructure::Vertex* node2 = createVertexFromCode({edgeVertexCodes[0], edgeVertexCodes[1], cellVertexCode1, cellVertexCode2});
						cellVertexCode1 = cellVertexCode2;
						if(i == 1) {
							node0 = node1 = node2;
						}
						else {
							microstructure->createEdge(node1, node2, face);
							node1 = node2;
						}
					}
				}
				Microstructure::Vertex* node2 = createVertexFromCode({edgeVertexCodes[0], edgeVertexCodes[1], cellVertexCode1, cellVertexCode0});
				microstructure->createEdge(node1, node2, face);
				microstructure->createEdge(node2, node0, face);
				
				// Create the opposite face.
				Microstructure::Face* oppositeFace = microstructure->createFace();
				oppositeFace->setBurgersVector(-slipVector);
				oppositeFace->setCluster(defaultCluster);
				oppositeFace->setSlipSurfaceFace(true);
				face->setOppositeFace(oppositeFace);
				oppositeFace->setOppositeFace(face);
				slipSurfaceMap.emplace(oppositeFace, std::make_pair(edgeVertexCodes[1], edgeVertexCodes[0]));
				Microstructure::Edge* edge = face->edges();
				do {
					microstructure->createEdge(edge->vertex2(), edge->vertex1(), oppositeFace);
					edge = edge->prevFaceEdge();
				}
				while(edge != face->edges());
			}
		}	
		else if(stream.lineStartsWith("nodes:")) {
			int nnodes;
			if(sscanf(stream.readLine(), "%i", &nnodes) != 1)
				throw Exception(tr("File parsing error. Invalid number of nodes (line %1):\n%2").arg(stream.lineNumber()).arg(stream.lineString()));
			for(int i = 0; i < nnodes; i++) {
				if(!setProgressValueIntermittent(stream.underlyingByteOffset() / progressUpdateInterval))
					return {};
				
				std::array<qulonglong,4> vertexCodes;
				Point3 position;
				if(sscanf(stream.readLine(), "%llx %llx %llx %llx " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING, 
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

	// Form continuous dislocation lines from the segments.
	microstructure->makeContinuousDislocationLines();

	// Connect half-edges of slip faces.
	connectSlipFaces(*microstructure, slipSurfaceMap);

	// Generate slip surfaces along which the slip vector is constant.
	microstructure->makeSlipSurfaces();

	frameData->setStatus(tr("Number of nodes: %1\nNumber of segments: %2")
		.arg(microstructure->vertices().size())
		.arg(segmentCount));

	return frameData;
}

/*************************************************************************************
* Connects the slip faces to form two-dimensional manifolds.
**************************************************************************************/
void DislocImporter::FrameLoader::connectSlipFaces(Microstructure& microstructure, const std::map<Microstructure::Face*, std::pair<qulonglong,qulonglong>>& slipSurfaceMap)
{
	// Link slip surface faces with their neighbors, i.e. find the opposite edge for every half-edge of a slip face. 
	for(Microstructure::Vertex* vertex1 : microstructure.vertices()) {
		for(Microstructure::Edge* edge1 = vertex1->edges(); edge1 != nullptr; edge1 = edge1->nextVertexEdge()) {
			// Only process edges which haven't been linked to their neighbors yet.
			if(edge1->oppositeEdge() != nullptr) continue;
			if(!edge1->face()->isSlipSurfaceFace()) continue;
			OVITO_ASSERT(edge1->face()->isSlipSurfaceFace());

			Microstructure::Edge* oppositeEdge1 = edge1->face()->oppositeFace()->findEdge(edge1->vertex2(), edge1->vertex1());;
			OVITO_ASSERT(oppositeEdge1 != nullptr);
			OVITO_ASSERT(edge1->nextManifoldEdge() == nullptr);
			OVITO_ASSERT(oppositeEdge1->nextManifoldEdge() == nullptr);

			// At an edge, either 1, 2, or 3 slip surface manifolds can meet.
			// Here, we will link them together in the right order.

			OVITO_ASSERT(slipSurfaceMap.find(edge1->face()) != slipSurfaceMap.end());
			const std::pair<qulonglong,qulonglong>& edgeVertexCodes = slipSurfaceMap.find(edge1->face())->second;

			// Find the other two manifolds meeting at the current edge (if they exist).
			Microstructure::Edge* edge2 = nullptr;
			Microstructure::Edge* edge3 = nullptr;
			Microstructure::Edge* oppositeEdge2 = nullptr;
			Microstructure::Edge* oppositeEdge3 = nullptr;
			for(Microstructure::Edge* e = vertex1->edges(); e != nullptr; e = e->nextVertexEdge()) {
				if(e->vertex2() == edge1->vertex2() && e->face()->isSlipSurfaceFace() && e->face() != edge1->face()) {					
					OVITO_ASSERT(slipSurfaceMap.find(e->face()) != slipSurfaceMap.end());
					const std::pair<qulonglong,qulonglong>& edgeVertexCodes2 = slipSurfaceMap.find(e->face())->second;
					if(edgeVertexCodes.second == edgeVertexCodes2.first) {
						OVITO_ASSERT(edgeVertexCodes.first != edgeVertexCodes2.second);
						OVITO_ASSERT(edge2 == nullptr);
						OVITO_ASSERT(e->oppositeEdge() == nullptr);
						OVITO_ASSERT(e->nextManifoldEdge() == nullptr);
						edge2 = e;
						oppositeEdge2 = edge2->face()->oppositeFace()->findEdge(e->vertex2(), e->vertex1());
						OVITO_ASSERT(oppositeEdge2);
						OVITO_ASSERT(oppositeEdge2->nextManifoldEdge() == nullptr);
					}
					else {
						OVITO_ASSERT(edgeVertexCodes.first == edgeVertexCodes2.second);
						OVITO_ASSERT(edge3 == nullptr);
						OVITO_ASSERT(e->oppositeEdge() == nullptr);
						OVITO_ASSERT(e->nextManifoldEdge() == nullptr);
						edge3 = e;
						oppositeEdge3 = edge3->face()->oppositeFace()->findEdge(e->vertex2(), e->vertex1());
						OVITO_ASSERT(oppositeEdge3);
						OVITO_ASSERT(oppositeEdge3->nextManifoldEdge() == nullptr);
					}
				}
			}

			if(edge2) {
				edge1->linkToOppositeEdge(oppositeEdge2);
				edge1->setNextManifoldEdge(edge2);
				oppositeEdge2->setNextManifoldEdge(oppositeEdge1);
				if(edge3) {
					edge2->linkToOppositeEdge(oppositeEdge3);
					edge3->linkToOppositeEdge(oppositeEdge1);
					edge2->setNextManifoldEdge(edge3);
					oppositeEdge3->setNextManifoldEdge(oppositeEdge2);
					edge3->setNextManifoldEdge(edge1);
					oppositeEdge1->setNextManifoldEdge(oppositeEdge3);
					OVITO_ASSERT(edge1->countManifolds() == 3);
					OVITO_ASSERT(edge2->countManifolds() == 3);
					OVITO_ASSERT(edge3->countManifolds() == 3);
				}
				else {
					edge2->linkToOppositeEdge(oppositeEdge1);
					edge2->setNextManifoldEdge(edge1);
					oppositeEdge1->setNextManifoldEdge(oppositeEdge2);
					OVITO_ASSERT(edge1->countManifolds() == 2);
					OVITO_ASSERT(edge2->countManifolds() == 2);
					OVITO_ASSERT(oppositeEdge1->countManifolds() == 2);
					OVITO_ASSERT(oppositeEdge2->countManifolds() == 2);
				}
			}
			else {
				if(edge3) {
					edge1->linkToOppositeEdge(oppositeEdge3);
					oppositeEdge1->linkToOppositeEdge(edge3);
					edge1->setNextManifoldEdge(edge3);
					oppositeEdge3->setNextManifoldEdge(oppositeEdge1);
					edge3->setNextManifoldEdge(edge1);
					oppositeEdge1->setNextManifoldEdge(oppositeEdge3);
					OVITO_ASSERT(edge1->countManifolds() == 2);
					OVITO_ASSERT(oppositeEdge1->countManifolds() == 2);
					OVITO_ASSERT(edge3->countManifolds() == 2);
					OVITO_ASSERT(oppositeEdge3->countManifolds() == 2);
				}
				else {
					oppositeEdge1->linkToOppositeEdge(edge1);
					edge1->setNextManifoldEdge(edge1);
					oppositeEdge1->setNextManifoldEdge(oppositeEdge1);
					OVITO_ASSERT(edge1->countManifolds() == 1);
					OVITO_ASSERT(oppositeEdge1->countManifolds() == 1);
				}
			}

			OVITO_ASSERT(edge1->nextManifoldEdge() != nullptr);
			OVITO_ASSERT(oppositeEdge1->nextManifoldEdge() != nullptr);
			OVITO_ASSERT(!edge2 || edge2->nextManifoldEdge() != nullptr);
			OVITO_ASSERT(!oppositeEdge2 || oppositeEdge2->nextManifoldEdge() != nullptr);
			OVITO_ASSERT(!edge3 || edge3->nextManifoldEdge() != nullptr);
			OVITO_ASSERT(!oppositeEdge3 || oppositeEdge3->nextManifoldEdge() != nullptr);
		}
	}
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
		
	if(microstructure()) {

		// Insert microstructure.
		OORef<MicrostructureObject> microstructureObj;
		microstructureObj = existing.findObject<MicrostructureObject>();
		if(!microstructureObj) {
			microstructureObj = new MicrostructureObject(dataset);
			// Create a display object for the dislocation lines.
			OORef<DislocationDisplay> dislocationDisplayObj = new DislocationDisplay(dataset);
			dislocationDisplayObj->loadUserDefaults();
			microstructureObj->addDisplayObject(dislocationDisplayObj);
			// Create a display object for the slip surfaces.
			OORef<SlipSurfaceDisplay> slipSurfaceDisplayObj = new SlipSurfaceDisplay(dataset);
			slipSurfaceDisplayObj->loadUserDefaults();
			microstructureObj->addDisplayObject(slipSurfaceDisplayObj);
		}
		microstructureObj->setDomain(output.findObject<SimulationCellObject>());
		microstructureObj->setStorage(microstructure());
		output.addObject(microstructureObj);

		// Insert cluster graph as a separate data object.
		OORef<ClusterGraphObject> clusterGraphObj;
		clusterGraphObj = existing.findObject<ClusterGraphObject>();
		if(!clusterGraphObj) {
			clusterGraphObj = new ClusterGraphObject(dataset);
		}
		clusterGraphObj->setStorage(microstructure()->clusterGraph());
		output.addObject(clusterGraphObj);		
	}

	return output;
}

}	// End of namespace
}	// End of namespace
}	// End of namespace
