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

#include <ovito/stdmod/StdMod.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/stdobj/properties/PropertyStorage.h>
#include <ovito/stdobj/properties/PropertyContainer.h>
#include <ovito/stdobj/properties/PropertyObject.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/dataset/animation/AnimationSettings.h>
#include <ovito/core/utilities/concurrent/ParallelFor.h>
#include "ComputePropertyModifier.h"

namespace Ovito { namespace StdMod {

IMPLEMENT_OVITO_CLASS(ComputePropertyModifierDelegate);

IMPLEMENT_OVITO_CLASS(ComputePropertyModifier);
DEFINE_PROPERTY_FIELD(ComputePropertyModifier, expressions);
DEFINE_PROPERTY_FIELD(ComputePropertyModifier, outputProperty);
DEFINE_PROPERTY_FIELD(ComputePropertyModifier, onlySelectedElements);
DEFINE_PROPERTY_FIELD(ComputePropertyModifier, useMultilineFields);
SET_PROPERTY_FIELD_LABEL(ComputePropertyModifier, expressions, "Expressions");
SET_PROPERTY_FIELD_LABEL(ComputePropertyModifier, outputProperty, "Output property");
SET_PROPERTY_FIELD_LABEL(ComputePropertyModifier, onlySelectedElements, "Compute only for selected elements");
SET_PROPERTY_FIELD_LABEL(ComputePropertyModifier, useMultilineFields, "Expand field(s)");

IMPLEMENT_OVITO_CLASS(ComputePropertyModifierApplication);
DEFINE_REFERENCE_FIELD(ComputePropertyModifierApplication, cachedVisElements);
DEFINE_PROPERTY_FIELD(ComputePropertyModifierApplication, inputVariableNames);
DEFINE_PROPERTY_FIELD(ComputePropertyModifierApplication, delegateInputVariableNames);
DEFINE_PROPERTY_FIELD(ComputePropertyModifierApplication, inputVariableTable);
SET_PROPERTY_FIELD_CHANGE_EVENT(ComputePropertyModifierApplication, inputVariableNames, ReferenceEvent::ObjectStatusChanged);
SET_PROPERTY_FIELD_CHANGE_EVENT(ComputePropertyModifierApplication, inputVariableTable, ReferenceEvent::ObjectStatusChanged);
SET_MODIFIER_APPLICATION_TYPE(ComputePropertyModifier, ComputePropertyModifierApplication);

/******************************************************************************
* Constructs a new instance of this class.
******************************************************************************/
ComputePropertyModifier::ComputePropertyModifier(DataSet* dataset) : AsynchronousDelegatingModifier(dataset),
	_expressions(QStringList("0")),
	_onlySelectedElements(false),
	_useMultilineFields(false)
{
	// Let this modifier act on particles by default.
	createDefaultModifierDelegate(ComputePropertyModifierDelegate::OOClass(), QStringLiteral("ParticlesComputePropertyModifierDelegate"));
	// Set default output property.
	if(delegate())
		setOutputProperty(PropertyReference(delegate()->inputContainerClass(), QStringLiteral("My property")));
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
	if(delegate())
		delegate()->setComponentCount(newComponentCount);
}

/******************************************************************************
* Is called when the value of a reference field of this RefMaker changes.
******************************************************************************/
void ComputePropertyModifier::referenceReplaced(const PropertyFieldDescriptor& field, RefTarget* oldTarget, RefTarget* newTarget)
{
	if(field == PROPERTY_FIELD(AsynchronousDelegatingModifier::delegate) && !isAboutToBeDeleted() && !isBeingLoaded() && !dataset()->undoStack().isUndoingOrRedoing()) {
		setOutputProperty(outputProperty().convertToContainerClass(delegate() ? delegate()->inputContainerClass() : nullptr));
		if(delegate()) delegate()->setComponentCount(expressions().size());
	}
	AsynchronousDelegatingModifier::referenceReplaced(field, oldTarget, newTarget);
}

/******************************************************************************
* Creates and initializes a computation engine that will compute the
* modifier's results.
******************************************************************************/
Future<AsynchronousModifier::ComputeEnginePtr> ComputePropertyModifier::createEngine(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input)
{
	// Get the delegate object that will take of the specific details.
	if(!delegate())
		throwException(tr("No delegate set for the compute property modifier."));

	// Look up the property container which we will operate on.
   	ConstDataObjectPath objectPath = input.expectObject(delegate()->inputContainerRef());
	const PropertyContainer* container = static_object_cast<PropertyContainer>(objectPath.back());
	if(outputProperty().containerClass() != delegate()->inputContainerClass())
		throwException(tr("Property %1 to be computed is not a %2 property.").arg(outputProperty().name()).arg(delegate()->inputContainerClass()->elementDescriptionName()));
	container->verifyIntegrity();

	// Get the number of input elements.
	size_t nelements = container->elementCount();

	// The current animation frame number.
	int currentFrame = dataset()->animationSettings()->timeToFrame(time);

	// Get input selection property and existing property data.
	ConstPropertyPtr selectionProperty;
	ConstPropertyPtr existingProperty;
	if(onlySelectedElements() && container->getOOMetaClass().isValidStandardPropertyId(PropertyStorage::GenericSelectionProperty)) {
		selectionProperty = container->getPropertyStorage(PropertyStorage::GenericSelectionProperty);
		if(!selectionProperty)
			throwException(tr("Compute property modifier has been restricted to selected elements, but no selection was previously defined."));

		const PropertyObject* existingPropertyObj = outputProperty().findInContainer(container);
		if(existingPropertyObj && existingPropertyObj->componentCount() == propertyComponentCount())
			existingProperty = existingPropertyObj->storage();
	}

	// Prepare output property.
	PropertyPtr outp;
	if(existingProperty) {
		// Copy existing data.
		outp = std::make_shared<PropertyStorage>(*existingProperty);
	}
	else {
		// Allocate new data array.
		if(outputProperty().type() != PropertyStorage::GenericUserProperty) {
			outp = container->getOOMetaClass().createStandardStorage(nelements, outputProperty().type(), onlySelectedElements(), objectPath);
		}
		else if(!outputProperty().name().isEmpty() && propertyComponentCount() > 0) {
			outp = std::make_shared<PropertyStorage>(nelements, PropertyStorage::Float, propertyComponentCount(), 0, outputProperty().name(), onlySelectedElements());
		}
		else {
			throwException(tr("Output property of compute property modifier has not been specified."));
		}
	}
	if(propertyComponentCount() != outp->componentCount())
		throwException(tr("Number of expressions does not match component count of output property."));

	TimeInterval validityInterval = input.stateValidity();

	// Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
	auto engine = delegate()->createEngine(time, input,
			container, std::move(outp),
			std::move(selectionProperty),
			expressions());

	// Determine if math expressions are time-dependent, i.e. if they reference the animation
	// frame number. If yes, then we have to restrict the validity interval of the computation
	// to the current time.
	if(engine->isTimeDependent()) {
		TimeInterval iv = engine->validityInterval();
		iv.intersect(time);
		engine->setValidityInterval(iv);
	}

	// Store the list of input variables in the ModifierApplication so that the UI component can display
	// it to the user.
	if(ComputePropertyModifierApplication* myModApp = dynamic_object_cast<ComputePropertyModifierApplication>(modApp)) {
		myModApp->setInputVariableNames(engine->inputVariableNames());
		myModApp->setDelegateInputVariableNames(engine->delegateInputVariableNames());
		myModApp->setInputVariableTable(engine->inputVariableTable());
		delegate()->notifyDependents(ReferenceEvent::ObjectStatusChanged);
	}

	return engine;
}

/******************************************************************************
* Constructor.
******************************************************************************/
ComputePropertyModifierDelegate::PropertyComputeEngine::PropertyComputeEngine(
		const TimeInterval& validityInterval,
		TimePoint time,
		const PipelineFlowState& input,
		const PropertyContainer* container,
		PropertyPtr outputProperty,
		ConstPropertyPtr selectionProperty,
		QStringList expressions,
		int frameNumber,
		std::unique_ptr<PropertyExpressionEvaluator> evaluator) :
	AsynchronousModifier::ComputeEngine(validityInterval),
	_selectionArray(std::move(selectionProperty)),
	_expressions(std::move(expressions)),
	_frameNumber(frameNumber),
	_outputProperty(std::move(outputProperty)),
	_evaluator(std::move(evaluator)),
	_outputArray(_outputProperty)
{
	OVITO_ASSERT(_expressions.size() == this->outputProperty()->componentCount());

	// Initialize expression evaluator.
	_evaluator->initialize(_expressions, input, container, _frameNumber);
}

/******************************************************************************
* Performs the actual computation. This method is executed in a worker thread.
******************************************************************************/
void ComputePropertyModifierDelegate::PropertyComputeEngine::perform()
{
	task()->setProgressText(tr("Computing property '%1'").arg(outputProperty()->name()));

	task()->setProgressValue(0);
	task()->setProgressMaximum(outputProperty()->size());

	// Parallelized loop over all data elements.
	parallelForChunks(outputProperty()->size(), *task(), [this](size_t startIndex, size_t count, Task& promise) {
		PropertyExpressionEvaluator::Worker worker(*_evaluator);

		size_t endIndex = startIndex + count;
		size_t componentCount = outputProperty()->componentCount();
		for(size_t elementIndex = startIndex; elementIndex < endIndex; elementIndex++) {

			// Update progress indicator.
			if((elementIndex % 1024) == 0)
				promise.incrementProgressValue(1024);

			// Exit if operation was canceled.
			if(promise.isCanceled())
				return;

			// Skip unselected particles if requested.
			if(selectionArray() && !selectionArray()[elementIndex])
				continue;

			for(size_t component = 0; component < componentCount; component++) {

				// Compute expression value.
				FloatType value = worker.evaluate(elementIndex, component);

				// Store results in output property.
				outputArray().set(elementIndex, component, value);
			}
		}
	});
}

/******************************************************************************
* Returns the list of available input variables.
******************************************************************************/
QStringList ComputePropertyModifierDelegate::PropertyComputeEngine::inputVariableNames() const
{
	if(_evaluator) {
		return _evaluator->inputVariableNames();
	}
	else {
		return {};
	}
}

/******************************************************************************
* Injects the computed results of the engine into the data pipeline.
******************************************************************************/
void ComputePropertyModifierDelegate::PropertyComputeEngine::emitResults(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state)
{
	ComputePropertyModifierApplication* myModApp = dynamic_object_cast<ComputePropertyModifierApplication>(modApp);
	ComputePropertyModifier* modifier = static_object_cast<ComputePropertyModifier>(modApp->modifier());

	if(!modifier->delegate())
		modifier->throwException(tr("No delegate set for the Compute Property modifier."));

	// Look up the container we are operating on.
	PropertyContainer* container = state.expectMutableLeafObject(modifier->delegate()->inputContainerRef());

	// Create the output property object in the container.
	PropertyObject* outputPropertyObj = container->createProperty(outputProperty());

	if(myModApp) {
		// Replace vis elements of output property with cached ones and cache any new vis elements.
		// This is required to avoid losing the output property's display settings
		// each time the modifier is re-evaluated or when serializing the modifier.
		QVector<DataVis*> currentVisElements = outputPropertyObj->visElements();
		// Replace with cached vis elements if they are of the same class type.
		for(int i = 0; i < currentVisElements.size() && i < myModApp->cachedVisElements().size(); i++) {
			if(currentVisElements[i]->getOOClass() == myModApp->cachedVisElements()[i]->getOOClass()) {
				currentVisElements[i] = myModApp->cachedVisElements()[i];
			}
		}
		outputPropertyObj->setVisElements(currentVisElements);
		myModApp->setCachedVisElements(std::move(currentVisElements));
	}
}

}	// End of namespace
}	// End of namespace
