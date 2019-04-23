///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2018) Alexander Stukowski
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
#include <core/app/Application.h>
#include <core/dataset/DataSet.h>
#include <plugins/stdobj/simcell/SimulationCellObject.h>
#include <plugins/stdobj/series/DataSeriesObject.h>
#include <core/dataset/pipeline/ModifierApplication.h>
#include <core/utilities/units/UnitsManager.h>
#include <core/utilities/concurrent/ParallelFor.h>
#include "CoordinationAnalysisModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

IMPLEMENT_OVITO_CLASS(CoordinationAnalysisModifier);
DEFINE_PROPERTY_FIELD(CoordinationAnalysisModifier, cutoff);
DEFINE_PROPERTY_FIELD(CoordinationAnalysisModifier, numberOfBins);
DEFINE_PROPERTY_FIELD(CoordinationAnalysisModifier, computePartialRDF);
SET_PROPERTY_FIELD_LABEL(CoordinationAnalysisModifier, cutoff, "Cutoff radius");
SET_PROPERTY_FIELD_LABEL(CoordinationAnalysisModifier, numberOfBins, "Number of histogram bins");
SET_PROPERTY_FIELD_LABEL(CoordinationAnalysisModifier, computePartialRDF, "Compute partial RDFs");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(CoordinationAnalysisModifier, cutoff, WorldParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_RANGE(CoordinationAnalysisModifier, numberOfBins, IntegerParameterUnit, 4, 100000);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
CoordinationAnalysisModifier::CoordinationAnalysisModifier(DataSet* dataset) : AsynchronousModifier(dataset),
	_cutoff(3.2),
	_numberOfBins(200),
	_computePartialRDF(false)
{
}

/******************************************************************************
* Asks the modifier whether it can be applied to the given input data.
******************************************************************************/
bool CoordinationAnalysisModifier::OOMetaClass::isApplicableTo(const DataCollection& input) const
{
	return input.containsObject<ParticlesObject>();
}

/******************************************************************************
* Creates and initializes a computation engine that will compute the modifier's results.
******************************************************************************/
Future<AsynchronousModifier::ComputeEnginePtr> CoordinationAnalysisModifier::createEngine(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input)
{
	// Get the current positions.
	const ParticlesObject* particles = input.expectObject<ParticlesObject>();
	const PropertyObject* posProperty = particles->expectProperty(ParticlesObject::PositionProperty);

	// Get simulation cell.
	const SimulationCellObject* inputCell = input.expectObject<SimulationCellObject>();

	// The number of sampling intervals for the radial distribution function.
	int rdfSampleCount = std::max(numberOfBins(), 4);
	if(rdfSampleCount > 100000)
		throwException(tr("Requested number of histogram bins is too large. Limit is 100,000 histogram bins."));

	if(cutoff() <= 0)
		throwException(tr("Invalid cutoff range value. Cutoff must be positive."));

	// Get particle types if partial RDF calculation has been requested.
	const PropertyObject* typeProperty = nullptr;
	boost::container::flat_map<int,QString> uniqueTypeIds;
	if(computePartialRDF()) {
		typeProperty = particles->getProperty(ParticlesObject::TypeProperty);
		if(!typeProperty)
			throwException(tr("Partial RDF calculation requires the '%1' property.").arg(ParticlesObject::OOClass().standardPropertyName(ParticlesObject::TypeProperty)));

		// Build the set of unique particle type IDs.
		for(const ElementType* pt : typeProperty->elementTypes()) {
#if BOOST_VERSION >= 106200
			uniqueTypeIds.insert_or_assign(pt->numericId(), pt->name().isEmpty() ? QString::number(pt->numericId()) : pt->name());
#else
			// For backward compatibility with older Boost versions, which do not know insert_or_assign():
			uniqueTypeIds[pt->numericId()] = pt->name().isEmpty() ? QString::number(pt->numericId()) : pt->name();
#endif
		}
		if(uniqueTypeIds.empty())
			throwException(tr("No particle types have been defined."));
	}

	// Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
	return std::make_shared<CoordinationAnalysisEngine>(particles, posProperty->storage(), inputCell->data(),
		cutoff(), rdfSampleCount, typeProperty ? typeProperty->storage() : nullptr, std::move(uniqueTypeIds));
}

/******************************************************************************
* Performs the actual computation. This method is executed in a worker thread.
******************************************************************************/
void CoordinationAnalysisModifier::CoordinationAnalysisEngine::perform()
{
	task()->setProgressText(tr("Coordination analysis"));

	// Prepare the neighbor list service.
	CutoffNeighborFinder neighborListBuilder;
	if(!neighborListBuilder.prepare(cutoff(), *positions(), cell(), nullptr, task().get()))
		return;

	size_t particleCount = positions()->size();
	task()->setProgressValue(0);
	task()->setProgressMaximum(particleCount);

	// Parallel calculation loop:
	std::mutex mutex;
	parallelForChunks(particleCount, *task(), [&neighborListBuilder, &mutex, this](size_t startIndex, size_t chunkSize, Task& promise) {
		size_t typeCount = _computePartialRdfs ? uniqueTypeIds().size() : 1;
		size_t binCount = rdfY()->size();
		size_t rdfCount = rdfY()->componentCount();
		FloatType rdfBinSize = cutoff() / binCount;
		std::vector<size_t> threadLocalRDF(rdfY()->size() * rdfY()->componentCount(), 0);
		for(size_t i = startIndex, endIndex = startIndex + chunkSize; i < endIndex; ) {
			int& coordination = coordinationNumbers()->dataInt()[i];
			OVITO_ASSERT(coordination == 0);

			size_t typeIndex1 = _computePartialRdfs ? uniqueTypeIds().index_of(uniqueTypeIds().find(particleTypes()->getInt(i))) : 0;
			if(typeIndex1 < typeCount) {
				for(CutoffNeighborFinder::Query neighQuery(neighborListBuilder, i); !neighQuery.atEnd(); neighQuery.next()) {
					coordination++;
					if(_computePartialRdfs) {
						size_t typeIndex2 = uniqueTypeIds().index_of(uniqueTypeIds().find(particleTypes()->getInt(neighQuery.current())));
						if(typeIndex2 < typeCount) {
							size_t lowerIndex = std::min(typeIndex1, typeIndex2);
							size_t upperIndex = std::max(typeIndex1, typeIndex2);
							size_t rdfIndex = (typeCount * lowerIndex) - ((lowerIndex - 1) * lowerIndex) / 2 + upperIndex - lowerIndex;
							OVITO_ASSERT(rdfIndex < rdfY()->componentCount());
							size_t rdfBin = (size_t)(sqrt(neighQuery.distanceSquared()) / rdfBinSize);
							threadLocalRDF[rdfIndex + std::min(rdfBin, binCount - 1) * rdfCount]++;
						}
					}
					else {
						size_t rdfBin = (size_t)(sqrt(neighQuery.distanceSquared()) / rdfBinSize);
						threadLocalRDF[std::min(rdfBin, binCount - 1)]++;
					}
				}
			}
			i++;

			// Update progress indicator.
			if((i % 1024ll) == 0)
				promise.incrementProgressValue(1024);
			// Abort loop when operation was canceled by the user.
			if(promise.isCanceled())
				return;
		}
		// Combine per-thread RDFs into a set of master histograms.
		std::lock_guard<std::mutex> lock(mutex);
		auto bin = rdfY()->dataFloat();
		for(auto iter = threadLocalRDF.cbegin(); iter != threadLocalRDF.cend(); ++iter)
			*bin++ += *iter;
	});
	if(task()->isCanceled())
		return;

	// Compute x values of histogram function.
	FloatType stepSize = cutoff() / rdfY()->size();

	// Helper function that normalizes a RDF histogram.
	auto normalizeRDF = [this,stepSize](size_t type1Count, size_t type2Count, size_t component = 0, FloatType prefactor = 1) {
		if(!cell().is2D()) {
			prefactor *= FloatType(4.0/3.0) * FLOATTYPE_PI * type1Count / cell().volume3D() * type2Count;
		}
		else {
			prefactor *= FLOATTYPE_PI * type1Count / cell().volume2D() * type2Count;
		}
		FloatType r1 = 0;
		size_t cmpntCount = rdfY()->componentCount();
		OVITO_ASSERT(component < cmpntCount);
		for(FloatType* y = rdfY()->dataFloat() + component, *y_end = y + rdfY()->size()*cmpntCount; y != y_end; y += cmpntCount) {
			double r2 = r1 + stepSize;
			FloatType vol = cell().is2D() ? (r2*r2 - r1*r1) : (r2*r2*r2 - r1*r1*r1);
			*y /= prefactor * vol;
			r1 = r2;
		}
	};

	if(!_computePartialRdfs) {
		normalizeRDF(particleCount, particleCount);
	}
	else {
		// Count particle type occurrences.
		std::vector<size_t> particleCounts(uniqueTypeIds().size(), 0);
		for(int t : particleTypes()->constIntRange()) {
			size_t typeIndex = uniqueTypeIds().index_of(uniqueTypeIds().find(t));
			if(typeIndex < particleCounts.size())
				particleCounts[typeIndex]++;
		}
		if(task()->isCanceled()) return;

		// Normalize RDFs.
		size_t component = 0;
		for(size_t i = 0; i < particleCounts.size(); i++) {
			for(size_t j = i; j < particleCounts.size(); j++) {
				normalizeRDF(particleCounts[i], particleCounts[j], component++, i == j ? 1 : 2);
			}
		}
	}
}

/******************************************************************************
* Injects the computed results of the engine into the data pipeline.
******************************************************************************/
void CoordinationAnalysisModifier::CoordinationAnalysisEngine::emitResults(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state)
{
	ParticlesObject* particles = state.expectMutableObject<ParticlesObject>();

	if(_inputFingerprint.hasChanged(particles))
		modApp->throwException(tr("Cached modifier results are obsolete, because the number or the storage order of input particles has changed."));

	// Output coordination numbers as a new particle property.
	OVITO_ASSERT(coordinationNumbers()->size() == particles->elementCount());
	particles->createProperty(coordinationNumbers());

	// Output RDF histogram(s).
	DataSeriesObject* seriesObj = state.createObject<DataSeriesObject>(QStringLiteral("coordination-rdf"), modApp, DataSeriesObject::Line, tr("Radial distribution function"), rdfY());
	seriesObj->setIntervalStart(0);
	seriesObj->setIntervalEnd(cutoff());
	seriesObj->setAxisLabelX(tr("Pair separation distance"));
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
