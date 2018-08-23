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

#include <plugins/stdobj/StdObj.h>
#include <core/dataset/DataSet.h>
#include "DataSeriesObject.h"
#include "DataSeriesProperty.h"

namespace Ovito { namespace StdObj {

IMPLEMENT_OVITO_CLASS(DataSeriesObject);
DEFINE_PROPERTY_FIELD(DataSeriesObject, title);
DEFINE_PROPERTY_FIELD(DataSeriesObject, intervalStart);
DEFINE_PROPERTY_FIELD(DataSeriesObject, intervalEnd);
DEFINE_PROPERTY_FIELD(DataSeriesObject, axisLabelX);
DEFINE_PROPERTY_FIELD(DataSeriesObject, axisLabelY);
SET_PROPERTY_FIELD_CHANGE_EVENT(DataSeriesObject, title, ReferenceEvent::TitleChanged);

/******************************************************************************
* Constructor.
******************************************************************************/
DataSeriesObject::DataSeriesObject(DataSet* dataset) : DataObject(dataset),
	_intervalStart(0),
	_intervalEnd(0)
{
}

/******************************************************************************
* Returns the display title of this object in the user interface.
******************************************************************************/
QString DataSeriesObject::objectTitle() 
{
	return !title().isEmpty() ? title() : identifier();
}

/******************************************************************************
* Returns the property object containing the y-coordinates of the data points.
******************************************************************************/
DataSeriesProperty* DataSeriesObject::getY(const PipelineFlowState& state) const
{
	return DataSeriesProperty::findInState(state, DataSeriesProperty::YProperty, identifier());
}

/******************************************************************************
* Returns the property object containing the x-coordinates of the data points.
******************************************************************************/
DataSeriesProperty* DataSeriesObject::getX(const PipelineFlowState& state) const
{
	return DataSeriesProperty::findInState(state, DataSeriesProperty::XProperty, identifier());
}

/******************************************************************************
* Returns the data array containing the y-coordinates of the data points.
******************************************************************************/
ConstPropertyPtr DataSeriesObject::getYStorage(const PipelineFlowState& state) const
{
	if(DataSeriesProperty* property = getY(state))
		return property->storage();
	return nullptr;
}

/******************************************************************************
* Returns the data array containing the x-coordinates of the data points.
* If no explicit x-coordinate data is available, the array is dynamically generated 
* from the x-axis interval set for this data series.
******************************************************************************/
ConstPropertyPtr DataSeriesObject::getXStorage(const PipelineFlowState& state) const
{
	if(DataSeriesProperty* xProperty = getX(state)) {
		return xProperty->storage();
	}
	else if(DataSeriesProperty* yProperty = getY(state)) {
		auto xdata = std::make_shared<PropertyStorage>(yProperty->size(), PropertyStorage::Float, 1, 0, QString(), false, DataSeriesProperty::XProperty);
		FloatType binSize = (intervalEnd() - intervalStart()) / xdata->size();
		FloatType x = intervalStart() + binSize * FloatType(0.5);
		for(FloatType& v : xdata->floatRange()) {
			v = x;
			x += binSize;
		}
		return std::move(xdata);
	}
	else return nullptr;
}

}	// End of namespace
}	// End of namespace
