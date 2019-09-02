///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2019) Alexander Stukowski
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
#include <plugins/crystalanalysis/objects/DislocationNetworkObject.h>
#include <plugins/crystalanalysis/objects/ClusterGraphObject.h>
#include <plugins/mesh/surface/SurfaceMesh.h>
#include <core/utilities/io/CompressedTextWriter.h>
#include <core/utilities/concurrent/Promise.h>
#include <core/utilities/concurrent/TaskManager.h>
#include <plugins/stdobj/simcell/SimulationCellObject.h>
#include <core/dataset/scene/PipelineSceneNode.h>
#include "CAExporter.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

IMPLEMENT_OVITO_CLASS(CAExporter);

/******************************************************************************
 * This is called once for every output file to be written and before
 * exportFrame() is called.
 *****************************************************************************/
bool CAExporter::openOutputFile(const QString& filePath, int numberOfFrames, AsyncOperation& operation)
{
	OVITO_ASSERT(!_outputFile.isOpen());
	OVITO_ASSERT(!_outputStream);

	_outputFile.setFileName(filePath);
	_outputStream.reset(new CompressedTextWriter(_outputFile, dataset()));

	return true;
}

/******************************************************************************
 * This is called once for every output file written after exportFrame()
 * has been called.
 *****************************************************************************/
void CAExporter::closeOutputFile(bool exportCompleted)
{
	_outputStream.reset();
	if(_outputFile.isOpen())
		_outputFile.close();

	if(!exportCompleted)
		_outputFile.remove();
}

/******************************************************************************
 * Exports a single animation frame to the current output file.
 *****************************************************************************/
