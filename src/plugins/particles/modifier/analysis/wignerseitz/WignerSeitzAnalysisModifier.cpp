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
SET_PROPERTY_FIELD_LABEL(WignerSeitzAnalysisModifier, perTypeOccupancy, "Output per-type occupancies");

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
WignerSeitzAnalysisModifier::WignerSeitzAnalysisModifier(DataSet* dataset) : ReferenceConfigurationModifier(dataset),
	_perTypeOccupancy(false)
{
}

/******************************************************************************
* Creates and initializes a computation engine that will compute the modifier's results.
******************************************************************************/
Future<AsynchronousModifier::ComputeEnginePtr> WignerSeitzAnalysisModifier::createEngineWithReference(TimePoint time, ModifierApplication* modApp, PipelineFlowState input, const PipelineFlowState& referenceState, TimeInterval validityInterval)
{
	ParticleInputHelper pih(dataset(), input);

	// Get the current positions.
	ParticleProperty* posProperty = pih.expectStandardProperty<ParticleProperty>(ParticleProperty::PositionProperty);

	// Get the reference position property.
	ParticleProperty* refPosProperty = ParticleProperty::findInState(referenceState, ParticleProperty::PositionProperty);
	if(!refPosProperty)
		throwException(tr("Reference configuration does not contain particle positions."));

	// Get simulation cells.
	SimulationCellObject* inputCell = pih.expectSimulationCell();
	SimulationCellObject* refCell = referenceState.findObject<SimulationCellObject>();
	if(!refCell)
		throwException(tr("Reference configuration does not contain simulation cell info."));

	// Check simulation cell(s).
	if(inputCell->is2D())
		throwException(tr("Wigner-Seitz analysis is not supported for 2d systems."));
	if(inputCell->volume3D() < FLOATTYPE_EPSILON)
		throwException(tr("Simulation cell is degenerate in the deformed configuration."));
	if(refCell->volume3D() < FLOATTYPE_EPSILON)
		throwException(tr("Simulation cell is degenerate in the reference configuration."));

	// Get the particle types.
	ConstPropertyPtr typeProperty;
	int ptypeMinId = std::numeric_limits<int>::max();
	int ptypeMaxId = std::numeric_limits<int>::lowest();
	if(perTypeOccupancy()) {
		ParticleProperty* ptypeProp = pih.expectStandardProperty<ParticleProperty>(ParticleProperty::TypeProperty);
		// Determine range of particle type IDs.
		for(ElementType* pt : ptypeProp->elementTypes()) {
			if(pt->id() < ptypeMinId) ptypeMinId = pt->id();
			if(pt->id() > ptypeMaxId) ptypeMaxId = pt->id();
		}
		typeProperty = ptypeProp->storage();
	}

	// Create the results storage, which holds a copy of the reference state.
	// We are going to need the reference state in the apply() method.
	auto resultStorage = std::make_shared<WignerSeitzAnalysisResults>(validityInterval, referenceState);

	// Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
	auto engine = std::make_shared<WignerSeitzAnalysisEngine>(resultStorage, posProperty->storage(), inputCell->data(),
			refPosProperty->storage(), refCell->data(), affineMapping(), std::move(typeProperty), ptypeMinId, ptypeMaxId);

	// Make sure the results storage, and with it the reference state, stay alive until we are back in the main thread.
	engine->finally(dataset()->executor(), [resultStorage = std::move(resultStorage)]() {});

	return engine;
}

