///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2013) Alexander Stukowski
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
#include <plugins/particles/util/CutoffNeighborFinder.h>
#include <plugins/particles/modifier/ParticleInputHelper.h>
#include <plugins/particles/modifier/ParticleOutputHelper.h>
#include <core/dataset/animation/AnimationSettings.h>
#include <core/dataset/pipeline/ModifierApplication.h>
#include <plugins/stdobj/simcell/SimulationCellObject.h>
#include <core/utilities/concurrent/ParallelFor.h>
#include <core/dataset/pipeline/AsynchronousModifier.h>
#include "ComputePropertyModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Properties)

IMPLEMENT_OVITO_CLASS(ComputePropertyModifier);
DEFINE_PROPERTY_FIELD(ComputePropertyModifier, expressions);
DEFINE_PROPERTY_FIELD(ComputePropertyModifier, outputProperty);
DEFINE_PROPERTY_FIELD(ComputePropertyModifier, onlySelectedParticles);
DEFINE_PROPERTY_FIELD(ComputePropertyModifier, neighborModeEnabled);
DEFINE_PROPERTY_FIELD(ComputePropertyModifier, neighborExpressions);
DEFINE_PROPERTY_FIELD(ComputePropertyModifier, cutoff);
SET_PROPERTY_FIELD_LABEL(ComputePropertyModifier, expressions, "Expressions");
SET_PROPERTY_FIELD_LABEL(ComputePropertyModifier, outputProperty, "Output property");
SET_PROPERTY_FIELD_LABEL(ComputePropertyModifier, onlySelectedParticles, "Compute only for selected particles");
SET_PROPERTY_FIELD_LABEL(ComputePropertyModifier, neighborModeEnabled, "Include neighbor terms");
SET_PROPERTY_FIELD_LABEL(ComputePropertyModifier, neighborExpressions, "Neighbor expressions");
SET_PROPERTY_FIELD_LABEL(ComputePropertyModifier, cutoff, "Cutoff radius");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(ComputePropertyModifier, cutoff, WorldParameterUnit, 0);

IMPLEMENT_OVITO_CLASS(ComputePropertyModifierApplication);
DEFINE_REFERENCE_FIELD(ComputePropertyModifierApplication, cachedDisplayObjects);

/******************************************************************************
* Constructs a new instance of this class.
******************************************************************************/
ComputePropertyModifier::ComputePropertyModifier(DataSet* dataset) : AsynchronousModifier(dataset),
	_outputProperty(tr("My property")), 
	_expressions(QStringList("0")), 
	_onlySelectedParticles(false),
	_neighborExpressions(QStringList("0")), 
	_cutoff(3), 
	_neighborModeEnabled(false)
{
}

/******************************************************************************
* Asks the modifier whether it can be applied to the given input data.
******************************************************************************/
bool ComputePropertyModifier::OOMetaClass::isApplicableTo(const PipelineFlowState& input) const
{
	return input.findObject<ParticleProperty>() != nullptr;
}

/******************************************************************************
* Create a new modifier application that refers to this modifier instance.
******************************************************************************/
OORef<ModifierApplication> ComputePropertyModifier::createModifierApplication()
{
	OORef<ModifierApplication> modApp = new ComputePropertyModifierApplication(dataset());
	modApp->setModifier(this);
	return modApp;
}

/******************************************************************************
* Sets the number of vector components of the property to create.
******************************************************************************/
void ComputePropertyModifier::setPropertyComponentCount(int newComponentCount)
{
	if(newComponentCount < expressions().size()) {
		setExpressions(expressions().mid(0, newComponentCount));
	}
	else if(newComponentCount > expressions().size()) {
		QStringList newList = expressions();
		while(newList.size() < newComponentCount)
			newList.append("0");
		setExpressions(newList);
	}

	if(newComponentCount < neighborExpressions().size()) {
		setNeighborExpressions(neighborExpressions().mid(0, newComponentCount));
	}
	else if(newComponentCount > neighborExpressions().size()) {
		QStringList newList = neighborExpressions();
		while(newList.size() < newComponentCount)
			newList.append("0");
		setNeighborExpressions(newList);
	}
}

/******************************************************************************
* Is called when the value of a property of this object has changed.
******************************************************************************/
void ComputePropertyModifier::propertyChanged(const PropertyFieldDescriptor& field)
{
	if(field == PROPERTY_FIELD(outputProperty)) {
		if(outputProperty().type() != ParticleProperty::UserProperty)
			setPropertyComponentCount(ParticleProperty::OOClass().standardPropertyComponentCount(outputProperty().type()));
		else
			setPropertyComponentCount(1);
	}

	AsynchronousModifier::propertyChanged(field);
}