bool CAExporter::exportFrame(int frameNumber, TimePoint time, const QString& filePath, AsyncOperation&& operation)
{
	// Evaluate data pipeline.
	const PipelineFlowState& state = getPipelineDataToBeExported(time, operation);
	if(operation.isCanceled())
		return false;

	// Set progress display.
	operation.setProgressText(tr("Writing file %1").arg(filePath));

	// Get simulation cell info.
	const SimulationCellObject* simulationCell = state.expectObject<SimulationCellObject>();

	// Get dislocation lines.
	const DislocationNetworkObject* dislocationObj = state.getObject<DislocationNetworkObject>();

	// Get microstructure object.
	const Microstructure* microstructureObj = state.getObject<Microstructure>();

	// Get defect surface mesh.
	const SurfaceMesh* defectMesh = meshExportEnabled() ? state.getObject<SurfaceMesh>() : nullptr;

	if(!dislocationObj && !defectMesh && !microstructureObj)
		throwException(tr("Dataset to be exported contains no dislocation lines nor a surface mesh. Cannot write CA file."));

	// Write file header.
	textStream() << "CA_FILE_VERSION 6\n";
	textStream() << "CA_LIB_VERSION 0.0.0\n";

	std::vector<MicrostructurePhase*> crystalStructures;
	if(dislocationObj) {
		for(MicrostructurePhase* phase : dislocationObj->crystalStructures()) {
			if(phase->numericId() != 0)
				crystalStructures.push_back(phase);
		}
	}
	else if(microstructureObj) {
		const PropertyObject* phaseProperty = microstructureObj->regions()->expectProperty(SurfaceMeshRegions::PhaseProperty);
		for(ElementType* t : phaseProperty->elementTypes()) {
			if(MicrostructurePhase* phase = dynamic_object_cast<MicrostructurePhase>(t))
				if(phase->numericId() != 0)
					crystalStructures.push_back(phase);
		}
	}

	// Write list of structure types.
	textStream() << "STRUCTURE_TYPES " << crystalStructures.size() << "\n";
	for(const MicrostructurePhase* s : crystalStructures) {
		textStream() << "STRUCTURE_TYPE " << s->numericId() << "\n";
		textStream() << "NAME " << (s->shortName().isEmpty() ? s->name() : s->shortName()) << "\n";
		textStream() << "FULL_NAME " << s->longName() << "\n";
		textStream() << "COLOR " << s->color().r() << " " << s->color().g() << " " << s->color().b() << "\n";
		if(s->dimensionality() == MicrostructurePhase::Dimensionality::Volumetric) textStream() << "TYPE LATTICE\n";
		else if(s->dimensionality() == MicrostructurePhase::Dimensionality::Planar) textStream() << "TYPE INTERFACE\n";
		else if(s->dimensionality() == MicrostructurePhase::Dimensionality::Pointlike) textStream() << "TYPE POINTDEFECT\n";
		textStream() << "BURGERS_VECTOR_FAMILIES " << s->burgersVectorFamilies().size() << "\n";
		int bvfId = 0;
		for(BurgersVectorFamily* bvf : s->burgersVectorFamilies()) {
			textStream() << "BURGERS_VECTOR_FAMILY ID " << bvfId << "\n" << bvf->name() << "\n";
			textStream() << bvf->burgersVector().x() << " " << bvf->burgersVector().y() << " " << bvf->burgersVector().z() << "\n";
			textStream() << bvf->color().r() << " " << bvf->color().g() << " " << bvf->color().b() << "\n";
			bvfId++;
		}
		textStream() << "END_STRUCTURE_TYPE\n";
	}

	// Write simulation cell geometry.
	const AffineTransformation& cell = simulationCell->cellMatrix();
	textStream() << "SIMULATION_CELL_ORIGIN " << cell.column(3).x() << " " << cell.column(3).y() << " " << cell.column(3).z() << "\n";
	textStream() << "SIMULATION_CELL_MATRIX" << "\n"
			<< cell.column(0).x() << " " << cell.column(1).x() << " " << cell.column(2).x() << "\n"
			<< cell.column(0).y() << " " << cell.column(1).y() << " " << cell.column(2).y() << "\n"
			<< cell.column(0).z() << " " << cell.column(1).z() << " " << cell.column(2).z() << "\n";
	textStream() << "PBC_FLAGS "
			<< (int)simulationCell->pbcFlags()[0] << " "
			<< (int)simulationCell->pbcFlags()[1] << " "
			<< (int)simulationCell->pbcFlags()[2] << "\n";

	// Select the dislocation network to be exported.
	// Optionally, convert the selected Microstructure object to a DislocationNetwork object for export.
	std::shared_ptr<DislocationNetwork> dislocations;
	if(dislocationObj) {
		dislocations = dislocationObj->storage();
	}
	else if(microstructureObj) {
		dislocations = std::make_shared<DislocationNetwork>(microstructureObj);
	}
	// Get cluster graph.
	const auto clusterGraph = dislocations->clusterGraph();

	// Write list of clusters.
	if(clusterGraph) {
		textStream() << "CLUSTERS " << (clusterGraph->clusters().size() - 1) << "\n";
		for(const Cluster* cluster : clusterGraph->clusters()) {
			if(cluster->id == 0) continue;
			OVITO_ASSERT(clusterGraph->clusters()[cluster->id] == cluster);
			textStream() << "CLUSTER " << cluster->id << "\n";
			textStream() << "CLUSTER_STRUCTURE " << cluster->structure << "\n";
			textStream() << "CLUSTER_ORIENTATION\n";
			for(size_t row = 0; row < 3; row++)
				textStream() << cluster->orientation(row,0) << " " << cluster->orientation(row,1) << " " << cluster->orientation(row,2) << "\n";
			textStream() << "CLUSTER_COLOR " << cluster->color.r() << " " << cluster->color.g() << " " << cluster->color.b() << "\n";
			textStream() << "CLUSTER_SIZE " << cluster->atomCount << "\n";
			textStream() << "END_CLUSTER\n";
		}

		// Count cluster transitions.
		size_t numClusterTransitions = 0;
		for(const ClusterTransition* t : clusterGraph->clusterTransitions()) {
			if(!t->isSelfTransition())
				numClusterTransitions++;
		}

		// Serialize cluster transitions.
		textStream() << "CLUSTER_TRANSITIONS " << numClusterTransitions << "\n";
		for(const ClusterTransition* t : clusterGraph->clusterTransitions()) {
			if(t->isSelfTransition()) continue;
			textStream() << "TRANSITION " << (t->cluster1->id - 1) << " " << (t->cluster2->id - 1) << "\n";
			textStream() << t->tm.column(0).x() << " " << t->tm.column(1).x() << " " << t->tm.column(2).x() << " "
					<< t->tm.column(0).y() << " " << t->tm.column(1).y() << " " << t->tm.column(2).y() << " "
					<< t->tm.column(0).z() << " " << t->tm.column(1).z() << " " << t->tm.column(2).z() << "\n";
		}
	}

	if(dislocations) {
		// Write list of dislocation segments.
		textStream() << "DISLOCATIONS " << dislocations->segments().size() << "\n";
		for(const DislocationSegment* segment : dislocations->segments()) {

			// Make sure consecutive identifiers have been assigned to segments.
			OVITO_ASSERT(segment->id >= 0 && segment->id < dislocations->segments().size());
			OVITO_ASSERT(dislocations->segments()[segment->id] == segment);

			textStream() << segment->id << "\n";
			textStream() << segment->burgersVector.localVec().x() << " " << segment->burgersVector.localVec().y() << " " << segment->burgersVector.localVec().z() << "\n";
			textStream() << segment->burgersVector.cluster()->id << "\n";

			// Write polyline.
			textStream() << segment->line.size() << "\n";
			if(segment->coreSize.empty()) {
				for(const Point3& p : segment->line) {
					textStream() << p.x() << " " << p.y() << " " << p.z() << "\n";
				}
			}
			else {
				OVITO_ASSERT(segment->coreSize.size() == segment->line.size());
				auto cs = segment->coreSize.cbegin();
				for(const Point3& p : segment->line) {
					textStream() << p.x() << " " << p.y() << " " << p.z() << " " << (*cs++) << "\n";
				}
			}
		}

		// Write dislocation connectivity information.
		textStream() << "DISLOCATION_JUNCTIONS\n";
		int index = 0;
		for(const DislocationSegment* segment : dislocations->segments()) {
			OVITO_ASSERT(segment->forwardNode().junctionRing->segment->id < dislocations->segments().size());
			OVITO_ASSERT(segment->backwardNode().junctionRing->segment->id < dislocations->segments().size());

			for(int nodeIndex = 0; nodeIndex < 2; nodeIndex++) {
				const DislocationNode* otherNode = segment->nodes[nodeIndex]->junctionRing;
				textStream() << (int)otherNode->isForwardNode() << " " << otherNode->segment->id << "\n";
			}
			index++;
		}
	}

	if(defectMesh && defectMesh->topology()->isClosed()) {
		defectMesh->verifyMeshIntegrity();
		const ConstPropertyPtr& vertexCoords = defectMesh->vertices()->getPropertyStorage(SurfaceMeshVertices::PositionProperty);
		ConstHalfEdgeMeshPtr topology = defectMesh->topology();

		// Serialize list of vertices.
		textStream() << "DEFECT_MESH_VERTICES " << vertexCoords->size() << "\n";
		for(const Point3& vertex : vertexCoords->constPoint3Range()) {
			textStream() << vertex.x() << " " << vertex.y() << " " << vertex.z() << "\n";
		}

		// Serialize list of facets.
		textStream() << "DEFECT_MESH_FACETS " << topology->faceCount() << "\n";
		for(HalfEdgeMesh::face_index face = 0; face < topology->faceCount(); face++) {
			HalfEdgeMesh::edge_index e = topology->firstFaceEdge(face);
			do {
				textStream() << topology->vertex1(e) << " ";
				e = topology->nextFaceEdge(e);
			}
			while(e != topology->firstFaceEdge(face));
			textStream() << "\n";
		}

		// Serialize face adjacency information.
		for(HalfEdgeMesh::face_index face = 0; face < topology->faceCount(); face++) {
			HalfEdgeMesh::edge_index e = topology->firstFaceEdge(face);
			do {
				textStream() << topology->adjacentFace(topology->oppositeEdge(e)) << " ";
				e = topology->nextFaceEdge(e);
			}
			while(e != topology->firstFaceEdge(face));
			textStream() << "\n";
		}
	}

	return true;
}

}	// End of namespace
}	// End of namespace
}	// End of namespace
