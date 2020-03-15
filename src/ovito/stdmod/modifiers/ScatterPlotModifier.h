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

#pragma once


#include <ovito/stdmod/StdMod.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>
#include <ovito/stdobj/properties/GenericPropertyModifier.h>
#include <ovito/stdobj/properties/PropertyReference.h>

namespace Ovito { namespace StdMod {

/**
 * \brief This modifier computes a scatter plot for two properties.
 */
class OVITO_STDMOD_EXPORT ScatterPlotModifier : public GenericPropertyModifier
{
	Q_OBJECT
	OVITO_CLASS(ScatterPlotModifier)
	Q_CLASSINFO("DisplayName", "Scatter plot");
#ifndef OVITO_BUILD_WEBGUI
	Q_CLASSINFO("ModifierCategory", "Analysis");
#else
	Q_CLASSINFO("ModifierCategory", "-");
#endif

public:

	/// Constructor.
	Q_INVOKABLE ScatterPlotModifier(DataSet* dataset);

	/// This method is called by the system after the modifier has been inserted into a data pipeline.
	virtual void initializeModifier(ModifierApplication* modApp) override;

	/// Modifies the input data synchronously.
	virtual void evaluateSynchronous(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state) override;

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
