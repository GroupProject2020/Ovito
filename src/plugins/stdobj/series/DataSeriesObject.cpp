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

#if 0
/******************************************************************************
* Determines the X value for the given array index.
******************************************************************************/
FloatType DataSeriesObject::getXValue(size_t index) const
{
	if(x() && x()->size() > index && x()->componentCount() == 1) {
		if(x()->dataType() == PropertyStorage::Float)
			return x()->getFloat(index);
		else if(x()->dataType() == PropertyStorage::Int)
			return x()->getInt(index);
		else if(x()->dataType() == PropertyStorage::Int64)
			return x()->getInt64(index);
	}
	if(y() && y()->size() != 0) {
		FloatType binSize = (intervalEnd() - intervalStart()) / y()->size();
		return intervalStart() + binSize * (FloatType(0.5) + index);
	}
	return 0;
}
#endif

}	// End of namespace
}	// End of namespace
