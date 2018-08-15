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
#include <plugins/stdobj/properties/PropertyObject.h>
#include <plugins/stdobj/properties/PropertyClass.h>
#include <plugins/stdobj/series/DataSeriesObject.h>
#include "OutputHelper.h"

namespace Ovito { namespace StdObj {

/******************************************************************************
* Creates a standard particle in the modifier's output.
* If the particle property already exists in the input, its contents are copied to the
* output property by this method.
******************************************************************************/
PropertyObject* OutputHelper::outputStandardProperty(const PropertyClass& propertyClass, int typeId, bool initializeMemory)
{
	if(propertyClass.isValidStandardPropertyId(typeId) == false) {
		if(typeId == PropertyStorage::GenericSelectionProperty)
			dataset()->throwException(PropertyObject::tr("Selection is not supported by the '%2' property class.").arg(propertyClass.propertyClassDisplayName()));
		else if(typeId == PropertyStorage::GenericColorProperty)
			dataset()->throwException(PropertyObject::tr("Coloring is not supported by the '%2' property class.").arg(propertyClass.propertyClassDisplayName()));
		else
			dataset()->throwException(PropertyObject::tr("%1 is not a standard property ID supported by the '%2' property class.").arg(typeId).arg(propertyClass.propertyClassDisplayName()));
	}

	// Check if property already exists in the output.
	if(PropertyObject* existingProperty = propertyClass.findInState(output(), typeId)) {
		PropertyObject* property = cloneIfNeeded(existingProperty);

		// If no memory initialization is requested, create a new storage buffer to avoid copying the contents of the old one when
		// a deep copy is made on the first write access.
		if(!initializeMemory && property != existingProperty) {
			property->setStorage(propertyClass.createStandardStorage(property->size(), typeId, false));
		}

		OVITO_ASSERT(property->size() == propertyClass.elementCount(output()));
		return property;
	}
	else {
		// Create a new particle property in the output.
		OORef<PropertyObject> property = propertyClass.createFromStorage(dataset(), propertyClass.createStandardStorage(propertyClass.elementCount(output()), typeId, initializeMemory));
		output().addObject(property);

		OVITO_ASSERT(property->size() == propertyClass.elementCount(output()));
		return property;
	}
}

/******************************************************************************
* Creates a standard particle in the modifier's output and sets its content.
******************************************************************************/
PropertyObject* OutputHelper::outputProperty(const PropertyClass& propertyClass, const PropertyPtr& storage)
{
	OVITO_CHECK_POINTER(storage);

	// Length of new property array must match the existing number of elements.
	if(storage->size() != propertyClass.elementCount(output()))
		dataset()->throwException(PropertyObject::tr("Cannot add new %1 property '%2': Number of elements does not match.").arg(propertyClass.propertyClassDisplayName()).arg(storage->name()));

	// Check if property already exists in the output.
	PropertyObject* existingProperty;
	if(storage->type() != 0) {
		existingProperty = propertyClass.findInState(output(), storage->type());
	}
	else {
		existingProperty = nullptr;
		for(DataObject* o : output().objects()) {
			PropertyObject* property = dynamic_object_cast<PropertyObject>(o);
			if(property && propertyClass.isMember(property) && property->type() == 0 && property->name() == storage->name()) {
				if(property->dataType() != storage->dataType() || property->dataTypeSize() != storage->dataTypeSize())
					dataset()->throwException(PropertyObject::tr("Existing property '%1' has a different data type.").arg(property->name()));
				if(property->componentCount() != storage->componentCount())
					dataset()->throwException(PropertyObject::tr("Existing property '%1' has a different number of components.").arg(property->name()));
					existingProperty = property;
				break;
			}
		}
	}
		
	// Check if property already exists in the output.
	if(existingProperty) {
		PropertyObject* property = cloneIfNeeded(existingProperty);
		OVITO_ASSERT(storage->size() == property->size());
		OVITO_ASSERT(storage->stride() == property->stride());
		property->setStorage(storage);

		OVITO_ASSERT(property->size() == propertyClass.elementCount(output()));
		return property;
	}
	else {
		// Create a new property in the output.
		OORef<PropertyObject> property = propertyClass.createFromStorage(dataset(), storage);
		output().addObject(property);

		OVITO_ASSERT(property->size() == propertyClass.elementCount(output()));
		return property;
	}
}

/******************************************************************************
* Creates a custom particle property in the modifier's output.
******************************************************************************/
PropertyObject* OutputHelper::outputCustomProperty(const PropertyClass& propertyClass, const QString& name, int dataType, size_t componentCount, size_t stride, bool initializeMemory)
{
	// Check if property already exists in the output.
	PropertyObject* existingProperty = nullptr;
	for(DataObject* o : output().objects()) {
		PropertyObject* property = dynamic_object_cast<PropertyObject>(o);
		if(property && propertyClass.isMember(property) && property->type() == 0 && property->name() == name) {
			if(property->dataType() != dataType)
				dataset()->throwException(PropertyObject::tr("Existing property '%1' has a different data type.").arg(name));
			if(property->componentCount() != componentCount)
				dataset()->throwException(PropertyObject::tr("Existing property '%1' has a different number of components.").arg(name));
			if(stride != 0 && property->stride() != stride)
				dataset()->throwException(PropertyObject::tr("Existing property '%1' has a different stride.").arg(name));
			existingProperty = property;
			break;
		}
	}

	// Check if property already exists in the output.
	if(existingProperty) {
		PropertyObject* property = cloneIfNeeded(existingProperty);

		// If no memory initialization is requested, create a new storage buffer to avoid copying the contents of the old one when
		// a deep copy is made on the first write access.
		if(!initializeMemory && property != existingProperty) {
			property->setStorage(std::make_shared<PropertyStorage>(propertyClass.elementCount(output()), dataType, componentCount, stride, name, false));
		}
		
		OVITO_ASSERT(property->size() == propertyClass.elementCount(output()));
		return property;
	}
	else {
		// Create a new property in the output.
		OORef<PropertyObject> property = propertyClass.createFromStorage(dataset(), std::make_shared<PropertyStorage>(propertyClass.elementCount(output()), dataType, componentCount, stride, name, initializeMemory));
		output().addObject(property);

		OVITO_ASSERT(property->size() == propertyClass.elementCount(output()));
		return property;
	}
}


/******************************************************************************
* Emits a new global attribute to the pipeline.
******************************************************************************/
void OutputHelper::outputAttribute(const QString& key, QVariant value)
{
	if(!output().attributes().contains(key)) {
		output().attributes().insert(key, std::move(value));
	}
	else {
		for(int i = 2; ; i++) {
			QString uniqueKey = key + QChar('.') + QString::number(i);
			if(!output().attributes().contains(uniqueKey)) {
				output().attributes().insert(uniqueKey, std::move(value));
				return;
			}
		}
		OVITO_ASSERT(false);
	}
}

/******************************************************************************
* Returns a name for a new data series object that does not collide with the 
* name of an existing data series in the same data collection.
******************************************************************************/
QString OutputHelper::generateUniqueSeriesName(const QString& baseName) const
{
	auto doesNameExist = [this](const QString& name) {
		for(DataObject* obj : output().objects()) {
			if(DataSeriesObject* seriesObj = dynamic_object_cast<DataSeriesObject>(obj)) {
				if(seriesObj->title() == name)
					return true;
			}
		}
		return false;
	};
	if(!doesNameExist(baseName)) {
		return baseName;
	}
	else {
		for(int i = 2; ; i++) {
			QString uniqueName = baseName + QChar('.') + QString::number(i);
			if(!doesNameExist(uniqueName)) {
				return uniqueName;
			}
		}
	}
	OVITO_ASSERT(false);
}

}	// End of namespace
}	// End of namespace
