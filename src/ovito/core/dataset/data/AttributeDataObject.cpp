////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2018 Alexander Stukowski
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

#include <ovito/core/Core.h>
#include "AttributeDataObject.h"

namespace Ovito {

IMPLEMENT_OVITO_CLASS(AttributeDataObject);
DEFINE_PROPERTY_FIELD(AttributeDataObject, value);
SET_PROPERTY_FIELD_LABEL(AttributeDataObject, value, "Value");

/******************************************************************************
* Saves the class' contents to the given stream.
******************************************************************************/
void AttributeDataObject::saveToStream(ObjectSaveStream& stream, bool excludeRecomputableData)
{
	DataObject::saveToStream(stream, excludeRecomputableData);
	stream.beginChunk(0x01);
	stream << value();
	stream.endChunk();
}

/******************************************************************************
* Loads the class' contents from the given stream.
******************************************************************************/
void AttributeDataObject::loadFromStream(ObjectLoadStream& stream)
{
	DataObject::loadFromStream(stream);
	stream.expectChunk(0x01);
	stream >> _value.mutableValue();
	stream.closeChunk();
}

}	// End of namespace
