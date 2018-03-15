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
#include <plugins/stdobj/properties/PropertyStorage.h>
#include <plugins/particles/modifier/ParticleInputHelper.h>
#include <plugins/particles/modifier/ParticleOutputHelper.h>
#include <core/utilities/concurrent/ParallelFor.h>
#include <core/utilities/concurrent/PromiseState.h>
#include <core/dataset/pipeline/ModifierApplication.h>
#include <plugins/stdobj/simcell/SimulationCellObject.h>
#include "PolyhedralTemplateMatchingModifier.h"

#include <ptm/ptm_functions.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

IMPLEMENT_OVITO_CLASS(PolyhedralTemplateMatchingModifier);
IMPLEMENT_OVITO_CLASS(PolyhedralTemplateMatchingModifierApplication);
DEFINE_PROPERTY_FIELD(PolyhedralTemplateMatchingModifier, rmsdCutoff);
DEFINE_PROPERTY_FIELD(PolyhedralTemplateMatchingModifier, outputRmsd);
DEFINE_PROPERTY_FIELD(PolyhedralTemplateMatchingModifier, outputInteratomicDistance);
DEFINE_PROPERTY_FIELD(PolyhedralTemplateMatchingModifier, outputOrientation);
DEFINE_PROPERTY_FIELD(PolyhedralTemplateMatchingModifier, outputDeformationGradient);
DEFINE_PROPERTY_FIELD(PolyhedralTemplateMatchingModifier, outputAlloyTypes);
SET_PROPERTY_FIELD_LABEL(PolyhedralTemplateMatchingModifier, rmsdCutoff, "RMSD cutoff");
SET_PROPERTY_FIELD_LABEL(PolyhedralTemplateMatchingModifier, outputRmsd, "Output RMSD values");
SET_PROPERTY_FIELD_LABEL(PolyhedralTemplateMatchingModifier, outputInteratomicDistance, "Output interatomic distance");
SET_PROPERTY_FIELD_LABEL(PolyhedralTemplateMatchingModifier, outputOrientation, "Output orientations");
SET_PROPERTY_FIELD_LABEL(PolyhedralTemplateMatchingModifier, outputDeformationGradient, "Output deformation gradients");
SET_PROPERTY_FIELD_LABEL(PolyhedralTemplateMatchingModifier, outputAlloyTypes, "Output alloy types");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(PolyhedralTemplateMatchingModifier, rmsdCutoff, FloatParameterUnit, 0);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
PolyhedralTemplateMatchingModifier::PolyhedralTemplateMatchingModifier(DataSet* dataset) : StructureIdentificationModifier(dataset),
		_rmsdCutoff(0), 
		_outputRmsd(false), 
		_outputInteratomicDistance(false),
		_outputOrientation(false), 
		_outputDeformationGradient(false), 
		_outputAlloyTypes(false)
{
	// Define the structure types.
	createStructureType(OTHER, ParticleType::PredefinedStructureType::OTHER);
	createStructureType(FCC, ParticleType::PredefinedStructureType::FCC);
	createStructureType(HCP, ParticleType::PredefinedStructureType::HCP);
	createStructureType(BCC, ParticleType::PredefinedStructureType::BCC);
	createStructureType(ICO, ParticleType::PredefinedStructureType::ICO);
	createStructureType(SC, ParticleType::PredefinedStructureType::SC);
	createStructureType(CUBIC_DIAMOND, ParticleType::PredefinedStructureType::CUBIC_DIAMOND);
	createStructureType(HEX_DIAMOND, ParticleType::PredefinedStructureType::HEX_DIAMOND);
}

/******************************************************************************
* Creates a new modifier application that refers to this modifier instance.
******************************************************************************/
OORef<ModifierApplication> PolyhedralTemplateMatchingModifier::createModifierApplication()
{
	OORef<ModifierApplication> modApp = new PolyhedralTemplateMatchingModifierApplication(dataset());
	modApp->setModifier(this);
	return modApp;
}

/******************************************************************************
* Is called when the value of a property of this object has changed.
******************************************************************************/
void PolyhedralTemplateMatchingModifier::propertyChanged(const PropertyFieldDescriptor& field)
{
	if(field == PROPERTY_FIELD(rmsdCutoff)) {
		// Immediately update viewports when RMSD cutoff has been changed by the user.
		notifyDependents(ReferenceEvent::PreliminaryStateAvailable);
	}
	StructureIdentificationModifier::propertyChanged(field);
}

