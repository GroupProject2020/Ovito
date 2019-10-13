////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2018 Alexander Stukowski
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
#include <ovito/stdobj/properties/PropertyStorage.h>
#include <ovito/stdobj/series/DataSeriesObject.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/core/utilities/concurrent/ParallelFor.h>
#include <ovito/core/utilities/concurrent/Task.h>
#include <ovito/core/utilities/units/UnitsManager.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>
#include <ovito/core/dataset/DataSet.h>
#include "PolyhedralTemplateMatchingModifier.h"

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
	createStructureType(PTMAlgorithm::OTHER, ParticleType::PredefinedStructureType::OTHER);
	createStructureType(PTMAlgorithm::FCC, ParticleType::PredefinedStructureType::FCC);
	createStructureType(PTMAlgorithm::HCP, ParticleType::PredefinedStructureType::HCP);
	createStructureType(PTMAlgorithm::BCC, ParticleType::PredefinedStructureType::BCC);
	createStructureType(PTMAlgorithm::ICO, ParticleType::PredefinedStructureType::ICO)->setEnabled(false);
	createStructureType(PTMAlgorithm::SC, ParticleType::PredefinedStructureType::SC)->setEnabled(false);
	createStructureType(PTMAlgorithm::CUBIC_DIAMOND, ParticleType::PredefinedStructureType::CUBIC_DIAMOND)->setEnabled(false);
	createStructureType(PTMAlgorithm::HEX_DIAMOND, ParticleType::PredefinedStructureType::HEX_DIAMOND)->setEnabled(false);
	createStructureType(PTMAlgorithm::GRAPHENE, ParticleType::PredefinedStructureType::GRAPHENE)->setEnabled(false);

	// Define the ordering types.
	for(int id = 0; id < PTMAlgorithm::NUM_ORDERING_TYPES; id++) {
		OORef<ParticleType> otype = new ParticleType(dataset);
		otype->setNumericId(id);
		otype->setColor({0.75f, 0.75f, 0.75f});
		_orderingTypes.push_back(this, PROPERTY_FIELD(orderingTypes), std::move(otype));
	}
	orderingTypes()[PTMAlgorithm::ORDERING_NONE]->setColor({0.95f, 0.95f, 0.95f});
	orderingTypes()[PTMAlgorithm::ORDERING_NONE]->setName(tr("Other"));
	orderingTypes()[PTMAlgorithm::ORDERING_PURE]->setName(tr("Pure"));
	orderingTypes()[PTMAlgorithm::ORDERING_L10]->setName(tr("L10"));
	orderingTypes()[PTMAlgorithm::ORDERING_L12_A]->setName(tr("L12 (A-site)"));
	orderingTypes()[PTMAlgorithm::ORDERING_L12_B]->setName(tr("L12 (B-site)"));
	orderingTypes()[PTMAlgorithm::ORDERING_B2]->setName(tr("B2"));
	orderingTypes()[PTMAlgorithm::ORDERING_ZINCBLENDE_WURTZITE]->setName(tr("Zincblende/Wurtzite"));
	orderingTypes()[PTMAlgorithm::ORDERING_BORON_NITRIDE]->setName(tr("Boron/Nitride"));
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
	if(structureTypes().size() != PTMAlgorithm::NUM_STRUCTURE_TYPES)
		throwException(tr("The number of structure types has changed. Please remove this modifier from the data pipeline and insert it again."));

	// Get modifier input.
	const ParticlesObject* particles = input.expectObject<ParticlesObject>();
	const PropertyObject* posProperty = particles->expectProperty(ParticlesObject::PositionProperty);
	const SimulationCellObject* simCell = input.expectObject<SimulationCellObject>();
	if(simCell->is2D())
		throwException(tr("The PTM modifier does not support 2D simulation cells."));

	// Get particle selection.
	ConstPropertyPtr selectionProperty;
	if(onlySelectedParticles())
		selectionProperty = particles->expectProperty(ParticlesObject::SelectionProperty)->storage();

	// Get particle types if needed.
	ConstPropertyPtr typeProperty;
	if(outputOrderingTypes())
		typeProperty = particles->expectProperty(ParticlesObject::TypeProperty)->storage();

	return std::make_shared<PTMEngine>(posProperty->storage(), particles, std::move(typeProperty), simCell->data(),
			getTypesToIdentify(PTMAlgorithm::NUM_STRUCTURE_TYPES), std::move(selectionProperty),
			outputInteratomicDistance(), outputOrientation(), outputDeformationGradient());
}

