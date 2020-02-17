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

#pragma once


#include <ovito/stdobj/StdObj.h>
#include <ovito/core/dataset/data/DataObject.h>
#include <ovito/core/dataset/data/DataObjectReference.h>
#include "PropertyObject.h"
#include "PropertyContainerClass.h"

namespace Ovito { namespace StdObj {

/**
 * \brief Stores an array of properties.
 */
class OVITO_STDOBJ_EXPORT PropertyContainer : public DataObject
{
	Q_OBJECT
	OVITO_CLASS_META(PropertyContainer, PropertyContainerClass)

public:

	/// Constructor.
	PropertyContainer(DataSet* dataset);

	/// Appends a new property to the list of properties.
	void addProperty(const PropertyObject* property) {
		OVITO_ASSERT(property);
		OVITO_ASSERT(properties().contains(const_cast<PropertyObject*>(property)) == false);
		if(properties().empty())
			_elementCount.set(this, PROPERTY_FIELD(elementCount), property->size());
		OVITO_ASSERT(property->size() == elementCount());
		_properties.push_back(this, PROPERTY_FIELD(properties), const_cast<PropertyObject*>(property));
	}

	/// Inserts a new property into the list of properties.
	void insertProperty(int index, const PropertyObject* property) {
		OVITO_ASSERT(property);
		OVITO_ASSERT(properties().contains(const_cast<PropertyObject*>(property)) == false);
		if(properties().empty())
			_elementCount.set(this, PROPERTY_FIELD(elementCount), property->size());
		OVITO_ASSERT(property->size() == elementCount());
		_properties.insert(this, PROPERTY_FIELD(properties), index, const_cast<PropertyObject*>(property));
	}

	/// Removes a property from this container.
	void removeProperty(const PropertyObject* property) {
		OVITO_ASSERT(property);
		int index = properties().indexOf(const_cast<PropertyObject*>(property));
		OVITO_ASSERT(index >= 0);
		_properties.remove(this, PROPERTY_FIELD(properties), index);
	}

	/// Looks up the standard property with the given ID.
	const PropertyObject* getProperty(int typeId) const {
		OVITO_ASSERT(typeId != 0);
		OVITO_ASSERT(getOOMetaClass().isValidStandardPropertyId(typeId));
		for(const PropertyObject* property : properties()) {
			if(property->type() == typeId)
				return property;
		}
		return nullptr;
	}

	/// Looks up the user-defined property with the given name.
	const PropertyObject* getProperty(const QString& name) const {
		OVITO_ASSERT(!name.isEmpty());
		for(const PropertyObject* property : properties()) {
			if(property->type() == 0 && property->name() == name)
				return property;
		}
		return nullptr;
	}

	/// Looks up the storage array for the standard property with the given ID.
	ConstPropertyPtr getPropertyStorage(int typeId) const {
		if(const PropertyObject* property = getProperty(typeId))
			return property->storage();
		return nullptr;
	}

	/// Returns the given standard property.
	/// If it does not exist, an exception is thrown.
	const PropertyObject* expectProperty(int typeId) const;

	/// Returns the property with the given name and data layout.
	/// If the container does not contain a property with the given name and data type, then an exception is thrown.
	const PropertyObject* expectProperty(const QString& propertyName, int dataType, size_t componentCount = 1) const;

	/// Returns the given standard property after making sure it can be safely modified.
	/// If it does not exist, an exception is thrown.
	PropertyObject* expectMutableProperty(int typeId) {
		return makeMutable(expectProperty(typeId));
	}

	/// Duplicates any property objects that are shared with other containers.
	/// After this method returns, all property objects are exclusively owned by the container and
	/// can be safely modified without unwanted side effects.
	void makePropertiesMutable();

	/// Creates a standard property and adds it to the container.
	/// In case the property already exists, it is made sure that it's safe to modify it.
	PropertyObject* createProperty(int typeId, bool initializeMemory = false, const ConstDataObjectPath& containerPath = {});

	/// Creates a user-defined property and adds it to the container.
	/// In case the property already exists, it is made sure that it's safe to modify it.
	PropertyObject* createProperty(const QString& name, int dataType, size_t componentCount, size_t stride, bool initializeMemory = false, QStringList componentNames = QStringList());

	/// Creates a property and adds it to the container.
	PropertyObject* createProperty(PropertyPtr storage);

	/// Sets the current number of data elements stored in the container.
	/// The lengths of the property arrays will be adjusted accordingly.
	void setElementCount(size_t count);

	/// Deletes those data elements for which the bit is set in the given bitmask array.
	/// Returns the number of deleted elements.
	virtual size_t deleteElements(const boost::dynamic_bitset<>& mask);

	/// Replaces the property arrays in this property container with a new set of properties.
	void setContent(size_t newElementCount, const std::vector<PropertyPtr>& newProperties);

	/// Duplicates all data elements by extending the property arrays and replicating the existing data N times.
	void replicate(size_t n, bool replicatePropertyValues = true);

	/// Makes sure that all property arrays in this container have a consistent length.
	/// If this is not the case, the method throws an exception.
	void verifyIntegrity() const;

protected:

	/// Saves the class' contents to the given stream.
	virtual void saveToStream(ObjectSaveStream& stream, bool excludeRecomputableData) override;

	/// Loads the class' contents from the given stream.
	virtual void loadFromStream(ObjectLoadStream& stream) override;

private:

	/// Holds the list of properties.
	DECLARE_MODIFIABLE_VECTOR_REFERENCE_FIELD(PropertyObject, properties, setProperties);

	/// Keeps track of the number of elements stored in this property container.
	DECLARE_PROPERTY_FIELD(size_t, elementCount);
};

/// Encapsulates a reference to a PropertyContainer in a PipelineFlowState.
using PropertyContainerReference = TypedDataObjectReference<PropertyContainer>;

}	// End of namespace
}	// End of namespace

Q_DECLARE_METATYPE(Ovito::StdObj::PropertyContainerReference);