/******************************************************************************
* Creates and initializes a computation engine that will compute the modifier's results.
******************************************************************************/
Future<AsynchronousModifier::ComputeEnginePtr> PolyhedralTemplateMatchingModifier::createEngine(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input)
{
	if(structureTypes().size() != NUM_STRUCTURE_TYPES)
		throwException(tr("The number of structure types has changed. Please remove this modifier from the modification pipeline and insert it again."));

	// Get modifier input.
	ParticleInputHelper pih(dataset(), input);
	ParticleProperty* posProperty = pih.expectStandardProperty<ParticleProperty>(ParticleProperty::PositionProperty);
	SimulationCellObject* simCell = pih.expectSimulationCell();
	if(simCell->is2D())
		throwException(tr("The PTM modifier does not support 2d simulation cells."));

	// Get particle selection.
	ConstPropertyPtr selectionProperty;
	if(onlySelectedParticles())
		selectionProperty = pih.expectStandardProperty<ParticleProperty>(ParticleProperty::SelectionProperty)->storage();

	// Get particle types.
	ConstPropertyPtr typeProperty;
	if(outputAlloyTypes())
		typeProperty = pih.expectStandardProperty<ParticleProperty>(ParticleProperty::TypeProperty)->storage();

	// Initialize PTM library.
	ptm_initialize_global();

	return std::make_shared<PTMEngine>(posProperty->storage(), std::move(typeProperty), simCell->data(),
			getTypesToIdentify(NUM_STRUCTURE_TYPES), std::move(selectionProperty),
			outputInteratomicDistance(), outputOrientation(), outputDeformationGradient(), outputAlloyTypes());
}

