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

#include <plugins/particles/Particles.h>
#include <plugins/particles/util/NearestNeighborFinder.h>
#include <plugins/particles/modifier/ParticleInputHelper.h>
#include <plugins/particles/modifier/ParticleOutputHelper.h>
#include <plugins/stdobj/simcell/SimulationCellObject.h>
#include <core/dataset/pipeline/ModifierApplication.h>
#include <core/utilities/concurrent/ParallelFor.h>
#include "WignerSeitzAnalysisModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

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
Future<AsynchronousModifier::ComputeEnginePtr> WignerSeitzAnalysisModifier::createEngineWithReference(TimePoint time, ModifierApplication* modApp, PipelineFlowState input, const PipelineFlowState& referenceState, TimeInterval validityInterval)
{
	ParticleInputHelper pih(dataset(), input);

	// Get the current particle positions.
	ParticleProperty* posProperty = pih.expectStandardProperty<ParticleProperty>(ParticleProperty::PositionProperty);

	// Get the reference particle position.
	ParticleProperty* refPosProperty = ParticleProperty::findInState(referenceState, ParticleProperty::PositionProperty);
	if(!refPosProperty)
		throwException(tr("Reference configuration does not contain particle positions."));

	// Get simulation cells.
	SimulationCellObject* inputCell = pih.expectSimulationCell();
	SimulationCellObject* refCell = referenceState.findObject<SimulationCellObject>();
	if(!refCell)
		throwException(tr("Reference configuration does not contain simulation cell info."));

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
		ParticleProperty* ptypeProp = pih.expectStandardProperty<ParticleProperty>(ParticleProperty::TypeProperty);
		// Determine value range of particle type IDs.
		for(ElementType* pt : ptypeProp->elementTypes()) {
			if(pt->id() < ptypeMinId) ptypeMinId = pt->id();
			if(pt->id() > ptypeMaxId) ptypeMaxId = pt->id();
		}
		typeProperty = ptypeProp->storage();
	}

	// If output of the displaced configuration is requested, obtain types of the reference sites.
	ConstPropertyPtr referenceTypeProperty;
	ConstPropertyPtr referenceIdentifierProperty;
	if(outputCurrentConfig()) {
		if(ParticleProperty* prop =  ParticleProperty::findInState(referenceState, ParticleProperty::TypeProperty))
			referenceTypeProperty = prop->storage();
		
		if(ParticleProperty* idProp = ParticleProperty::findInState(referenceState, ParticleProperty::IdentifierProperty)) {
			referenceIdentifierProperty = idProp->storage();
		}
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
	if(!neighborTree.prepare(*refPositions(), refCell(), nullptr, task().get()))
		return;

	// Determine the number of components of the occupancy property.
	int ncomponents = 1;
	int typemin, typemax;
	if(particleTypes()) {
		auto minmax = std::minmax_element(particleTypes()->constDataInt(), particleTypes()->constDataInt() + particleTypes()->size());
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
	if(ncomponents == 1) {
		// Without per-type occupancies:
		parallelFor(positions()->size(), *task(), [this, &neighborTree, tm, &occupancyArray, &atomsToSites](size_t index) {
			const Point3& p = positions()->getPoint3(index);
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
		parallelFor(positions()->size(), *task(), [this, &neighborTree, typemin, ncomponents, tm, &occupancyArray, &atomsToSites](size_t index) {
			const Point3& p = positions()->getPoint3(index);
			FloatType closestDistanceSq;
			size_t closestIndex = neighborTree.findClosestParticle((affineMapping() == TO_REFERENCE_CELL) ? (tm * p) : p, closestDistanceSq);
			int offset = particleTypes()->getInt(index) - typemin;
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
	if(!siteTypes()) {
		std::copy(occupancyArray.begin(), occupancyArray.end(), occupancyNumbers()->dataInt());
	}
	else {
		// Map occupancy numbers from sites to atoms.
		int* occ = occupancyNumbers()->dataInt();
		int* st = siteTypes()->dataInt();
		auto sidx = siteIndices()->dataInt64();
		auto sid = siteIdentifiers() ? siteIdentifiers()->dataInt64() : nullptr;
		for(size_t siteIndex : atomsToSites) {
			for(int j = 0; j < ncomponents; j++) {
				*occ++ = occupancyArray[siteIndex * ncomponents + j];
			}
			*st++ = _referenceTypeProperty ? _referenceTypeProperty->getInt(siteIndex) : 0;
			*sidx++ = siteIndex;
			if(sid) *sid++ = _referenceIdentifierProperty->getInt64(siteIndex); 
		}
		OVITO_ASSERT(occ == occupancyNumbers()->dataInt() + occupancyNumbers()->size() * occupancyNumbers()->componentCount());
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
PipelineFlowState WignerSeitzAnalysisModifier::WignerSeitzAnalysisEngine::emitResults(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input)
{
	PipelineFlowState output;
	if(!siteTypes()) {
		// Replace complete pipeline state with the reference configuration (except global attributes).
		output = referenceState();
		output.setStateValidity(input.stateValidity());
		output.attributes() = input.attributes();
	}
	else {
		// Keep current particle configuration.
		output = input;
	}

	ParticleOutputHelper poh(modApp->dataset(), output);

	if(occupancyNumbers()->size() != poh.outputParticleCount())
		modApp->throwException(tr("Cached modifier results are obsolete, because the number of input particles has changed."));

	ParticleProperty* posProperty = ParticleProperty::findInState(output, ParticleProperty::PositionProperty);
	if(!posProperty)
		modApp->throwException(tr("This modifier cannot be evaluated, because the reference configuration does not contain any particles."));
	OVITO_ASSERT(poh.outputParticleCount() == posProperty->size());

	poh.outputProperty<ParticleProperty>(occupancyNumbers());
	if(siteTypes()) {
		PropertyObject* outProp = poh.outputProperty<ParticleProperty>(siteTypes());
		// Transfer particle type list from reference type property to output site type property.
		if(ParticleProperty* inProp =  ParticleProperty::findInState(referenceState(), ParticleProperty::TypeProperty)) {
			outProp->setElementTypes(inProp->elementTypes());
		}
	}
	if(siteIndices())
		poh.outputProperty<ParticleProperty>(siteIndices());
	if(siteIdentifiers())
		poh.outputProperty<ParticleProperty>(siteIdentifiers());

	poh.outputAttribute(QStringLiteral("WignerSeitz.vacancy_count"), QVariant::fromValue(vacancyCount()));
	poh.outputAttribute(QStringLiteral("WignerSeitz.interstitial_count"), QVariant::fromValue(interstitialCount()));

	output.setStatus(PipelineStatus(PipelineStatus::Success, tr("Found %1 vacancies and %2 interstitials").arg(vacancyCount()).arg(interstitialCount())));
	return output;
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
