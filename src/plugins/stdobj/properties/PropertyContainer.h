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
#include <core/dataset/data/DataObject.h>
#include "PropertyObject.h"

namespace Ovito { namespace StdObj {
	
/**
 * \brief Stores an array of properties.
 */
class OVITO_STDOBJ_EXPORT PropertyContainer : public DataObject
{
	Q_OBJECT
	OVITO_CLASS(PropertyContainer)
	
public:

	/// Constructor.
	PropertyContainer(DataSet* dataset);

	/// Returns the class of properties that this container can store.
	virtual const PropertyClass& propertyClass() const = 0;

	/// Appends a new property to the list of properties.
	void addProperty(PropertyObject* property) {
		OVITO_ASSERT(property);
		OVITO_ASSERT(propertyClass().isMember(property));
		OVITO_ASSERT(properties().contains(property) == false);
		_properties.push_back(this, PROPERTY_FIELD(properties), property);
	}

	/// Removes a property from this container.
	void removeProperty(PropertyObject* property) {
		OVITO_ASSERT(property);
		OVITO_ASSERT(propertyClass().isMember(property));
		int index = properties().indexOf(property);
		OVITO_ASSERT(index >= 0);
		_properties.remove(this, PROPERTY_FIELD(properties), index);
	}

	/// Looks up the standard property with given ID.
	PropertyObject* getProperty(int typeId) const {
		OVITO_ASSERT(typeId != 0);
		OVITO_ASSERT(propertyClass().isValidStandardPropertyId(typeId));
		for(PropertyObject* property : properties()) {
			if(property->type() == typeId)
				return property;
		}
		return nullptr;
	}

	/// Looks up the standard property with given ID.
	template<class PropertyObjectClass>
	PropertyObjectClass* getProperty(typename PropertyObjectClass::Type typeId) const {
		return static_object_cast<PropertyObjectClass>(getProperty(typeId));
	}

	/// Returns the given standard property.
	/// If it does not exist, an exception is thrown.
	PropertyObject* expectProperty(int typeId) const;

	/// Looks up the standard property with given ID.
	/// If it does not exist, an exception is thrown.
	template<class PropertyObjectClass>
	PropertyObjectClass* expectProperty(typename PropertyObjectClass::Type typeId) const {
		return static_object_cast<PropertyObjectClass>(expectProperty(typeId));
	}

	/// Returns the number of data elements (i.e. the length of the property arrays).
	size_t elementCount() const {
		return properties().empty() ? 0 : properties().front()->size();
	}

	/// Duplicates any property objects that are shared with other containers.
	/// After this method returns, all property objects are exclusively owned by the container and 
	/// can be safely modified without unwanted side effects.
	void makePropertiesUnique();

	/// Creates a property and adds it to the container. 
	/// In case the property already exists, it is made sure that it's safe to modify it.
	PropertyObject* createStandardProperty(int typeId, bool initializeMemory = false);

	/// Creates a property and adds it to the container. 
	/// In case the property already exists, it is made sure that it's safe to modify it.
	template<class PropertyObjectClass>
	PropertyObjectClass* createStandardProperty(typename PropertyObjectClass::Type typeId, bool initializeMemory = false) {
		return static_object_cast<PropertyObjectClass>(createStandardProperty(typeId, initializeMemory));
	}

private:

	/// Holds the list of properties.
	DECLARE_MODIFIABLE_VECTOR_REFERENCE_FIELD(PropertyObject, properties, setProperties);
};

}	// End of namespace
}	// End of namespace
