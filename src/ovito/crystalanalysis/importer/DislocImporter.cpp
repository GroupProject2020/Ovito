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
#include <ovito/crystalanalysis/objects/Microstructure.h>
#include <ovito/crystalanalysis/objects/MicrostructurePhase.h>
#include <ovito/crystalanalysis/objects/DislocationVis.h>
#include <ovito/crystalanalysis/objects/SlipSurfaceVis.h>
#include <ovito/crystalanalysis/modifier/microstructure/SimplifyMicrostructureModifier.h>
#include <ovito/core/app/Application.h>
#include <ovito/core/dataset/io/FileSource.h>
#include "DislocImporter.h"

#include <3rdparty/netcdf_integration/NetCDFIntegration.h>
#include <netcdf.h>
#include <boost/functional/hash.hpp>

namespace Ovito { namespace CrystalAnalysis {

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
//	OORef<SimplifyMicrostructureModifier> modifier = new SimplifyMicrostructureModifier(pipeline->dataset());
//	if(Application::instance()->executionContext() == Application::ExecutionContext::Interactive)
//		modifier->loadUserDefaults();
//	pipeline->applyModifier(modifier);
}

/******************************************************************************
* Parses the given input file.
******************************************************************************/
FileSourceImporter::FrameDataPtr DislocImporter::FrameLoader::loadFile(QFile& file)
{
	setProgressText(tr("Reading disloc file %1").arg(frame().sourceFile.toString(QUrl::RemovePassword | QUrl::PreferLocalFile | QUrl::PrettyDecoded)));

	// Create the container structures for holding the loaded data.
	auto frameData = std::make_shared<DislocFrameData>();
	MicrostructureData& microstructure = frameData->microstructure();

	// Fix disloc specific data.
	std::vector<Vector3> latticeVectors;
	std::vector<Vector3> transformedLatticeVectors;
	size_t segmentCount = 0;

	// Temporary data structure.
	std::vector<std::pair<qlonglong,qlonglong>> slipSurfaceMap;

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
			throw Exception(tr("NetCDF file follows '%1' conventions; expected 'FixDisloc' convention.").arg(conventions_str.get()));

		// Read lattice structure.
		NCERR(nc_inq_attlen(root_ncid, NC_GLOBAL, "LatticeStructure", &len));
		auto lattice_structure_str = std::make_unique<char[]>(len+1);
		NCERR(nc_get_att_text(root_ncid, NC_GLOBAL, "LatticeStructure", lattice_structure_str.get()));
		lattice_structure_str[len] = '\0';

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
		frameData->microstructure().cell() = frameData->simulationCell();

		// Read lattice orientation matrix.
		int lattice_orientation_var;
		NCERR(nc_inq_varid(root_ncid, "lattice_orientation", &lattice_orientation_var));
		Matrix3 latticeOrientation;
#ifdef FLOATTYPE_FLOAT
		NCERR(nc_get_var_float(root_ncid, lattice_orientation_var, latticeOrientation.elements()));
#else
		NCERR(nc_get_var_double(root_ncid, lattice_orientation_var, latticeOrientation.elements()));
#endif
		if(strcmp(lattice_structure_str.get(), "bcc") == 0) frameData->setLatticeStructure(ParticleType::PredefinedStructureType::BCC, latticeOrientation);
		else if(strcmp(lattice_structure_str.get(), "fcc") == 0) frameData->setLatticeStructure(ParticleType::PredefinedStructureType::FCC, latticeOrientation);
		else if(strcmp(lattice_structure_str.get(), "fcc_perfect") == 0) frameData->setLatticeStructure(ParticleType::PredefinedStructureType::FCC, latticeOrientation);
		else throw Exception(tr("File parsing error. Unknown lattice structure type: %1").arg(lattice_structure_str.get()));

		// Create microstructure regions.
		int emptyRegion = microstructure.createRegion(0);
		int crystalRegion = microstructure.createRegion(frameData->latticeStructure());

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
		std::vector<MicrostructureData::vertex_index> vertexMap(numNodeRecords);
		std::unordered_map<std::array<qlonglong,4>, MicrostructureData::vertex_index, boost::hash<std::array<qlonglong,4>>> idMap;
		auto vertexMapIter = vertexMap.begin();
		auto nodalPositionsIter = nodalPositions.cbegin();
		for(const auto& id : nodalIds) {
			auto iter = idMap.find(id);
			if(iter == idMap.end())
				iter = idMap.emplace(id, microstructure.createVertex(Point3(*nodalPositionsIter))).first;
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
			MicrostructureData::vertex_index vertex1 = vertexMap[seg[0]];
			MicrostructureData::vertex_index vertex2 = vertexMap[seg[1]];
			microstructure.createDislocationSegment(vertex1, vertex2, Vector3(*burgersVector++), crystalRegion);
		}
		segmentCount = dislocationSegments.size();

		// Form continuous dislocation lines from the segments.
		microstructure.makeContinuousDislocationLines();

		// Read slip facets.
		int slip_facets_dim = -1, slip_facet_vertices_dim = -1;
		nc_inq_dimid(root_ncid, "slip_facets", &slip_facets_dim);
		nc_inq_dimid(root_ncid, "slip_facet_vertices", &slip_facet_vertices_dim);
		if(slip_facets_dim != -1) {
			int slipped_edges_var;
			int slip_vectors_var;
			int slip_facet_normals_var;
			int slip_facet_edge_counts_var;
			int slip_facet_vertices_var;
			NCERR(nc_inq_varid(root_ncid, "slipped_edges", &slipped_edges_var));
			NCERR(nc_inq_varid(root_ncid, "slip_vectors", &slip_vectors_var));
			if(nc_inq_varid(root_ncid, "slip_facet_normals", &slip_facet_normals_var) != NC_NOERR)
				slip_facet_normals_var = -1;
			NCERR(nc_inq_varid(root_ncid, "slip_facet_edge_counts", &slip_facet_edge_counts_var));
			NCERR(nc_inq_varid(root_ncid, "slip_facet_vertices", &slip_facet_vertices_var));
			size_t numSlipFacets, numSlipFacetVertices;
			NCERR(nc_inq_dimlen(root_ncid, slip_facets_dim, &numSlipFacets));
			NCERR(nc_inq_dimlen(root_ncid, slip_facet_vertices_dim, &numSlipFacetVertices));
			std::vector<Vector_3<float>> slipVectors(numSlipFacets);
			std::vector<Vector_3<float>> slipFacetNormals;
			std::vector<std::array<qlonglong,2>> slippedEdges(numSlipFacets);
			std::vector<int> slipFacetEdgeCounts(numSlipFacets);
			std::vector<qlonglong> slipFacetVertices(numSlipFacetVertices);
			if(numSlipFacets) {
				NCERR(nc_get_var_float(root_ncid, slip_vectors_var, slipVectors.front().data()));
				if(slip_facet_normals_var != -1) {
					slipFacetNormals.resize(numSlipFacets);
					NCERR(nc_get_var_float(root_ncid, slip_facet_normals_var, slipFacetNormals.front().data()));
				}
				NCERR(nc_get_var_longlong(root_ncid, slipped_edges_var, slippedEdges.front().data()));
				NCERR(nc_get_var_int(root_ncid, slip_facet_edge_counts_var, slipFacetEdgeCounts.data()));
			}
			if(numSlipFacetVertices) {
				NCERR(nc_get_var_longlong(root_ncid, slip_facet_vertices_var, slipFacetVertices.data()));
			}

			// Create slip surface facets (two mesh faces per slip facet).
			auto slipVector = slipVectors.cbegin();
			auto slipFacetNormal = slipFacetNormals.cbegin();
			auto slipFacetEdgeCount = slipFacetEdgeCounts.cbegin();
			auto slipFacetVertex = slipFacetVertices.cbegin();
			slipSurfaceMap.resize(microstructure.faceCount());
			slipSurfaceMap.reserve(slipSurfaceMap.size() + numSlipFacets*2);
			for(const auto& slippedEdge : slippedEdges) {

				// Create first mesh face.
				MicrostructureData::face_index face = microstructure.createFace({}, crystalRegion, MicrostructureData::SLIP_FACET,
					Vector3(*slipVector), slipFacetNormals.empty() ? Vector3::Zero() : Vector3(*slipFacetNormal));
				MicrostructureData::vertex_index node0 = vertexMap[*slipFacetVertex++];
				MicrostructureData::vertex_index node1 = node0;
				MicrostructureData::vertex_index node2;
				for(int i = 1; i < *slipFacetEdgeCount; i++, node1 = node2) {
					node2 = vertexMap[*slipFacetVertex++];
					microstructure.createEdge(node1, node2, face);
				}
				microstructure.createEdge(node1, node0, face);

				// Create the opposite mesh face.
				MicrostructureData::face_index oppositeFace = microstructure.createFace({}, crystalRegion, MicrostructureData::SLIP_FACET,
					-Vector3(*slipVector), slipFacetNormals.empty() ? Vector3::Zero() : -Vector3(*slipFacetNormal));
				MicrostructureData::edge_index edge = microstructure.firstFaceEdge(face);
				do {
					microstructure.createEdge(microstructure.vertex2(edge), microstructure.vertex1(edge), oppositeFace);
					edge = microstructure.prevFaceEdge(edge);
				}
				while(edge != microstructure.firstFaceEdge(face));
				microstructure.topology()->linkOppositeFaces(face, oppositeFace);

				slipSurfaceMap.push_back({ slippedEdge[0], slippedEdge[1] });
				slipSurfaceMap.push_back({ slippedEdge[1], slippedEdge[0] });

				++slipVector;
				++slipFacetEdgeCount;
				if(!slipFacetNormals.empty())
					++slipFacetNormal;
			}
			OVITO_ASSERT(slipFacetVertex == slipFacetVertices.cend());
			OVITO_ASSERT(slipSurfaceMap.size() == microstructure.faceCount());
		}

		// Close the input file again.
		NCERR(nc_close(root_ncid));
	}
	catch(...) {
		if(root_ncid)
			nc_close(root_ncid);
		throw;
	}

