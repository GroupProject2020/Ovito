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
#include "PropertyContainer.h"

namespace Ovito { namespace StdObj {

IMPLEMENT_OVITO_CLASS(PropertyContainer);
DEFINE_REFERENCE_FIELD(PropertyContainer, properties);
DEFINE_PROPERTY_FIELD(PropertyContainer, elementCount);
SET_PROPERTY_FIELD_LABEL(PropertyContainer, properties, "Properties");
SET_PROPERTY_FIELD_LABEL(PropertyContainer, elementCount, "Element count");

/******************************************************************************
* Constructor.
******************************************************************************/
PropertyContainer::PropertyContainer(DataSet* dataset) : DataObject(dataset), _elementCount(0)
{
}

/******************************************************************************
* Returns the given standard property. If it does not exist, an exception is thrown.
******************************************************************************/
const PropertyObject* PropertyContainer::expectProperty(int typeId) const
{
	if(!getOOMetaClass().isValidStandardPropertyId(typeId))
		throwException(tr("Selections are not supported for %1.").arg(getOOMetaClass().propertyClassDisplayName()));
	const PropertyObject* property = getProperty(typeId);
	if(!property)
		throwException(tr("Required property '%1' does not exist in the input dataset.").arg(getOOMetaClass().standardPropertyName(typeId)));
	if(property->size() != elementCount())
		throwException(tr("Property array '%1' has wrong length. It does not match the number of elements in the parent container.").arg(property->name()));
	return property;
}

/******************************************************************************
* Returns the property with the given name and data layout.
******************************************************************************/
const PropertyObject* PropertyContainer::expectProperty(const QString& propertyName, int dataType, size_t componentCount) const
{
	const PropertyObject* property = getProperty(propertyName);
	if(!property)
		throwException(tr("Required property '%1' does not exist in the input dataset.").arg(propertyName));
	if(property->dataType() != dataType)
		throwException(tr("Property '%1' does not have the required data type in the pipeline dataset.").arg(property->name()));
	if(property->componentCount() != componentCount)
		throwException(tr("Property '%1' does not have the required number of components in the pipeline dataset.").arg(property->name()));
	if(property->size() != elementCount())
		throwException(tr("Property array '%1' has wrong length. It does not match the number of elements in the parent container.").arg(property->name()));
	return property;
}

/******************************************************************************
* Duplicates any property objects that are shared with other containers.
* After this method returns, all property objects are exclusively owned by the container and
* can be safely modified without unwanted side effects.
******************************************************************************/
void PropertyContainer::makePropertiesMutable()
{
	for(int i = properties().size() - 1; i >= 0; i--) {
		makeMutable(properties()[i]);
	}
}

/******************************************************************************
* Sets the current number of data elements stored in the container.
* The lengths of the property arrays will be adjusted accordingly.
******************************************************************************/
void PropertyContainer::setElementCount(size_t count)
{
	if(count == elementCount())
		return;

	// Make sure the property arrays can be safely modified.
    makePropertiesMutable();

	// Resize the arrays.
	for(PropertyObject* prop : properties())
		prop->resize(count, true);

	// Update internal element counter.
	_elementCount.set(this, PROPERTY_FIELD(elementCount), count);
}

/******************************************************************************
* Deletes those data elements for which the bit is set in the given bitmask array.
* Returns the number of deleted elements.
******************************************************************************/
size_t PropertyContainer::deleteElements(const boost::dynamic_bitset<>& mask)
{
	OVITO_ASSERT(mask.size() == elementCount());

	size_t deleteCount = mask.count();
	size_t oldElementCount = elementCount();
	size_t newElementCount = oldElementCount - deleteCount;
	if(deleteCount == 0)
		return 0;	// Nothing to delete.

    // Make sure the property arrays can be safely modified.
    makePropertiesMutable();

	// Filter the property arrays and reduce their lengths.
	for(PropertyObject* property : properties()) {
        OVITO_ASSERT(property->size() == oldElementCount);
        property->filterResize(mask);
        OVITO_ASSERT(property->size() == newElementCount);
	}

	// Update internal element counter.
	_elementCount.set(this, PROPERTY_FIELD(elementCount), newElementCount);

	return deleteCount;
}

/******************************************************************************
* Creates a property and adds it to the container.
* In case the property already exists, it is made sure that it's safe to modify it.
******************************************************************************/
PropertyObject* PropertyContainer::createProperty(int typeId, bool initializeMemory, const ConstDataObjectPath& containerPath, size_t elementCountHint)
{
	// Undo recording should never be active during pipeline evaluation.
	OVITO_ASSERT(!dataset()->undoStack().isRecording());

	if(getOOMetaClass().isValidStandardPropertyId(typeId) == false) {
		if(typeId == PropertyStorage::GenericSelectionProperty)
			throwException(tr("Creating selections is not supported for %1.").arg(getOOMetaClass().propertyClassDisplayName()));
		else if(typeId == PropertyStorage::GenericColorProperty)
			throwException(tr("Assigning colors is not supported for %1.").arg(getOOMetaClass().propertyClassDisplayName()));
		else
			throwException(tr("%1 is not a standard property ID supported by the '%2' object class.").arg(typeId).arg(getOOMetaClass().propertyClassDisplayName()));
	}

	// Check if property already exists in the output.
	if(const PropertyObject* existingProperty = getProperty(typeId)) {
		PropertyObject* newProperty = makeMutable(existingProperty);
		if(newProperty != existingProperty && !initializeMemory) {
			// If no memory initialization is requested, create a new storage buffer to avoid copying the contents of the old one when
			// a deep copy is made on the first write access.
			newProperty->setStorage(getOOMetaClass().createStandardStorage(newProperty->size(), typeId, false));
		}
		OVITO_ASSERT(newProperty->numberOfStrongReferences() == 1);
		OVITO_ASSERT(newProperty->size() == elementCount());
		return newProperty;
	}
	else {
		// Create a new property object.
		size_t count = !properties().empty() ? elementCount() : elementCountHint;
		OORef<PropertyObject> newProperty = getOOMetaClass().createFromStorage(dataset(), getOOMetaClass().createStandardStorage(count, typeId, initializeMemory, containerPath));
		addProperty(newProperty);
		return newProperty;
	}
}

/******************************************************************************
* Creates a user-defined property and adds it to the container.
* In case the property already exists, it is made sure that it's safe to modify it.
******************************************************************************/
PropertyObject* PropertyContainer::createProperty(const QString& name, int dataType, size_t componentCount, size_t stride, bool initializeMemory, size_t elementCountHint)
{
	// Undo recording should never be active during pipeline evaluation.
	OVITO_ASSERT(!dataset()->undoStack().isRecording());

	// Check if property already exists in the output.
	const PropertyObject* existingProperty = getProperty(name);

	// Check if property already exists in the output.
	if(existingProperty) {
		if(existingProperty->dataType() != dataType)
			throwException(tr("Existing property '%1' has a different data type.").arg(name));
		if(existingProperty->componentCount() != componentCount)
			throwException(tr("Existing property '%1' has a different number of components.").arg(name));
		if(stride != 0 && existingProperty->stride() != stride)
			throwException(tr("Existing property '%1' has a different stride.").arg(name));

		PropertyObject* newProperty = makeMutable(existingProperty);
		if(newProperty != existingProperty && !initializeMemory) {
			// If no memory initialization is requested, create a new storage buffer to avoid copying the contents of the old one when
			// a deep copy is made on the first write access.
			newProperty->setStorage(std::make_shared<PropertyStorage>(newProperty->size(), dataType, componentCount, stride, name, false));
		}
		OVITO_ASSERT(newProperty->numberOfStrongReferences() == 1);
		OVITO_ASSERT(newProperty->size() == elementCount());
		return newProperty;
	}
	else {
		// Create a new property object.
		size_t count = !properties().empty() ? elementCount() : elementCountHint;
		OORef<PropertyObject> newProperty = getOOMetaClass().createFromStorage(dataset(), std::make_shared<PropertyStorage>(count, dataType, componentCount, stride, name, initializeMemory));
		addProperty(newProperty);
		return newProperty;
	}
}

/******************************************************************************
* Creates a new particle property and adds it to the container.
******************************************************************************/
PropertyObject* PropertyContainer::createProperty(const PropertyPtr& storage)
{
	OVITO_CHECK_POINTER(storage);

	// Undo recording should never be active during pipeline evaluation.
	OVITO_ASSERT(!dataset()->undoStack().isRecording());

	// Length of first property array determines number of data elements in the container.
	if(properties().empty() && elementCount() == 0)
		_elementCount.set(this, PROPERTY_FIELD(elementCount), storage->size());

	// Length of new property array must match the existing number of elements.
	if(storage->size() != elementCount()) {
#ifdef OVITO_DEBUG
		qDebug() << "Property array size mismatch. Container has" << elementCount() << "existing elements. New property" << storage->name() << "to be added has" << storage->size() << "elements.";
#endif
		throwException(tr("Cannot add new %1 property '%2': Array length is not consistent with number of elements in the parent container.").arg(getOOMetaClass().propertyClassDisplayName()).arg(storage->name()));
	}

	// Check if property already exists in the output.
	const PropertyObject* existingProperty;
	if(storage->type() != 0) {
		existingProperty = getProperty(storage->type());
	}
	else {
		existingProperty = nullptr;
		for(const PropertyObject* property : properties()) {
			if(property->type() == 0 && property->name() == storage->name()) {
				if(property->dataType() != storage->dataType() || property->dataTypeSize() != storage->dataTypeSize())
					throwException(tr("Existing property '%1' in the pipeline dataset has a different data type.").arg(property->name()));
				if(property->componentCount() != storage->componentCount())
					throwException(tr("Existing property '%1' in the pipeline dataset has a different number of components.").arg(property->name()));
					existingProperty = property;
				break;
			}
		}
	}

	if(existingProperty) {
		PropertyObject* newProperty = makeMutable(existingProperty);
		OVITO_ASSERT(storage->stride() == newProperty->stride());
		newProperty->setStorage(storage);
		return newProperty;
	}
	else {
		// Create a new property in the output.
		OORef<PropertyObject> newProperty = getOOMetaClass().createFromStorage(dataset(), storage);
		addProperty(newProperty);
		OVITO_ASSERT(newProperty->size() == elementCount());
		return newProperty;
	}
}

/******************************************************************************
* Replaces the property arrays in this property container with a new set of
* properties.
******************************************************************************/
void PropertyContainer::setContent(size_t newElementCount, const std::vector<PropertyPtr>& newProperties)
{
	OVITO_ASSERT(!dataset()->undoStack().isRecording());

	// Removal phase:
	for(int i = properties().size() - 1; i >= 0; i--) {
		PropertyObject* property = properties()[i];
		if(std::find(newProperties.cbegin(), newProperties.cend(), property->storage()) == newProperties.cend())
			removeProperty(property);
	}

	// Update internal element counter.
	_elementCount.set(this, PROPERTY_FIELD(elementCount), newElementCount);

	// Insertion phase:
	for(const auto& property : newProperties) {
		// Lengths of new property arrays must be consistent.
		if(property->size() != newElementCount) {
			OVITO_ASSERT(false);
			throwException(tr("Cannot add new %1 property '%2': Array length does not match number of elements in the parent container.").arg(getOOMetaClass().propertyClassDisplayName()).arg(property->name()));
		}

		const PropertyObject* propertyObj = (property->type() != 0) ? getProperty(property->type()) : getProperty(property->name());
		if(propertyObj) {
			makeMutable(propertyObj)->setStorage(property);
		}
		else {
			OORef<PropertyObject> newProperty = getOOMetaClass().createFromStorage(dataset(), property);
			addProperty(newProperty);
		}
	}
}


/******************************************************************************
* Duplicates all data elements by extensing the property arrays and
* replicating the existing data N times.
******************************************************************************/
void PropertyContainer::replicate(size_t n)
{
	OVITO_ASSERT(n >= 1);
	if(n <= 1) return;

	size_t newCount = elementCount() * n;
	if(newCount / n != elementCount())
		throwException(tr("Replicate operation failed: Maximum number of elements exceeded."));

	// Make sure the property arrays can be safely modified.
    makePropertiesMutable();

	for(PropertyObject* property : properties())
		property->replicate(n);

	setElementCount(newCount);
}

}	// End of namespace
}	// End of namespace