/******************************************************************************
* This method is called by the system when the modifier has been inserted
* into a pipeline.
******************************************************************************/
void ComputePropertyModifier::initializeModifier(ModifierApplication* modApp)
{
	AsynchronousModifier::initializeModifier(modApp);

	// Generate list of available input variables.
	PipelineFlowState input = modApp->evaluateInputPreliminary();
	ParticleExpressionEvaluator evaluator;
	evaluator.initialize(QStringList(), input);
	_inputVariableNames = evaluator.inputVariableNames();
	_inputVariableTable = evaluator.inputVariableTable();
}

/******************************************************************************
* Creates and initializes a computation engine that will compute the 
* modifier's results.
******************************************************************************/
Future<AsynchronousModifier::ComputeEnginePtr> ComputePropertyModifier::createEngine(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input)
{
	ParticleInputHelper pih(dataset(), input);

	// Get the particle positions.
	ParticleProperty* posProperty = pih.expectStandardProperty<ParticleProperty>(ParticleProperty::PositionProperty);

	// Get simulation cell.
	SimulationCellObject* inputCell = pih.expectSimulationCell();

	// The current animation frame number.
	int currentFrame = dataset()->animationSettings()->timeToFrame(time);

	// Build list of all input particle properties, which will be passed to the compute engine.
	std::vector<ConstPropertyPtr> inputProperties;
	for(DataObject* obj : input.objects()) {
		if(ParticleProperty* prop = dynamic_object_cast<ParticleProperty>(obj)) {
			inputProperties.emplace_back(prop->storage());
		}
	}

	// Get particle selection
	ConstPropertyPtr selProperty;
	if(onlySelectedParticles()) {
		ParticleProperty* selPropertyObj = pih.inputStandardProperty<ParticleProperty>(ParticleProperty::SelectionProperty);
		if(!selPropertyObj)
			throwException(tr("Compute modifier has been restricted to selected particles, but no particle selection is defined."));
		OVITO_ASSERT(selPropertyObj->size() == pih.inputParticleCount());
		selProperty = selPropertyObj->storage();
	}

	// Prepare output property.
	PropertyPtr outp;
	if(outputProperty().type() != ParticleProperty::UserProperty) {
		outp = ParticleProperty::OOClass().createStandardStorage(posProperty->size(), outputProperty().type(), onlySelectedParticles());
	}
	else if(!outputProperty().name().isEmpty() && propertyComponentCount() > 0) {
		outp = std::make_shared<PropertyStorage>(posProperty->size(), PropertyStorage::Float, propertyComponentCount(), 0, outputProperty().name(), onlySelectedParticles());
	}
	else {
		throwException(tr("Output property has not been specified."));
	}
	if(expressions().size() != outp->componentCount())
		throwException(tr("Number of expressions does not match component count of output property."));
	if(neighborModeEnabled() && neighborExpressions().size() != outp->componentCount())
		throwException(tr("Number of neighbor expressions does not match component count of output property."));

	TimeInterval validityInterval = input.stateValidity();

	// Initialize output property with original values when computation is restricted to selected particles.
	if(onlySelectedParticles()) {
		ParticleProperty* originalPropertyObj = nullptr;
		if(outputProperty().type() != ParticleProperty::UserProperty) {
			originalPropertyObj = pih.inputStandardProperty<ParticleProperty>(outputProperty().type());
		}
		else {
			for(DataObject* o : input.objects()) {
				ParticleProperty* property = dynamic_object_cast<ParticleProperty>(o);
				if(property && property->type() == ParticleProperty::UserProperty && property->name() == outp->name()) {
					originalPropertyObj = property;
					break;
				}
			}

		}
		if(originalPropertyObj && originalPropertyObj->dataType() == outp->dataType() &&
				originalPropertyObj->componentCount() == outp->componentCount() && originalPropertyObj->stride() == outp->stride()) {
			memcpy(outp->data(), originalPropertyObj->constData(), outp->stride() * outp->size());
		}
		else if(outputProperty().type() == ParticleProperty::ColorProperty) {
			std::vector<Color> colors = pih.inputParticleColors(time, validityInterval);
			OVITO_ASSERT(outp->stride() == sizeof(Color) && outp->size() == colors.size());
			memcpy(outp->data(), colors.data(), outp->stride() * outp->size());
		}
		else if(outputProperty().type() == ParticleProperty::RadiusProperty) {
			std::vector<FloatType> radii = pih.inputParticleRadii(time, validityInterval);
			OVITO_ASSERT(outp->stride() == sizeof(FloatType) && outp->size() == radii.size());
			memcpy(outp->data(), radii.data(), outp->stride() * outp->size());
		}
	}

	// Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
	return std::make_shared<PropertyComputeEngine>(validityInterval, time, std::move(outp), posProperty->storage(),
			std::move(selProperty), inputCell->data(), neighborModeEnabled() ? cutoff() : 0,
			expressions(), neighborExpressions(),
			std::move(inputProperties), currentFrame, input.attributes());
}