	// Connect half-edges of slip faces.
	connectSlipFaces(microstructure, slipSurfaceMap);

	frameData->setStatus(tr("Number of nodes: %1\nNumber of segments: %2")
		.arg(microstructure.vertexCount())
		.arg(segmentCount));

	// Verify dislocation network (Burgers vector conservation at nodes).
	for(MicrostructureData::vertex_index vertex = 0; vertex < microstructure.vertexCount(); vertex++) {
		Vector3 sum = Vector3::Zero();
		for(auto e = microstructure.firstVertexEdge(vertex); e != HalfEdgeMesh::InvalidIndex; e = microstructure.nextVertexEdge(e)) {
			if(microstructure.isPhysicalDislocationEdge(e))
				sum += microstructure.burgersVector(microstructure.adjacentFace(e));
		}
		if(!sum.isZero(1e-6))
			qDebug() << "Detected violation of Burgers vector conservation at location" << microstructure.vertexPosition(vertex) << "(" << microstructure.countDislocationArms(vertex) << "arms; delta_b =" << sum << ")";
	}

	return frameData;
}

/*************************************************************************************
* Connects the slip faces to form two-dimensional manifolds.
**************************************************************************************/
void DislocImporter::FrameLoader::connectSlipFaces(MicrostructureData& microstructure, const std::vector<std::pair<qlonglong,qlonglong>>& slipSurfaceMap)
{
	// Link slip surface faces with their neighbors, i.e. find the opposite edge for every half-edge of a slip face.
	MicrostructureData::size_type edgeCount = microstructure.edgeCount();
	for(MicrostructureData::edge_index edge1 = 0; edge1 < edgeCount; edge1++) {
		// Only process edges which haven't been linked to their neighbors yet.
		if(microstructure.nextManifoldEdge(edge1) != HalfEdgeMesh::InvalidIndex) continue;
		MicrostructureData::face_index face1 = microstructure.adjacentFace(edge1);
		if(!microstructure.isSlipSurfaceFace(face1)) continue;

		OVITO_ASSERT(!microstructure.hasOppositeEdge(edge1));
		MicrostructureData::vertex_index vertex1 = microstructure.vertex1(edge1);
		MicrostructureData::vertex_index vertex2 = microstructure.vertex2(edge1);
		MicrostructureData::edge_index oppositeEdge1 = microstructure.findEdge(microstructure.oppositeFace(face1), vertex2, vertex1);
		OVITO_ASSERT(oppositeEdge1 != HalfEdgeMesh::InvalidIndex);
		OVITO_ASSERT(microstructure.nextManifoldEdge(edge1) == HalfEdgeMesh::InvalidIndex);
		OVITO_ASSERT(microstructure.nextManifoldEdge(oppositeEdge1) == HalfEdgeMesh::InvalidIndex);

		// At an edge, either 1, 2, or 3 slip surface manifolds can meet.
		// Here, we will link them together in the right order.

		const std::pair<qulonglong,qulonglong>& edgeVertexCodes = slipSurfaceMap[face1];

		// Find the other two manifolds meeting at the current edge (if they exist).
		MicrostructureData::edge_index edge2 = HalfEdgeMesh::InvalidIndex;
		MicrostructureData::edge_index edge3 = HalfEdgeMesh::InvalidIndex;
		MicrostructureData::edge_index oppositeEdge2 = HalfEdgeMesh::InvalidIndex;
		MicrostructureData::edge_index oppositeEdge3 = HalfEdgeMesh::InvalidIndex;
		for(MicrostructureData::edge_index e = microstructure.firstVertexEdge(vertex1); e != HalfEdgeMesh::InvalidIndex; e = microstructure.nextVertexEdge(e)) {
			MicrostructureData::face_index face2 = microstructure.adjacentFace(e);
			if(microstructure.vertex2(e) == vertex2 && microstructure.isSlipSurfaceFace(face2) && face2 != face1) {
				const std::pair<qulonglong,qulonglong>& edgeVertexCodes2 = slipSurfaceMap[face2];
				if(edgeVertexCodes.second == edgeVertexCodes2.first) {
					OVITO_ASSERT(edgeVertexCodes.first != edgeVertexCodes2.second);
					OVITO_ASSERT(edge2 == HalfEdgeMesh::InvalidIndex);
					OVITO_ASSERT(!microstructure.hasOppositeEdge(e));
					OVITO_ASSERT(microstructure.nextManifoldEdge(e) == HalfEdgeMesh::InvalidIndex);
					edge2 = e;
					oppositeEdge2 = microstructure.findEdge(microstructure.oppositeFace(face2), vertex2, vertex1);
					OVITO_ASSERT(oppositeEdge2 != HalfEdgeMesh::InvalidIndex);
					OVITO_ASSERT(microstructure.nextManifoldEdge(oppositeEdge2) == HalfEdgeMesh::InvalidIndex);
				}
				else {
					OVITO_ASSERT(edgeVertexCodes.first == edgeVertexCodes2.second);
					OVITO_ASSERT(edge3 == HalfEdgeMesh::InvalidIndex);
					OVITO_ASSERT(!microstructure.hasOppositeEdge(e));
					OVITO_ASSERT(microstructure.nextManifoldEdge(e) == HalfEdgeMesh::InvalidIndex);
					edge3 = e;
					oppositeEdge3 = microstructure.findEdge(microstructure.oppositeFace(face2), vertex2, vertex1);
					OVITO_ASSERT(oppositeEdge3 != HalfEdgeMesh::InvalidIndex);
					OVITO_ASSERT(microstructure.nextManifoldEdge(oppositeEdge3) == HalfEdgeMesh::InvalidIndex);
				}
			}
		}

		if(edge2 != HalfEdgeMesh::InvalidIndex) {
			microstructure.linkOppositeEdges(edge1, oppositeEdge2);
			microstructure.setNextManifoldEdge(edge1, edge2);
			microstructure.setNextManifoldEdge(oppositeEdge2, oppositeEdge1);
			if(edge3 != HalfEdgeMesh::InvalidIndex) {
				microstructure.linkOppositeEdges(edge2, oppositeEdge3);
				microstructure.linkOppositeEdges(edge3, oppositeEdge1);
				microstructure.setNextManifoldEdge(edge2, edge3);
				microstructure.setNextManifoldEdge(oppositeEdge3, oppositeEdge2);
				microstructure.setNextManifoldEdge(edge3, edge1);
				microstructure.setNextManifoldEdge(oppositeEdge1, oppositeEdge3);
				OVITO_ASSERT(microstructure.countManifolds(edge1) == 3);
				OVITO_ASSERT(microstructure.countManifolds(edge2) == 3);
				OVITO_ASSERT(microstructure.countManifolds(edge3) == 3);
			}
			else {
				microstructure.linkOppositeEdges(edge2, oppositeEdge1);
				microstructure.setNextManifoldEdge(edge2, edge1);
				microstructure.setNextManifoldEdge(oppositeEdge1, oppositeEdge2);
				OVITO_ASSERT(microstructure.countManifolds(edge1) == 2);
				OVITO_ASSERT(microstructure.countManifolds(edge2) == 2);
				OVITO_ASSERT(microstructure.countManifolds(oppositeEdge1) == 2);
				OVITO_ASSERT(microstructure.countManifolds(oppositeEdge2) == 2);
			}
		}
		else {
			if(edge3 != HalfEdgeMesh::InvalidIndex) {
				microstructure.linkOppositeEdges(edge1, oppositeEdge3);
				microstructure.linkOppositeEdges(oppositeEdge1, edge3);
				microstructure.setNextManifoldEdge(edge1, edge3);
				microstructure.setNextManifoldEdge(oppositeEdge3, oppositeEdge1);
				microstructure.setNextManifoldEdge(edge3, edge1);
				microstructure.setNextManifoldEdge(oppositeEdge1, oppositeEdge3);
				OVITO_ASSERT(microstructure.countManifolds(edge1) == 2);
				OVITO_ASSERT(microstructure.countManifolds(oppositeEdge1) == 2);
				OVITO_ASSERT(microstructure.countManifolds(edge3) == 2);
				OVITO_ASSERT(microstructure.countManifolds(oppositeEdge3) == 2);
			}
			else {
				microstructure.setNextManifoldEdge(edge1, edge1);
				microstructure.setNextManifoldEdge(oppositeEdge1, oppositeEdge1);
				OVITO_ASSERT(microstructure.countManifolds(edge1) == 1);
				OVITO_ASSERT(microstructure.countManifolds(oppositeEdge1) == 1);
			}
		}

		OVITO_ASSERT(microstructure.nextManifoldEdge(edge1) != HalfEdgeMesh::InvalidIndex);
		OVITO_ASSERT(microstructure.vertex2(microstructure.nextManifoldEdge(edge1)) == vertex2);
		OVITO_ASSERT(microstructure.vertex1(microstructure.nextManifoldEdge(edge1)) == vertex1);
		OVITO_ASSERT(microstructure.nextManifoldEdge(oppositeEdge1) != HalfEdgeMesh::InvalidIndex);
		OVITO_ASSERT(edge2 == HalfEdgeMesh::InvalidIndex || microstructure.nextManifoldEdge(edge2) != HalfEdgeMesh::InvalidIndex);
		OVITO_ASSERT(oppositeEdge2 == HalfEdgeMesh::InvalidIndex || microstructure.nextManifoldEdge(oppositeEdge2) != HalfEdgeMesh::InvalidIndex);
		OVITO_ASSERT(edge3 == HalfEdgeMesh::InvalidIndex || microstructure.nextManifoldEdge(edge3) != HalfEdgeMesh::InvalidIndex);
		OVITO_ASSERT(oppositeEdge3 == HalfEdgeMesh::InvalidIndex || microstructure.nextManifoldEdge(oppositeEdge3) != HalfEdgeMesh::InvalidIndex);
	}
}

