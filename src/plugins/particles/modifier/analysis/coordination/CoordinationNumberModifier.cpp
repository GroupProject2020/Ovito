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
#include <plugins/particles/modifier/ParticleInputHelper.h>
#include <plugins/particles/modifier/ParticleOutputHelper.h>
#include <core/app/Application.h>
#include <core/dataset/DataSet.h>
#include <plugins/stdobj/simcell/SimulationCellObject.h>
#include <plugins/stdobj/plot/PlotObject.h>
#include <core/dataset/pipeline/ModifierApplication.h>
#include <core/utilities/units/UnitsManager.h>
#include <core/utilities/concurrent/ParallelFor.h>
#include "CoordinationNumberModifier.h"

#include <QtConcurrent>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

IMPLEMENT_OVITO_CLASS(CoordinationNumberModifier);
DEFINE_PROPERTY_FIELD(CoordinationNumberModifier, cutoff);
DEFINE_PROPERTY_FIELD(CoordinationNumberModifier, numberOfBins);
DEFINE_PROPERTY_FIELD(CoordinationNumberModifier, computePartialRDF);
SET_PROPERTY_FIELD_LABEL(CoordinationNumberModifier, cutoff, "Cutoff radius");
SET_PROPERTY_FIELD_LABEL(CoordinationNumberModifier, numberOfBins, "Number of histogram bins");
SET_PROPERTY_FIELD_LABEL(CoordinationNumberModifier, computePartialRDF, "Compute partial RDFs");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(CoordinationNumberModifier, cutoff, WorldParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_RANGE(CoordinationNumberModifier, numberOfBins, IntegerParameterUnit, 4, 100000);

IMPLEMENT_OVITO_CLASS(CoordinationNumberModifierApplication);
SET_MODIFIER_APPLICATION_TYPE(CoordinationNumberModifier, CoordinationNumberModifierApplication);
DEFINE_PROPERTY_FIELD(CoordinationNumberModifierApplication, rdf);
SET_PROPERTY_FIELD_CHANGE_EVENT(CoordinationNumberModifierApplication, rdf, ReferenceEvent::ObjectStatusChanged);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
CoordinationNumberModifier::CoordinationNumberModifier(DataSet* dataset) : AsynchronousModifier(dataset),
	_cutoff(3.2), 
	_numberOfBins(200),
	_computePartialRDF(false)
{
}

/******************************************************************************
* Asks the modifier whether it can be applied to the given input data.
******************************************************************************/
bool CoordinationNumberModifier::OOMetaClass::isApplicableTo(const PipelineFlowState& input) const
{
	return input.findObject<ParticleProperty>() != nullptr;
}

/******************************************************************************
* Creates and initializes a computation engine that will compute the modifier's results.
******************************************************************************/
Future<AsynchronousModifier::ComputeEnginePtr> CoordinationNumberModifier::createEngine(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input)
{
	// Get the current positions.
	ParticleInputHelper pih(dataset(), input);
	ParticleProperty* posProperty = pih.expectStandardProperty<ParticleProperty>(ParticleProperty::PositionProperty);

	// Get simulation cell.
	SimulationCellObject* inputCell = pih.expectSimulationCell();

	// The number of sampling intervals for the radial distribution function.
	int rdfSampleCount = std::max(numberOfBins(), 4);
	if(rdfSampleCount > 100000)
		throwException(tr("Requested number of histogram bins is too large. Limit is 100,000 histogram bins."));

	if(cutoff() <= 0)
		throwException(tr("Invalid cutoff range value. Cutoff must be positive."));

	// Get particle types if partial RDF calculation has been requested.
	ParticleProperty* typeProperty = nullptr;
	boost::container::flat_map<int,QString> uniqueTypeIds;
	if(computePartialRDF()) {
		typeProperty = ParticleProperty::findInState(input, ParticleProperty::TypeProperty);
		if(!typeProperty)
			throwException(tr("Partial RDF calculation requires the '%1' property.").arg(ParticleProperty::OOClass().standardPropertyName(ParticleProperty::TypeProperty)));

		// Build the set of unique particle type IDs.
		for(ElementType* pt : typeProperty->elementTypes()) {
			uniqueTypeIds.insert_or_assign(pt->id(), pt->name().isEmpty() ? QString::number(pt->id()) : pt->name());
		}
		if(uniqueTypeIds.empty())
			throwException(tr("No particle types have been defined."));
	}

	// Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
	return std::make_shared<CoordinationAnalysisEngine>(input, posProperty->storage(), inputCell->data(), 
		cutoff(), rdfSampleCount, typeProperty ? typeProperty->storage() : nullptr, std::move(uniqueTypeIds));
}

/******************************************************************************
* Performs the actual computation. This method is executed in a worker thread.
******************************************************************************/
void CoordinationNumberModifier::CoordinationAnalysisEngine::perform()
{
	task()->setProgressText(tr("Computing coordination numbers"));

	// Prepare the neighbor list service.
	CutoffNeighborFinder neighborListBuilder;
	if(!neighborListBuilder.prepare(cutoff(), *positions(), cell(), nullptr, task().get()))
		return;

	size_t particleCount = positions()->size();
	task()->setProgressValue(0);
	task()->setProgressMaximum(particleCount);

	// Perform neighbor counting in parallel.
	std::mutex mutex;
	if(!_computePartialRdfs) {
		// Total RDF case:
		parallelForChunks(particleCount, *task(), [&neighborListBuilder, &mutex, this](size_t startIndex, size_t chunkSize, PromiseState& promise) {
			size_t binCount = rdfX()->size();
			FloatType rdfBinSize = cutoff() / binCount;
			std::vector<size_t> threadLocalRDF(binCount, 0);
			for(size_t i = startIndex, endIndex = startIndex + chunkSize; i < endIndex; ) {
				int& coordination = coordinationNumbers()->dataInt()[i];
				OVITO_ASSERT(coordination == 0);
				for(CutoffNeighborFinder::Query neighQuery(neighborListBuilder, i); !neighQuery.atEnd(); neighQuery.next()) {
					coordination++;
					size_t rdfBin = (size_t)(sqrt(neighQuery.distanceSquared()) / rdfBinSize);
					threadLocalRDF[std::min(rdfBin, binCount - 1)]++;
				}
				i++;

				// Update progress indicator.
				if((i % 1024ll) == 0)
					promise.incrementProgressValue(1024);
				// Abort loop when operation was canceled by the user.
				if(promise.isCanceled())
					return;
			}
			// Combine per-thread RDF data into one master histogram.
			std::lock_guard<std::mutex> lock(mutex);
			auto bin = rdfY()->dataFloat();
			for(auto iter = threadLocalRDF.cbegin(); iter != threadLocalRDF.cend(); ++iter)
				*bin++ += *iter;
		});
	}
	else {
		// Partial RDFs case:
		parallelForChunks(particleCount, *task(), [&neighborListBuilder, &mutex, this](size_t startIndex, size_t chunkSize, PromiseState& promise) {
			size_t typeCount = uniqueTypeIds().size();
			size_t binCount = rdfY()->size();
			size_t rdfCount = rdfY()->componentCount();
			FloatType rdfBinSize = cutoff() / binCount;
			std::vector<size_t> threadLocalRDF(rdfY()->size() * rdfY()->componentCount(), 0);
			for(size_t i = startIndex, endIndex = startIndex + chunkSize; i < endIndex; ) {
				int& coordination = coordinationNumbers()->dataInt()[i];
				OVITO_ASSERT(coordination == 0);

				size_t typeIndex1 = uniqueTypeIds().index_of(uniqueTypeIds().find(particleTypes()->getInt(i)));
				if(typeIndex1 < typeCount) {
					for(CutoffNeighborFinder::Query neighQuery(neighborListBuilder, i); !neighQuery.atEnd(); neighQuery.next()) {
						coordination++;
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
	}
	if(task()->isCanceled())
		return;

	// Compute x values of histogram function. 
	FloatType stepSize = cutoff() / rdfX()->size();
	FloatType r = stepSize / 2;
	for(auto& x : rdfX()->floatRange()) {
		x = r;
		r += stepSize;
	}

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
PipelineFlowState CoordinationNumberModifier::CoordinationAnalysisEngine::emitResults(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input)
{
	if(_inputFingerprint.hasChanged(input))
		modApp->throwException(tr("Cached modifier results are obsolete, because the number or the storage order of input particles has changed."));

	PipelineFlowState output = input;
	ParticleOutputHelper poh(modApp->dataset(), output);
	OVITO_ASSERT(coordinationNumbers()->size() == poh.outputParticleCount());
	poh.outputProperty<ParticleProperty>(coordinationNumbers());

	// Output RDF histogram(s).
	OORef<PlotObject> rdfPlotObj = new PlotObject(modApp->dataset());
	rdfPlotObj->setx(rdfX());
	rdfPlotObj->sety(rdfY());
	rdfPlotObj->setTitle(poh.generateUniquePlotName(QStringLiteral("RDF")));
	output.addObject(rdfPlotObj);

	// Store the RDF data in the ModifierApplication in order to display the RDF in the modifier's UI panel.
	static_object_cast<CoordinationNumberModifierApplication>(modApp)->setRdf(std::move(rdfPlotObj));

	return output;
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
