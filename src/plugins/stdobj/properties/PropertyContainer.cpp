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
#include <core/oo/CloneHelper.h>
#include "PropertyContainer.h"

#include <boost/optional.hpp>

namespace Ovito { namespace StdObj {

IMPLEMENT_OVITO_CLASS(PropertyContainer);	
DEFINE_REFERENCE_FIELD(PropertyContainer, properties);
SET_PROPERTY_FIELD_LABEL(PropertyContainer, properties, "Properties");

/******************************************************************************
* Constructor.
******************************************************************************/
PropertyContainer::PropertyContainer(DataSet* dataset) : DataObject(dataset)
{
}

/******************************************************************************
* Returns the given standard property. If it does not exist, an exception is thrown.
******************************************************************************/
PropertyObject* PropertyContainer::expectProperty(int typeId) const
{
	PropertyObject* property = getProperty(typeId);
	if(!property)
		throwException(tr("Property '%1' is required but does not exist.").arg(propertyClass().standardPropertyName(typeId)));
	return property;
}

/******************************************************************************
* Duplicates any property objects that are shared with other containers.
* After this method returns, all property objects are exclusively owned by the container and 
* can be safely modified without unwanted side effects.
******************************************************************************/
void PropertyContainer::makePropertiesUnique()
{
	boost::optional<CloneHelper> cloneHelper;
	for(int i = properties().size() - 1; i >= 0; i--) {
		OVITO_ASSERT(properties()[i]->numberOfStrongReferences() >= 1);
		if(properties()[i]->numberOfStrongReferences() > 1) {
			if(!cloneHelper) cloneHelper.emplace();
			_properties.set(this, PROPERTY_FIELD(properties), i, cloneHelper->cloneObject(properties()[i], false));
		}
		OVITO_ASSERT(properties()[i]->numberOfStrongReferences() == 1);
	}
}

/******************************************************************************
* Creates a property and adds it to the container. 
* In case the property already exists, it is made sure that it's safe to modify it.
******************************************************************************/
PropertyObject* PropertyContainer::createStandardProperty(int typeId, bool initializeMemory)
{
	if(propertyClass().isValidStandardPropertyId(typeId) == false) {
		if(typeId == PropertyStorage::GenericSelectionProperty)
			throwException(tr("Selection is not supported by the '%2' property class.").arg(propertyClass().propertyClassDisplayName()));
		else if(typeId == PropertyStorage::GenericColorProperty)
			throwException(tr("Coloring is not supported by the '%2' property class.").arg(propertyClass().propertyClassDisplayName()));
		else
			throwException(tr("%1 is not a standard property ID supported by the '%2' property class.").arg(typeId).arg(propertyClass().propertyClassDisplayName()));
	}

	// Check if property already exists in the output.
	if(PropertyObject* property = getProperty(typeId)) {
		if(property->numberOfStrongReferences() > 1) {
			int index = properties().indexOf(property);
			property = CloneHelper().cloneObject(property, false);
			_properties.set(this, PROPERTY_FIELD(properties), index, property);

			// If no memory initialization is requested, create a new storage buffer to avoid copying the contents of the old one when
			// a deep copy is made on the first write access.
			if(!initializeMemory) {
				property->setStorage(propertyClass().createStandardStorage(property->size(), typeId, false));
			}
		}
		OVITO_ASSERT(property->numberOfStrongReferences() == 1);
		OVITO_ASSERT(property->size() == elementCount());
		return property;
	}
	else {
		// Create a new property object.
		OORef<PropertyObject> newProperty = propertyClass().createFromStorage(dataset(), propertyClass().createStandardStorage(elementCount(), typeId, initializeMemory));
		addProperty(newProperty);
		return newProperty;
	}
}

}	// End of namespace
}	// End of namespace
