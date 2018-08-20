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
#include <plugins/stdobj/series/DataSeriesObject.h>
#include <plugins/stdobj/util/InputHelper.h>
#include <plugins/stdobj/util/OutputHelper.h>
#include <core/app/PluginManager.h>
#include <core/app/Application.h>
#include <core/utilities/units/UnitsManager.h>
#include "HistogramModifier.h"

namespace Ovito { namespace StdMod {

IMPLEMENT_OVITO_CLASS(HistogramModifier);
DEFINE_PROPERTY_FIELD(HistogramModifier, numberOfBins);
DEFINE_PROPERTY_FIELD(HistogramModifier, selectInRange);
DEFINE_PROPERTY_FIELD(HistogramModifier, selectionRangeStart);
DEFINE_PROPERTY_FIELD(HistogramModifier, selectionRangeEnd);
DEFINE_PROPERTY_FIELD(HistogramModifier, fixXAxisRange);
DEFINE_PROPERTY_FIELD(HistogramModifier, xAxisRangeStart);
DEFINE_PROPERTY_FIELD(HistogramModifier, xAxisRangeEnd);
DEFINE_PROPERTY_FIELD(HistogramModifier, fixYAxisRange);
DEFINE_PROPERTY_FIELD(HistogramModifier, yAxisRangeStart);
DEFINE_PROPERTY_FIELD(HistogramModifier, yAxisRangeEnd);
DEFINE_PROPERTY_FIELD(HistogramModifier, sourceProperty);
DEFINE_PROPERTY_FIELD(HistogramModifier, onlySelected);
SET_PROPERTY_FIELD_LABEL(HistogramModifier, numberOfBins, "Number of histogram bins");
SET_PROPERTY_FIELD_LABEL(HistogramModifier, selectInRange, "Select value range");
SET_PROPERTY_FIELD_LABEL(HistogramModifier, selectionRangeStart, "Selection range start");
SET_PROPERTY_FIELD_LABEL(HistogramModifier, selectionRangeEnd, "Selection range end");
SET_PROPERTY_FIELD_LABEL(HistogramModifier, fixXAxisRange, "Fix x-range");
SET_PROPERTY_FIELD_LABEL(HistogramModifier, xAxisRangeStart, "X-range start");
SET_PROPERTY_FIELD_LABEL(HistogramModifier, xAxisRangeEnd, "X-range end");
SET_PROPERTY_FIELD_LABEL(HistogramModifier, fixYAxisRange, "Fix y-range");
SET_PROPERTY_FIELD_LABEL(HistogramModifier, yAxisRangeStart, "Y-range start");
SET_PROPERTY_FIELD_LABEL(HistogramModifier, yAxisRangeEnd, "Y-range end");
SET_PROPERTY_FIELD_LABEL(HistogramModifier, sourceProperty, "Source property");
SET_PROPERTY_FIELD_LABEL(HistogramModifier, onlySelected, "Use only selected elements");
SET_PROPERTY_FIELD_UNITS_AND_RANGE(HistogramModifier, numberOfBins, IntegerParameterUnit, 1, 100000);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
HistogramModifier::HistogramModifier(DataSet* dataset) : GenericPropertyModifier(dataset),
	_numberOfBins(200), 
	_selectInRange(false),
	_selectionRangeStart(0), 
	_selectionRangeEnd(1),
	_fixXAxisRange(false), 
	_xAxisRangeStart(0), 
	_xAxisRangeEnd(0),
	_fixYAxisRange(false), 
	_yAxisRangeStart(0), 
	_yAxisRangeEnd(0),
	_onlySelected(false)
{
	// Operate on particle properties by default.
	setPropertyClass(static_cast<const PropertyClass*>(
		PluginManager::instance().findClass(QStringLiteral("Particles"), QStringLiteral("ParticleProperty"))));	
}

/******************************************************************************
* This method is called by the system when the modifier has been inserted
* into a pipeline.
******************************************************************************/
void HistogramModifier::initializeModifier(ModifierApplication* modApp)
{
	GenericPropertyModifier::initializeModifier(modApp);

	// Use the first available property from the input state as data source when the modifier is newly created.
	if(sourceProperty().isNull() && propertyClass() && Application::instance()->guiMode()) {	
		const PipelineFlowState& input = modApp->evaluateInputPreliminary();
		PropertyReference bestProperty;
		for(DataObject* o : input.objects()) {
			if(PropertyObject* property = dynamic_object_cast<PropertyObject>(o)) {
				if(propertyClass()->isMember(property) && (property->dataType() == PropertyStorage::Int || property->dataType() == PropertyStorage::Float)) {
					bestProperty = PropertyReference(property, (property->componentCount() > 1) ? 0 : -1);
				}
			}
		}
		if(!bestProperty.isNull()) {
			setSourceProperty(bestProperty);
		}
	}
}

/******************************************************************************
* Is called when the value of a property of this object has changed.
******************************************************************************/
void HistogramModifier::propertyChanged(const PropertyFieldDescriptor& field)
{
	// Whenever the selected property class of this modifier changes, update the source property reference accordingly.
	if(field == PROPERTY_FIELD(GenericPropertyModifier::propertyClass) && !isBeingLoaded() && !dataset()->undoStack().isUndoingOrRedoing()) {
		setSourceProperty(sourceProperty().convertToPropertyClass(propertyClass()));
	}
	GenericPropertyModifier::propertyChanged(field);
}

/******************************************************************************
* Modifies the input data in an immediate, preliminary way.
******************************************************************************/
PipelineFlowState HistogramModifier::evaluatePreliminary(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input)
{
	if(!propertyClass())
		throwException(tr("No input property class selected."));
	if(sourceProperty().isNull())
		throwException(tr("No input property selected."));

	// Check if the source property is the right kind of property.
	if(sourceProperty().propertyClass() != propertyClass())
		throwException(tr("Modifier was set to operate on '%1', but the selected input is a '%2' property.")
			.arg(propertyClass()->pythonName()).arg(sourceProperty().propertyClass()->propertyClassDisplayName()));
		
	// Get the input property.
	PropertyObject* property = sourceProperty().findInState(input);
	if(!property)
		throwException(tr("The selected input property '%1' is not present.").arg(sourceProperty().name()));

	size_t vecComponent = std::max(0, sourceProperty().vectorComponent());
	if(vecComponent >= property->componentCount())
		throwException(tr("The selected vector component is out of range. The property '%1' has only %2 components per element.").arg(property->name()).arg(property->componentCount()));
	size_t vecComponentCount = property->componentCount();

	// Get the input selection if filtering was enabled by the user.
	ConstPropertyPtr inputSelection;
	if(onlySelected()) {
		inputSelection = InputHelper(dataset(), input).expectStandardProperty(*propertyClass(), PropertyStorage::GenericSelectionProperty)->storage();
		OVITO_ASSERT(inputSelection->size() == property->size());
	}

	PipelineFlowState output = input;	
	OutputHelper oh(dataset(), output);
	
	// Create storage for output selection.
	PropertyPtr outputSelection;
	if(selectInRange()) {
		outputSelection = oh.outputStandardProperty(*propertyClass(), PropertyStorage::GenericSelectionProperty, true)->modifiableStorage();
	}

	// Create selection property for output.
	FloatType selectionRangeStart = this->selectionRangeStart();
	FloatType selectionRangeEnd = this->selectionRangeEnd();
	if(selectionRangeStart > selectionRangeEnd) std::swap(selectionRangeStart, selectionRangeEnd);
	size_t numSelected = 0;

	FloatType intervalStart = xAxisRangeStart();
	FloatType intervalEnd = xAxisRangeEnd();

	// Allocate output data array.
	auto histogram = std::make_shared<PropertyStorage>(std::max(1, numberOfBins()), PropertyStorage::Int64, 1, 0, tr("Count"), true);
	auto histogramData = histogram->dataInt64();

	if(property->size() > 0) {
		if(property->dataType() == PropertyStorage::Float) {
			auto v_begin = property->constDataFloat() + vecComponent;
			auto v_end = v_begin + (property->size() * vecComponentCount);
			// Determine value range.
			if(!fixXAxisRange()) {
				intervalStart = std::numeric_limits<FloatType>::max();
				intervalEnd = std::numeric_limits<FloatType>::lowest();
				const int* sel = inputSelection ? inputSelection->constDataInt() : nullptr;
				for(auto v = v_begin; v != v_end; v += vecComponentCount) {
					if(sel && !*sel++) continue;
					if(*v < intervalStart) intervalStart = *v;
					if(*v > intervalEnd) intervalEnd = *v;
				}
			}
			// Perform binning.
			if(intervalEnd > intervalStart) {
				FloatType binSize = (intervalEnd - intervalStart) / histogram->size();
				const int* sel = inputSelection ? inputSelection->constDataInt() : nullptr;
				for(auto v = v_begin; v != v_end; v += vecComponentCount) {
					if(sel && !*sel++) continue;
					if(*v < intervalStart || *v > intervalEnd) continue;
					int binIndex = (*v - intervalStart) / binSize;
					histogramData[std::max(0, std::min(binIndex, (int)histogram->size() - 1))]++;
				}
			}
			else {
				if(!inputSelection)
					histogramData[0] = property->size();
				else
					histogramData[0] = property->size() - std::count(inputSelection->constDataInt(), inputSelection->constDataInt() + inputSelection->size(), 0);
			}
			if(outputSelection) {
				OVITO_ASSERT(outputSelection->size() == property->size());
				int* s = outputSelection->dataInt();
				int* s_end = s + outputSelection->size();
				const int* sel = inputSelection ? inputSelection->constDataInt() : nullptr;
				for(auto v = v_begin; v != v_end; v += vecComponentCount, ++s) {
					if((!sel || *sel++) && *v >= selectionRangeStart && *v <= selectionRangeEnd) {
						*s = 1;
						numSelected++;
					}
					else *s = 0;
				}
			}
		}
		else if(property->dataType() == PropertyStorage::Int) {
			auto v_begin = property->constDataInt() + vecComponent;
			auto v_end = v_begin + (property->size() * vecComponentCount);
			// Determine value range.
			if(!fixXAxisRange()) {
				intervalStart = std::numeric_limits<FloatType>::max();
				intervalEnd = std::numeric_limits<FloatType>::lowest();
				const int* sel = inputSelection ? inputSelection->constDataInt() : nullptr;
				for(auto v = v_begin; v != v_end; v += vecComponentCount) {
					if(sel && !*sel++) continue;
					if(*v < intervalStart) intervalStart = *v;
					if(*v > intervalEnd) intervalEnd = *v;
				}
			}
			// Perform binning.
			if(intervalEnd > intervalStart) {
				FloatType binSize = (intervalEnd - intervalStart) / histogram->size();
				const int* sel = inputSelection ? inputSelection->constDataInt() : nullptr;
				for(auto v = v_begin; v != v_end; v += vecComponentCount) {
					if(sel && !*sel++) continue;
					if(*v < intervalStart || *v > intervalEnd) continue;
					int binIndex = ((FloatType)*v - intervalStart) / binSize;
					histogramData[std::max(0, std::min(binIndex, (int)histogram->size() - 1))]++;
				}
			}
			else {
				if(!inputSelection)
					histogramData[0] = property->size();
				else
					histogramData[0] = property->size() - std::count(inputSelection->constDataInt(), inputSelection->constDataInt() + inputSelection->size(), 0);
			}
			if(outputSelection) {
				OVITO_ASSERT(outputSelection->size() == property->size());
				int* s = outputSelection->dataInt();
				int* s_end = s + outputSelection->size();
				const int* sel = inputSelection ? inputSelection->constDataInt() : nullptr;
				for(auto v = v_begin; v != v_end; v += vecComponentCount, ++s) {
					if((!sel || *sel++) && *v >= selectionRangeStart && *v <= selectionRangeEnd) {
						*s = 1;
						numSelected++;
					}
					else *s = 0;
				}
			}
		}
		else {
			throwException(tr("The property '%1' has a data type that is not supported by the histogram modifier.").arg(property->name()));
		}
	}
	else {
		intervalStart = intervalEnd = 0;
	}

	// Output a data series object with the histogram data.
	DataSeriesObject* seriesObj = oh.outputDataSeries(QStringLiteral("histogram/") + sourceProperty().nameWithComponent(), tr("Histogram [%1]").arg(sourceProperty().nameWithComponent()), std::move(histogram));
	seriesObj->setAxisLabelX(sourceProperty().nameWithComponent());
	seriesObj->setIntervalStart(intervalStart);
	seriesObj->setIntervalEnd(intervalEnd);

	QString statusMessage;
	if(outputSelection) {
		statusMessage = tr("%1 %2 selected (%3%)")
				.arg(numSelected)
				.arg(propertyClass()->elementDescriptionName())
				.arg((FloatType)numSelected * 100 / std::max(1,(int)outputSelection->size()), 0, 'f', 1);
	}
	output.setStatus(PipelineStatus(PipelineStatus::Success, std::move(statusMessage)));
	return output;
}

}	// End of namespace
}	// End of namespace
