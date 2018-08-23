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
#include <plugins/particles/util/NearestNeighborFinder.h>
#include <plugins/stdobj/properties/PropertyStorage.h>
#include <plugins/stdobj/series/DataSeriesObject.h>
#include <plugins/stdobj/series/DataSeriesProperty.h>
#include <plugins/stdobj/simcell/SimulationCellObject.h>
#include <plugins/particles/modifier/ParticleInputHelper.h>
#include <plugins/particles/modifier/ParticleOutputHelper.h>
#include <core/utilities/concurrent/ParallelFor.h>
#include <core/utilities/concurrent/PromiseState.h>
#include <core/dataset/pipeline/ModifierApplication.h>
#include "PolyhedralTemplateMatchingModifier.h"

#include <ptm/ptm_functions.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

IMPLEMENT_OVITO_CLASS(PolyhedralTemplateMatchingModifier);
DEFINE_PROPERTY_FIELD(PolyhedralTemplateMatchingModifier, rmsdCutoff);
DEFINE_PROPERTY_FIELD(PolyhedralTemplateMatchingModifier, outputRmsd);
DEFINE_PROPERTY_FIELD(PolyhedralTemplateMatchingModifier, outputInteratomicDistance);
DEFINE_PROPERTY_FIELD(PolyhedralTemplateMatchingModifier, outputOrientation);
DEFINE_PROPERTY_FIELD(PolyhedralTemplateMatchingModifier, outputDeformationGradient);
DEFINE_PROPERTY_FIELD(PolyhedralTemplateMatchingModifier, outputOrderingTypes);
DEFINE_REFERENCE_FIELD(PolyhedralTemplateMatchingModifier, orderingTypes);
SET_PROPERTY_FIELD_LABEL(PolyhedralTemplateMatchingModifier, rmsdCutoff, "RMSD cutoff");
SET_PROPERTY_FIELD_LABEL(PolyhedralTemplateMatchingModifier, outputRmsd, "Output RMSD values");
SET_PROPERTY_FIELD_LABEL(PolyhedralTemplateMatchingModifier, outputInteratomicDistance, "Output interatomic distance");
SET_PROPERTY_FIELD_LABEL(PolyhedralTemplateMatchingModifier, outputOrientation, "Output lattice orientations");
SET_PROPERTY_FIELD_LABEL(PolyhedralTemplateMatchingModifier, outputDeformationGradient, "Output deformation gradients");
SET_PROPERTY_FIELD_LABEL(PolyhedralTemplateMatchingModifier, outputOrderingTypes, "Output ordering types");
SET_PROPERTY_FIELD_LABEL(PolyhedralTemplateMatchingModifier, orderingTypes, "Ordering types");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(PolyhedralTemplateMatchingModifier, rmsdCutoff, FloatParameterUnit, 0);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
PolyhedralTemplateMatchingModifier::PolyhedralTemplateMatchingModifier(DataSet* dataset) : StructureIdentificationModifier(dataset),
		_rmsdCutoff(0.1), 
		_outputRmsd(false), 
		_outputInteratomicDistance(false),
		_outputOrientation(false), 
		_outputDeformationGradient(false), 
		_outputOrderingTypes(false)
{
	// Define the structure types.
	createStructureType(OTHER, ParticleType::PredefinedStructureType::OTHER);
	createStructureType(FCC, ParticleType::PredefinedStructureType::FCC);
	createStructureType(HCP, ParticleType::PredefinedStructureType::HCP);
	createStructureType(BCC, ParticleType::PredefinedStructureType::BCC);
	createStructureType(ICO, ParticleType::PredefinedStructureType::ICO)->setEnabled(false);
	createStructureType(SC, ParticleType::PredefinedStructureType::SC)->setEnabled(false);
	createStructureType(CUBIC_DIAMOND, ParticleType::PredefinedStructureType::CUBIC_DIAMOND)->setEnabled(false);
	createStructureType(HEX_DIAMOND, ParticleType::PredefinedStructureType::HEX_DIAMOND)->setEnabled(false);

	// Define the ordering types.
	for(int id = 0; id < NUM_ORDERING_TYPES; id++) {
		OORef<ParticleType> otype = new ParticleType(dataset);
		otype->setId(id);
		otype->setColor({0.75f, 0.75f, 0.75f});
		_orderingTypes.push_back(this, PROPERTY_FIELD(orderingTypes), std::move(otype));	
	}
	orderingTypes()[ORDERING_NONE]->setColor({0.95f, 0.95f, 0.95f});
	orderingTypes()[ORDERING_NONE]->setName(tr("Other"));
	orderingTypes()[ORDERING_PURE]->setName(tr("Pure"));
	orderingTypes()[ORDERING_L10]->setName(tr("L10"));
	orderingTypes()[ORDERING_L12_A]->setName(tr("L12 (A-site)"));
	orderingTypes()[ORDERING_L12_B]->setName(tr("L12 (B-site)"));
	orderingTypes()[ORDERING_B2]->setName(tr("B2"));
	orderingTypes()[ORDERING_ZINCBLENDE_WURTZITE]->setName(tr("Zincblende/Wurtzite"));
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
		throwException(tr("The number of structure types has changed. Please remove this modifier from the data pipeline and insert it again."));

	// Get modifier input.
	ParticleInputHelper pih(dataset(), input);
	ParticleProperty* posProperty = pih.expectStandardProperty<ParticleProperty>(ParticleProperty::PositionProperty);
	SimulationCellObject* simCell = pih.expectSimulationCell();
	if(simCell->is2D())
		throwException(tr("The PTM modifier does not support 2D simulation cells."));

	// Get particle selection.
	ConstPropertyPtr selectionProperty;
	if(onlySelectedParticles())
		selectionProperty = pih.expectStandardProperty<ParticleProperty>(ParticleProperty::SelectionProperty)->storage();

	// Get particle types if needed.
	ConstPropertyPtr typeProperty;
	if(outputOrderingTypes())
		typeProperty = pih.expectStandardProperty<ParticleProperty>(ParticleProperty::TypeProperty)->storage();

	// Initialize PTM library.
	ptm_initialize_global();

	return std::make_shared<PTMEngine>(posProperty->storage(), input, std::move(typeProperty), simCell->data(),
			getTypesToIdentify(NUM_STRUCTURE_TYPES), std::move(selectionProperty),
			outputInteratomicDistance(), outputOrientation(), outputDeformationGradient(), outputOrderingTypes());
}

