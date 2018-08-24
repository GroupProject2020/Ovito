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
	PropertyObjectClass* getProperty(int typeId) const {
		return static_object_cast<PropertyObjectClass>(getProperty(typeId));
	}

	/// Returns the given standard property.
	/// If it does not exist, an exception is thrown.
	PropertyObject* expectProperty(int typeId) const;

private:

	/// Holds the list of properties.
	DECLARE_MODIFIABLE_VECTOR_REFERENCE_FIELD(PropertyObject, properties, setProperties);
};

}	// End of namespace
}	// End of namespace
