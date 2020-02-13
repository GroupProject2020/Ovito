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

#include <ovito/particles/Particles.h>
#include <ovito/particles/util/NearestNeighborFinder.h>
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/stdobj/properties/PropertyAccess.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>
#include <ovito/core/dataset/data/AttributeDataObject.h>
#include <ovito/core/utilities/concurrent/ParallelFor.h>
#include "WignerSeitzAnalysisModifier.h"

namespace Ovito { namespace Particles {

IMPLEMENT_OVITO_CLASS(WignerSeitzAnalysisModifier);
DEFINE_PROPERTY_FIELD(WignerSeitzAnalysisModifier, perTypeOccupancy);
DEFINE_PROPERTY_FIELD(WignerSeitzAnalysisModifier, outputCurrentConfig);
SET_PROPERTY_FIELD_LABEL(WignerSeitzAnalysisModifier, perTypeOccupancy, "Compute per-type occupancies");
SET_PROPERTY_FIELD_LABEL(WignerSeitzAnalysisModifier, outputCurrentConfig, "Output current configuration");

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
WignerSeitzAnalysisModifier::WignerSeitzAnalysisModifier(DataSet* dataset) : ReferenceConfigurationModifier(dataset),
	_perTypeOccupancy(false),
	_outputCurrentConfig(false)
{
}

/******************************************************************************
* Creates and initializes a computation engine that will compute the modifier's results.
******************************************************************************/
Future<AsynchronousModifier::ComputeEnginePtr> WignerSeitzAnalysisModifier::createEngineInternal(const PipelineEvaluationRequest& request, ModifierApplication* modApp, PipelineFlowState input, const PipelineFlowState& referenceState, TimeInterval validityInterval)
{
	// Get the current particle positions.
	const ParticlesObject* particles = input.expectObject<ParticlesObject>();
	particles->verifyIntegrity();
	const PropertyObject* posProperty = particles->expectProperty(ParticlesObject::PositionProperty);

	// Get the reference particle position.
	const ParticlesObject* refParticles = referenceState.getObject<ParticlesObject>();
	if(!refParticles)
		throwException(tr("Reference configuration does not contain any particles."));
	refParticles->verifyIntegrity();
	const PropertyObject* refPosProperty = refParticles->expectProperty(ParticlesObject::PositionProperty);

	// Get simulation cells.
	const SimulationCellObject* inputCell = input.expectObject<SimulationCellObject>();
	const SimulationCellObject* refCell = referenceState.getObject<SimulationCellObject>();
	if(!refCell)
		throwException(tr("Reference configuration has no simulation cell."));

	// Validate simulation cells.
	if(inputCell->is2D())
		throwException(tr("Wigner-Seitz analysis is not supported for 2d systems."));
	if(inputCell->volume3D() < FLOATTYPE_EPSILON)
		throwException(tr("Simulation cell is degenerate in the current configuration."));
	if(refCell->volume3D() < FLOATTYPE_EPSILON)
		throwException(tr("Simulation cell is degenerate in the reference configuration."));

	// Get the particle types of the current configuration.
	ConstPropertyPtr typeProperty;
	int ptypeMinId = std::numeric_limits<int>::max();
	int ptypeMaxId = std::numeric_limits<int>::lowest();
	if(perTypeOccupancy()) {
		const PropertyObject* ptypeProp = particles->expectProperty(ParticlesObject::TypeProperty);
		// Determine value range of particle type IDs.
		for(const ElementType* pt : ptypeProp->elementTypes()) {
			if(pt->numericId() < ptypeMinId) ptypeMinId = pt->numericId();
			if(pt->numericId() > ptypeMaxId) ptypeMaxId = pt->numericId();
		}
		typeProperty = ptypeProp->storage();
	}

	// If output of the displaced configuration is requested, obtain types of the reference sites.
	ConstPropertyPtr referenceTypeProperty;
	ConstPropertyPtr referenceIdentifierProperty;
	if(outputCurrentConfig()) {
		referenceTypeProperty = refParticles->getPropertyStorage(ParticlesObject::TypeProperty);
		referenceIdentifierProperty = refParticles->getPropertyStorage(ParticlesObject::IdentifierProperty);
	}

	// Create compute engine instance. Pass all relevant modifier parameters and the input data to the engine.
	auto engine = std::make_shared<WignerSeitzAnalysisEngine>(validityInterval, posProperty->storage(), inputCell->data(),
			referenceState,
			refPosProperty->storage(), refCell->data(), affineMapping(), std::move(typeProperty), ptypeMinId, ptypeMaxId,
			std::move(referenceTypeProperty), std::move(referenceIdentifierProperty));

	// Create output properties:
	if(outputCurrentConfig()) {
		if(referenceIdentifierProperty)
			engine->setSiteIdentifiers(std::make_shared<PropertyStorage>(posProperty->size(), PropertyStorage::Int64, 1, 0, tr("Site Identifier"), false));
		engine->setSiteTypes(std::make_shared<PropertyStorage>(posProperty->size(), PropertyStorage::Int, 1, 0, tr("Site Type"), false));
		engine->setSiteIndices(std::make_shared<PropertyStorage>(posProperty->size(), PropertyStorage::Int64, 1, 0, tr("Site Index"), false));
	}

	return engine;
}

/******************************************************************************
* Performs the actual computation. This method is executed in a worker thread.
******************************************************************************/
void WignerSeitzAnalysisModifier::WignerSeitzAnalysisEngine::perform()
{
	task()->setProgressText(tr("Performing Wigner-Seitz cell analysis"));

	if(affineMapping() == TO_CURRENT_CELL)
		throw Exception(tr("Remapping coordinates to the current cell is not supported by the Wigner-Seitz analysis routine. Only remapping to the reference cell or no mapping at all are supported options."));

	size_t particleCount = positions()->size();
	if(refPositions()->size() == 0)
		throw Exception(tr("Reference configuration for Wigner-Seitz analysis contains no atomic sites."));

	// Prepare the closest-point query structure.
	NearestNeighborFinder neighborTree(0);
	if(!neighborTree.prepare(refPositions(), refCell(), {}, task().get()))
		return;

	// Determine the number of components of the occupancy property.
	int ncomponents = 1;
	int typemin, typemax;
	if(particleTypes()) {
		ConstPropertyAccess<int> particleTypesArray(particleTypes());
		auto minmax = std::minmax_element(particleTypesArray.cbegin(), particleTypesArray.cend());
		typemin = std::min(_ptypeMinId, *minmax.first);
		typemax = std::max(_ptypeMaxId, *minmax.second);
		if(typemin < 0)
			throw Exception(tr("Negative particle type IDs are not supported by this modifier."));
		if(typemax > 32)
			throw Exception(tr("Number of particle types is too large for this modifier. Cannot compute occupancy numbers for more than 32 particle types."));
		ncomponents = typemax - typemin + 1;
	}

	AffineTransformation tm;
	if(affineMapping() == TO_REFERENCE_CELL)
		tm = refCell().matrix() * cell().inverseMatrix();

	// Create array for atomic counting.
	size_t arraySize = refPositions()->size() * ncomponents;
	std::vector<std::atomic_int> occupancyArray(arraySize);
	for(auto& o : occupancyArray)
		o.store(0, std::memory_order_relaxed);

	// Allocate atoms -> sites lookup map if needed.
	std::vector<size_t> atomsToSites;
	if(siteTypes()) {
		atomsToSites.resize(positions()->size());
	}

	// Assign particles to reference sites.
	ConstPropertyAccess<Point3> positionsArray(positions());
	if(ncomponents == 1) {
		// Without per-type occupancies:
		parallelFor(positions()->size(), *task(), [&](size_t index) {
			const Point3& p = positionsArray[index];
			FloatType closestDistanceSq;
			size_t closestIndex = neighborTree.findClosestParticle((affineMapping() == TO_REFERENCE_CELL) ? (tm * p) : p, closestDistanceSq);
			OVITO_ASSERT(closestIndex < occupancyArray.size());
			occupancyArray[closestIndex].fetch_add(1, std::memory_order_relaxed);
			if(!atomsToSites.empty())
				atomsToSites[index] = closestIndex;
		});
	}
	else {
		// With per-type occupancies:
		ConstPropertyAccess<int> particleTypesArray(particleTypes());
		parallelFor(positions()->size(), *task(), [&](size_t index) {
			const Point3& p = positionsArray[index];
			FloatType closestDistanceSq;
			size_t closestIndex = neighborTree.findClosestParticle((affineMapping() == TO_REFERENCE_CELL) ? (tm * p) : p, closestDistanceSq);
			int offset = particleTypesArray[index] - typemin;
			OVITO_ASSERT(closestIndex * ncomponents + offset < occupancyArray.size());
			occupancyArray[closestIndex * ncomponents + offset].fetch_add(1, std::memory_order_relaxed);
			if(!atomsToSites.empty())
				atomsToSites[index] = closestIndex;
		});
	}
	if(task()->isCanceled()) return;

	// Create output storage.
	setOccupancyNumbers(std::make_shared<PropertyStorage>(
		siteTypes() ? positions()->size() : refPositions()->size(),
		PropertyStorage::Int, ncomponents, 0, tr("Occupancy"), false));
	if(ncomponents > 1 && typemin != 1) {
		QStringList componentNames;
		for(int i = typemin; i <= typemax; i++)
			componentNames.push_back(QString::number(i));
		occupancyNumbers()->setComponentNames(componentNames);
	}

	// Copy data from atomic array to output buffer.
	PropertyAccess<int,true> occupancyNumbersArray(occupancyNumbers());
	if(!siteTypes()) {
		boost::copy(occupancyArray, occupancyNumbersArray.begin());
	}
	else {
		// Map occupancy numbers from sites to atoms.
		PropertyAccess<int> siteTypesArray(siteTypes());
		PropertyAccess<qlonglong> siteIndicesArray(siteIndices());
		PropertyAccess<qlonglong> siteIdentifiersArray(siteIdentifiers());
		ConstPropertyAccess<int> referenceTypeArray(_referenceTypeProperty);
		ConstPropertyAccess<qlonglong> referenceIdentifierArray(_referenceIdentifierProperty);
		int* occ = occupancyNumbersArray.begin();
		int* st = siteTypesArray.begin();
		auto sidx = siteIndicesArray.begin();
		auto sid = siteIdentifiersArray ? siteIdentifiersArray.begin() : nullptr;
		for(size_t siteIndex : atomsToSites) {
			for(int j = 0; j < ncomponents; j++) {
				*occ++ = occupancyArray[siteIndex * ncomponents + j];
			}
			*st++ = referenceTypeArray ? referenceTypeArray[siteIndex] : 0;
			*sidx++ = siteIndex;
			if(sid) *sid++ = referenceIdentifierArray[siteIndex];
		}
	}

	// Count defects.
	if(ncomponents == 1) {
		for(int oc : occupancyArray) {
			if(oc == 0) incrementVacancyCount();
			else if(oc > 1) incrementInterstitialCount(oc - 1);
		}
	}
	else {
		auto o = occupancyArray.cbegin();
		for(size_t i = 0; i < refPositions()->size(); i++) {
			int oc = 0;
			for(int j = 0; j < ncomponents; j++) {
				oc += *o++;
			}
			if(oc == 0) incrementVacancyCount();
			else if(oc > 1) incrementInterstitialCount(oc - 1);
		}
	}
}

/******************************************************************************
* Injects the computed results of the engine into the data pipeline.
******************************************************************************/
void WignerSeitzAnalysisModifier::WignerSeitzAnalysisEngine::emitResults(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state)
{
	const ParticlesObject* refParticles = referenceState().getObject<ParticlesObject>();
	if(!refParticles)
		modApp->throwException(tr("This modifier cannot be evaluated, because the reference configuration does not contain any particles."));

	if(!siteTypes()) {
		// Replace complete particles set with the reference configuration.
		state.mutableData()->replaceObject(state.expectObject<ParticlesObject>(), refParticles);
		// Also replace simulation cell with reference cell.
		if(const SimulationCellObject* cell = state.getObject<SimulationCellObject>())
			state.mutableData()->replaceObject(cell, referenceState().getObject<SimulationCellObject>());
	}

	ParticlesObject* particles = state.expectMutableObject<ParticlesObject>();
	if(occupancyNumbers()->size() != particles->elementCount())
		modApp->throwException(tr("Cached modifier results are obsolete, because the number of input particles has changed."));
	const PropertyObject* posProperty = particles->expectProperty(ParticlesObject::PositionProperty);

	particles->createProperty(occupancyNumbers());
	if(siteTypes()) {
		PropertyObject* outProp = particles->createProperty(siteTypes());
		// Transfer particle type list from reference type property to output site type property.
		if(const PropertyObject* inProp = refParticles->getProperty(ParticlesObject::TypeProperty)) {
			outProp->setElementTypes(inProp->elementTypes());
		}
	}
	if(siteIndices())
		particles->createProperty(siteIndices());
	if(siteIdentifiers())
		particles->createProperty(siteIdentifiers());

	state.addAttribute(QStringLiteral("WignerSeitz.vacancy_count"), QVariant::fromValue(vacancyCount()), modApp);
	state.addAttribute(QStringLiteral("WignerSeitz.interstitial_count"), QVariant::fromValue(interstitialCount()), modApp);

	state.setStatus(PipelineStatus(PipelineStatus::Success, tr("Found %1 vacancies and %2 interstitials").arg(vacancyCount()).arg(interstitialCount())));
}

}	// End of namespace
}	// End of namespace