/******************************************************************************
* Performs the actual analysis. This method is executed in a worker thread.
******************************************************************************/
void PolyhedralTemplateMatchingModifier::PTMEngine::perform()
{
	task()->setProgressText(tr("Performing polyhedral template matching"));

	// Prepare the neighbor list.
	NearestNeighborFinder neighFinder(MAX_NEIGHBORS);
	if(!neighFinder.prepare(*positions(), cell(), selection().get(), task().get()))
		return;

	task()->setProgressValue(0);
	task()->setProgressMaximum(positions()->size());

	// Perform analysis on each particle.
	parallelForChunks(positions()->size(), *task(), [this, &neighFinder](size_t startIndex, size_t count, PromiseState& promise) {

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
				structures()->setInt(index, OTHER);
				rmsd()->setFloat(index, 0);
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

			// Build list of particle types for ordering identification.
			if(orderingTypes()) {
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
			ptm_index(ptm_local_handle, flags, numNeighbors + 1, points, orderingTypes() ? atomTypes : nullptr, true,
					&type, &alloy_type, &scale, &rmsd, q,
					deformationGradients() ? F : nullptr,
					deformationGradients() ? F_res : nullptr,
					nullptr, nullptr, nullptr, &interatomic_distance, nullptr);


			// Convert PTM classification to our own scheme and store computed quantities.
			if(type == PTM_MATCH_NONE) {
				structures()->setInt(index, OTHER);
				this->rmsd()->setFloat(index, 0);
			}
			else {
				if(type == PTM_MATCH_SC) structures()->setInt(index, SC);
				else if(type == PTM_MATCH_FCC) structures()->setInt(index, FCC);
				else if(type == PTM_MATCH_HCP) structures()->setInt(index, HCP);
				else if(type == PTM_MATCH_ICO) structures()->setInt(index, ICO);
				else if(type == PTM_MATCH_BCC) structures()->setInt(index, BCC);
				else if(type == PTM_MATCH_DCUB) structures()->setInt(index, CUBIC_DIAMOND);
				else if(type == PTM_MATCH_DHEX) structures()->setInt(index, HEX_DIAMOND);
				else OVITO_ASSERT(false);
				this->rmsd()->setFloat(index, rmsd);
				if(interatomicDistances()) interatomicDistances()->setFloat(index, interatomic_distance);
				if(orientations()) orientations()->setQuaternion(index, Quaternion((FloatType)q[1], (FloatType)q[2], (FloatType)q[3], (FloatType)q[0]));
				if(deformationGradients()) {
					for(size_t j = 0; j < 9; j++)
						deformationGradients()->setFloatComponent(index, j, (FloatType)F[j]);
				}
			}
			if(orderingTypes())
				orderingTypes()->setInt(index, alloy_type);
		}

		// Release thread-local storage of PTM routine.
		ptm_uninitialize_local(ptm_local_handle);
	});
	if(task()->isCanceled() || positions()->size() == 0)
		return;

	// Determine histogram bin size based on maximum RMSD value.
	const size_t numHistogramBins = 100;
	_rmsdHistogram = std::make_shared<PropertyStorage>(numHistogramBins, PropertyStorage::Int64, 1, 0, tr("Count"), true, DataSeriesProperty::YProperty); 
	FloatType rmsdHistogramBinSize = FloatType(1.01) * *std::max_element(rmsd()->constDataFloat(), rmsd()->constDataFloat() + rmsd()->size()) / numHistogramBins;
	if(rmsdHistogramBinSize <= 0) rmsdHistogramBinSize = 1;
	_rmsdHistogramRange = rmsdHistogramBinSize * numHistogramBins;

	// Perform binning of RMSD values.
	for(size_t index = 0; index < structures()->size(); index++) {
		if(structures()->getInt(index) != OTHER) {
			OVITO_ASSERT(rmsd()->getFloat(index) >= 0);
			int binIndex = rmsd()->getFloat(index) / rmsdHistogramBinSize;
			if(binIndex < numHistogramBins)
				_rmsdHistogram->dataInt64()[binIndex]++;
		}
	}
}

