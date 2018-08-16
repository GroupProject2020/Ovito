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

#pragma once


#include <plugins/stdobj/StdObj.h>
#include <plugins/stdobj/properties/PropertyStorage.h>
#include <core/dataset/data/DataObject.h>

namespace Ovito { namespace StdObj {
	
/**
 * \brief Holds a series of data values.
 */
class OVITO_STDOBJ_EXPORT DataSeriesObject : public DataObject
{
	Q_OBJECT
	OVITO_CLASS(DataSeriesObject)
	
public:

	/// Constructor.
	Q_INVOKABLE DataSeriesObject(DataSet* dataset);

	/// Returns the x-axis data array after making sure it is not 
	/// shared with others and can be safely modified.
	const PropertyPtr& modifiableX();

	/// Returns the y-axis data array after making sure it is not 
	/// shared with others and can be safely modified.
	const PropertyPtr& modifiableY();

	/// Returns the text label for the x-axis.
	const QString& getAxisLabelXAny() const {
		if(x() && !x()->name().isEmpty()) return x()->name();
		else return axisLabelX();
	}

	/// Returns the text label for the y-axis.
	const QString& getAxisLabelYAny() const {
		if(y() && !y()->name().isEmpty()) return y()->name();
		else return axisLabelY();
	}

	/// Determines the X value for the given array index.
	FloatType getXValue(size_t index) const;

	//////////////////////////////// from RefTarget //////////////////////////////

	/// Returns the display title of this object in the user interface.
	virtual QString objectTitle() override;

protected:

	/// Saves the class' contents to the given stream.
	virtual void saveToStream(ObjectSaveStream& stream, bool excludeRecomputableData) override;

	/// Loads the class' contents from the given stream.
	virtual void loadFromStream(ObjectLoadStream& stream) override;

private:

	/// The title of the data series.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(QString, title, setTitle);

	/// The x-coordinates of the data points (optional).
	DECLARE_RUNTIME_PROPERTY_FIELD(PropertyPtr, x, setX);

	/// The y-coordinates of the data points.
	DECLARE_RUNTIME_PROPERTY_FIELD(PropertyPtr, y, setY);

	/// The lower bound of the x-interval of the histogram if data points have no explicit x-coordinates.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, intervalStart, setIntervalStart);

	/// The upper bound of the x-interval of the histogram if data points have no explicit x-coordinates.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, intervalEnd, setIntervalEnd);

	/// The label of the x-axis (optional).
	DECLARE_MODIFIABLE_PROPERTY_FIELD(QString, axisLabelX, setAxisLabelX);

	/// The label of the y-axis (optional).
	DECLARE_MODIFIABLE_PROPERTY_FIELD(QString, axisLabelY, setAxisLabelY);
};

}	// End of namespace
}	// End of namespace