/******************************************************************************
* Inserts the data loaded by the FrameLoader into the provided data collection.
* This function is called by the system from the main thread after the
* asynchronous loading operation has finished.
******************************************************************************/
OORef<DataCollection> DislocImporter::DislocFrameData::handOver(const DataCollection* existing, bool isNewFile, FileSource* fileSource)
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

		// Create a visual element for the slip surfaces.
		OORef<SlipSurfaceVis> slipSurfaceVis = new SlipSurfaceVis(fileSource->dataset());
		if(Application::instance()->executionContext() == Application::ExecutionContext::Interactive)
			slipSurfaceVis->loadUserDefaults();
		microstructureObj->addVisElement(slipSurfaceVis);
	}
	else {
		output->addObject(microstructureObj);
	}
	microstructureObj->setDomain(output->getObject<SimulationCellObject>());
	microstructure().transferTo(microstructureObj);

	// Define crystal phase.
	OVITO_ASSERT(latticeStructure() != 0);
	OVITO_ASSERT(!microstructureObj->dataset()->undoStack().isRecording());
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
		}
	}

	// Store lattice orientation information.
	OVITO_ASSERT(microstructureObj->regions()->elementCount() == 2);
	PropertyAccess<Matrix3> correspondenceProperty = microstructureObj->regions()->createProperty(SurfaceMeshRegions::LatticeCorrespondenceProperty);
	correspondenceProperty[0] = Matrix3::Zero();		// The "empty" region.
	correspondenceProperty[1] = _latticeOrientation;	// The "crystal" region.

	return output;
}

}	// End of namespace
}	// End of namespace
