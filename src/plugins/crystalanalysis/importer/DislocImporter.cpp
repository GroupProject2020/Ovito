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
#include <plugins/crystalanalysis/objects/dislocations/DislocationVis.h>
#include <plugins/crystalanalysis/objects/slip_surface/SlipSurfaceVis.h>
#include <plugins/crystalanalysis/objects/clusters/ClusterGraphObject.h>
#include <plugins/crystalanalysis/objects/patterns/PatternCatalog.h>
#include <plugins/crystalanalysis/modifier/microstructure/SimplifyMicrostructureModifier.h>
#include <plugins/crystalanalysis/data/Microstructure.h>
#include <core/app/Application.h>
#include <core/dataset/io/FileSource.h>
#include <core/utilities/io/CompressedTextReader.h>
#include "DislocImporter.h"

#include <3rdparty/netcdf_integration/NetCDFIntegration.h>
#include <netcdf.h>

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

IMPLEMENT_OVITO_CLASS(DislocImporter);

/******************************************************************************
* Checks if the given file has format that can be read by this importer.
******************************************************************************/
bool DislocImporter::OOMetaClass::checkFileFormat(QFileDevice& input, const QUrl& sourceLocation) const
{
	// Only serial access to NetCDF functions is allowed, because they are not thread-safe.
	NetCDFExclusiveAccess locker;

	// Check if we can open the input file for reading.
	int ncid;
	int err = nc_open(QDir::toNativeSeparators(input.fileName()).toLocal8Bit().constData(), NC_NOWRITE, &ncid);
	if(err == NC_NOERR) {

		// Make sure we have the right file conventions.
		size_t len;
		if(nc_inq_attlen(ncid, NC_GLOBAL, "Conventions", &len) == NC_NOERR) {
			std::unique_ptr<char[]> conventions_str(new char[len+1]);
			if(nc_get_att_text(ncid, NC_GLOBAL, "Conventions", conventions_str.get()) == NC_NOERR) {
				conventions_str[len] = '\0';
				if(strcmp(conventions_str.get(), "FixDisloc") == 0) {
					nc_close(ncid);
					return true;
				}
			}
		}

		nc_close(ncid);
	}

	return false;
}

/******************************************************************************
* This method is called when the pipeline node for the FileSource is created.
******************************************************************************/
void DislocImporter::setupPipeline(PipelineSceneNode* pipeline, FileSource* importObj)
{
	ParticleImporter::setupPipeline(pipeline, importObj);

	// Insert a SimplyMicrostructureModifier into the data pipeline by default.
	OORef<SimplifyMicrostructureModifier> modifier = new SimplifyMicrostructureModifier(pipeline->dataset());
	if(!Application::instance()->scriptMode())
		modifier->loadUserDefaults();
	pipeline->applyModifier(modifier);
}

