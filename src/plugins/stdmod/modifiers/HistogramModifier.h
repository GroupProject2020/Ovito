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
 * \brief This modifier computes a value histogram for a property.
 */
class OVITO_STDMOD_EXPORT HistogramModifier : public GenericPropertyModifier
{
	Q_OBJECT
	OVITO_CLASS(HistogramModifier)
	
public:

	/// Constructor.
	Q_INVOKABLE HistogramModifier(DataSet* dataset);

	/// \brief Create a new modifier application that refers to this modifier instance.
	virtual OORef<ModifierApplication> createModifierApplication() override;
	
	/// This method is called by the system after the modifier has been inserted into a data pipeline.
	virtual void initializeModifier(ModifierApplication* modApp) override;

	/// Modifies the input data in an immediate, preliminary way.
	virtual PipelineFlowState evaluatePreliminary(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input) override;
	
	/// Set start and end value of the x-axis.
	void setXAxisRange(FloatType start, FloatType end) { 
		setXAxisRangeStart(start); 
		setXAxisRangeEnd(end); 
	}

	/// Set start and end value of the y-axis.
	void setYAxisRange(FloatType start, FloatType end) { 
		setYAxisRangeStart(start); 
		setYAxisRangeEnd(end); 
	}

protected:
	
	/// Is called when the value of a property of this object has changed.
	virtual void propertyChanged(const PropertyFieldDescriptor& field) override;
	
private:

	/// The property that serves as data source of the histogram.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(PropertyReference, sourceProperty, setSourceProperty);

	/// Controls the number of histogram bins.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(int, numberOfBins, setNumberOfBins, PROPERTY_FIELD_MEMORIZE);

	/// Controls the whether elements within the specified range should be selected.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, selectInRange, setSelectInRange);

	/// Controls the start value of the selection interval.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, selectionRangeStart, setSelectionRangeStart, PROPERTY_FIELD_MEMORIZE);

	/// Controls the end value of the selection interval.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, selectionRangeEnd, setSelectionRangeEnd, PROPERTY_FIELD_MEMORIZE);

	/// Controls the whether the range of the x-axis of the histogram should be fixed.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, fixXAxisRange, setFixXAxisRange);

	/// Controls the start value of the x-axis.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, xAxisRangeStart, setXAxisRangeStart, PROPERTY_FIELD_MEMORIZE);

	/// Controls the end value of the x-axis.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, xAxisRangeEnd, setXAxisRangeEnd, PROPERTY_FIELD_MEMORIZE);

	/// Controls the whether the range of the y-axis of the histogram should be fixed.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, fixYAxisRange, setFixYAxisRange);

	/// Controls the start value of the y-axis.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, yAxisRangeStart, setYAxisRangeStart, PROPERTY_FIELD_MEMORIZE);

	/// Controls the end value of the y-axis.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, yAxisRangeEnd, setYAxisRangeEnd, PROPERTY_FIELD_MEMORIZE);

	/// Controls whether the modifier should take into account only selected elements.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, onlySelected, setOnlySelected);

	Q_CLASSINFO("DisplayName", "Histogram");
	Q_CLASSINFO("ModifierCategory", "Analysis");
};

/**
 * \brief The type of ModifierApplication create for a HistogramModifier 
 *        when it is inserted into in a data pipeline.
 */
class OVITO_STDMOD_EXPORT HistogramModifierApplication : public ModifierApplication
{
	OVITO_CLASS(HistogramModifierApplication)
	Q_OBJECT
	
public:

	/// Constructor.
	Q_INVOKABLE HistogramModifierApplication(DataSet* dataset) : ModifierApplication(dataset) {}
 
	/// Returns the stored histogram data.
	const QVector<size_t>& histogramData() const { return _histogramData; }

	/// Returns the start of the histogram's range along the x-axis. 
	FloatType intervalStart() const { return _intervalStart; }

	/// Returns the end of the histogram's range along the x-axis. 
	FloatType intervalEnd() const { return _intervalEnd; }
	
	/// Replaces the stored data.
	void setHistogramData(QVector<size_t> histogramData, FloatType intervalStart, FloatType intervalEnd) {
		_histogramData = std::move(histogramData);
		_intervalStart = intervalStart;
		_intervalEnd = intervalEnd;
		notifyDependents(ReferenceEvent::ObjectStatusChanged);
	}
 
private:

	/// Stores the histogram data.
	QVector<size_t> _histogramData;

	FloatType _intervalStart;
	FloatType _intervalEnd;
};

}	// End of namespace
}	// End of namespace