/******************************************************************************
* Performs the actual analysis. This method is executed in a worker thread.
******************************************************************************/
void PolyhedralTemplateMatchingModifier::PTMEngine::perform()
{
//TODO: separate pre-calculation of neighbor ordering, so that we don't have to call it again unless the pipeline has changed.
//	i.e.: if user adds the option "Output RMSD", the old neighbor ordering is still valid.

	if(cell().is2D())
		throw Exception(tr("The PTM modifier does not support 2D simulation cells."));

	// Specify the structure types the PTM should look for.
	for(int i = 0; i < typesToIdentify().size() && i < PTMAlgorithm::NUM_STRUCTURE_TYPES; i++) {
		_algorithm->setStructureTypeIdentification(static_cast<PTMAlgorithm::StructureType>(i), typesToIdentify()[i]);
	}

	// Initialize the algorithm object.
	if(!_algorithm->prepare(*positions(), cell(), selection().get(), task().get()))
		return;

	task()->setProgressValue(0);
	task()->setProgressMaximum(positions()->size());
	task()->setProgressText(tr("Pre-calculating neighbor ordering"));

	// Pre-order neighbors of each particle
	std::vector< uint64_t > cachedNeighbors(positions()->size());
	parallelForChunks(positions()->size(), *task(), [this, &cachedNeighbors](size_t startIndex, size_t count, Task& task) {
		// Create a thread-local kernel for the PTM algorithm.
		PTMAlgorithm::Kernel kernel(*_algorithm);

//size_t startIndex = 0, count = positions()->size();

		// Loop over input particles.
		size_t endIndex = startIndex + count;
		for(size_t index = startIndex; index < endIndex; index++) {

			// Update progress indicator.
			if((index % 256) == 0)
				task.incrementProgressValue(256);

			// Break out of loop when operation was canceled.
			if(task.isCanceled())
				break;

			// Skip particles that are not included in the analysis.
			if(selection() && !selection()->getInt(index)) {
				structures()->setInt(index, PTMAlgorithm::OTHER);
				rmsd()->setFloat(index, 0);
				continue;
			}

			// Calculate ordering of neighbors
			kernel.precacheNeighbors(index, &cachedNeighbors[index]);
		}
	});
	if(task()->isCanceled() || positions()->size() == 0)
		return;

	task()->setProgressValue(0);
	task()->setProgressText(tr("Performing polyhedral template matching"));

	// Perform analysis on each particle.
	parallelForChunks(positions()->size(), *task(), [this, &cachedNeighbors](size_t startIndex, size_t count, Task& task) {

		// Create a thread-local kernel for the PTM algorithm.
		PTMAlgorithm::Kernel kernel(*_algorithm);

//size_t startIndex = 0, count = positions()->size();
		// Loop over input particles.
		size_t endIndex = startIndex + count;
		for(size_t index = startIndex; index < endIndex; index++) {

			// Update progress indicator.
			if((index % 256) == 0)
				task.incrementProgressValue(256);

			// Break out of loop when operation was canceled.
			if(task.isCanceled())
				break;

			// Skip particles that are not included in the analysis.
			if(selection() && !selection()->getInt(index)) {
				structures()->setInt(index, PTMAlgorithm::OTHER);
				rmsd()->setFloat(index, 0);
				continue;
			}

			// Perform the PTM analysis for the current particle.
			PTMAlgorithm::StructureType type = kernel.identifyStructure(index, cachedNeighbors, nullptr);

			// Store results in the output arrays.
			structures()->setInt(index, type);
			rmsd()->setFloat(index, kernel.rmsd());
			if(type != PTMAlgorithm::OTHER) {
				if(interatomicDistances()) interatomicDistances()->setFloat(index, kernel.interatomicDistance());
				if(orientations()) orientations()->setQuaternion(index, kernel.orientation());
				if(deformationGradients()) deformationGradients()->setMatrix3(index, kernel.deformationGradient());
				if(orderingTypes()) orderingTypes()->setInt(index, kernel.orderingType());
			}
		}
	});
	if(task()->isCanceled() || positions()->size() == 0)
		return;

	// Determine histogram bin size based on maximum RMSD value.
	const size_t numHistogramBins = 100;
	_rmsdHistogram = std::make_shared<PropertyStorage>(numHistogramBins, PropertyStorage::Int64, 1, 0, tr("Count"), true, DataSeriesObject::YProperty);
	FloatType rmsdHistogramBinSize = FloatType(1.01) * *std::max_element(rmsd()->constDataFloat(), rmsd()->constDataFloat() + rmsd()->size()) / numHistogramBins;
	if(rmsdHistogramBinSize <= 0) rmsdHistogramBinSize = 1;
	_rmsdHistogramRange = rmsdHistogramBinSize * numHistogramBins;

	// Perform binning of RMSD values.
	for(size_t index = 0; index < structures()->size(); index++) {
		if(structures()->getInt(index) != PTMAlgorithm::OTHER) {
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
				finalStructureTypes->setInt(i, PTMAlgorithm::OTHER);
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
void PolyhedralTemplateMatchingModifier::PTMEngine::emitResults(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state)
{
	StructureIdentificationEngine::emitResults(time, modApp, state);

	// Also output structure type counts, which have been computed by the base class.
	state.addAttribute(QStringLiteral("PolyhedralTemplateMatching.counts.OTHER"), QVariant::fromValue(getTypeCount(PTMAlgorithm::OTHER)), modApp);
	state.addAttribute(QStringLiteral("PolyhedralTemplateMatching.counts.FCC"), QVariant::fromValue(getTypeCount(PTMAlgorithm::FCC)), modApp);
	state.addAttribute(QStringLiteral("PolyhedralTemplateMatching.counts.HCP"), QVariant::fromValue(getTypeCount(PTMAlgorithm::HCP)), modApp);
	state.addAttribute(QStringLiteral("PolyhedralTemplateMatching.counts.BCC"), QVariant::fromValue(getTypeCount(PTMAlgorithm::BCC)), modApp);
	state.addAttribute(QStringLiteral("PolyhedralTemplateMatching.counts.ICO"), QVariant::fromValue(getTypeCount(PTMAlgorithm::ICO)), modApp);
	state.addAttribute(QStringLiteral("PolyhedralTemplateMatching.counts.SC"), QVariant::fromValue(getTypeCount(PTMAlgorithm::SC)), modApp);
	state.addAttribute(QStringLiteral("PolyhedralTemplateMatching.counts.CUBIC_DIAMOND"), QVariant::fromValue(getTypeCount(PTMAlgorithm::CUBIC_DIAMOND)), modApp);
	state.addAttribute(QStringLiteral("PolyhedralTemplateMatching.counts.HEX_DIAMOND"), QVariant::fromValue(getTypeCount(PTMAlgorithm::HEX_DIAMOND)), modApp);
	state.addAttribute(QStringLiteral("PolyhedralTemplateMatching.counts.GRAPHENE"), QVariant::fromValue(getTypeCount(PTMAlgorithm::GRAPHENE)), modApp);

	PolyhedralTemplateMatchingModifier* modifier = static_object_cast<PolyhedralTemplateMatchingModifier>(modApp->modifier());
	OVITO_ASSERT(modifier);
	ParticlesObject* particles = state.expectMutableObject<ParticlesObject>();

	// Output per-particle properties.
	if(rmsd() && modifier->outputRmsd()) {
		particles->createProperty(rmsd());
	}
	if(interatomicDistances() && modifier->outputInteratomicDistance()) {
		particles->createProperty(interatomicDistances());
	}
	if(orientations() && modifier->outputOrientation()) {
		particles->createProperty(orientations());
	}
	if(deformationGradients() && modifier->outputDeformationGradient()) {
		particles->createProperty(deformationGradients());
	}
	if(orderingTypes() && modifier->outputOrderingTypes()) {
		PropertyObject* orderingProperty = particles->createProperty(orderingTypes());
		// Attach ordering types to output particle property.
		orderingProperty->setElementTypes(modifier->orderingTypes());
	}

	// Output RMSD histogram.
	DataSeriesObject* seriesObj = state.createObject<DataSeriesObject>(QStringLiteral("ptm-rmsd"), modApp, DataSeriesObject::Line, tr("RMSD distribution"), rmsdHistogram());
	seriesObj->setAxisLabelX(tr("RMSD"));
	seriesObj->setIntervalStart(0);
	seriesObj->setIntervalEnd(rmsdHistogramRange());
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