/******************************************************************************
* Parses the given input file.
******************************************************************************/
FileSourceImporter::FrameDataPtr DislocImporter::FrameLoader::loadFile(QFile& file)
{
	setProgressText(tr("Reading disloc file %1").arg(frame().sourceFile.toString(QUrl::RemovePassword | QUrl::PreferLocalFile | QUrl::PrettyDecoded)));

	// Create the container structures for holding the loaded data.
	auto frameData = std::make_shared<DislocFrameData>();
	auto microstructure = frameData->microstructure();
	Cluster* defaultCluster = microstructure->clusterGraph()->createCluster(1);
	
	// Fix disloc specific data.
	std::vector<Vector3> latticeVectors;
	std::vector<Vector3> transformedLatticeVectors;
	size_t segmentCount = 0;

	// Working data structures.
	//std::map<std::array<qlonglong,4>, Microstructure::Vertex*> vertexMap;
	std::map<Microstructure::Face*, std::pair<qlonglong,qlonglong>> slipSurfaceMap;

	// Only serial access to NetCDF functions is allowed, because they are not thread-safe.
	NetCDFExclusiveAccess locker(this);
	if(!locker.isLocked()) return {};

	int root_ncid = 0;
	try {
		// Open the input file for reading.
		NCERR(nc_open(qPrintable(file.fileName()), NC_NOWRITE, &root_ncid));

		// Make sure we have the right file conventions
		size_t len;
		NCERR(nc_inq_attlen(root_ncid, NC_GLOBAL, "Conventions", &len));
		auto conventions_str = std::make_unique<char[]>(len+1);
		NCERR(nc_get_att_text(root_ncid, NC_GLOBAL, "Conventions", conventions_str.get()));
		conventions_str[len] = '\0';
		if(strcmp(conventions_str.get(), "FixDisloc"))
			throw Exception(tr("NetCDF file follows '%1' conventions; expected 'FixDisloc'.").arg(conventions_str.get()));

		// Read lattice structure.
		NCERR(nc_inq_attlen(root_ncid, NC_GLOBAL, "LatticeStructure", &len));
		auto lattice_structure_str = std::make_unique<char[]>(len+1);
		NCERR(nc_get_att_text(root_ncid, NC_GLOBAL, "LatticeStructure", lattice_structure_str.get()));
		lattice_structure_str[len] = '\0';		
		if(strcmp(lattice_structure_str.get(), "bcc") == 0) defaultCluster->structure = StructureAnalysis::LATTICE_BCC;
		else if(strcmp(lattice_structure_str.get(), "fcc") == 0) defaultCluster->structure = StructureAnalysis::LATTICE_FCC;
		else if(strcmp(lattice_structure_str.get(), "fcc_perfect") == 0) defaultCluster->structure = StructureAnalysis::LATTICE_FCC;
		else throw Exception(tr("File parsing error. Unknown lattice structure type: %1").arg(lattice_structure_str.get()));

		// Get NetCDF dimensions.
		int spatial_dim, nodes_dim, dislocation_segments_dim, pair_dim, node_id_dim;
		NCERR(nc_inq_dimid(root_ncid, "spatial", &spatial_dim));
		NCERR(nc_inq_dimid(root_ncid, "nodes", &nodes_dim));
		NCERR(nc_inq_dimid(root_ncid, "dislocations", &dislocation_segments_dim));
		NCERR(nc_inq_dimid(root_ncid, "pair", &pair_dim));
		NCERR(nc_inq_dimid(root_ncid, "node_id", &node_id_dim));

		// Get NetCDF variables.
		int cell_vectors_var, cell_origin_var, cell_pbc_var;
		NCERR(nc_inq_varid(root_ncid, "cell_vectors", &cell_vectors_var));
		NCERR(nc_inq_varid(root_ncid, "cell_origin", &cell_origin_var));
		NCERR(nc_inq_varid(root_ncid, "cell_pbc", &cell_pbc_var));

		// Read simulation cell information.
		AffineTransformation cellMatrix;
		int cellPbc[3];
		size_t startp[2] = { 0, 0 };
		size_t countp[2] = { 3, 3 };
#ifdef FLOATTYPE_FLOAT
		NCERR(nc_get_vara_float(root_ncid, cell_vectors_var, startp, countp, cellMatrix.elements()));
		NCERR(nc_get_vara_float(root_ncid, cell_origin_var, startp, countp, cellMatrix.column(3).data()));
#else
		NCERR(nc_get_vara_double(root_ncid, cell_vectors_var, startp, countp, cellMatrix.elements()));
		NCERR(nc_get_vara_double(root_ncid, cell_origin_var, startp, countp, cellMatrix.column(3).data()));
#endif
		NCERR(nc_get_vara_int(root_ncid, cell_pbc_var, startp, countp, cellPbc));
		frameData->simulationCell().setPbcFlags({cellPbc[0] != 0, cellPbc[1] != 0, cellPbc[2] != 0});
		frameData->simulationCell().setMatrix(cellMatrix);

		// Read lattice orientation matrix.
		int lattice_orientation_var;
		NCERR(nc_inq_varid(root_ncid, "lattice_orientation", &lattice_orientation_var));
#ifdef FLOATTYPE_FLOAT
		NCERR(nc_get_var_float(root_ncid, lattice_orientation_var, defaultCluster->orientation.elements()));
#else
		NCERR(nc_get_var_double(root_ncid, lattice_orientation_var, defaultCluster->orientation.elements()));		
#endif		

		// Read node list.
		int nodal_ids_var, nodal_positions_var;
		NCERR(nc_inq_varid(root_ncid, "nodal_ids", &nodal_ids_var));
		NCERR(nc_inq_varid(root_ncid, "nodal_positions", &nodal_positions_var));
		size_t numNodeRecords;
		NCERR(nc_inq_dimlen(root_ncid, nodes_dim, &numNodeRecords));
		std::vector<Point_3<float>> nodalPositions(numNodeRecords);
		std::vector<std::array<qlonglong,4>> nodalIds(numNodeRecords);
		if(numNodeRecords) {
			NCERR(nc_get_var_float(root_ncid, nodal_positions_var, nodalPositions.front().data()));
			NCERR(nc_get_var_longlong(root_ncid, nodal_ids_var, nodalIds.front().data()));
		}

		// Build list of unique nodes.
		std::vector<Microstructure::Vertex*> vertexMap(numNodeRecords);
		std::map<std::array<qlonglong,4>, Microstructure::Vertex*> idMap;
		auto vertexMapIter = vertexMap.begin();
		auto nodalPositionsIter = nodalPositions.cbegin();
		for(const auto& id : nodalIds) {
			auto iter = idMap.find(id);
			if(iter == idMap.end())
				iter = idMap.emplace(id, microstructure->createVertex(Point3(*nodalPositionsIter))).first;
			*vertexMapIter = iter->second;
			++vertexMapIter;
			++nodalPositionsIter;
		}

		// Read dislocation segments.
		int burgers_vectors_var, dislocation_segments_var;
		NCERR(nc_inq_varid(root_ncid, "burgers_vectors", &burgers_vectors_var));
		NCERR(nc_inq_varid(root_ncid, "dislocation_segments", &dislocation_segments_var));
		size_t numDislocationSegments;
		NCERR(nc_inq_dimlen(root_ncid, dislocation_segments_dim, &numDislocationSegments));
		std::vector<Vector_3<float>> burgersVectors(numDislocationSegments);
		std::vector<std::array<qlonglong,2>> dislocationSegments(numDislocationSegments);
		if(numDislocationSegments) {
			NCERR(nc_get_var_float(root_ncid, burgers_vectors_var, burgersVectors.front().data()));
			NCERR(nc_get_var_longlong(root_ncid, dislocation_segments_var, dislocationSegments.front().data()));
		}

		// Create dislocation segments.
		auto burgersVector = burgersVectors.cbegin();
		for(const auto& seg : dislocationSegments) {
			OVITO_ASSERT(seg[0] >= 0 && seg[0] < vertexMap.size());
			OVITO_ASSERT(seg[1] >= 0 && seg[1] < vertexMap.size());
			Microstructure::Vertex* vertex1	= vertexMap[seg[0]];
			Microstructure::Vertex* vertex2	= vertexMap[seg[1]];
			microstructure->createDislocationSegment(vertex1, vertex2, Vector3(*burgersVector++), defaultCluster);
		}
		segmentCount = dislocationSegments.size();

		// Read slip facets.
		int slip_facets_dim = -1, slip_facet_vertices_dim = -1;
		nc_inq_dimid(root_ncid, "slip_facets", &slip_facets_dim);
		nc_inq_dimid(root_ncid, "slip_facet_vertices", &slip_facet_vertices_dim);
		if(slip_facets_dim != -1) {
			int slipped_edges_var;
			int slip_vectors_var;
			int slip_facet_edge_counts_var;
			int slip_facet_vertices_var;
			NCERR(nc_inq_varid(root_ncid, "slipped_edges", &slipped_edges_var));
			NCERR(nc_inq_varid(root_ncid, "slip_vectors", &slip_vectors_var));
			NCERR(nc_inq_varid(root_ncid, "slip_facet_edge_counts", &slip_facet_edge_counts_var));
			NCERR(nc_inq_varid(root_ncid, "slip_facet_vertices", &slip_facet_vertices_var));
			size_t numSlipFacets, numSlipFacetVertices;
			NCERR(nc_inq_dimlen(root_ncid, slip_facets_dim, &numSlipFacets));
			NCERR(nc_inq_dimlen(root_ncid, slip_facet_vertices_dim, &numSlipFacetVertices));
			std::vector<Vector_3<float>> slipVectors(numSlipFacets);
			std::vector<std::array<qlonglong,2>> slippedEdges(numSlipFacets);
			std::vector<int> slipFacetEdgeCounts(numSlipFacets);
			std::vector<qlonglong> slipFacetVertices(numSlipFacetVertices);
			if(numSlipFacets) {
				NCERR(nc_get_var_float(root_ncid, slip_vectors_var, slipVectors.front().data()));
				NCERR(nc_get_var_longlong(root_ncid, slipped_edges_var, slippedEdges.front().data()));
				NCERR(nc_get_var_int(root_ncid, slip_facet_edge_counts_var, slipFacetEdgeCounts.data()));
			}
			if(numSlipFacetVertices) {
				NCERR(nc_get_var_longlong(root_ncid, slip_facet_vertices_var, slipFacetVertices.data()));				
			}

			// Create slip surfaces.
			auto slipVector = slipVectors.cbegin();
			auto slipFacetEdgeCount = slipFacetEdgeCounts.cbegin();
			auto slipFacetVertex = slipFacetVertices.cbegin();
			for(const auto& slippedEdge : slippedEdges) {
				Microstructure::Face* face = microstructure->createFace();
				face->setBurgersVector(Vector3(*slipVector));
				face->setCluster(defaultCluster);
				face->setSlipSurfaceFace(true);
				slipSurfaceMap.emplace(face, std::make_pair(slippedEdge[0], slippedEdge[1]));
				Microstructure::Vertex* node0 = vertexMap[*slipFacetVertex++];
				Microstructure::Vertex* node1 = node0;
				Microstructure::Vertex* node2;
				for(int i = 1; i < *slipFacetEdgeCount; i++, node1 = node2) {
					node2 = vertexMap[*slipFacetVertex++];
					microstructure->createEdge(node1, node2, face);
				}
				microstructure->createEdge(node1, node0, face);
				
				// Create the opposite face.
				Microstructure::Face* oppositeFace = microstructure->createFace();
				oppositeFace->setBurgersVector(-face->burgersVector());
				oppositeFace->setCluster(defaultCluster);
				oppositeFace->setSlipSurfaceFace(true);
				face->setOppositeFace(oppositeFace);
				oppositeFace->setOppositeFace(face);
				slipSurfaceMap.emplace(oppositeFace, std::make_pair(slippedEdge[1], slippedEdge[0]));
				Microstructure::Edge* edge = face->edges();
				do {
					microstructure->createEdge(edge->vertex2(), edge->vertex1(), oppositeFace);
					edge = edge->prevFaceEdge();
				}
				while(edge != face->edges());

				++slipVector;
				++slipFacetEdgeCount;
			}
			OVITO_ASSERT(slipFacetVertex == slipFacetVertices.cend());
		}

		// Close the input file again.
		NCERR(nc_close(root_ncid));
	}
	catch(...) {
		if(root_ncid)
			nc_close(root_ncid);
		throw;
	}	

#if 0	
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
#endif

	// Form continuous dislocation lines from the segments.
	microstructure->makeContinuousDislocationLines();

	// Connect half-edges of slip faces.
	connectSlipFaces(*microstructure, slipSurfaceMap);

	// Generate slip surfaces along which the slip vector is constant.
	microstructure->makeSlipSurfaces();

	frameData->setStatus(tr("Number of nodes: %1\nNumber of segments: %2")
		.arg(microstructure->vertices().size())
		.arg(segmentCount));

#if 0
	// Verify dislocation network.
	for(Microstructure::Vertex* vertex : microstructure->vertices()) {
		Vector3 sum = Vector3::Zero();
		for(auto e = vertex->edges(); e != nullptr; e = e->nextVertexEdge()) {
			if(e->isDislocation()) sum += e->burgersVector();
		}
		if(!sum.isZero())
			qDebug() << "Detected violation of Burgers vector conservation at location" << vertex->pos() << "(" << vertex->countDislocationArms() << "arms; delta_b =" << sum << ")";

	}
#endif

	return frameData;
}