/******************************************************************************
* Performs the actual computation. This method is executed in a worker thread.
******************************************************************************/
void WignerSeitzAnalysisModifier::WignerSeitzAnalysisEngine::perform()
{
	setProgressText(tr("Performing Wigner-Seitz cell analysis"));

	if(affineMapping() == TO_CURRENT_CELL)
		throw Exception(tr("Mapping coordinates to the current cell is not supported by the W-S analysis routine."));

	size_t particleCount = positions()->size();
	if(refPositions()->size() == 0)
		throw Exception(tr("Reference configuration for WS analysis contains no sites."));

	// Prepare the closest-point query structure.
	NearestNeighborFinder neighborTree(0);
	if(!neighborTree.prepare(*refPositions(), refCell(), nullptr, this))
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

	// Create output storage.
	_results->setOccupancyNumbers(std::make_shared<PropertyStorage>(refPositions()->size(), PropertyStorage::Int, ncomponents, 0, tr("Occupancy"), true));
	if(ncomponents > 1 && typemin != 1) {
		QStringList componentNames;
		for(int i = typemin; i <= typemax; i++)
			componentNames.push_back(QString::number(i));
		_results->occupancyNumbers()->setComponentNames(componentNames);
	}

	AffineTransformation tm;
	if(affineMapping() == TO_REFERENCE_CELL)
		tm = refCell().matrix() * cell().inverseMatrix();

	// Create array for atomic counting.
	size_t arraySize = _results->occupancyNumbers()->size() * _results->occupancyNumbers()->componentCount();
	std::vector<std::atomic_int> occupancyArray(arraySize);
	for(auto& o : occupancyArray)
		o.store(0, std::memory_order_relaxed);

	// Assign particles to reference sites.
	if(ncomponents == 1) {
		parallelFor(positions()->size(), *this, [this, &neighborTree, tm, &occupancyArray](size_t index) {
			const Point3& p = positions()->getPoint3(index);
			FloatType closestDistanceSq;
			int closestIndex = neighborTree.findClosestParticle((affineMapping() == TO_REFERENCE_CELL) ? (tm * p) : p, closestDistanceSq);
			OVITO_ASSERT(closestIndex >= 0 && closestIndex < _results->occupancyNumbers()->size());
			occupancyArray[closestIndex].fetch_add(1, std::memory_order_relaxed);
		});
	}
	else {
		parallelFor(positions()->size(), *this, [this, &neighborTree, typemin, ncomponents, tm, &occupancyArray](size_t index) {
			const Point3& p = positions()->getPoint3(index);
			FloatType closestDistanceSq;
			int closestIndex = neighborTree.findClosestParticle((affineMapping() == TO_REFERENCE_CELL) ? (tm * p) : p, closestDistanceSq);
			OVITO_ASSERT(closestIndex >= 0 && closestIndex < _results->occupancyNumbers()->size());
			int offset = particleTypes()->getInt(index) - typemin;
			OVITO_ASSERT(offset >= 0 && offset < _results->occupancyNumbers()->componentCount());
			occupancyArray[closestIndex * ncomponents + offset].fetch_add(1, std::memory_order_relaxed);
		});
	}
	
	// Copy data from atomic array to output buffer.
	std::copy(occupancyArray.begin(), occupancyArray.end(), _results->occupancyNumbers()->dataInt());

	// Count defects.
	if(ncomponents == 1) {
		for(int oc : _results->occupancyNumbers()->constIntRange()) {
			if(oc == 0) _results->incrementVacancyCount();
			else if(oc > 1) _results->incrementInterstitialCount(oc - 1);
		}
	}
	else {
		const int* o = _results->occupancyNumbers()->constDataInt();
		for(size_t i = 0; i < refPositions()->size(); i++) {
			int oc = 0;
			for(int j = 0; j < ncomponents; j++) {
				oc += *o++;
			}
			if(oc == 0) _results->incrementVacancyCount();
			else if(oc > 1) _results->incrementInterstitialCount(oc - 1);
		}
	}
	
	// Return the results of the compute engine.
	setResult(std::move(_results));
}

/******************************************************************************
* Injects the computed results of the engine into the data pipeline.
******************************************************************************/
PipelineFlowState WignerSeitzAnalysisModifier::WignerSeitzAnalysisResults::apply(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input)
{
	// Replace pipeline contents with reference configuration.
	PipelineFlowState output = referenceState();
	output.setStateValidity(input.stateValidity());
	output.attributes() = input.attributes();

	ParticleOutputHelper poh(modApp->dataset(), output);	

	if(occupancyNumbers()->size() != poh.outputParticleCount())
		modApp->throwException(tr("Cached modifier results are obsolete, because the number of input particles has changed."));

	ParticleProperty* posProperty = ParticleProperty::findInState(output, ParticleProperty::PositionProperty);
	if(!posProperty)
		modApp->throwException(tr("This modifier cannot be evaluated, because the reference configuration does not contain any particles."));		
	OVITO_ASSERT(poh.outputParticleCount() == posProperty->size());

	poh.outputProperty<ParticleProperty>(occupancyNumbers());

	output.attributes().insert(QStringLiteral("WignerSeitz.vacancy_count"), QVariant::fromValue(vacancyCount()));
	output.attributes().insert(QStringLiteral("WignerSeitz.interstitial_count"), QVariant::fromValue(interstitialCount()));

	output.setStatus(PipelineStatus(PipelineStatus::Success, tr("Found %1 vacancies and %2 interstitials").arg(vacancyCount()).arg(interstitialCount())));
	return output;
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