/******************************************************************************
* Performs the actual analysis. This method is executed in a worker thread.
******************************************************************************/
void PolyhedralTemplateMatchingModifier::PTMEngine::perform()
{
	setProgressText(tr("Performing polyhedral template matching"));

	// Prepare the neighbor list.
	NearestNeighborFinder neighFinder(MAX_NEIGHBORS);
	if(!neighFinder.prepare(*positions(), cell(), selection().get(), this))
		return;

	setProgressValue(0);
	setProgressMaximum(positions()->size());

	// Perform analysis on each particle.
	parallelForChunks(positions()->size(), *this, [this, &neighFinder](size_t startIndex, size_t count, PromiseState& promise) {

		// Initialize thread-local storage for PTM routine.
		ptm_local_handle_t ptm_local_handle = ptm_initialize_local();

		size_t endIndex = startIndex + count;
		for(size_t index = startIndex; index < endIndex; index++) {

			// Update progress indicator.
			if((index % 256) == 0)
				promise.incrementProgressValue(256);

			// Break out of loop when operation was canceled.
			if(promise.isCanceled())
				break;

			// Skip particles that are not included in the analysis.
			if(selection() && !selection()->getInt(index)) {
				_results->structures()->setInt(index, OTHER);
				_results->rmsd()->setFloat(index, 0);
				continue;
			}

			// Find nearest neighbors.
			NearestNeighborFinder::Query<MAX_NEIGHBORS> neighQuery(neighFinder);
			neighQuery.findNeighbors(index);
			int numNeighbors = neighQuery.results().size();
			OVITO_ASSERT(numNeighbors <= MAX_NEIGHBORS);

			// Bring neighbor coordinates into a form suitable for the PTM library.
			double points[MAX_NEIGHBORS+1][3];
			int32_t atomTypes[MAX_NEIGHBORS+1];
			points[0][0] = points[0][1] = points[0][2] = 0;
			for(int i = 0; i < numNeighbors; i++) {
				points[i+1][0] = neighQuery.results()[i].delta.x();
				points[i+1][1] = neighQuery.results()[i].delta.y();
				points[i+1][2] = neighQuery.results()[i].delta.z();
			}

			// Build list of particle types for alloy structure identification.
			if(_results->alloyTypes()) {
				atomTypes[0] = _particleTypes->getInt(index);
				for(int i = 0; i < numNeighbors; i++) {
					atomTypes[i + 1] = _particleTypes->getInt(neighQuery.results()[i].index);
				}
			}

			// Determine which structures to look for. This depends on how
			// much neighbors are present.
			int32_t flags = 0;
			if(numNeighbors >= 6 && typesToIdentify()[SC]) flags |= PTM_CHECK_SC;
			if(numNeighbors >= 12) {
				if(typesToIdentify()[FCC]) flags |= PTM_CHECK_FCC;
				if(typesToIdentify()[HCP]) flags |= PTM_CHECK_HCP;
				if(typesToIdentify()[ICO]) flags |= PTM_CHECK_ICO;
			}
			if(numNeighbors >= 14 && typesToIdentify()[BCC]) flags |= PTM_CHECK_BCC;

			if(numNeighbors >= 16) {
				if(typesToIdentify()[CUBIC_DIAMOND]) flags |= PTM_CHECK_DCUB;
				if(typesToIdentify()[HEX_DIAMOND]) flags |= PTM_CHECK_DHEX;
			}

			// Call PTM library to identify local structure.
			int32_t type, alloy_type = PTM_ALLOY_NONE;
			double scale, interatomic_distance;
			double rmsd;
			double q[4];
			double F[9], F_res[3];
			ptm_index(ptm_local_handle, flags, numNeighbors + 1, points, _results->alloyTypes() ? atomTypes : nullptr, true,
					&type, &alloy_type, &scale, &rmsd, q,
					_results->deformationGradients() ? F : nullptr,
					_results->deformationGradients() ? F_res : nullptr,
					nullptr, nullptr, nullptr, &interatomic_distance, nullptr);


			// Convert PTM classification to our own scheme and store computed quantities.
			if(type == PTM_MATCH_NONE) {
				_results->structures()->setInt(index, OTHER);
				_results->rmsd()->setFloat(index, 0);
			}
			else {
				if(type == PTM_MATCH_SC) _results->structures()->setInt(index, SC);
				else if(type == PTM_MATCH_FCC) _results->structures()->setInt(index, FCC);
				else if(type == PTM_MATCH_HCP) _results->structures()->setInt(index, HCP);
				else if(type == PTM_MATCH_ICO) _results->structures()->setInt(index, ICO);
				else if(type == PTM_MATCH_BCC) _results->structures()->setInt(index, BCC);
				else if(type == PTM_MATCH_DCUB) _results->structures()->setInt(index, CUBIC_DIAMOND);
				else if(type == PTM_MATCH_DHEX) _results->structures()->setInt(index, HEX_DIAMOND);
				else OVITO_ASSERT(false);
				_results->rmsd()->setFloat(index, rmsd);
				if(_results->interatomicDistances()) _results->interatomicDistances()->setFloat(index, interatomic_distance);
				if(_results->orientations()) _results->orientations()->setQuaternion(index, Quaternion((FloatType)q[1], (FloatType)q[2], (FloatType)q[3], (FloatType)q[0]));
				if(_results->deformationGradients()) {
					for(size_t j = 0; j < 9; j++)
						_results->deformationGradients()->setFloatComponent(index, j, (FloatType)F[j]);
				}
			}
			if(_results->alloyTypes())
				_results->alloyTypes()->setInt(index, alloy_type);
		}

		// Release thread-local storage of PTM routine.
		ptm_uninitialize_local(ptm_local_handle);
	});
	if(isCanceled() || positions()->size() == 0)
		return;

	// Determine histogram bin size based on maximum RMSD value.
	QVector<int> rmsdHistogramData(100, 0);
	FloatType rmsdHistogramBinSize = FloatType(1.01) * *std::max_element(_results->rmsd()->constDataFloat(), _results->rmsd()->constDataFloat() + _results->rmsd()->size());
	rmsdHistogramBinSize /= rmsdHistogramData.size();
	if(rmsdHistogramBinSize <= 0) rmsdHistogramBinSize = 1;

	// Build RMSD histogram.
	for(size_t index = 0; index < _results->structures()->size(); index++) {
		if(_results->structures()->getInt(index) != OTHER) {
			OVITO_ASSERT(_results->rmsd()->getFloat(index) >= 0);
			int binIndex = _results->rmsd()->getFloat(index) / rmsdHistogramBinSize;
			if(binIndex < rmsdHistogramData.size())
				rmsdHistogramData[binIndex]++;
		}
	}
	_results->setRmsdHistogram(std::move(rmsdHistogramData), rmsdHistogramBinSize);

	// Return the results of the compute engine.
	setResult(std::move(_results));	
}

