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

#include <ovito/stdobj/StdObj.h>
#include <ovito/core/dataset/DataSet.h>
#include "PropertyObject.h"

namespace Ovito { namespace StdObj {

IMPLEMENT_OVITO_CLASS(PropertyObject);
DEFINE_PROPERTY_FIELD(PropertyObject, storage);
DEFINE_REFERENCE_FIELD(PropertyObject, elementTypes);
DEFINE_PROPERTY_FIELD(PropertyObject, title);
SET_PROPERTY_FIELD_LABEL(PropertyObject, elementTypes, "Element types");
SET_PROPERTY_FIELD_LABEL(PropertyObject, title, "Title");
SET_PROPERTY_FIELD_CHANGE_EVENT(PropertyObject, title, ReferenceEvent::TitleChanged);

/// Holds a shared, empty instance of the PropertyStorage class,
/// which is used in places where a default storage is needed.
/// This singleton instance is never modified.
static const PropertyPtr defaultStorage = std::make_shared<PropertyStorage>();

/******************************************************************************
* Constructor.
******************************************************************************/
PropertyObject::PropertyObject(DataSet* dataset, const PropertyPtr& storage) : DataObject(dataset), _storage(storage ? storage : defaultStorage)
{
}

/******************************************************************************
* Returns the data encapsulated by this object after making sure it is not
* shared with other owners.
******************************************************************************/
const PropertyPtr& PropertyObject::modifiableStorage()
{
	// Copy data buffer if there is more than one active reference to the storage.
	return PropertyStorage::makeMutable(_storage.mutableValue());
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
QString PropertyObject::objectTitle() const
{
	return title().isEmpty() ? name() : title();
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
	setStorage(std::move(s));
	stream.closeChunk();
}

/******************************************************************************
* Extends the data array and replicates the old data N times.
******************************************************************************/
void PropertyObject::replicate(size_t n, bool replicateValues)
{
	OVITO_ASSERT(n >= 1);
	if(n <= 1) return;
	ConstPropertyPtr oldData = storage();
	resize(oldData->size() * n, false);
	if(replicateValues) {
		// Replicate data values N times.
		size_t chunkSize = stride() * oldData->size();
		for(size_t i = 0; i < n; i++) {
			std::memcpy((char*)data() + i * chunkSize, oldData->constData(), chunkSize);
		}
	}
	else {
		// Copy just one replica of the data from the old memory buffer to the new one.
		std::memcpy((char*)data(), oldData->constData(), stride() * oldData->size());
	}
}

/******************************************************************************
* Puts the property array into a writable state.
* In the writable state, the Python binding layer will allow write access
* to the property's internal data.
******************************************************************************/
void PropertyObject::makeWritableFromPython()
{
	if(!isSafeToModify())
		throwException(tr("Modifying the values of this property is not allowed, because it is currently shared by more than one property container or data collection. Please explicitly request a mutable version of the property by using the '_' notation."));
	_isWritableFromPython++;
}

/******************************************************************************
* Helper method that remaps the existing type IDs to a contiguous range starting at the given
* base ID. This method is mainly used for file output, because some file formats
* work with numeric particle types only, which must form a contiguous range.
* The method returns the mapping of output type IDs to original type IDs
* and a copy of the property array in which the original type ID values have
* been remapped to the output IDs.
******************************************************************************/
std::tuple<std::map<int,int>, ConstPropertyPtr> PropertyObject::generateContiguousTypeIdMapping(int baseId) const
{
	// Generate sorted list of existing type IDs.
	std::set<int> typeIds;
	for(const ElementType* t : elementTypes())
		typeIds.insert(t->numericId());

	// Add ID values that occur in the property array but which have not been defined as a type.
	for(int t : storage()->constIntRange())
		typeIds.insert(t);

	// Build the mappings between old and new IDs.
	std::map<int,int> oldToNewMap;
	std::map<int,int> newToOldMap;
	bool remappingRequired = false;
	for(int id : typeIds) {
		if(id != baseId) remappingRequired = true;
		oldToNewMap.emplace(id, baseId);
		newToOldMap.emplace(baseId++, id);
	}

	// Create a copy of the per-element type array in which old IDs have been replaced with new ones.
	PropertyPtr remappedArray;
	if(remappingRequired) {
		remappedArray = std::make_shared<PropertyStorage>(*storage());
		for(int& id : remappedArray->intRange())
			id = oldToNewMap[id];
	}
	else {
		remappedArray = storage();
	}

	return std::make_tuple(std::move(newToOldMap), std::move(remappedArray));
}


}	// End of namespace
}	// End of namespace