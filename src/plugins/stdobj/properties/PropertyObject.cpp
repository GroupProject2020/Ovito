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

#include <plugins/stdobj/StdObj.h>
#include <core/dataset/DataSet.h>
#include <core/dataset/pipeline/PipelineFlowState.h>
#include "PropertyObject.h"

namespace Ovito { namespace StdObj {

IMPLEMENT_OVITO_CLASS(PropertyObject);	
DEFINE_PROPERTY_FIELD(PropertyObject, storage);
DEFINE_REFERENCE_FIELD(PropertyObject, elementTypes);
SET_PROPERTY_FIELD_LABEL(PropertyObject, elementTypes, "Element types");

/// Holds a shared, empty instance of the PropertyStorage class, 
/// which is used in places where a default storage is needed.
/// This singleton instance is never modified.
static const PropertyPtr defaultStorage = std::make_shared<PropertyStorage>();

/******************************************************************************
* Constructor.
******************************************************************************/
PropertyObject::PropertyObject(DataSet* dataset) : DataObject(dataset), _storage(defaultStorage)
{
}

/******************************************************************************
* Returns the data encapsulated by this object after making sure it is not 
* shared with other owners.
******************************************************************************/
const PropertyPtr& PropertyObject::modifiableStorage() 
{
	// Copy data storage on write if there is more than one reference to the storage.
	OVITO_ASSERT(storage());
	OVITO_ASSERT(storage().use_count() >= 1);
	if(storage().use_count() > 1)
		_storage.mutableValue() = std::make_shared<PropertyStorage>(*storage());
	OVITO_ASSERT(storage().use_count() == 1);
	return storage();
}

/******************************************************************************
* Resizes the property storage.
******************************************************************************/
void PropertyObject::resize(size_t newSize, bool preserveData)
{
	modifiableStorage()->resize(newSize, preserveData);
	notifyTargetChanged();
}

/******************************************************************************
* Sets the property's name.
******************************************************************************/
void PropertyObject::setName(const QString& newName)
{
	if(newName == name())
		return;

	modifiableStorage()->setName(newName);
	notifyTargetChanged();
}

/******************************************************************************
* Returns the display title of this property object in the user interface.
******************************************************************************/
QString PropertyObject::objectTitle() 
{
	// User-defined properties always have a user-defined name.
	if(type() == 0)
		return name();

	return getOOMetaClass().standardPropertyTitle(type());
}

/******************************************************************************
* Saves the class' contents to the given stream.
******************************************************************************/
void PropertyObject::saveToStream(ObjectSaveStream& stream, bool excludeRecomputableData)
{
	DataObject::saveToStream(stream, excludeRecomputableData);

	stream.beginChunk(0x01);
	OVITO_ASSERT(storage());
	storage()->saveToStream(stream, excludeRecomputableData);
	stream.endChunk();
}

/******************************************************************************
* Loads the class' contents from the given stream.
******************************************************************************/
void PropertyObject::loadFromStream(ObjectLoadStream& stream)
{
	DataObject::loadFromStream(stream);

	stream.expectChunk(0x01);
	PropertyPtr s = std::make_shared<PropertyStorage>();
	s->loadFromStream(stream);
	setStorage(s);
	stream.closeChunk();
}

/******************************************************************************
* Determines whether this property object belongs to the given property bundle.
******************************************************************************/
bool PropertyObject::belongsToBundle(const QString& bundleName) const
{
	return identifier() == bundleName;
}

/******************************************************************************
* Makes this property object part of the givenb property bundle.
******************************************************************************/
void PropertyObject::setBundle(const QString& bundleName)
{
	setIdentifier(bundleName);
}


}	// End of namespace
}	// End of namespace
