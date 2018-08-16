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
DEFINE_PROPERTY_FIELD(DataSeriesObject, x);
DEFINE_PROPERTY_FIELD(DataSeriesObject, y);
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
	return title();
}

/******************************************************************************
* Returns the x-axis data array after making sure it is not 
* shared with others and can be safely modified.
******************************************************************************/
const PropertyPtr& DataSeriesObject::modifiableX() 
{
	// Copy data storage on write if there is more than one reference to the storage.
	if(x()) {
		OVITO_ASSERT(x().use_count() >= 1);
		if(x().use_count() > 1)
			_x.mutableValue() = std::make_shared<PropertyStorage>(*x());
		OVITO_ASSERT(x().use_count() == 1);
	}
	return x();
}

/******************************************************************************
* Returns the y-axis data array after making sure it is not 
* shared with others and can be safely modified.
******************************************************************************/
const PropertyPtr& DataSeriesObject::modifiableY() 
{
	// Copy data storage on write if there is more than one reference to the storage.
	if(y()) {
		OVITO_ASSERT(y().use_count() >= 1);
		if(y().use_count() > 1)
			_y.mutableValue() = std::make_shared<PropertyStorage>(*y());
		OVITO_ASSERT(y().use_count() == 1);
	}
	return y();
}

/******************************************************************************
* Saves the class' contents to the given stream.
******************************************************************************/
void DataSeriesObject::saveToStream(ObjectSaveStream& stream, bool excludeRecomputableData)
{
	DataObject::saveToStream(stream, excludeRecomputableData);

	if(x()) {
		stream.beginChunk(0x0101);
		x()->saveToStream(stream, excludeRecomputableData);
		stream.endChunk();
	}
	else {
		stream.beginChunk(0x0100);
		stream.endChunk();
	}

	if(y()) {
		stream.beginChunk(0x0201);
		y()->saveToStream(stream, excludeRecomputableData);
		stream.endChunk();
	}
	else {
		stream.beginChunk(0x0200);
		stream.endChunk();
	}
}

/******************************************************************************
* Loads the class' contents from the given stream.
******************************************************************************/
void DataSeriesObject::loadFromStream(ObjectLoadStream& stream)
{
	DataObject::loadFromStream(stream);

	if(stream.expectChunkRange(0x0100, 2) == 1) {
		PropertyPtr s = std::make_shared<PropertyStorage>();
		s->loadFromStream(stream);
		setX(s);
	}
	stream.closeChunk();

	if(stream.expectChunkRange(0x0200, 2) == 1) {
		PropertyPtr s = std::make_shared<PropertyStorage>();
		s->loadFromStream(stream);
		setY(s);
	}
	stream.closeChunk();
}

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

}	// End of namespace
}	// End of namespace