/******************************************************************************
* Constructor.
******************************************************************************/
ComputePropertyModifier::PropertyComputeEngine::PropertyComputeEngine(
		const TimeInterval& validityInterval, 
		TimePoint time,
		PropertyPtr outputProperty, 
		ConstPropertyPtr positions, 
		ConstPropertyPtr selectionProperty,
		const SimulationCell& simCell, 
		FloatType cutoff,
		QStringList expressions, 
		QStringList neighborExpressions,
		std::vector<ConstPropertyPtr> inputProperties,
		int frameNumber, 
		QVariantMap attributes) :
	_positions(std::move(positions)), 
	_simCell(simCell),
	_selection(std::move(selectionProperty)),
	_expressions(std::move(expressions)), 
	_neighborExpressions(std::move(neighborExpressions)),
	_cutoff(cutoff),
	_frameNumber(frameNumber), 
	_attributes(std::move(attributes)),
	_inputProperties(std::move(inputProperties)),
	_results(std::make_shared<PropertyComputeResults>(validityInterval, std::move(outputProperty))) 
{
	OVITO_ASSERT(_expressions.size() == this->outputProperty()->componentCount());
	setResult(_results);
	
	// Initialize expression evaluators.
	_evaluator.initialize(_expressions, _inputProperties, &cell(), _attributes, _frameNumber);
	_inputVariableNames = _evaluator.inputVariableNames();
	_inputVariableTable = _evaluator.inputVariableTable();

	// Only used when neighbor mode is active.
	if(neighborMode()) {
		_evaluator.registerGlobalParameter("Cutoff", _cutoff);
		_evaluator.registerGlobalParameter("NumNeighbors", 0);
		OVITO_ASSERT(_neighborExpressions.size() == this->outputProperty()->componentCount());
		_neighborEvaluator.initialize(_neighborExpressions, _inputProperties, &cell(), _attributes, _frameNumber);
		_neighborEvaluator.registerGlobalParameter("Cutoff", _cutoff);
		_neighborEvaluator.registerGlobalParameter("NumNeighbors", 0);
		_neighborEvaluator.registerGlobalParameter("Distance", 0);
		_neighborEvaluator.registerGlobalParameter("Delta.X", 0);
		_neighborEvaluator.registerGlobalParameter("Delta.Y", 0);
		_neighborEvaluator.registerGlobalParameter("Delta.Z", 0);
	}

	// Determine if math expressions are time-dependent, i.e. if they reference the animation
	// frame number. If yes, then we have to restrict the validity interval of the computation
	// to the current time.
	bool isTimeDependent = false;
	ParticleExpressionEvaluator::Worker worker(_evaluator);
	if(worker.isVariableUsed("Frame") || worker.isVariableUsed("Timestep"))
		isTimeDependent = true;
	else if(neighborMode()) {
		ParticleExpressionEvaluator::Worker worker(_neighborEvaluator);
		if(worker.isVariableUsed("Frame") || worker.isVariableUsed("Timestep"))
			isTimeDependent = true;
	}
	if(isTimeDependent) {
		TimeInterval iv = _results->validityInterval();
		iv.intersect(time);
		_results->setValidityInterval(iv);
	}
}

