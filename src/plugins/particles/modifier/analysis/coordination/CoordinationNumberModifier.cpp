///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2014) Alexander Stukowski
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
#include <core/dataset/pipeline/ModifierApplication.h>
#include <core/utilities/units/UnitsManager.h>
#include "CoordinationNumberModifier.h"

#include <QtConcurrent>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

IMPLEMENT_OVITO_CLASS(CoordinationNumberModifier);
IMPLEMENT_OVITO_CLASS(CoordinationNumberModifierApplication);
DEFINE_PROPERTY_FIELD(CoordinationNumberModifier, cutoff);
DEFINE_PROPERTY_FIELD(CoordinationNumberModifier, numberOfBins);
SET_PROPERTY_FIELD_LABEL(CoordinationNumberModifier, cutoff, "Cutoff radius");
SET_PROPERTY_FIELD_LABEL(CoordinationNumberModifier, numberOfBins, "Number of histogram bins");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(CoordinationNumberModifier, cutoff, WorldParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_RANGE(CoordinationNumberModifier, numberOfBins, IntegerParameterUnit, 4, 100000);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
CoordinationNumberModifier::CoordinationNumberModifier(DataSet* dataset) : AsynchronousModifier(dataset),
	_cutoff(3.2), 
	_numberOfBins(200)
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
* Create a new modifier application that refers to this modifier instance.
******************************************************************************/
OORef<ModifierApplication> CoordinationNumberModifier::createModifierApplication()
{
	OORef<ModifierApplication> modApp = new CoordinationNumberModifierApplication(dataset());
	modApp->setModifier(this);
	return modApp;
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
		throwException(tr("Number of histogram bins is too large."));

	// Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
	return std::make_shared<CoordinationAnalysisEngine>(input.stateValidity(), posProperty->storage(), inputCell->data(), cutoff(), rdfSampleCount);
}

/******************************************************************************
* Performs the actual computation. This method is executed in a worker thread.
******************************************************************************/
void CoordinationNumberModifier::CoordinationAnalysisEngine::perform()
{
	setProgressText(tr("Computing coordination numbers"));

	// Prepare the neighbor list.
	CutoffNeighborFinder neighborListBuilder;
	if(!neighborListBuilder.prepare(_cutoff, *positions(), cell(), nullptr, this))
		return;

	size_t particleCount = positions()->size();
	setProgressValue(0);
	setProgressMaximum(particleCount / 1000);

	QVector<double> rdfHistogram(_rdfSampleCount, 0.0);

	// Perform analysis on each particle in parallel.
	std::vector<QFuture<void>> workers;
	size_t num_threads = Application::instance()->idealThreadCount();
	size_t chunkSize = particleCount / num_threads;
	size_t startIndex = 0;
	size_t endIndex = chunkSize;
	std::mutex mutex;
	for(size_t t = 0; t < num_threads; t++) {
		if(t == num_threads - 1) {
			endIndex += particleCount % num_threads;
		}
		workers.push_back(QtConcurrent::run([&neighborListBuilder, startIndex, endIndex, &mutex, &rdfHistogram, this]() {
			FloatType rdfBinSize = (_cutoff + FLOATTYPE_EPSILON) / _rdfSampleCount;
			std::vector<double> threadLocalRDF(_rdfSampleCount, 0);
			int* coordOutput = _results->coordinationNumbers()->dataInt() + startIndex;
			for(size_t i = startIndex; i < endIndex; ++coordOutput) {

				OVITO_ASSERT(*coordOutput == 0);
				for(CutoffNeighborFinder::Query neighQuery(neighborListBuilder, i); !neighQuery.atEnd(); neighQuery.next()) {
					(*coordOutput)++;
					size_t rdfInterval = (size_t)(sqrt(neighQuery.distanceSquared()) / rdfBinSize);
					rdfInterval = std::min(rdfInterval, threadLocalRDF.size() - 1);
					threadLocalRDF[rdfInterval]++;
				}

				i++;

				// Update progress indicator.
				if((i % 1000) == 0)
					incrementProgressValue();
				// Abort loop when operation was canceled by the user.
				if(isCanceled())
					return;
			}
			std::lock_guard<std::mutex> lock(mutex);
			auto iter_out = rdfHistogram.begin();
			for(auto iter = threadLocalRDF.cbegin(); iter != threadLocalRDF.cend(); ++iter, ++iter_out)
				*iter_out += *iter;
		}));
		startIndex = endIndex;
		endIndex += chunkSize;
	}

	for(auto& t : workers)
		t.waitForFinished();

	// Normalize RDF.
	_results->rdfX().resize(_rdfSampleCount);
	_results->rdfY().resize(_rdfSampleCount);
	if(!cell().is2D()) {
		double rho = positions()->size() / cell().volume3D();
		double constant = 4.0/3.0 * FLOATTYPE_PI * rho * positions()->size();
		double stepSize = cutoff() / _rdfSampleCount;
		for(int i = 0; i < _rdfSampleCount; i++) {
			double r = stepSize * i;
			double r2 = r + stepSize;
			_results->rdfX()[i] = r + 0.5 * stepSize;
			_results->rdfY()[i] = rdfHistogram[i] / (constant * (r2*r2*r2 - r*r*r));
		}
	}
	else {
		double rho = positions()->size() / cell().volume2D();
		double constant = FLOATTYPE_PI * rho * positions()->size();
		double stepSize = cutoff() / _rdfSampleCount;
		for(int i = 0; i < _rdfSampleCount; i++) {
			double r = stepSize * i;
			double r2 = r + stepSize;
			_results->rdfX()[i] = r + 0.5 * stepSize;
			_results->rdfY()[i] = rdfHistogram[i] / (constant * (r2*r2 - r*r));
		}
	}

	// Return the results of the compute engine.
	setResult(std::move(_results));		
}

/******************************************************************************
* Injects the computed results of the engine into the data pipeline.
******************************************************************************/
PipelineFlowState CoordinationNumberModifier::CoordinationAnalysisResults::apply(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input)
{
	PipelineFlowState output = input;
	ParticleOutputHelper poh(modApp->dataset(), output);
	if(coordinationNumbers()->size() != poh.outputParticleCount())
		modApp->throwException(tr("Cached modifier results are obsolete, because the number of input particles has changed."));
	poh.outputProperty<ParticleProperty>(coordinationNumbers());

	// Store the RDF data points in the ModifierApplication.
	static_object_cast<CoordinationNumberModifierApplication>(modApp)->setRDF(rdfX(), rdfY());
		
	return output;
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
