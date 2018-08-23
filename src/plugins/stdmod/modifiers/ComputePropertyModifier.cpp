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

#include <plugins/stdmod/StdMod.h>
#include <plugins/stdobj/simcell/SimulationCellObject.h>
#include <plugins/stdobj/properties/PropertyStorage.h>
#include <plugins/stdobj/properties/PropertyClass.h>
#include <plugins/stdobj/properties/PropertyObject.h>
#include <plugins/stdobj/util/InputHelper.h>
#include <plugins/stdobj/util/OutputHelper.h>
#include <core/dataset/DataSet.h>
#include <core/dataset/animation/AnimationSettings.h>
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
		setOutputProperty(PropertyReference(&delegate()->propertyClass(), QStringLiteral("My property")));
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
	if(field == PROPERTY_FIELD(AsynchronousDelegatingModifier::delegate)) {
		if(!dataset()->undoStack().isUndoingOrRedoing() && !isBeingLoaded() && delegate()) {
			setOutputProperty(outputProperty().convertToPropertyClass(&delegate()->propertyClass()));
			delegate()->setComponentCount(expressions().size());
		}
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

	// Do we have a valid pipeline input?
	const PropertyClass& propertyClass = delegate()->propertyClass();
	if(!propertyClass.isDataPresent(input))
		throwException(tr("Cannot compute property '%1', because the input data contains no %2.").arg(outputProperty().name()).arg(propertyClass.elementDescriptionName()));
	if(outputProperty().propertyClass() != &propertyClass)
		throwException(tr("Property %1 to be computed is not a %2 property.").arg(outputProperty().name()).arg(propertyClass.elementDescriptionName()));

	// Get the number of input elements.
	size_t nelements = propertyClass.elementCount(input);

	// The current animation frame number.
	int currentFrame = dataset()->animationSettings()->timeToFrame(time);

	// Get selection property.
	ConstPropertyPtr selProperty;
	if(onlySelectedElements()) {
		if(PropertyObject* selPropertyObj = propertyClass.findInState(input, PropertyStorage::GenericSelectionProperty))
			selProperty = selPropertyObj->storage();
		else
			throwException(tr("Compute property modifier has been restricted to selected elements, but no selection was previously defined."));
	}

	// Prepare output property.
	PropertyPtr outp;
	if(outputProperty().type() != PropertyStorage::GenericUserProperty) {
		outp = propertyClass.createStandardStorage(nelements, outputProperty().type(), onlySelectedElements());
	}
	else if(!outputProperty().name().isEmpty() && propertyComponentCount() > 0) {
		outp = std::make_shared<PropertyStorage>(nelements, PropertyStorage::Float, propertyComponentCount(), 0, outputProperty().name(), onlySelectedElements());
	}
	else {
		throwException(tr("Output property of compute property modifier has not been specified."));
	}
	if(expressions().size() != outp->componentCount())
		throwException(tr("Number of expressions does not match component count of output property."));

	TimeInterval validityInterval = input.stateValidity();

	// Initialize output property with original values when computation is restricted to selected elements.
	bool initializeOutputProperty = false;
	if(onlySelectedElements()) {
		PropertyObject* originalPropertyObj = outputProperty().findInState(input);
		if(originalPropertyObj && originalPropertyObj->dataType() == outp->dataType() &&
				originalPropertyObj->componentCount() == outp->componentCount() && originalPropertyObj->stride() == outp->stride()) {
			memcpy(outp->data(), originalPropertyObj->constData(), outp->stride() * outp->size());
		}
		else {
			initializeOutputProperty = true;
		}
	}

	// Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
	auto engine = delegate()->createEngine(time, input, std::move(outp), 
			std::move(selProperty),
			expressions(), initializeOutputProperty);

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
		myModApp->notifyDependents(ReferenceEvent::ObjectStatusChanged);
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
		const PropertyClass& propertyClass,
		PropertyPtr outputProperty, 
		ConstPropertyPtr selectionProperty,
		QStringList expressions, 
		int frameNumber, 
		std::unique_ptr<PropertyExpressionEvaluator> evaluator) :
	AsynchronousModifier::ComputeEngine(validityInterval), 
	_selection(std::move(selectionProperty)),
	_expressions(std::move(expressions)), 
	_frameNumber(frameNumber), 
	_outputProperty(std::move(outputProperty)),
	_evaluator(std::move(evaluator))
{
	OVITO_ASSERT(_expressions.size() == this->outputProperty()->componentCount());
	
	// Initialize expression evaluator.
	_evaluator->initialize(_expressions, input, propertyClass, QString(), _frameNumber);
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
PipelineFlowState ComputePropertyModifierDelegate::PropertyComputeEngine::emitResults(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input)
{
	ComputePropertyModifierApplication* myModApp = dynamic_object_cast<ComputePropertyModifierApplication>(modApp);
	ComputePropertyModifier* modifier = static_object_cast<ComputePropertyModifier>(modApp->modifier());

	if(!modifier->delegate())
		modifier->throwException(tr("No delegate set for the Compute Property modifier."));

	PipelineFlowState output = input;
	OutputHelper poh(modifier->dataset(), output, modApp);
	PropertyObject* outputPropertyObj = poh.outputProperty(modifier->delegate()->propertyClass(), outputProperty());

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

	return output;
}

}	// End of namespace
}	// End of namespace