/******************************************************************************
* Performs the actual computation. This method is executed in a worker thread.
******************************************************************************/
void ComputePropertyModifier::PropertyComputeEngine::perform()
{
	setProgressText(tr("Computing particle property '%1'").arg(outputProperty()->name()));

	// Only used when neighbor mode is active.
	CutoffNeighborFinder neighborFinder;
	if(neighborMode()) {
		// Prepare the neighbor list.
		if(!neighborFinder.prepare(_cutoff, *positions(), cell(), nullptr, this))
			return;
	}

	setProgressValue(0);
	setProgressMaximum(positions()->size());

	// Parallelized loop over all particles.
	parallelForChunks(positions()->size(), *this, [this, &neighborFinder](size_t startIndex, size_t count, PromiseState& promise) {
		ParticleExpressionEvaluator::Worker worker(_evaluator);
		ParticleExpressionEvaluator::Worker neighborWorker(_neighborEvaluator);

		double* distanceVar;
		double* deltaX;
		double* deltaY;
		double* deltaZ;
		double* selfNumNeighbors = nullptr;
		double* neighNumNeighbors = nullptr;

		if(neighborMode()) {
			distanceVar = neighborWorker.variableAddress("Distance");
			deltaX = neighborWorker.variableAddress("Delta.X");
			deltaY = neighborWorker.variableAddress("Delta.Y");
			deltaZ = neighborWorker.variableAddress("Delta.Z");
			selfNumNeighbors = worker.variableAddress("NumNeighbors");
			neighNumNeighbors = neighborWorker.variableAddress("NumNeighbors");
			if(!worker.isVariableUsed("NumNeighbors") && !neighborWorker.isVariableUsed("NumNeighbors"))
				selfNumNeighbors = neighNumNeighbors = nullptr;
		}

		size_t endIndex = startIndex + count;
		size_t componentCount = outputProperty()->componentCount();
		for(size_t particleIndex = startIndex; particleIndex < endIndex; particleIndex++) {

			// Update progress indicator.
			if((particleIndex % 1024) == 0)
				promise.incrementProgressValue(1024);

			// Stop loop if canceled.
			if(promise.isCanceled())
				return;

			// Skip unselected particles if requested.
			if(selection() && !selection()->getInt(particleIndex))
				continue;

			if(selfNumNeighbors != nullptr) {
				// Determine number of neighbors.
				int nneigh = 0;
				for(CutoffNeighborFinder::Query neighQuery(neighborFinder, particleIndex); !neighQuery.atEnd(); neighQuery.next())
					nneigh++;
				*selfNumNeighbors = *neighNumNeighbors = nneigh;
			}

			for(size_t component = 0; component < componentCount; component++) {

				// Compute self term.
				FloatType value = worker.evaluate(particleIndex, component);

				if(neighborMode()) {
					// Compute sum of neighbor terms.
					for(CutoffNeighborFinder::Query neighQuery(neighborFinder, particleIndex); !neighQuery.atEnd(); neighQuery.next()) {
						*distanceVar = sqrt(neighQuery.distanceSquared());
						*deltaX = neighQuery.delta().x();
						*deltaY = neighQuery.delta().y();
						*deltaZ = neighQuery.delta().z();
						value += neighborWorker.evaluate(neighQuery.current(), component);
					}
				}

				// Store results.
				if(outputProperty()->dataType() == PropertyStorage::Int) {
					outputProperty()->setIntComponent(particleIndex, component, (int)value);
				}
				else if(outputProperty()->dataType() == PropertyStorage::Int64) {
					outputProperty()->setInt64Component(particleIndex, component, (qlonglong)value);
				}
				else if(outputProperty()->dataType() == PropertyStorage::Float) {
					outputProperty()->setFloatComponent(particleIndex, component, value);
				}
			}
		}
	});
}

/******************************************************************************
* Injects the computed results of the engine into the data pipeline.
******************************************************************************/
PipelineFlowState ComputePropertyModifier::PropertyComputeResults::apply(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input)
{
	PipelineFlowState output = input;
	ParticleOutputHelper poh(modApp->dataset(), output);
	if(outputProperty()->size() != poh.outputParticleCount())
		modApp->throwException(tr("Cached modifier results are obsolete, because the number of input particles has changed."));
	ParticleProperty* outputPropertyObj = poh.outputProperty<ParticleProperty>(outputProperty());

	ComputePropertyModifierApplication* myModApp = dynamic_object_cast<ComputePropertyModifierApplication>(modApp);
	if(myModApp) {
		// Replace display objects of output property with cached ones and cache any new display objects.
		// This is required to avoid losing the output property's display settings
		// each time the modifier is re-evaluated or when serializing the modifier.
		QVector<DisplayObject*> currentDisplayObjs = outputPropertyObj->displayObjects();
		// Replace with cached display objects if they are of the same class type.
		for(int i = 0; i < currentDisplayObjs.size() && i < myModApp->cachedDisplayObjects().size(); i++) {
			if(currentDisplayObjs[i]->getOOClass() == myModApp->cachedDisplayObjects()[i]->getOOClass()) {
				currentDisplayObjs[i] = myModApp->cachedDisplayObjects()[i];
			}
		}
		outputPropertyObj->setDisplayObjects(currentDisplayObjs);
		myModApp->setCachedDisplayObjects(currentDisplayObjs);
	}

	return output;
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