/*************************************************************************************
* Connects the slip faces to form two-dimensional manifolds.
**************************************************************************************/
void DislocImporter::FrameLoader::connectSlipFaces(Microstructure& microstructure, const std::map<Microstructure::Face*, std::pair<qlonglong,qlonglong>>& slipSurfaceMap)
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
PipelineFlowState DislocImporter::DislocFrameData::handOver(const PipelineFlowState& existing, bool isNewFile, FileSource* fileSource)
{
	// Insert simulation cell.
	PipelineFlowState output = ParticleFrameData::handOver(existing, isNewFile, fileSource);

	// Insert pattern catalog.
	OORef<PatternCatalog> patternCatalog = existing.getObject<PatternCatalog>();
	if(!patternCatalog) {
		patternCatalog = output.createObject<PatternCatalog>(fileSource);

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
				stype = new StructurePattern(patternCatalog->dataset());
				stype->setId(id);
				stype->setStructureType(StructurePattern::Lattice);
				patternCatalog->addPattern(stype);
			}
			stype->setName(ParticleType::getPredefinedStructureTypeName(predefTypes[id]));
			stype->setColor(ParticleType::getDefaultParticleColor(ParticlesObject::StructureTypeProperty, stype->name(), id));
		}

		// Create Burgers vector families.

		StructurePattern* fccPattern = patternCatalog->structureById(StructureAnalysis::LATTICE_FCC);
		fccPattern->setSymmetryType(StructurePattern::CubicSymmetry);
		fccPattern->setShortName(QStringLiteral("fcc"));
		fccPattern->addBurgersVectorFamily(new BurgersVectorFamily(patternCatalog->dataset(), 1, tr("1/2<110> (Perfect)"), Vector3(1.0f/2.0f, 1.0f/2.0f, 0.0f), Color(0.2f,0.2f,1)));
		fccPattern->addBurgersVectorFamily(new BurgersVectorFamily(patternCatalog->dataset(), 2, tr("1/6<112> (Shockley)"), Vector3(1.0f/6.0f, 1.0f/6.0f, 2.0f/6.0f), Color(0,1,0)));
		fccPattern->addBurgersVectorFamily(new BurgersVectorFamily(patternCatalog->dataset(), 3, tr("1/6<110> (Stair-rod)"), Vector3(1.0f/6.0f, 1.0f/6.0f, 0.0f/6.0f), Color(1,0,1)));
		fccPattern->addBurgersVectorFamily(new BurgersVectorFamily(patternCatalog->dataset(), 4, tr("1/3<001> (Hirth)"), Vector3(1.0f/3.0f, 0.0f, 0.0f), Color(1,1,0)));
		fccPattern->addBurgersVectorFamily(new BurgersVectorFamily(patternCatalog->dataset(), 5, tr("1/3<111> (Frank)"), Vector3(1.0f/3.0f, 1.0f/3.0f, 1.0f/3.0f), Color(0,1,1)));

		StructurePattern* bccPattern = patternCatalog->structureById(StructureAnalysis::LATTICE_BCC);
		bccPattern->setSymmetryType(StructurePattern::CubicSymmetry);
		bccPattern->setShortName(QStringLiteral("bcc"));
		bccPattern->addBurgersVectorFamily(new BurgersVectorFamily(patternCatalog->dataset(), 11, tr("1/2<111>"), Vector3(1.0f/2.0f, 1.0f/2.0f, 1.0f/2.0f), Color(0,1,0)));
		bccPattern->addBurgersVectorFamily(new BurgersVectorFamily(patternCatalog->dataset(), 12, tr("<100>"), Vector3(1.0f, 0.0f, 0.0f), Color(1, 0.3f, 0.8f)));
		bccPattern->addBurgersVectorFamily(new BurgersVectorFamily(patternCatalog->dataset(), 13, tr("<110>"), Vector3(1.0f, 1.0f, 0.0f), Color(0.2f, 0.5f, 1.0f)));

		StructurePattern* hcpPattern = patternCatalog->structureById(StructureAnalysis::LATTICE_HCP);
		hcpPattern->setShortName(QStringLiteral("hcp"));
		hcpPattern->setSymmetryType(StructurePattern::HexagonalSymmetry);
		hcpPattern->addBurgersVectorFamily(new BurgersVectorFamily(patternCatalog->dataset(), 21, tr("1/3<1-210>"), Vector3(sqrt(0.5f), 0.0f, 0.0f), Color(0,1,0)));
		hcpPattern->addBurgersVectorFamily(new BurgersVectorFamily(patternCatalog->dataset(), 22, tr("<0001>"), Vector3(0.0f, 0.0f, sqrt(4.0f/3.0f)), Color(0.2f,0.2f,1)));
		hcpPattern->addBurgersVectorFamily(new BurgersVectorFamily(patternCatalog->dataset(), 23, tr("<1-100>"), Vector3(0.0f, sqrt(3.0f/2.0f), 0.0f), Color(1,0,1)));
		hcpPattern->addBurgersVectorFamily(new BurgersVectorFamily(patternCatalog->dataset(), 24, tr("1/3<1-100>"), Vector3(0.0f, sqrt(3.0f/2.0f)/3.0f, 0.0f), Color(1,0.5f,0)));
		hcpPattern->addBurgersVectorFamily(new BurgersVectorFamily(patternCatalog->dataset(), 25, tr("1/3<1-213>"), Vector3(sqrt(0.5f), 0.0f, sqrt(4.0f/3.0f)), Color(1,1,0)));

		StructurePattern* cubicDiaPattern = patternCatalog->structureById(StructureAnalysis::LATTICE_CUBIC_DIAMOND);
		cubicDiaPattern->setShortName(QStringLiteral("diamond"));
		cubicDiaPattern->setSymmetryType(StructurePattern::CubicSymmetry);
		cubicDiaPattern->addBurgersVectorFamily(new BurgersVectorFamily(patternCatalog->dataset(), 31, tr("1/2<110>"), Vector3(1.0f/2.0f, 1.0f/2.0f, 0.0f), Color(0.2f,0.2f,1)));
		cubicDiaPattern->addBurgersVectorFamily(new BurgersVectorFamily(patternCatalog->dataset(), 32, tr("1/6<112>"), Vector3(1.0f/6.0f, 1.0f/6.0f, 2.0f/6.0f), Color(0,1,0)));
		cubicDiaPattern->addBurgersVectorFamily(new BurgersVectorFamily(patternCatalog->dataset(), 33, tr("1/6<110>"), Vector3(1.0f/6.0f, 1.0f/6.0f, 0.0f), Color(1,0,1)));
		cubicDiaPattern->addBurgersVectorFamily(new BurgersVectorFamily(patternCatalog->dataset(), 34, tr("1/3<111>"), Vector3(1.0f/3.0f, 1.0f/3.0f, 1.0f/3.0f), Color(0,1,1)));

		StructurePattern* hexDiaPattern = patternCatalog->structureById(StructureAnalysis::LATTICE_HEX_DIAMOND);
		hexDiaPattern->setShortName(QStringLiteral("hex_diamond"));
		hexDiaPattern->setSymmetryType(StructurePattern::HexagonalSymmetry);
		hexDiaPattern->addBurgersVectorFamily(new BurgersVectorFamily(patternCatalog->dataset(), 41, tr("1/3<1-210>"), Vector3(sqrt(0.5f), 0.0f, 0.0f), Color(0,1,0)));
		hexDiaPattern->addBurgersVectorFamily(new BurgersVectorFamily(patternCatalog->dataset(), 42, tr("<0001>"), Vector3(0.0f, 0.0f, sqrt(4.0f/3.0f)), Color(0.2f,0.2f,1)));
		hexDiaPattern->addBurgersVectorFamily(new BurgersVectorFamily(patternCatalog->dataset(), 43, tr("<1-100>"), Vector3(0.0f, sqrt(3.0f/2.0f), 0.0f), Color(1,0,1)));
		hexDiaPattern->addBurgersVectorFamily(new BurgersVectorFamily(patternCatalog->dataset(), 44, tr("1/3<1-100>"), Vector3(0.0f, sqrt(3.0f/2.0f)/3.0f, 0.0f), Color(1,0.5f,0)));		
	}
	else {
		output.addObject(patternCatalog);
	}
		
	if(microstructure()) {

		// Insert microstructure.
		MicrostructureObject* microstructureObj = const_cast<MicrostructureObject*>(existing.getObject<MicrostructureObject>());
		if(!microstructureObj) {
			microstructureObj = output.createObject<MicrostructureObject>(fileSource);
			// Create a display object for the dislocation lines.
			OORef<DislocationVis> dislocationVis = new DislocationVis(fileSource->dataset());
			if(!Application::instance()->scriptMode())
				dislocationVis->loadUserDefaults();
			microstructureObj->addVisElement(dislocationVis);
			// Create a display object for the slip surfaces.
			OORef<SlipSurfaceVis> slipSurfaceVis = new SlipSurfaceVis(fileSource->dataset());
			if(!Application::instance()->scriptMode())
				slipSurfaceVis->loadUserDefaults();
			microstructureObj->addVisElement(slipSurfaceVis);
		}
		else {
			output.addObject(microstructureObj);
		}
		microstructureObj->setDomain(output.getObject<SimulationCellObject>());
		microstructureObj->setStorage(microstructure());

		// Insert cluster graph as a separate data object.
		OORef<ClusterGraphObject> clusterGraphObj = existing.getObject<ClusterGraphObject>();
		if(!clusterGraphObj) {
			clusterGraphObj = output.createObject<ClusterGraphObject>(fileSource);
		}
		else {
			output.addObject(clusterGraphObj);		
		}
		clusterGraphObj->setStorage(microstructure()->clusterGraph());
	}

	return output;
}

}	// End of namespace
}	// End of namespace
}	// End of namespace
