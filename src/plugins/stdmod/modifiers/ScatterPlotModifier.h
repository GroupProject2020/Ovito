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

#pragma once


#include <plugins/stdmod/StdMod.h>
#include <core/dataset/pipeline/ModifierApplication.h>
#include <plugins/stdobj/properties/GenericPropertyModifier.h>
#include <plugins/stdobj/properties/PropertyReference.h>

namespace Ovito { namespace StdMod {

/**
 * \brief This modifier computes a scatter plot for two properties.
 */
class OVITO_STDMOD_EXPORT ScatterPlotModifier : public GenericPropertyModifier
{
	Q_OBJECT
	OVITO_CLASS(ScatterPlotModifier)
	Q_CLASSINFO("DisplayName", "Scatter plot");
	Q_CLASSINFO("ModifierCategory", "Analysis");
	
public:

	/// Constructor.
	Q_INVOKABLE ScatterPlotModifier(DataSet* dataset);
	
	/// This method is called by the system after the modifier has been inserted into a data pipeline.
	virtual void initializeModifier(ModifierApplication* modApp) override;

	/// Modifies the input data in an immediate, preliminary way.
	virtual PipelineFlowState evaluatePreliminary(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input) override;
	
	/// Set start and end value of the x-axis.
	void setXAxisRange(FloatType start, FloatType end) { setXAxisRangeStart(start); setXAxisRangeEnd(end); }

	/// Set start and end value of the y-axis.
	void setYAxisRange(FloatType start, FloatType end) { setYAxisRangeStart(start); setYAxisRangeEnd(end); }

protected:
	
	/// Is called when the value of a property of this object has changed.
	virtual void propertyChanged(const PropertyFieldDescriptor& field) override;
	
private:

	/// The property that is used as source for the x-axis.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(PropertyReference, xAxisProperty, setXAxisProperty);

	/// The property that is used as source for the y-axis.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(PropertyReference, yAxisProperty, setYAxisProperty);

	/// Controls the whether elements within the specified range should be selected (x-axis).
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, selectXAxisInRange, setSelectXAxisInRange);

	/// Controls the start value of the selection interval (x-axis).
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, selectionXAxisRangeStart, setSelectionXAxisRangeStart, PROPERTY_FIELD_MEMORIZE);

	/// Controls the end value of the selection interval (x-axis).
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, selectionXAxisRangeEnd, setSelectionXAxisRangeEnd, PROPERTY_FIELD_MEMORIZE);

	/// Controls the whether elements within the specified range should be selected (y-axis).
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, selectYAxisInRange, setSelectYAxisInRange);

	/// Controls the start value of the selection interval (y-axis).
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, selectionYAxisRangeStart, setSelectionYAxisRangeStart, PROPERTY_FIELD_MEMORIZE);

	/// Controls the end value of the selection interval (y-axis).
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, selectionYAxisRangeEnd, setSelectionYAxisRangeEnd, PROPERTY_FIELD_MEMORIZE);

	/// Controls the whether the range of the x-axis of the scatter plot should be fixed.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, fixXAxisRange, setFixXAxisRange);

	/// Controls the start value of the x-axis.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, xAxisRangeStart, setXAxisRangeStart, PROPERTY_FIELD_MEMORIZE);

	/// Controls the end value of the x-axis.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, xAxisRangeEnd, setXAxisRangeEnd, PROPERTY_FIELD_MEMORIZE);

	/// Controls the whether the range of the y-axis of the scatter plot should be fixed.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, fixYAxisRange, setFixYAxisRange);

	/// Controls the start value of the y-axis.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, yAxisRangeStart, setYAxisRangeStart, PROPERTY_FIELD_MEMORIZE);

	/// Controls the end value of the y-axis.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, yAxisRangeEnd, setYAxisRangeEnd, PROPERTY_FIELD_MEMORIZE);
};

}	// End of namespace
}	// End of namespace
