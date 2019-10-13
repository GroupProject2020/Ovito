////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2017 Alexander Stukowski
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
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>
#include <ovito/stdobj/properties/PropertyStorage.h>
#include <ovito/stdobj/properties/PropertyObject.h>
#include <ovito/stdobj/properties/PropertyContainer.h>
#include <ovito/core/dataset/animation/controller/Controller.h>
#include <ovito/core/viewport/ViewportConfiguration.h>
#include <ovito/core/utilities/concurrent/TaskManager.h>
#include <ovito/core/utilities/concurrent/Promise.h>
#include <ovito/core/app/Application.h>
#include <ovito/core/app/PluginManager.h>
#include "ColorCodingModifier.h"

namespace Ovito { namespace StdMod {

IMPLEMENT_OVITO_CLASS(ColorCodingModifierDelegate);

IMPLEMENT_OVITO_CLASS(ColorCodingGradient);
IMPLEMENT_OVITO_CLASS(ColorCodingHSVGradient);
IMPLEMENT_OVITO_CLASS(ColorCodingGrayscaleGradient);
IMPLEMENT_OVITO_CLASS(ColorCodingHotGradient);
IMPLEMENT_OVITO_CLASS(ColorCodingJetGradient);
IMPLEMENT_OVITO_CLASS(ColorCodingBlueWhiteRedGradient);
IMPLEMENT_OVITO_CLASS(ColorCodingViridisGradient);
IMPLEMENT_OVITO_CLASS(ColorCodingMagmaGradient);
IMPLEMENT_OVITO_CLASS(ColorCodingTableGradient);
IMPLEMENT_OVITO_CLASS(ColorCodingImageGradient);
IMPLEMENT_OVITO_CLASS(ColorCodingModifier);
DEFINE_REFERENCE_FIELD(ColorCodingModifier, startValueController);
DEFINE_REFERENCE_FIELD(ColorCodingModifier, endValueController);
DEFINE_REFERENCE_FIELD(ColorCodingModifier, colorGradient);
DEFINE_PROPERTY_FIELD(ColorCodingModifier, colorOnlySelected);
DEFINE_PROPERTY_FIELD(ColorCodingModifier, keepSelection);
DEFINE_PROPERTY_FIELD(ColorCodingModifier, sourceProperty);
SET_PROPERTY_FIELD_LABEL(ColorCodingModifier, startValueController, "Start value");
SET_PROPERTY_FIELD_LABEL(ColorCodingModifier, endValueController, "End value");
SET_PROPERTY_FIELD_LABEL(ColorCodingModifier, colorGradient, "Color gradient");
SET_PROPERTY_FIELD_LABEL(ColorCodingModifier, colorOnlySelected, "Color only selected elements");
SET_PROPERTY_FIELD_LABEL(ColorCodingModifier, keepSelection, "Keep selection");
SET_PROPERTY_FIELD_LABEL(ColorCodingModifier, sourceProperty, "Source property");

DEFINE_PROPERTY_FIELD(ColorCodingImageGradient, image);
DEFINE_PROPERTY_FIELD(ColorCodingTableGradient, table);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
ColorCodingModifier::ColorCodingModifier(DataSet* dataset) : DelegatingModifier(dataset),
	_colorOnlySelected(false),
	_keepSelection(true)
{
	setColorGradient(new ColorCodingHSVGradient(dataset));
	setStartValueController(ControllerManager::createFloatController(dataset));
	setEndValueController(ControllerManager::createFloatController(dataset));

	// Let this modifier act on particles by default.
	createDefaultModifierDelegate(ColorCodingModifierDelegate::OOClass(), QStringLiteral("ParticlesColorCodingModifierDelegate"));
}

/******************************************************************************
* Loads the user-defined default values of this object's parameter fields from the
* application's settings store.
******************************************************************************/
void ColorCodingModifier::loadUserDefaults()
{
	DelegatingModifier::loadUserDefaults();

	// Load the default gradient type set by the user.
	QSettings settings;
	settings.beginGroup(ColorCodingModifier::OOClass().plugin()->pluginId());
	settings.beginGroup(ColorCodingModifier::OOClass().name());
	QString typeString = settings.value(PROPERTY_FIELD(colorGradient).identifier()).toString();
	if(!typeString.isEmpty()) {
		try {
			OvitoClassPtr gradientType = OvitoClass::decodeFromString(typeString);
			if(!colorGradient() || colorGradient()->getOOClass() != *gradientType) {
				OORef<ColorCodingGradient> gradient = dynamic_object_cast<ColorCodingGradient>(gradientType->createInstance(dataset()));
				if(gradient) setColorGradient(gradient);
			}
		}
		catch(...) {}
	}

	// In the graphical program environment, we let the modifier clear the selection by default
	// in order to make the newly assigned colors visible.
	setKeepSelection(false);
}

/******************************************************************************
* Asks the modifier for its validity interval at the given time.
******************************************************************************/
TimeInterval ColorCodingModifier::modifierValidity(TimePoint time)
{
	TimeInterval interval = DelegatingModifier::modifierValidity(time);
	if(startValueController()) interval.intersect(startValueController()->validityInterval(time));
	if(endValueController()) interval.intersect(endValueController()->validityInterval(time));
	return interval;
}

/******************************************************************************
* This method is called by the system when the modifier has been inserted
* into a pipeline.
******************************************************************************/
void ColorCodingModifier::initializeModifier(ModifierApplication* modApp)
{
	DelegatingModifier::initializeModifier(modApp);

	// When the modifier is inserted, automatically select the most recently added property from the input.
	if(sourceProperty().isNull() && delegate() && Application::instance()->executionContext() == Application::ExecutionContext::Interactive) {
		const PipelineFlowState& input = modApp->evaluateInputPreliminary();
		if(const PropertyContainer* container = input.getLeafObject(delegate()->inputContainerRef())) {
			PropertyReference bestProperty;
			for(PropertyObject* property : container->properties()) {
				bestProperty = PropertyReference(delegate()->inputContainerClass(), property, (property->componentCount() > 1) ? 0 : -1);
			}
			if(!bestProperty.isNull())
				setSourceProperty(bestProperty);
		}
	}

	// Automatically adjust value range.
	if(startValue() == 0 && endValue() == 0)
		adjustRange();
}

/******************************************************************************
* Is called when the value of a reference field of this RefMaker changes.
******************************************************************************/
void ColorCodingModifier::referenceReplaced(const PropertyFieldDescriptor& field, RefTarget* oldTarget, RefTarget* newTarget)
{
	// Whenever the delegate of this modifier is being replaced, update the source property reference.
	if(field == PROPERTY_FIELD(DelegatingModifier::delegate) && !isBeingLoaded() && !isAboutToBeDeleted() && !dataset()->undoStack().isUndoingOrRedoing()) {
		setSourceProperty(sourceProperty().convertToContainerClass(delegate() ? delegate()->inputContainerClass() : nullptr));
	}
	DelegatingModifier::referenceReplaced(field, oldTarget, newTarget);
}

/******************************************************************************
* Determines the range of values in the input data for the selected property.
******************************************************************************/
bool ColorCodingModifier::determinePropertyValueRange(const PipelineFlowState& state, FloatType& min, FloatType& max)
{
	if(!delegate() || state.isEmpty())
		return false;
	const PropertyContainer* container = state.getLeafObject(delegate()->inputContainerRef());
	if(!container)
		return false;
	const PropertyObject* propertyObj = sourceProperty().findInContainer(container);
	if(!propertyObj)
		return false;
	const PropertyStorage* property = propertyObj->storage().get();
	if(sourceProperty().vectorComponent() >= (int)property->componentCount())
		return false;
	int vecComponent = std::max(0, sourceProperty().vectorComponent());

	int stride = property->stride() / property->dataTypeSize();

	// Iterate over all particles/bonds.
	FloatType maxValue = FLOATTYPE_MIN;
	FloatType minValue = FLOATTYPE_MAX;
	if(property->dataType() == PropertyStorage::Float) {
		auto v = property->constDataFloat() + vecComponent;
		auto vend = v + (property->size() * stride);
		for(; v != vend; v += stride) {
			if(*v > maxValue) maxValue = *v;
			if(*v < minValue) minValue = *v;
		}
	}
	else if(property->dataType() == PropertyStorage::Int) {
		auto v = property->constDataInt() + vecComponent;
		auto vend = v + (property->size() * stride);
		for(; v != vend; v += stride) {
			if(*v > maxValue) maxValue = *v;
			if(*v < minValue) minValue = *v;
		}
	}
	else if(property->dataType() == PropertyStorage::Int64) {
		auto v = property->constDataInt64() + vecComponent;
		auto vend = v + (property->size() * stride);
		for(; v != vend; v += stride) {
			if(*v > maxValue) maxValue = *v;
			if(*v < minValue) minValue = *v;
		}
	}
	if(minValue == FLOATTYPE_MAX)
		return false;

	// Clamp to finite range.
	if(!std::isfinite(minValue)) minValue = std::numeric_limits<FloatType>::lowest();
	if(!std::isfinite(maxValue)) maxValue = std::numeric_limits<FloatType>::max();

	if(minValue < min) min = minValue;
	if(maxValue > max) max = maxValue;

	return true;
}

/******************************************************************************
* Sets the start and end value to the minimum and maximum value
* in the selected particle or bond property.
* Returns true if successful.
******************************************************************************/
bool ColorCodingModifier::adjustRange()
{
	FloatType minValue = std::numeric_limits<FloatType>::max();
	FloatType maxValue = std::numeric_limits<FloatType>::lowest();

	// Loop over all input data.
	bool success = false;
	for(ModifierApplication* modApp : modifierApplications()) {
		const PipelineFlowState& inputState = modApp->evaluateInputPreliminary();

		// Determine the minimum and maximum values of the selected property.
		success |= determinePropertyValueRange(inputState, minValue, maxValue);
	}
	if(!success)
		return false;

	// Adjust range of color coding.
	if(startValueController())
		startValueController()->setCurrentFloatValue(minValue);
	if(endValueController())
		endValueController()->setCurrentFloatValue(maxValue);

	return true;
}

/******************************************************************************
* Sets the start and end value to the minimum and maximum value of the selected
* particle or bond property determined over the entire animation sequence.
******************************************************************************/
bool ColorCodingModifier::adjustRangeGlobal(Task& operation)
{
	ViewportSuspender noVPUpdates(this);

	TimeInterval interval = dataset()->animationSettings()->animationInterval();
	operation.setProgressMaximum(interval.duration() / dataset()->animationSettings()->ticksPerFrame() + 1);

	FloatType minValue = std::numeric_limits<FloatType>::max();
	FloatType maxValue = std::numeric_limits<FloatType>::lowest();

	// Loop over all animation frames, evaluate data pipeline, and determine
	// minimum and maximum values.
	for(TimePoint time = interval.start(); time <= interval.end() && !operation.isCanceled(); time += dataset()->animationSettings()->ticksPerFrame()) {
		operation.setProgressText(tr("Analyzing frame %1").arg(dataset()->animationSettings()->timeToFrame(time)));

		for(ModifierApplication* modApp : modifierApplications()) {

			// Evaluate data pipeline up to this color coding modifier.
			SharedFuture<PipelineFlowState> stateFuture = modApp->evaluateInput(time);
			if(!operation.waitForFuture(stateFuture))
				break;

			// Determine min/max value of the selected property.
			determinePropertyValueRange(stateFuture.result(), minValue, maxValue);
		}
		operation.incrementProgressValue(1);
	}

	if(!operation.isCanceled()) {
		// Adjust range of color coding to the min/max values.
		if(startValueController() && minValue != std::numeric_limits<FloatType>::max())
			startValueController()->setCurrentFloatValue(minValue);
		if(endValueController() && maxValue != std::numeric_limits<FloatType>::lowest())
			endValueController()->setCurrentFloatValue(maxValue);

		return true;
	}
	return false;
}

/******************************************************************************
* Applies the modifier operation to the data in a pipeline flow state.
******************************************************************************/
PipelineStatus ColorCodingModifierDelegate::apply(Modifier* modifier, PipelineFlowState& state, TimePoint time, ModifierApplication* modApp, const std::vector<std::reference_wrapper<const PipelineFlowState>>& additionalInputs)
{
	const ColorCodingModifier* mod = static_object_cast<ColorCodingModifier>(modifier);

	if(!mod->colorGradient())
		throwException(tr("No color gradient has been selected."));

	// Get the source property.
	const PropertyReference& sourceProperty = mod->sourceProperty();
	int vecComponent;
	if(sourceProperty.isNull())
		throwException(tr("No source property was set as input for color coding."));

	// Look up the selected property container. Make sure we can safely modify it.
	DataObjectPath objectPath = state.expectMutableObject(inputContainerRef());
	PropertyContainer* container = static_object_cast<PropertyContainer>(objectPath.back());

	// Check if the source property is the right kind of property.
	if(sourceProperty.containerClass() != &container->getOOMetaClass())
		throwException(tr("Color coding modifier was set to operate on '%1', but the selected input is a '%2' property.")
			.arg(getOOMetaClass().pythonDataName()).arg(sourceProperty.containerClass()->propertyClassDisplayName()));

	const PropertyObject* propertyObj = sourceProperty.findInContainer(container);
	if(!propertyObj)
		throwException(tr("The property with the name '%1' does not exist.").arg(sourceProperty.name()));
	ConstPropertyPtr property = propertyObj->storage();
	if(sourceProperty.vectorComponent() >= (int)property->componentCount())
		throwException(tr("The vector component is out of range. The property '%1' has only %2 values per data element.").arg(sourceProperty.name()).arg(property->componentCount()));
	vecComponent = std::max(0, sourceProperty.vectorComponent());

	// Get the selection property if enabled by the user.
	ConstPropertyPtr selProperty;
	if(mod->colorOnlySelected()) {
		if(const PropertyObject* selPropertyObj = container->getProperty(PropertyStorage::GenericSelectionProperty)) {
			selProperty = selPropertyObj->storage();

			// Clear selection if requested.
			if(!mod->keepSelection()) {
				container->removeProperty(selPropertyObj);
			}
		}
	}

	// Create the color output property.
    PropertyPtr colorProperty = container->createProperty(outputColorPropertyId(), (bool)selProperty, objectPath)->modifiableStorage();

	// Get modifier's parameter values.
	FloatType startValue = 0, endValue = 0;
	if(mod->startValueController()) startValue = mod->startValueController()->getFloatValue(time, state.mutableStateValidity());
	if(mod->endValueController()) endValue = mod->endValueController()->getFloatValue(time, state.mutableStateValidity());

	// Clamp to finite range.
	if(!std::isfinite(startValue)) startValue = std::numeric_limits<FloatType>::lowest();
	if(!std::isfinite(endValue)) endValue = std::numeric_limits<FloatType>::max();

	// Get the particle selection property if enabled by the user.
	const int* sel = selProperty ? selProperty->constDataInt() : nullptr;

	OVITO_ASSERT(colorProperty->size() == property->size());
	Color* c_begin = colorProperty->dataColor();
	Color* c_end = c_begin + colorProperty->size();
	Color* c = c_begin;
	int stride = property->stride() / property->dataTypeSize();

	if(property->dataType() == PropertyStorage::Float) {
		auto v = property->constDataFloat() + vecComponent;
		for(; c != c_end; ++c, v += stride) {
			if(sel && !(*sel++))
				continue;

			// Compute linear interpolation.
			FloatType t;
			if(startValue == endValue) {
				if((*v) == startValue) t = FloatType(0.5);
				else if((*v) > startValue) t = 1;
				else t = 0;
			}
			else t = ((*v) - startValue) / (endValue - startValue);

			// Clamp values.
			if(std::isnan(t)) t = 0;
			else if(t == std::numeric_limits<FloatType>::infinity()) t = 1;
			else if(t == -std::numeric_limits<FloatType>::infinity()) t = 0;
			else if(t < 0) t = 0;
			else if(t > 1) t = 1;

			*c = mod->colorGradient()->valueToColor(t);
		}
	}
	else if(property->dataType() == PropertyStorage::Int) {
		auto v = property->constDataInt() + vecComponent;
		for(; c != c_end; ++c, v += stride) {

			if(sel && !(*sel++))
				continue;

			// Compute linear interpolation.
			FloatType t;
			if(startValue == endValue) {
				if((*v) == startValue) t = FloatType(0.5);
				else if((*v) > startValue) t = 1;
				else t = 0;
			}
			else t = ((*v) - startValue) / (endValue - startValue);

			// Clamp values.
			if(t < 0) t = 0;
			else if(t > 1) t = 1;

			*c = mod->colorGradient()->valueToColor(t);
		}
	}
	else if(property->dataType() == PropertyStorage::Int64) {
		auto v = property->constDataInt64() + vecComponent;
		for(; c != c_end; ++c, v += stride) {

			if(sel && !(*sel++))
				continue;

			// Compute linear interpolation.
			FloatType t;
			if(startValue == endValue) {
				if((*v) == startValue) t = FloatType(0.5);
				else if((*v) > startValue) t = 1;
				else t = 0;
			}
			else t = ((*v) - startValue) / (endValue - startValue);

			// Clamp values.
			if(t < 0) t = 0;
			else if(t > 1) t = 1;

			*c = mod->colorGradient()->valueToColor(t);
		}
	}
	else {
		throwException(tr("The property '%1' has an invalid or non-numeric data type.").arg(property->name()));
	}

	return PipelineStatus::Success;
}

/******************************************************************************
* Converts a scalar value to a color value.
******************************************************************************/
Color ColorCodingTableGradient::valueToColor(FloatType t)
{
	if(table().empty()) return Color(0,0,0);
	if(table().size() == 1) return table()[0];
	t *= (table().size() - 1);
	FloatType t0 = std::floor(t);
	const Color& c1 = table()[(size_t)t0];
	const Color& c2 = table()[(size_t)std::ceil(t)];
	return c1 * (FloatType(1) - (t - t0)) + c2 * (t - t0);
}

/******************************************************************************
* Loads the given image file from disk.
******************************************************************************/
void ColorCodingImageGradient::loadImage(const QString& filename)
{
	QImage image(filename);
	if(image.isNull())
		throwException(tr("Could not load image file '%1'.").arg(filename));
	setImage(image);
}

/******************************************************************************
* Converts a scalar value to a color value.
******************************************************************************/
Color ColorCodingImageGradient::valueToColor(FloatType t)
{
	if(image().isNull()) return Color(0,0,0);
	QPoint p;
	if(image().width() > image().height())
		p = QPoint(std::min((int)(t * image().width()), image().width()-1), 0);
	else
		p = QPoint(0, std::min((int)(t * image().height()), image().height()-1));
	return Color(image().pixel(p));
}

}	// End of namespace
}	// End of namespace