/******************************************************************************
* Injects the computed results of the engine into the data pipeline.
******************************************************************************/
PropertyPtr PolyhedralTemplateMatchingModifier::PTMEngine::postProcessStructureTypes(TimePoint time, ModifierApplication* modApp, const PropertyPtr& structures)
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
PipelineFlowState PolyhedralTemplateMatchingModifier::PTMEngine::emitResults(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input)
{
	PipelineFlowState output = StructureIdentificationEngine::emitResults(time, modApp, input);
	ParticleOutputHelper poh(modApp->dataset(), output, modApp);
	
	// Also output structure type counts, which have been computed by the base class.
	poh.outputAttribute(QStringLiteral("PolyhedralTemplateMatching.counts.OTHER"), QVariant::fromValue(getTypeCount(OTHER)));
	poh.outputAttribute(QStringLiteral("PolyhedralTemplateMatching.counts.FCC"), QVariant::fromValue(getTypeCount(FCC)));
	poh.outputAttribute(QStringLiteral("PolyhedralTemplateMatching.counts.HCP"), QVariant::fromValue(getTypeCount(HCP)));
	poh.outputAttribute(QStringLiteral("PolyhedralTemplateMatching.counts.BCC"), QVariant::fromValue(getTypeCount(BCC)));
	poh.outputAttribute(QStringLiteral("PolyhedralTemplateMatching.counts.ICO"), QVariant::fromValue(getTypeCount(ICO)));
	poh.outputAttribute(QStringLiteral("PolyhedralTemplateMatching.counts.SC"), QVariant::fromValue(getTypeCount(SC)));
	poh.outputAttribute(QStringLiteral("PolyhedralTemplateMatching.counts.CUBIC_DIAMOND"), QVariant::fromValue(getTypeCount(CUBIC_DIAMOND)));
	poh.outputAttribute(QStringLiteral("PolyhedralTemplateMatching.counts.HEX_DIAMOND"), QVariant::fromValue(getTypeCount(HEX_DIAMOND)));

	PolyhedralTemplateMatchingModifier* modifier = static_object_cast<PolyhedralTemplateMatchingModifier>(modApp->modifier());
	OVITO_ASSERT(modifier);

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
	if(orderingTypes() && modifier->outputOrderingTypes()) {
		ParticleProperty* orderingProperty = poh.outputProperty<ParticleProperty>(orderingTypes());
		// Attach ordering types to output particle property.
		orderingProperty->setElementTypes(modifier->orderingTypes());		
	}

	// Output RMSD histogram.
	DataSeriesObject* seriesObj = poh.outputDataSeries(QStringLiteral("ptm/rmsd"), tr("RMSD distribution"), rmsdHistogram());
	seriesObj->setAxisLabelX(tr("RMSD"));
	seriesObj->setIntervalStart(0);
	seriesObj->setIntervalEnd(rmsdHistogramRange());

	return output;
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
