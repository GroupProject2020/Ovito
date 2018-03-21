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
#include <plugins/crystalanalysis/objects/clusters/ClusterGraphObject.h>
#include <plugins/crystalanalysis/objects/patterns/StructurePattern.h>
#include <plugins/particles/objects/BondProperty.h>
#include <plugins/particles/modifier/ParticleInputHelper.h>
#include <plugins/particles/modifier/ParticleOutputHelper.h>
#include <plugins/mesh/tri/TriMeshObject.h>
#include <plugins/mesh/tri/TriMeshVis.h>
#include <plugins/mesh/surface/SurfaceMesh.h>
#include <plugins/stdobj/simcell/SimulationCellObject.h>
#include <core/dataset/DataSetContainer.h>
#include "DislocationAnalysisModifier.h"
#include "DislocationAnalysisEngine.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

IMPLEMENT_OVITO_CLASS(DislocationAnalysisModifier);
DEFINE_PROPERTY_FIELD(DislocationAnalysisModifier, inputCrystalStructure);
DEFINE_PROPERTY_FIELD(DislocationAnalysisModifier, maxTrialCircuitSize);
DEFINE_PROPERTY_FIELD(DislocationAnalysisModifier, circuitStretchability);
DEFINE_PROPERTY_FIELD(DislocationAnalysisModifier, outputInterfaceMesh);
DEFINE_PROPERTY_FIELD(DislocationAnalysisModifier, onlyPerfectDislocations);
DEFINE_REFERENCE_FIELD(DislocationAnalysisModifier, patternCatalog);
DEFINE_PROPERTY_FIELD(DislocationAnalysisModifier, defectMeshSmoothingLevel);
DEFINE_PROPERTY_FIELD(DislocationAnalysisModifier, lineSmoothingEnabled);
DEFINE_PROPERTY_FIELD(DislocationAnalysisModifier, lineSmoothingLevel);
DEFINE_PROPERTY_FIELD(DislocationAnalysisModifier, lineCoarseningEnabled);
DEFINE_PROPERTY_FIELD(DislocationAnalysisModifier, linePointInterval);
DEFINE_REFERENCE_FIELD(DislocationAnalysisModifier, dislocationVis);
DEFINE_REFERENCE_FIELD(DislocationAnalysisModifier, defectMeshVis);
DEFINE_REFERENCE_FIELD(DislocationAnalysisModifier, interfaceMeshVis);
SET_PROPERTY_FIELD_LABEL(DislocationAnalysisModifier, inputCrystalStructure, "Input crystal structure");
SET_PROPERTY_FIELD_LABEL(DislocationAnalysisModifier, maxTrialCircuitSize, "Trial circuit length");
SET_PROPERTY_FIELD_LABEL(DislocationAnalysisModifier, circuitStretchability, "Circuit stretchability");
SET_PROPERTY_FIELD_LABEL(DislocationAnalysisModifier, outputInterfaceMesh, "Output interface mesh");
SET_PROPERTY_FIELD_LABEL(DislocationAnalysisModifier, onlyPerfectDislocations, "Generate perfect dislocations");
SET_PROPERTY_FIELD_LABEL(DislocationAnalysisModifier, defectMeshSmoothingLevel, "Surface smoothing level");
SET_PROPERTY_FIELD_LABEL(DislocationAnalysisModifier, lineSmoothingEnabled, "Line smoothing");
SET_PROPERTY_FIELD_LABEL(DislocationAnalysisModifier, lineSmoothingLevel, "Smoothing level");
SET_PROPERTY_FIELD_LABEL(DislocationAnalysisModifier, lineCoarseningEnabled, "Line coarsening");
SET_PROPERTY_FIELD_LABEL(DislocationAnalysisModifier, linePointInterval, "Point separation");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(DislocationAnalysisModifier, maxTrialCircuitSize, IntegerParameterUnit, 3);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(DislocationAnalysisModifier, circuitStretchability, IntegerParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(DislocationAnalysisModifier, defectMeshSmoothingLevel, IntegerParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(DislocationAnalysisModifier, lineSmoothingLevel, IntegerParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(DislocationAnalysisModifier, linePointInterval, FloatParameterUnit, 0);

IMPLEMENT_OVITO_CLASS(DislocationAnalysisModifierApplication);
SET_MODIFIER_APPLICATION_TYPE(DislocationAnalysisModifier, DislocationAnalysisModifierApplication);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
DislocationAnalysisModifier::DislocationAnalysisModifier(DataSet* dataset) : StructureIdentificationModifier(dataset),
	_inputCrystalStructure(StructureAnalysis::LATTICE_FCC),
	_maxTrialCircuitSize(14),
	_circuitStretchability(9),
	_outputInterfaceMesh(false),
	_onlyPerfectDislocations(false),
	_defectMeshSmoothingLevel(8),
	_lineSmoothingEnabled(true), 
	_lineCoarseningEnabled(true),
	_lineSmoothingLevel(1),
	_linePointInterval(2.5)
{
	// Create the vis elements.
	setDislocationVis(new DislocationVis(dataset));

	setDefectMeshVis(new SurfaceMeshVis(dataset));
	defectMeshVis()->setShowCap(true);
	defectMeshVis()->setSmoothShading(true);
	defectMeshVis()->setCapTransparency(0.5);
	defectMeshVis()->setObjectTitle(tr("Defect mesh"));

	setInterfaceMeshVis(new SurfaceMeshVis(dataset));
	interfaceMeshVis()->setShowCap(false);
	interfaceMeshVis()->setSmoothShading(false);
	interfaceMeshVis()->setCapTransparency(0.5);
	interfaceMeshVis()->setObjectTitle(tr("Interface mesh"));		
	
	// Create pattern catalog.
	setPatternCatalog(new PatternCatalog(dataset));
	while(patternCatalog()->patterns().empty() == false)
		patternCatalog()->removePattern(0);

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
		OORef<StructurePattern> stype = patternCatalog()->structureById(id);
		if(!stype) {
			stype = new StructurePattern(dataset);
			stype->setId(id);
			stype->setStructureType(StructurePattern::Lattice);
			patternCatalog()->addPattern(stype);
		}
		stype->setName(ParticleType::getPredefinedStructureTypeName(predefTypes[id]));
		stype->setColor(ParticleType::getDefaultParticleColor(ParticleProperty::StructureTypeProperty, stype->name(), id));
		addStructureType(stype);
	}

	// Create Burgers vector families.

	StructurePattern* fccPattern = patternCatalog()->structureById(StructureAnalysis::LATTICE_FCC);
	fccPattern->setSymmetryType(StructurePattern::CubicSymmetry);
	fccPattern->setShortName(QStringLiteral("fcc"));
	fccPattern->addBurgersVectorFamily(new BurgersVectorFamily(dataset, tr("1/2<110> (Perfect)"), Vector3(1.0f/2.0f, 1.0f/2.0f, 0.0f), Color(0.2f,0.2f,1)));
	fccPattern->addBurgersVectorFamily(new BurgersVectorFamily(dataset, tr("1/6<112> (Shockley)"), Vector3(1.0f/6.0f, 1.0f/6.0f, 2.0f/6.0f), Color(0,1,0)));
	fccPattern->addBurgersVectorFamily(new BurgersVectorFamily(dataset, tr("1/6<110> (Stair-rod)"), Vector3(1.0f/6.0f, 1.0f/6.0f, 0.0f/6.0f), Color(1,0,1)));
	fccPattern->addBurgersVectorFamily(new BurgersVectorFamily(dataset, tr("1/3<001> (Hirth)"), Vector3(1.0f/3.0f, 0.0f, 0.0f), Color(1,1,0)));
	fccPattern->addBurgersVectorFamily(new BurgersVectorFamily(dataset, tr("1/3<111> (Frank)"), Vector3(1.0f/3.0f, 1.0f/3.0f, 1.0f/3.0f), Color(0,1,1)));

	StructurePattern* bccPattern = patternCatalog()->structureById(StructureAnalysis::LATTICE_BCC);
	bccPattern->setSymmetryType(StructurePattern::CubicSymmetry);
	bccPattern->setShortName(QStringLiteral("bcc"));
	bccPattern->addBurgersVectorFamily(new BurgersVectorFamily(dataset, tr("1/2<111>"), Vector3(1.0f/2.0f, 1.0f/2.0f, 1.0f/2.0f), Color(0,1,0)));
	bccPattern->addBurgersVectorFamily(new BurgersVectorFamily(dataset, tr("<100>"), Vector3(1.0f, 0.0f, 0.0f), Color(1, 0.3f, 0.8f)));
	bccPattern->addBurgersVectorFamily(new BurgersVectorFamily(dataset, tr("<110>"), Vector3(1.0f, 1.0f, 0.0f), Color(0.2f, 0.5f, 1.0f)));

	StructurePattern* hcpPattern = patternCatalog()->structureById(StructureAnalysis::LATTICE_HCP);
	hcpPattern->setShortName(QStringLiteral("hcp"));
	hcpPattern->setSymmetryType(StructurePattern::HexagonalSymmetry);
	hcpPattern->addBurgersVectorFamily(new BurgersVectorFamily(dataset, tr("1/3<1-210>"), Vector3(sqrt(0.5f), 0.0f, 0.0f), Color(0,1,0)));
	hcpPattern->addBurgersVectorFamily(new BurgersVectorFamily(dataset, tr("<0001>"), Vector3(0.0f, 0.0f, sqrt(4.0f/3.0f)), Color(0.2f,0.2f,1)));
	hcpPattern->addBurgersVectorFamily(new BurgersVectorFamily(dataset, tr("<1-100>"), Vector3(0.0f, sqrt(3.0f/2.0f), 0.0f), Color(1,0,1)));
	hcpPattern->addBurgersVectorFamily(new BurgersVectorFamily(dataset, tr("1/3<1-100>"), Vector3(0.0f, sqrt(3.0f/2.0f)/3.0f, 0.0f), Color(1,0.5f,0)));
	hcpPattern->addBurgersVectorFamily(new BurgersVectorFamily(dataset, tr("1/3<1-213>"), Vector3(sqrt(0.5f), 0.0f, sqrt(4.0f/3.0f)), Color(1,1,0)));

	StructurePattern* cubicDiaPattern = patternCatalog()->structureById(StructureAnalysis::LATTICE_CUBIC_DIAMOND);
	cubicDiaPattern->setShortName(QStringLiteral("diamond"));
	cubicDiaPattern->setSymmetryType(StructurePattern::CubicSymmetry);
	cubicDiaPattern->addBurgersVectorFamily(new BurgersVectorFamily(dataset, tr("1/2<110>"), Vector3(1.0f/2.0f, 1.0f/2.0f, 0.0f), Color(0.2f,0.2f,1)));
	cubicDiaPattern->addBurgersVectorFamily(new BurgersVectorFamily(dataset, tr("1/6<112>"), Vector3(1.0f/6.0f, 1.0f/6.0f, 2.0f/6.0f), Color(0,1,0)));
	cubicDiaPattern->addBurgersVectorFamily(new BurgersVectorFamily(dataset, tr("1/6<110>"), Vector3(1.0f/6.0f, 1.0f/6.0f, 0.0f), Color(1,0,1)));
	cubicDiaPattern->addBurgersVectorFamily(new BurgersVectorFamily(dataset, tr("1/3<111>"), Vector3(1.0f/3.0f, 1.0f/3.0f, 1.0f/3.0f), Color(0,1,1)));

	StructurePattern* hexDiaPattern = patternCatalog()->structureById(StructureAnalysis::LATTICE_HEX_DIAMOND);
	hexDiaPattern->setShortName(QStringLiteral("hex_diamond"));
	hexDiaPattern->setSymmetryType(StructurePattern::HexagonalSymmetry);
	hexDiaPattern->addBurgersVectorFamily(new BurgersVectorFamily(dataset, tr("1/3<1-210>"), Vector3(sqrt(0.5f), 0.0f, 0.0f), Color(0,1,0)));
	hexDiaPattern->addBurgersVectorFamily(new BurgersVectorFamily(dataset, tr("<0001>"), Vector3(0.0f, 0.0f, sqrt(4.0f/3.0f)), Color(0.2f,0.2f,1)));
	hexDiaPattern->addBurgersVectorFamily(new BurgersVectorFamily(dataset, tr("<1-100>"), Vector3(0.0f, sqrt(3.0f/2.0f), 0.0f), Color(1,0,1)));
	hexDiaPattern->addBurgersVectorFamily(new BurgersVectorFamily(dataset, tr("1/3<1-100>"), Vector3(0.0f, sqrt(3.0f/2.0f)/3.0f, 0.0f), Color(1,0.5f,0)));
}

/******************************************************************************
* Creates and initializes a computation engine that will compute the modifier's results.
******************************************************************************/
Future<AsynchronousModifier::ComputeEnginePtr> DislocationAnalysisModifier::createEngine(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input)
{
	// Get modifier inputs.
	ParticleInputHelper pih(dataset(), input);
	ParticleProperty* posProperty = pih.expectStandardProperty<ParticleProperty>(ParticleProperty::PositionProperty);
	SimulationCellObject* simCell = pih.expectSimulationCell();
	if(simCell->is2D())
		throwException(tr("The DXA modifier does not support 2d simulation cells."));

	// Get particle selection.
	ConstPropertyPtr selectionProperty;
	if(onlySelectedParticles())
		selectionProperty = pih.expectStandardProperty<ParticleProperty>(ParticleProperty::SelectionProperty)->storage();

	// Build list of preferred crystal orientations.
	std::vector<Matrix3> preferredCrystalOrientations;
	if(inputCrystalStructure() == StructureAnalysis::LATTICE_FCC || inputCrystalStructure() == StructureAnalysis::LATTICE_BCC || inputCrystalStructure() == StructureAnalysis::LATTICE_CUBIC_DIAMOND) {
		preferredCrystalOrientations.push_back(Matrix3::Identity());
	}

	// Get cluster property.
	ParticleProperty* clusterProperty = pih.inputStandardProperty<ParticleProperty>(ParticleProperty::ClusterProperty);

	// Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
	return std::make_shared<DislocationAnalysisEngine>(posProperty->storage(),
			simCell->data(), inputCrystalStructure(), maxTrialCircuitSize(), circuitStretchability(),
			std::move(selectionProperty),
			clusterProperty ? clusterProperty->storage() : nullptr, std::move(preferredCrystalOrientations),
			onlyPerfectDislocations(), defectMeshSmoothingLevel(),
			lineSmoothingEnabled() ? lineSmoothingLevel() : 0, lineCoarseningEnabled() ? linePointInterval() : 0,
			outputInterfaceMesh());
}

/******************************************************************************
* Injects the computed results of the engine into the data pipeline.
******************************************************************************/
PipelineFlowState DislocationAnalysisResults::apply(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input)
{
	DislocationAnalysisModifierApplication* myModApp = static_object_cast<DislocationAnalysisModifierApplication>(modApp);
	DislocationAnalysisModifier* modifier = static_object_cast<DislocationAnalysisModifier>(modApp->modifier());
	
	PipelineFlowState output = StructureIdentificationResults::apply(time, modApp, input);
	ParticleOutputHelper poh(modApp->dataset(), output);

	// Output defect mesh.
	OORef<SurfaceMesh> defectMeshObj(new SurfaceMesh(modApp->dataset()));
	defectMeshObj->setStorage(defectMesh());
	defectMeshObj->setIsCompletelySolid(isBadEverywhere());	
	defectMeshObj->setDomain(input.findObject<SimulationCellObject>());
	defectMeshObj->setVisElement(modifier->defectMeshVis());
	output.addObject(defectMeshObj);

	// Output interface mesh.
	if(interfaceMesh()) {
		OORef<SurfaceMesh> interfaceMeshObj(new SurfaceMesh(modApp->dataset()));
		interfaceMeshObj->setStorage(interfaceMesh());
		interfaceMeshObj->setIsCompletelySolid(isBadEverywhere());
		interfaceMeshObj->setDomain(input.findObject<SimulationCellObject>());
		interfaceMeshObj->setVisElement(modifier->interfaceMeshVis());
		output.addObject(interfaceMeshObj);
	}

	// Output cluster graph.
	OORef<ClusterGraphObject> clusterGraphObj(new ClusterGraphObject(modApp->dataset()));
	clusterGraphObj->setStorage(clusterGraph());
	if(ClusterGraphObject* oldClusterGraph = output.findObject<ClusterGraphObject>())
		output.removeObject(oldClusterGraph);
	output.addObject(clusterGraphObj);

	// Output dislocations.
	OORef<DislocationNetworkObject> dislocationsObj(new DislocationNetworkObject(modApp->dataset()));
	dislocationsObj->setStorage(dislocationNetwork());
	dislocationsObj->setDomain(input.findObject<SimulationCellObject>());
	dislocationsObj->setVisElement(modifier->dislocationVis());
	output.addObject(dislocationsObj);

	std::map<BurgersVectorFamily*,FloatType> dislocationLengths;
	std::map<BurgersVectorFamily*,int> segmentCounts;
	std::map<BurgersVectorFamily*,StructurePattern*> dislocationStructurePatterns;
	StructurePattern* defaultPattern = modifier->patternCatalog()->structureById(modifier->inputCrystalStructure());
	if(defaultPattern) {
		for(BurgersVectorFamily* family : defaultPattern->burgersVectorFamilies()) {
			dislocationLengths[family] = 0;
			dislocationStructurePatterns[family] = defaultPattern;
		}
	}
	
	// Classify, count and measure length of dislocation segments.
	FloatType totalLineLength = 0;
	int totalSegmentCount = 0;
	for(DislocationSegment* segment : dislocationsObj->storage()->segments()) {
		FloatType len = segment->calculateLength();
		totalLineLength += len;
		totalSegmentCount++;

		Cluster* cluster = segment->burgersVector.cluster();
		OVITO_ASSERT(cluster != nullptr);
		StructurePattern* pattern = modifier->patternCatalog()->structureById(cluster->structure);
		if(pattern == nullptr) continue;
		BurgersVectorFamily* family = pattern->defaultBurgersVectorFamily();
		for(BurgersVectorFamily* f : pattern->burgersVectorFamilies()) {
			if(f->isMember(segment->burgersVector.localVec(), pattern)) {
				family = f;
				break;
			}
		}
		segmentCounts[family]++;
		dislocationLengths[family] += len;
		dislocationStructurePatterns[family] = pattern;
	}

	// Output pattern catalog.
	if(modifier->patternCatalog()) {
		if(PatternCatalog* oldCatalog = output.findObject<PatternCatalog>())
			output.removeObject(oldCatalog);
		output.addObject(modifier->patternCatalog());
	}

	// Output particle properties.
	if(atomClusters())
		poh.outputProperty<ParticleProperty>(atomClusters());

	output.attributes().insert(QStringLiteral("DislocationAnalysis.total_line_length"), QVariant::fromValue(totalLineLength));
	output.attributes().insert(QStringLiteral("DislocationAnalysis.counts.OTHER"), QVariant::fromValue(myModApp->structureCounts()[StructureAnalysis::LATTICE_OTHER]));
	output.attributes().insert(QStringLiteral("DislocationAnalysis.counts.FCC"), QVariant::fromValue(myModApp->structureCounts()[StructureAnalysis::LATTICE_FCC]));
	output.attributes().insert(QStringLiteral("DislocationAnalysis.counts.HCP"), QVariant::fromValue(myModApp->structureCounts()[StructureAnalysis::LATTICE_HCP]));
	output.attributes().insert(QStringLiteral("DislocationAnalysis.counts.BCC"), QVariant::fromValue(myModApp->structureCounts()[StructureAnalysis::LATTICE_BCC]));
	output.attributes().insert(QStringLiteral("DislocationAnalysis.counts.CubicDiamond"), QVariant::fromValue(myModApp->structureCounts()[StructureAnalysis::LATTICE_CUBIC_DIAMOND]));
	output.attributes().insert(QStringLiteral("DislocationAnalysis.counts.HexagonalDiamond"), QVariant::fromValue(myModApp->structureCounts()[StructureAnalysis::LATTICE_HEX_DIAMOND]));

	for(const auto& dlen : dislocationLengths) {
		StructurePattern* pattern = dislocationStructurePatterns[dlen.first];
		QString bstr;
		if(dlen.first->burgersVector() != Vector3::Zero()) {
			bstr = DislocationVis::formatBurgersVector(dlen.first->burgersVector(), pattern);
			bstr.remove(QChar(' '));
			bstr.replace(QChar('['), QChar('<'));
			bstr.replace(QChar(']'), QChar('>'));
		}
		else bstr = "other";
		output.attributes().insert(QStringLiteral("DislocationAnalysis.length.%1").arg(bstr), QVariant::fromValue(dlen.second));
	}
	output.attributes().insert(QStringLiteral("DislocationAnalysis.cell_volume"), QVariant::fromValue(simCellVolume()));

	// Store the summary results in the ModifierApplication.
	myModApp->setResults(std::move(segmentCounts), std::move(dislocationLengths));
	
	if(totalSegmentCount == 0)
		output.setStatus(PipelineStatus(PipelineStatus::Success, DislocationAnalysisModifier::tr("No dislocations found")));
	else
		output.setStatus(PipelineStatus(PipelineStatus::Success, DislocationAnalysisModifier::tr("Found %1 dislocation segments\nTotal line length: %2").arg(totalSegmentCount).arg(totalLineLength)));
	
	return output;
}

}	// End of namespace
}	// End of namespace
}	// End of namespace
