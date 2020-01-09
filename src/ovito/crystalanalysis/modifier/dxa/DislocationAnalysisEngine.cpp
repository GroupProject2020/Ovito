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
#include <ovito/crystalanalysis/objects/DislocationNetworkObject.h>
#include <ovito/crystalanalysis/objects/ClusterGraphObject.h>
#include <ovito/mesh/surface/SurfaceMesh.h>
#include <ovito/stdobj/table/DataTable.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>
#include <ovito/core/dataset/DataSet.h>
#include "DislocationAnalysisEngine.h"
#include "DislocationAnalysisModifier.h"

#if 0
#include <fstream>
#endif

namespace Ovito { namespace CrystalAnalysis {

/******************************************************************************
* Constructor.
******************************************************************************/
DislocationAnalysisEngine::DislocationAnalysisEngine(
		ParticleOrderingFingerprint fingerprint,
		ConstPropertyPtr positions, const SimulationCell& simCell,
		int inputCrystalStructure, int maxTrialCircuitSize, int maxCircuitElongation,
		ConstPropertyPtr particleSelection,
		ConstPropertyPtr crystalClusters,
		std::vector<Matrix3> preferredCrystalOrientations,
		bool onlyPerfectDislocations, int defectMeshSmoothingLevel,
		int lineSmoothingLevel, FloatType linePointInterval,
		bool doOutputInterfaceMesh) :
	StructureIdentificationModifier::StructureIdentificationEngine(std::move(fingerprint), positions, simCell, {}, std::move(particleSelection)),
	_simCellVolume(simCell.volume3D()),
	_structureAnalysis(std::make_unique<StructureAnalysis>(positions, simCell, (StructureAnalysis::LatticeStructureType)inputCrystalStructure, selection(), structures(), std::move(preferredCrystalOrientations), !onlyPerfectDislocations)),
	_tessellation(std::make_unique<DelaunayTessellation>()),
	_elasticMapping(std::make_unique<ElasticMapping>(*_structureAnalysis, *_tessellation)),
	_interfaceMesh(std::make_unique<InterfaceMesh>(*_elasticMapping)),
	_dislocationTracer(std::make_unique<DislocationTracer>(*_interfaceMesh, _structureAnalysis->clusterGraph(), maxTrialCircuitSize, maxCircuitElongation)),
	_inputCrystalStructure(inputCrystalStructure),
	_crystalClusters(crystalClusters),
	_onlyPerfectDislocations(onlyPerfectDislocations),
	_defectMeshSmoothingLevel(defectMeshSmoothingLevel),
	_lineSmoothingLevel(lineSmoothingLevel),
	_linePointInterval(linePointInterval),
	_doOutputInterfaceMesh(doOutputInterfaceMesh)
{
	setAtomClusters(_structureAnalysis->atomClusters());
	setDislocationNetwork(_dislocationTracer->network());
	setClusterGraph(_dislocationTracer->clusterGraph());
}

/******************************************************************************
* Performs the actual analysis. This method is executed in a worker thread.
******************************************************************************/
void DislocationAnalysisEngine::perform()
{
	task()->setProgressText(DislocationAnalysisModifier::tr("Dislocation analysis (DXA)"));

	task()->beginProgressSubStepsWithWeights({ 35, 6, 1, 220, 60, 1, 53, 190, 146, 20, 4, 4 });
	if(!_structureAnalysis->identifyStructures(*task()))
		return;

	task()->nextProgressSubStep();
	if(!_structureAnalysis->buildClusters(*task()))
		return;

	task()->nextProgressSubStep();
	if(!_structureAnalysis->connectClusters(*task()))
		return;

#if 0
	Point3 corners[8];
	corners[0] = _structureAnalysis.cell().reducedToAbsolute(Point3(0,0,0));
	corners[1] = _structureAnalysis.cell().reducedToAbsolute(Point3(1,0,0));
	corners[2] = _structureAnalysis.cell().reducedToAbsolute(Point3(1,1,0));
	corners[3] = _structureAnalysis.cell().reducedToAbsolute(Point3(0,1,0));
	corners[4] = _structureAnalysis.cell().reducedToAbsolute(Point3(0,0,1));
	corners[5] = _structureAnalysis.cell().reducedToAbsolute(Point3(1,0,1));
	corners[6] = _structureAnalysis.cell().reducedToAbsolute(Point3(1,1,1));
	corners[7] = _structureAnalysis.cell().reducedToAbsolute(Point3(0,1,1));

	std::ofstream stream("cell.vtk");
	stream << "# vtk DataFile Version 3.0" << std::endl;
	stream << "# Simulation cell" << std::endl;
	stream << "ASCII" << std::endl;
	stream << "DATASET UNSTRUCTURED_GRID" << std::endl;
	stream << "POINTS 8 double" << std::endl;
	for(int i = 0; i < 8; i++)
		stream << corners[i].x() << " " << corners[i].y() << " " << corners[i].z() << std::endl;

	stream << std::endl << "CELLS 1 9" << std::endl;
	stream << "8 0 1 2 3 4 5 6 7" << std::endl;

	stream << std::endl << "CELL_TYPES 1" << std::endl;
	stream << "12" << std::endl;  // Hexahedron
#endif

	task()->nextProgressSubStep();
	FloatType ghostLayerSize = FloatType(3.0) * _structureAnalysis->maximumNeighborDistance();
	if(!_tessellation->generateTessellation(_structureAnalysis->cell(), 
			ConstPropertyAccess<Point3>(positions()).cbegin(),
			_structureAnalysis->atomCount(), 
			ghostLayerSize, 
			selection() ? ConstPropertyAccess<int>(selection()).cbegin() : nullptr, 
			*task()))
		return;

	// Build list of edges in the tessellation.
	task()->nextProgressSubStep();
	if(!_elasticMapping->generateTessellationEdges(*task()))
		return;

	// Assign each vertex to a cluster.
	task()->nextProgressSubStep();
	if(!_elasticMapping->assignVerticesToClusters(*task()))
		return;

	// Determine the ideal vector corresponding to each edge of the tessellation.
	task()->nextProgressSubStep();
	if(!_elasticMapping->assignIdealVectorsToEdges(4, *task()))
		return;

	// Free some memory that is no longer needed.
	_structureAnalysis->freeNeighborLists();

	// Create the mesh facets.
	task()->nextProgressSubStep();
	if(!_interfaceMesh->createMesh(_structureAnalysis->maximumNeighborDistance(), crystalClusters(), *task()))
		return;

	// Trace dislocation lines.
	task()->nextProgressSubStep();
	if(!_dislocationTracer->traceDislocationSegments(*task()))
		return;
	_dislocationTracer->finishDislocationSegments(_inputCrystalStructure);

#if 0

	auto isWrappedFacet = [this](const InterfaceMesh::Face* f) -> bool {
		InterfaceMesh::edge_index e = f->edges();
		do {
			Vector3 v = e->vertex1()->pos() - e->vertex2()->pos();
			if(_structureAnalysis.cell().isWrappedVector(v))
				return true;
			e = e->nextFaceEdge();
		}
		while(e != f->edges());
		return false;
	};

	// Count facets which are not crossing the periodic boundaries.
	size_t numFacets = 0;
	for(const InterfaceMesh::Face* f : _interfaceMesh.faces()) {
		if(isWrappedFacet(f) == false)
			numFacets++;
	}

	std::ofstream stream("mesh.vtk");
	stream << "# vtk DataFile Version 3.0\n";
	stream << "# Interface mesh\n";
	stream << "ASCII\n";
	stream << "DATASET UNSTRUCTURED_GRID\n";
	stream << "POINTS " << _interfaceMesh.vertices().size() << " float\n";
	for(const InterfaceMesh::Vertex* n : _interfaceMesh.vertices()) {
		const Point3& pos = n->pos();
		stream << pos.x() << " " << pos.y() << " " << pos.z() << "\n";
	}
	stream << "\nCELLS " << numFacets << " " << (numFacets*4) << "\n";
	for(const InterfaceMesh::Face* f : _interfaceMesh.faces()) {
		if(isWrappedFacet(f) == false) {
			stream << f->edgeCount();
			InterfaceMesh::edge_index e = f->edges();
			do {
				stream << " " << e->vertex1()->index();
				e = e->nextFaceEdge();
			}
			while(e != f->edges());
			stream << "\n";
		}
	}

	stream << "\nCELL_TYPES " << numFacets << "\n";
	for(size_t i = 0; i < numFacets; i++)
		stream << "5\n";	// Triangle

	stream << "\nCELL_DATA " << numFacets << "\n";

	stream << "\nSCALARS dislocation_segment int 1\n";
	stream << "\nLOOKUP_TABLE default\n";
	for(const InterfaceMesh::Face* f : _interfaceMesh.faces()) {
		if(isWrappedFacet(f) == false) {
			if(f->circuit != NULL && (f->circuit->isDangling == false || f->testFlag(1))) {
				DislocationSegment* segment = f->circuit->dislocationNode->segment;
				while(segment->replacedWith != NULL) segment = segment->replacedWith;
				stream << segment->id << "\n";
			}
			else
				stream << "-1\n";
		}
	}

	stream << "\nSCALARS is_primary_segment int 1\n";
	stream << "\nLOOKUP_TABLE default\n";
	for(const InterfaceMesh::Face* f : _interfaceMesh.faces()) {
		if(isWrappedFacet(f) == false)
			stream << f->testFlag(1) << "\n";
	}

	stream.close();
#endif

	// Generate the defect mesh.
	task()->nextProgressSubStep();
	if(!_interfaceMesh->generateDefectMesh(*_dislocationTracer, _defectMesh, *task()))
		return;

#if 0
	_tessellation.dumpToVTKFile("tessellation.vtk");
#endif

	task()->nextProgressSubStep();

	// Post-process surface mesh.
	if(_defectMeshSmoothingLevel > 0 && !_defectMesh.smoothMesh(_defectMeshSmoothingLevel, *task()))
		return;

	task()->nextProgressSubStep();

	// Post-process dislocation lines.
	if(_lineSmoothingLevel > 0 || _linePointInterval > 0) {
		if(!dislocationNetwork()->smoothDislocationLines(_lineSmoothingLevel, _linePointInterval, *task()))
			return;
	}

	task()->endProgressSubSteps();

	// Return the results of the compute engine.
	if(_doOutputInterfaceMesh) {
		_outputInterfaceMesh = interfaceMesh().topology();
		_outputInterfaceMeshVerts = interfaceMesh().vertexProperty(SurfaceMeshVertices::PositionProperty);
	}
}

/******************************************************************************
* Injects the computed results of the engine into the data pipeline.
******************************************************************************/
void DislocationAnalysisEngine::emitResults(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state)
{
	DislocationAnalysisModifier* modifier = static_object_cast<DislocationAnalysisModifier>(modApp->modifier());
	StructureIdentificationEngine::emitResults(time, modApp, state);

	// Output defect mesh.
	SurfaceMesh* defectMeshObj = state.createObject<SurfaceMesh>(QStringLiteral("dxa-defect-mesh"), modApp, DislocationAnalysisModifier::tr("Defect mesh"));
	defectMesh().transferTo(defectMeshObj);
	defectMeshObj->setDomain(state.getObject<SimulationCellObject>());
	defectMeshObj->setVisElement(modifier->defectMeshVis());

	// Output interface mesh.
	if(outputInterfaceMesh()) {
		SurfaceMesh* interfaceMeshObj = state.createObject<SurfaceMesh>(QStringLiteral("dxa-interface-mesh"), modApp, DislocationAnalysisModifier::tr("Interface mesh"));
		interfaceMeshObj->setTopology(outputInterfaceMesh());
		interfaceMeshObj->vertices()->createProperty(_outputInterfaceMeshVerts);
		interfaceMeshObj->setSpaceFillingRegion(defectMesh().spaceFillingRegion());
		interfaceMeshObj->setDomain(state.getObject<SimulationCellObject>());
		interfaceMeshObj->setVisElement(modifier->interfaceMeshVis());
	}

	// Output cluster graph.
	if(const ClusterGraphObject* oldClusterGraph = state.getObject<ClusterGraphObject>())
		state.removeObject(oldClusterGraph);
	ClusterGraphObject* clusterGraphObj = state.createObject<ClusterGraphObject>(modApp);
	clusterGraphObj->setStorage(clusterGraph());

	// Output dislocations.
	DislocationNetworkObject* dislocationsObj = state.createObject<DislocationNetworkObject>(modApp);
	dislocationsObj->setStorage(dislocationNetwork());
	while(!dislocationsObj->crystalStructures().empty())
		dislocationsObj->removeCrystalStructure(dislocationsObj->crystalStructures().size()-1);
	for(ElementType* stype : modifier->structureTypes())
		dislocationsObj->addCrystalStructure(static_object_cast<MicrostructurePhase>(stype));
	dislocationsObj->setDomain(state.getObject<SimulationCellObject>());
	dislocationsObj->setVisElement(modifier->dislocationVis());

	std::map<BurgersVectorFamily*,FloatType> dislocationLengths;
	std::map<BurgersVectorFamily*,int> segmentCounts;
	std::map<BurgersVectorFamily*,MicrostructurePhase*> dislocationCrystalStructures;
	MicrostructurePhase* defaultStructure = dislocationsObj->structureById(modifier->inputCrystalStructure());
	if(defaultStructure) {
		for(BurgersVectorFamily* family : defaultStructure->burgersVectorFamilies()) {
			dislocationLengths[family] = 0;
			segmentCounts[family] = 0;
			dislocationCrystalStructures[family] = defaultStructure;
		}
	}

	// Classify, count and measure length of dislocation segments.
	FloatType totalLineLength = 0;
	int totalSegmentCount = 0;
	for(const DislocationSegment* segment : dislocationsObj->storage()->segments()) {
		FloatType len = segment->calculateLength();
		totalLineLength += len;
		totalSegmentCount++;

		Cluster* cluster = segment->burgersVector.cluster();
		OVITO_ASSERT(cluster != nullptr);
		MicrostructurePhase* structure = dislocationsObj->structureById(cluster->structure);
		if(structure == nullptr) continue;
		BurgersVectorFamily* family = structure->defaultBurgersVectorFamily();
		for(BurgersVectorFamily* f : structure->burgersVectorFamilies()) {
			if(f->isMember(segment->burgersVector.localVec(), structure)) {
				family = f;
				break;
			}
		}
		segmentCounts[family]++;
		dislocationLengths[family] += len;
		dislocationCrystalStructures[family] = structure;
	}

	// Output a data table with the dislocation line lengths.
	int maxId = 0;
	for(const auto& entry : dislocationLengths)
		maxId = std::max(maxId, entry.first->numericId());
	PropertyAccessAndRef<FloatType> dislocationLengthsProperty = std::make_shared<PropertyStorage>(maxId+1, PropertyStorage::Float, 1, 0, DislocationAnalysisModifier::tr("Total line length"), true, DataTable::YProperty);
	for(const auto& entry : dislocationLengths)
		dislocationLengthsProperty[entry.first->numericId()] = entry.second;
	PropertyAccessAndRef<int> dislocationTypeIds = std::make_shared<PropertyStorage>(maxId+1, PropertyStorage::Int, 1, 0, DislocationAnalysisModifier::tr("Dislocation type"), false, DataTable::XProperty);
	boost::algorithm::iota_n(dislocationTypeIds.begin(), 0, dislocationTypeIds.size());
	DataTable* lengthTableObj = state.createObject<DataTable>(QStringLiteral("disloc-lengths"), modApp, DataTable::BarChart, DislocationAnalysisModifier::tr("Dislocation lengths"), dislocationLengthsProperty.takeStorage(), dislocationTypeIds.takeStorage());
	PropertyObject* xProperty = lengthTableObj->expectMutableProperty(DataTable::XProperty);
	for(const auto& entry : dislocationLengths)
		xProperty->addElementType(entry.first);

	// Output a data table with the dislocation segment counts.
	PropertyAccessAndRef<int> dislocationCountsProperty = std::make_shared<PropertyStorage>(maxId+1, PropertyStorage::Int, 1, 0, DislocationAnalysisModifier::tr("Dislocation count"), true, DataTable::YProperty);
	for(const auto& entry : segmentCounts)
		dislocationCountsProperty[entry.first->numericId()] = entry.second;
	DataTable* countTableObj = state.createObject<DataTable>(QStringLiteral("disloc-counts"), modApp, DataTable::BarChart, DislocationAnalysisModifier::tr("Dislocation counts"), dislocationCountsProperty.takeStorage());
	countTableObj->insertProperty(0, xProperty);

	// Output particle properties.
	if(atomClusters()) {
		ParticlesObject* particles = state.expectMutableObject<ParticlesObject>();
		particles->createProperty(atomClusters());
	}

	state.addAttribute(QStringLiteral("DislocationAnalysis.total_line_length"), QVariant::fromValue(totalLineLength), modApp);
	state.addAttribute(QStringLiteral("DislocationAnalysis.counts.OTHER"), QVariant::fromValue(getTypeCount(StructureAnalysis::LATTICE_OTHER)), modApp);
	state.addAttribute(QStringLiteral("DislocationAnalysis.counts.FCC"), QVariant::fromValue(getTypeCount(StructureAnalysis::LATTICE_FCC)), modApp);
	state.addAttribute(QStringLiteral("DislocationAnalysis.counts.HCP"), QVariant::fromValue(getTypeCount(StructureAnalysis::LATTICE_HCP)), modApp);
	state.addAttribute(QStringLiteral("DislocationAnalysis.counts.BCC"), QVariant::fromValue(getTypeCount(StructureAnalysis::LATTICE_BCC)), modApp);
	state.addAttribute(QStringLiteral("DislocationAnalysis.counts.CubicDiamond"), QVariant::fromValue(getTypeCount(StructureAnalysis::LATTICE_CUBIC_DIAMOND)), modApp);
	state.addAttribute(QStringLiteral("DislocationAnalysis.counts.HexagonalDiamond"), QVariant::fromValue(getTypeCount(StructureAnalysis::LATTICE_HEX_DIAMOND)), modApp);

	for(const auto& dlen : dislocationLengths) {
		MicrostructurePhase* structure = dislocationCrystalStructures[dlen.first];
		QString bstr;
		if(dlen.first->burgersVector() != Vector3::Zero()) {
			bstr = DislocationVis::formatBurgersVector(dlen.first->burgersVector(), structure);
			bstr.remove(QChar(' '));
			bstr.replace(QChar('['), QChar('<'));
			bstr.replace(QChar(']'), QChar('>'));
		}
		else bstr = "other";
		state.addAttribute(QStringLiteral("DislocationAnalysis.length.%1").arg(bstr), QVariant::fromValue(dlen.second), modApp);
	}
	state.addAttribute(QStringLiteral("DislocationAnalysis.cell_volume"), QVariant::fromValue(simCellVolume()), modApp);

	if(totalSegmentCount == 0)
		state.setStatus(PipelineStatus(PipelineStatus::Success, DislocationAnalysisModifier::tr("No dislocations found")));
	else
		state.setStatus(PipelineStatus(PipelineStatus::Success, DislocationAnalysisModifier::tr("Found %1 dislocation segments\nTotal line length: %2").arg(totalSegmentCount).arg(totalLineLength)));
}

}	// End of namespace
}	// End of namespace