/******************************************************************************
* Injects the computed results of the engine into the data pipeline.
******************************************************************************/
PropertyPtr PolyhedralTemplateMatchingModifier::PTMResults::postProcessStructureTypes(TimePoint time, ModifierApplication* modApp, const PropertyPtr& structures)
{
	PolyhedralTemplateMatchingModifier* modifier = static_object_cast<PolyhedralTemplateMatchingModifier>(modApp->modifier());
	OVITO_ASSERT(modifier);

	// Enforce RMSD cutoff.
	if(modifier->rmsdCutoff() > 0 && rmsd()) {
		
		// Start off with the original particle classifications and make a copy.
		const PropertyPtr finalStructureTypes = std::make_shared<PropertyStorage>(*structures);

		// Mark those particles whose RMSD exceeds the cutoff as 'OTHER'.
		for(size_t i = 0; i < rmsd()->size(); i++) {
			if(rmsd()->getFloat(i) > modifier->rmsdCutoff())
				finalStructureTypes->setInt(i, OTHER);
		}

		// Replace old classifications with updated ones.
		return finalStructureTypes;
	}
	else {
		return structures;
	}
}

/******************************************************************************
* Injects the computed results of the engine into the data pipeline.
******************************************************************************/
PipelineFlowState PolyhedralTemplateMatchingModifier::PTMResults::apply(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input)
{
	PipelineFlowState output = StructureIdentificationResults::apply(time, modApp, input);
	
	// Also output structure type counts, which have been computed by the base class.
	StructureIdentificationModifierApplication* myModApp = static_object_cast<StructureIdentificationModifierApplication>(modApp);
	output.attributes().insert(QStringLiteral("PolyhedralTemplateMatching.counts.OTHER"), QVariant::fromValue(myModApp->structureCounts()[OTHER]));
	output.attributes().insert(QStringLiteral("PolyhedralTemplateMatching.counts.FCC"), QVariant::fromValue(myModApp->structureCounts()[FCC]));
	output.attributes().insert(QStringLiteral("PolyhedralTemplateMatching.counts.HCP"), QVariant::fromValue(myModApp->structureCounts()[HCP]));
	output.attributes().insert(QStringLiteral("PolyhedralTemplateMatching.counts.BCC"), QVariant::fromValue(myModApp->structureCounts()[BCC]));
	output.attributes().insert(QStringLiteral("PolyhedralTemplateMatching.counts.ICO"), QVariant::fromValue(myModApp->structureCounts()[ICO]));
	output.attributes().insert(QStringLiteral("PolyhedralTemplateMatching.counts.SC"), QVariant::fromValue(myModApp->structureCounts()[SC]));
	output.attributes().insert(QStringLiteral("PolyhedralTemplateMatching.counts.CUBIC_DIAMOND"), QVariant::fromValue(myModApp->structureCounts()[CUBIC_DIAMOND]));
	output.attributes().insert(QStringLiteral("PolyhedralTemplateMatching.counts.HEX_DIAMOND"), QVariant::fromValue(myModApp->structureCounts()[HEX_DIAMOND]));
	
	PolyhedralTemplateMatchingModifier* modifier = static_object_cast<PolyhedralTemplateMatchingModifier>(modApp->modifier());
	OVITO_ASSERT(modifier);

	ParticleOutputHelper poh(modApp->dataset(), output);

	// Output per-particle properties.
	if(rmsd() && modifier->outputRmsd()) {
		poh.outputProperty<ParticleProperty>(rmsd());
	}
	if(interatomicDistances() && modifier->outputInteratomicDistance()) {
		poh.outputProperty<ParticleProperty>(interatomicDistances());
	}
	if(orientations() && modifier->outputOrientation()) {
		poh.outputProperty<ParticleProperty>(orientations());
	}
	if(deformationGradients() && modifier->outputDeformationGradient()) {
		poh.outputProperty<ParticleProperty>(deformationGradients());
	}
	if(alloyTypes() && modifier->outputAlloyTypes()) {
		poh.outputProperty<ParticleProperty>(alloyTypes());
	}

	// Store the RMSD histogram in the ModifierApplication.
	static_object_cast<PolyhedralTemplateMatchingModifierApplication>(modApp)->setRmsdHistogram(rmsdHistogramData(), rmsdHistogramBinSize());
	
	return output;
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
