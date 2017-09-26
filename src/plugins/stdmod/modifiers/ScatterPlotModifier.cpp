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

#include <plugins/stdmod/StdMod.h>
#include <core/dataset/DataSet.h>
#include <core/dataset/pipeline/ModifierApplication.h>
#include <plugins/stdobj/properties/PropertyObject.h>
#include <plugins/stdobj/util/InputHelper.h>
#include <plugins/stdobj/util/OutputHelper.h>
#include <core/app/PluginManager.h>
#include "ScatterPlotModifier.h"

namespace Ovito { namespace StdMod {

IMPLEMENT_OVITO_CLASS(ScatterPlotModifier);
IMPLEMENT_OVITO_CLASS(ScatterPlotModifierApplication);
DEFINE_PROPERTY_FIELD(ScatterPlotModifier, selectXAxisInRange);
DEFINE_PROPERTY_FIELD(ScatterPlotModifier, selectionXAxisRangeStart);
DEFINE_PROPERTY_FIELD(ScatterPlotModifier, selectionXAxisRangeEnd);
DEFINE_PROPERTY_FIELD(ScatterPlotModifier, selectYAxisInRange);
DEFINE_PROPERTY_FIELD(ScatterPlotModifier, selectionYAxisRangeStart);
DEFINE_PROPERTY_FIELD(ScatterPlotModifier, selectionYAxisRangeEnd);
DEFINE_PROPERTY_FIELD(ScatterPlotModifier, fixXAxisRange);
DEFINE_PROPERTY_FIELD(ScatterPlotModifier, xAxisRangeStart);
DEFINE_PROPERTY_FIELD(ScatterPlotModifier, xAxisRangeEnd);
DEFINE_PROPERTY_FIELD(ScatterPlotModifier, fixYAxisRange);
DEFINE_PROPERTY_FIELD(ScatterPlotModifier, yAxisRangeStart);
DEFINE_PROPERTY_FIELD(ScatterPlotModifier, yAxisRangeEnd);
DEFINE_PROPERTY_FIELD(ScatterPlotModifier, xAxisProperty);
DEFINE_PROPERTY_FIELD(ScatterPlotModifier, yAxisProperty);
SET_PROPERTY_FIELD_LABEL(ScatterPlotModifier, selectXAxisInRange, "Select elements in x-range");
SET_PROPERTY_FIELD_LABEL(ScatterPlotModifier, selectionXAxisRangeStart, "Selection x-range start");
SET_PROPERTY_FIELD_LABEL(ScatterPlotModifier, selectionXAxisRangeEnd, "Selection x-range end");
SET_PROPERTY_FIELD_LABEL(ScatterPlotModifier, selectYAxisInRange, "Select elements in y-range");
SET_PROPERTY_FIELD_LABEL(ScatterPlotModifier, selectionYAxisRangeStart, "Selection y-range start");
SET_PROPERTY_FIELD_LABEL(ScatterPlotModifier, selectionYAxisRangeEnd, "Selection y-range end");
SET_PROPERTY_FIELD_LABEL(ScatterPlotModifier, fixXAxisRange, "Fix x-range");
SET_PROPERTY_FIELD_LABEL(ScatterPlotModifier, xAxisRangeStart, "X-range start");
SET_PROPERTY_FIELD_LABEL(ScatterPlotModifier, xAxisRangeEnd, "X-range end");
SET_PROPERTY_FIELD_LABEL(ScatterPlotModifier, fixYAxisRange, "Fix y-range");
SET_PROPERTY_FIELD_LABEL(ScatterPlotModifier, yAxisRangeStart, "Y-range start");
SET_PROPERTY_FIELD_LABEL(ScatterPlotModifier, yAxisRangeEnd, "Y-range end");
SET_PROPERTY_FIELD_LABEL(ScatterPlotModifier, xAxisProperty, "X-axis property");
SET_PROPERTY_FIELD_LABEL(ScatterPlotModifier, yAxisProperty, "Y-axis property");

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
ScatterPlotModifier::ScatterPlotModifier(DataSet* dataset) : GenericPropertyModifier(dataset),
	_selectXAxisInRange(false),	
	_selectionXAxisRangeStart(0), 
	_selectionXAxisRangeEnd(1),
	_selectYAxisInRange(false),	
	_selectionYAxisRangeStart(0), 
	_selectionYAxisRangeEnd(1),
	_fixXAxisRange(false), 
	_xAxisRangeStart(0),
	_xAxisRangeEnd(0), 
	_fixYAxisRange(false),
	_yAxisRangeStart(0), 
	_yAxisRangeEnd(0)
{
	// Operate on particle properties by default.
	setPropertyClass(static_cast<const PropertyClass*>(
		PluginManager::instance().findClass(QStringLiteral("Particles"), QStringLiteral("ParticleProperty"))));	
}

/******************************************************************************
* Create a new modifier application that refers to this modifier instance.
******************************************************************************/
OORef<ModifierApplication> ScatterPlotModifier::createModifierApplication()
{
	OORef<ModifierApplication> modApp = new ScatterPlotModifierApplication(dataset());
	modApp->setModifier(this);
	return modApp;
}

/******************************************************************************
* This method is called by the system when the modifier has been inserted
* into a pipeline.
******************************************************************************/
void ScatterPlotModifier::initializeModifier(ModifierApplication* modApp)
{
	GenericPropertyModifier::initializeModifier(modApp);

	// Use the first available property from the input state as data source when the modifier is newly created.
	if((xAxisProperty().isNull() || yAxisProperty().isNull()) && propertyClass()) {	
		PipelineFlowState input = modApp->evaluateInputPreliminary();
		PropertyReference bestProperty;
		for(DataObject* o : input.objects()) {
			if(PropertyObject* property = dynamic_object_cast<PropertyObject>(o)) {
				if(propertyClass()->isMember(property) && (property->dataType() == qMetaTypeId<int>() || property->dataType() == qMetaTypeId<FloatType>())) {
					bestProperty = PropertyReference(property, (property->componentCount() > 1) ? 0 : -1);
				}
			}
		}
		if(xAxisProperty().isNull() && !bestProperty.isNull()) {
			setXAxisProperty(bestProperty);
		}
		if(yAxisProperty().isNull() && !bestProperty.isNull()) {
			setYAxisProperty(bestProperty);
		}	
	}
}

/******************************************************************************
* Is called when the value of a property of this object has changed.
******************************************************************************/
void ScatterPlotModifier::propertyChanged(const PropertyFieldDescriptor& field)
{
	// Whenever the selected property class of this modifier is changed, clear the source property references.
	// Otherwise they might be pointing to the wrong kind of property.
	if(field == PROPERTY_FIELD(GenericPropertyModifier::propertyClass) && !isBeingLoaded()) {
		setXAxisProperty({});
		setYAxisProperty({});
	}
	GenericPropertyModifier::propertyChanged(field);
}

/******************************************************************************
* Modifies the input data in an immediate, preliminary way.
******************************************************************************/
PipelineFlowState ScatterPlotModifier::evaluatePreliminary(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input)
{
	// Reset the stored results in the ModifierApplication.
	static_object_cast<ScatterPlotModifierApplication>(modApp)->setScatterData({}, {}, {});

	if(!propertyClass())
		throwException(tr("No input property class selected."));
	if(xAxisProperty().isNull())
		throwException(tr("No input property for x-axis selected."));
	if(yAxisProperty().isNull())
		throwException(tr("No input property for y-axis selected."));

	// Check if the source property is the right kind of property.
	if(xAxisProperty().propertyClass() != propertyClass())
		throwException(tr("Modifier was set to operate on '%1', but the selected input is a '%2' property.")
			.arg(propertyClass()->pythonName()).arg(xAxisProperty().propertyClass()->propertyClassDisplayName()));

	// Check if the source property is the right kind of property.
	if(yAxisProperty().propertyClass() != propertyClass())
		throwException(tr("Modifier was set to operate on '%1', but the selected input is a '%2' property.")
			.arg(propertyClass()->pythonName()).arg(yAxisProperty().propertyClass()->propertyClassDisplayName()));

	// Get the input properties.
	PropertyObject* xPropertyObj = xAxisProperty().findInState(input);
	if(!xPropertyObj)
		throwException(tr("The selected input property '%1' is not present.").arg(xAxisProperty().name()));
	PropertyObject* yPropertyObj = yAxisProperty().findInState(input);
	if(!yPropertyObj)
		throwException(tr("The selected input property '%1' is not present.").arg(yAxisProperty().name()));

	ConstPropertyPtr xProperty = xPropertyObj->storage();
	ConstPropertyPtr yProperty = yPropertyObj->storage();
	OVITO_ASSERT(xProperty->size() == yProperty->size());		
				
	size_t xVecComponent = std::max(0, xAxisProperty().vectorComponent());
	size_t xVecComponentCount = xProperty->componentCount();
	size_t yVecComponent = std::max(0, yAxisProperty().vectorComponent());
	size_t yVecComponentCount = yProperty->componentCount();
	if(xVecComponent >= xProperty->componentCount())
		throwException(tr("The selected vector component is out of range. The property '%1' has only %2 components per element.").arg(xProperty->name()).arg(xProperty->componentCount()));
	if(yVecComponent >= yProperty->componentCount())
		throwException(tr("The selected vector component is out of range. The property '%1' has only %2 components per element.").arg(yProperty->name()).arg(yProperty->componentCount()));

	// Allocate output array.
	std::vector<QPointF> xyData(xProperty->size());
	std::map<int, Color> colorMap;
	std::vector<int> typeData;
	
	// Use the types of the input elements to color the scatter points.
	InputHelper ih(dataset(), input);
	if(PropertyObject* typeProperty = ih.inputStandardProperty(*propertyClass(), PropertyStorage::GenericTypeProperty)) {
		colorMap = typeProperty->typeColorMap();
		typeData = std::vector<int>(typeProperty->constDataInt(), typeProperty->constDataInt() + typeProperty->size());
	}

	// Get selection ranges.
	FloatType selectionXAxisRangeStart = this->selectionXAxisRangeStart();
	FloatType selectionXAxisRangeEnd = this->selectionXAxisRangeEnd();
	FloatType selectionYAxisRangeStart = this->selectionYAxisRangeStart();
	FloatType selectionYAxisRangeEnd = this->selectionYAxisRangeEnd();
	if(selectionXAxisRangeStart > selectionXAxisRangeEnd)
		std::swap(selectionXAxisRangeStart, selectionXAxisRangeEnd);
	if(selectionYAxisRangeStart > selectionYAxisRangeEnd)
		std::swap(selectionYAxisRangeStart, selectionYAxisRangeEnd);
	
	PipelineFlowState output = input;		

	// Create storage for output selection.
	PropertyPtr outputSelection;
	size_t numSelected = 0;
	if(selectXAxisInRange() || selectYAxisInRange()) {
		outputSelection = OutputHelper(dataset(), output).outputStandardProperty(*propertyClass(), PropertyStorage::GenericSelectionProperty, false)->modifiableStorage();
		std::fill(outputSelection->dataInt(), outputSelection->dataInt() + outputSelection->size(), 1);
		numSelected = outputSelection->size();
	}

	FloatType xIntervalStart = xAxisRangeStart();
	FloatType xIntervalEnd = xAxisRangeEnd();
	FloatType yIntervalStart = yAxisRangeStart();
	FloatType yIntervalEnd = yAxisRangeEnd();

	// Collect X coordinates.
	if(xProperty->dataType() == qMetaTypeId<FloatType>()) {
		for(size_t i = 0; i < xProperty->size(); i++) {
			xyData[i].rx() = xProperty->getFloatComponent(i, xVecComponent);
		}
	}
	else if(xProperty->dataType() == qMetaTypeId<int>()) {
		for(size_t i = 0; i < xProperty->size(); i++) {
			xyData[i].rx() = xProperty->getIntComponent(i, xVecComponent);
		}
	}
	else throwException(tr("Property '%1' has an invalid data type.").arg(xProperty->name()));

	// Collect Y coordinates.
	if(yProperty->dataType() == qMetaTypeId<FloatType>()) {
		for(size_t i = 0; i < yProperty->size(); i++) {
			xyData[i].ry() = yProperty->getFloatComponent(i, yVecComponent);
		}
	}
	else if(yProperty->dataType() == qMetaTypeId<int>()) {
		for(size_t i = 0; i < yProperty->size(); i++) {
			xyData[i].ry() = yProperty->getIntComponent(i, yVecComponent);
		}
	}
	else throwException(tr("Property '%1' has an invalid data type.").arg(yProperty->name()));

	// Determine value ranges.
	if(fixXAxisRange() == false || fixYAxisRange() == false) {
		Box2 bbox;
		for(const QPointF& p : xyData) {
			bbox.addPoint(p.x(), p.y());
		}
		if(fixXAxisRange() == false) {
			xIntervalStart = bbox.minc.x();
			xIntervalEnd = bbox.maxc.x();
		}
		if(fixYAxisRange() == false) {
			yIntervalStart = bbox.minc.y();
			yIntervalEnd = bbox.maxc.y();
		}
	}

	if(outputSelection && selectXAxisInRange()) {
		OVITO_ASSERT(outputSelection->size() == xyData.size());
		int* s = outputSelection->dataInt();
		int* s_end = s + outputSelection->size();
		for(const QPointF& p : xyData) {
			if(p.x() < selectionXAxisRangeStart || p.x() > selectionXAxisRangeEnd) {
				*s = 0;
				numSelected--;
			}
			++s;
		}
	}

	if(outputSelection && selectYAxisInRange()) {
		OVITO_ASSERT(outputSelection->size() == xyData.size());
		int* s = outputSelection->dataInt();
		int* s_end = s + outputSelection->size();
		for(const QPointF& p : xyData) {
			if(p.y() < selectionYAxisRangeStart || p.y() > selectionYAxisRangeEnd) {
				if(*s) {
					*s = 0;
					numSelected--;
				}
			}
			++s;
		}
	}

	// Store results in the ModifierApplication.
	static_object_cast<ScatterPlotModifierApplication>(modApp)->setScatterData(std::move(xyData), std::move(typeData), std::move(colorMap));

	QString statusMessage;
	if(outputSelection) {
		statusMessage = tr("%1 %2 selected (%3%)").arg(numSelected)
				.arg(propertyClass()->elementDescriptionName())
				.arg((FloatType)numSelected * 100 / std::max(1,(int)outputSelection->size()), 0, 'f', 1);
	}

	output.setStatus(PipelineStatus(PipelineStatus::Success, std::move(statusMessage)));
	return output;
}

}	// End of namespace
}	// End of namespace
