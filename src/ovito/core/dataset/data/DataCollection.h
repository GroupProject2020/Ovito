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


#include <ovito/core/Core.h>
#include <ovito/core/dataset/animation/TimeInterval.h>
#include <ovito/core/oo/RefMaker.h>
#include <ovito/core/dataset/pipeline/PipelineStatus.h>
#include <ovito/core/dataset/data/DataObject.h>
#include <ovito/core/dataset/data/DataObjectReference.h>
#include <ovito/core/oo/CloneHelper.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Scene)

/// Utility class that is used to reference a particular data object in a DataCollection
/// as a path through the hierarchy of nested data objects.
class ConstDataObjectPath : public QVarLengthArray<const DataObject*, 3>
{
public:

	/// Inherit constructors from base class.
	using QVarLengthArray::QVarLengthArray;

	/// Converts the path to a string representation.
	QString toString() const {
		QString s;
		for(const DataObject* o : *this) {
			if(!s.isEmpty()) s += QChar('/');
			s += o->identifier();
		}
		return s;
	}
};

/// Utility class that is used to reference a particular data object in a DataCollection
/// as a path through the hierarchy of nested data objects.
class DataObjectPath : public QVarLengthArray<DataObject*, 3>
{
public:

	/// Inherit constructors from base class.
	using QVarLengthArray::QVarLengthArray;

	/// Converts the path to a string representation.
	QString toString() const {
		QString s;
		for(DataObject* o : *this) {
			if(!s.isEmpty()) s += QChar('/');
			s += o->identifier();
		}
		return s;
	}

	/// A path to a mutable object can be implicitly converted to a path to a constant object.
	operator const ConstDataObjectPath&() const {
		return *reinterpret_cast<const ConstDataObjectPath*>(this);
	}
};

/**
 * \brief This data structure holds the list of data objects that flows down a data pipeline.
 */
class OVITO_CORE_EXPORT DataCollection : public DataObject
{
	Q_OBJECT
	OVITO_CLASS(DataCollection)

public:

	/// \brief Constructor.
	Q_INVOKABLE DataCollection(DataSet* dataset) : DataObject(dataset) {}

	/// \brief Discards all contents of this data collection.
	void clear() {
		_objects.clear(this, PROPERTY_FIELD(objects));
	}

	/// \brief Returns true if the given object is part of this pipeline flow state.
	/// \note The method ignores the revision number of the object.
	bool contains(const DataObject* obj) const;

	/// \brief Adds an additional data object to this state.
	void addObject(const DataObject* obj);

	/// \brief Inserts an additional data object into this state.
	void insertObject(int index, const DataObject* obj);

	/// \brief Replaces a data object with a new one.
	bool replaceObject(const DataObject* oldObj, const DataObject* newObj);

	/// \brief Removes a data object from this state.
	void removeObject(const DataObject* obj) { replaceObject(obj, nullptr); }

	/// \brief Removes a data object from this state.
	void removeObjectByIndex(int index);

	/// \brief Finds an object of the given type in the list of data objects stored in this flow state.
	const DataObject* getObject(const DataObject::OOMetaClass& objectClass) const;

	/// \brief Finds an object of the given type in the list of data objects stored in this flow state.
	template<class DataObjectClass>
	const DataObjectClass* getObject() const {
		return static_object_cast<DataObjectClass>(getObject(DataObjectClass::OOClass()));
	}

	/// \brief Determines if an object of the given type is in this flow state.
	template<class DataObjectClass>
	bool containsObject() const {
		return getObject(DataObjectClass::OOClass()) != nullptr;
	}

	/// Throws an exception if the input does not contain a data object of the given type.
	const DataObject* expectObject(const DataObject::OOMetaClass& objectClass) const;

	/// Throws an exception if the input does not contain a data object of the given type.
	template<class DataObjectClass>
	const DataObjectClass* expectObject() const {
		return static_object_cast<DataObjectClass>(expectObject(DataObjectClass::OOClass()));
	}

	/// Throws an exception if the input does not contain a data object of the given type.
	DataObject* expectMutableObject(const DataObject::OOMetaClass& objectClass) {
		return makeMutable(expectObject(objectClass));
	}

	/// Throws an exception if the input does not contain a data object of the given type.
	template<class DataObjectClass>
	DataObjectClass* expectMutableObject() {
		return static_object_cast<DataObjectClass>(expectMutableObject(DataObjectClass::OOClass()));
	}

	/// Finds an object of the given type in the list of data objects stored in this flow state
	/// or among any of their sub-objects.
	bool containsObjectRecursive(const DataObject::OOMetaClass& objectClass) const {
		for(const DataObject* obj : objects()) {
			if(containsObjectRecursiveImpl(obj, objectClass))
				return true;
		}
		return false;
	}

	/// Finds all objects of the given type in this flow state (also searching among sub-objects).
	/// Returns them as a list of object paths.
	std::vector<ConstDataObjectPath> getObjectsRecursive(const DataObject::OOMetaClass& objectClass) const {
		std::vector<ConstDataObjectPath> result;
		ConstDataObjectPath path(1);
		for(const DataObject* obj : objects()) {
			OVITO_ASSERT(path.size() == 1);
			path[0] = obj;
			getObjectsRecursiveImpl(path, objectClass, result);
		}
		return result;
	}

	/// Finds an object of the given type and under the hierarchy path in this flow state.
	ConstDataObjectPath getObject(const DataObject::OOMetaClass& objectClass, const QString& pathString) const;

	/// Finds an object of the given type and under the hierarchy path in this flow state.
	ConstDataObjectPath getObject(const DataObjectReference& dataRef) const {
		OVITO_ASSERT(dataRef);
		return getObject(*dataRef.dataClass(), dataRef.dataPath());
	}

	/// Finds an object of the given type and under the hierarchy path in this flow state.
	template<class DataObjectClass>
	ConstDataObjectPath getObject(const QString& pathString) const { return getObject(DataObjectClass::OOClass(), pathString); }

	/// Finds an object of the given type and under the hierarchy path in this flow state.
	template<class DataObjectClass>
	ConstDataObjectPath getObject(const TypedDataObjectReference<DataObjectClass>& dataRef) const {
		OVITO_ASSERT(dataRef);
		OVITO_ASSERT(dataRef.dataClass()->isDerivedFrom(DataObjectClass::OOClass()));
		return getObject(*dataRef.dataClass(), dataRef.dataPath());
	}

	/// Throws an exception if the input does not contain any a data object of the given type and under the given hierarchy path.
	ConstDataObjectPath expectObject(const DataObject::OOMetaClass& objectClass, const QString& pathString) const;

	/// Throws an exception if the input does not contain any a data object of the given type and under the given hierarchy path.
	ConstDataObjectPath expectObject(const DataObjectReference& dataRef) const {
		OVITO_ASSERT(dataRef);
		return expectObject(*dataRef.dataClass(), dataRef.dataPath());
	}

	/// Throws an exception if the input does not contain any a data object of the given type and under the given hierarchy path.
	template<class DataObjectClass>
	ConstDataObjectPath expectObject(const QString& pathString) const { return expectObject(DataObjectClass::OOClass(), pathString); }

	/// Finds an object of the given type and under the hierarchy path in this flow state.
	template<class DataObjectClass>
	ConstDataObjectPath expectObject(const TypedDataObjectReference<DataObjectClass>& dataRef) const {
		OVITO_ASSERT(dataRef);
		OVITO_ASSERT(dataRef.dataClass()->isDerivedFrom(DataObjectClass::OOClass()));
		return expectObject(*dataRef.dataClass(), dataRef.dataPath());
	}

	/// Finds an object of the given type and under the hierarchy path in this flow state.
	const DataObject* getLeafObject(const DataObject::OOMetaClass& objectClass, const QString& pathString) const;

	/// Finds an object of the given type and under the hierarchy path in this flow state.
	const DataObject* getLeafObject(const DataObjectReference& dataRef) const {
		OVITO_ASSERT(dataRef);
		return getLeafObject(*dataRef.dataClass(), dataRef.dataPath());
	}

	/// Finds an object of the given type and under the hierarchy path in this flow state.
	template<class DataObjectClass>
	const DataObjectClass* getLeafObject(const TypedDataObjectReference<DataObjectClass>& dataRef) const {
		OVITO_ASSERT(dataRef);
		OVITO_ASSERT(dataRef.dataClass()->isDerivedFrom(DataObjectClass::OOClass()));
		return static_object_cast<DataObjectClass>(getLeafObject(*dataRef.dataClass(), dataRef.dataPath()));
	}

	/// Throws an exception if the input does not contain a data object of the given type.
	const DataObject* expectLeafObject(const DataObject::OOMetaClass& objectClass, const QString& pathString) const;

	/// Throws an exception if the input does not contain a data object of the given type.
	const DataObject* expectLeafObject(const DataObjectReference& dataRef) const {
		OVITO_ASSERT(dataRef);
		return expectLeafObject(*dataRef.dataClass(), dataRef.dataPath());
	}

	/// Throws an exception if the input does not contain a data object of the given type.
	template<class DataObjectClass>
	const DataObjectClass* expectLeafObject(const TypedDataObjectReference<DataObjectClass>& dataRef) const {
		OVITO_ASSERT(dataRef);
		OVITO_ASSERT(dataRef.dataClass()->isDerivedFrom(DataObjectClass::OOClass()));
		return static_object_cast<DataObjectClass>(expectLeafObject(*dataRef.dataClass(), dataRef.dataPath()));
	}

	/// Finds an object of the given type and with the given identifier in the list of data objects stored in this flow state.
	const DataObject* getObjectBy(const DataObject::OOMetaClass& objectClass, const PipelineObject* dataSource, const QString& identifier) const;

	/// Finds an object of the given type and with the given identifier in the list of data objects stored in this flow state.
	template<class DataObjectClass>
	const DataObjectClass* getObjectBy(const PipelineObject* dataSource, const QString& identifier) const {
		return static_object_cast<DataObjectClass>(getObjectBy(DataObjectClass::OOClass(), dataSource, identifier));
	}

	/// Finds an object of the given type and under the hierarchy path in this flow state.
	/// Duplicates it, and all its parent objects, if needed so that it can be safely modified without unwanted side effects.
	DataObjectPath getMutableObject(const DataObject::OOMetaClass& objectClass, const QString& pathString);

	/// Finds an object of the given type and under the hierarchy path in this flow state.
	/// Duplicates it, and all its parent objects, if needed so that it can be safely modified without unwanted side effects.
	template<class DataObjectClass>
	DataObjectPath getMutableObject(const TypedDataObjectReference<DataObjectClass>& dataRef) {
		OVITO_ASSERT(dataRef);
		OVITO_ASSERT(dataRef.dataClass()->isDerivedFrom(DataObjectClass::OOClass()));
		return getMutableObject(*dataRef.dataClass(), dataRef.dataPath());
	}

	/// Finds an object of the given type and under the hierarchy path in this flow state.
	DataObject* getMutableLeafObject(const DataObject::OOMetaClass& objectClass, const QString& pathString);

	/// Finds an object of the given type and under the hierarchy path in this flow state.
	DataObject* getMutableLeafObject(const DataObjectReference& dataRef) {
		OVITO_ASSERT(dataRef);
		return getMutableLeafObject(*dataRef.dataClass(), dataRef.dataPath());
	}

	/// Finds an object of the given type and under the hierarchy path in this flow state.
	template<class DataObjectClass>
	DataObjectClass* getMutableLeafObject(const DataObject::OOMetaClass& objectClass, const QString& pathString) {
		OVITO_ASSERT(objectClass.isDerivedFrom(DataObjectClass::OOClass()));
		return static_object_cast<DataObjectClass>(getMutableLeafObject(objectClass, pathString));
	}

	/// Finds an object of the given type and under the hierarchy path in this flow state.
	template<class DataObjectClass>
	DataObjectClass* getMutableLeafObject(const TypedDataObjectReference<DataObjectClass>& dataRef) {
		OVITO_ASSERT(dataRef);
		return getMutableLeafObject<DataObjectClass>(*dataRef.dataClass(), dataRef.dataPath());
	}

	/// Throws an exception if the input does not contain a data object of the given type and under the given hierarchy path.
	DataObjectPath expectMutableObject(const DataObject::OOMetaClass& objectClass, const QString& pathString);

	/// Throws an exception if the input does not contain a data object of the given type and under the given hierarchy path.
	DataObjectPath expectMutableObject(const DataObjectReference& dataRef) {
		OVITO_ASSERT(dataRef);
		return expectMutableObject(*dataRef.dataClass(), dataRef.dataPath());
	}

	/// Throws an exception if the input does not contain a data object of the given type and under the given hierarchy path.
	DataObject* expectMutableLeafObject(const DataObject::OOMetaClass& objectClass, const QString& pathString);

	/// Throws an exception if the input does not contain a data object of the given type and under the given hierarchy path.
	DataObject* expectMutableLeafObject(const DataObjectReference& dataRef) {
		OVITO_ASSERT(dataRef);
		return expectMutableLeafObject(*dataRef.dataClass(), dataRef.dataPath());
	}

	/// Throws an exception if the input does not contain a data object of the given type and under the given hierarchy path.
	template<class DataObjectClass>
	DataObjectClass* expectMutableLeafObject(const DataObject::OOMetaClass& objectClass, const QString& pathString) {
		OVITO_ASSERT(objectClass.isDerivedFrom(DataObjectClass::OOClass()));
		return static_object_cast<DataObjectClass>(expectMutableLeafObject(objectClass, pathString));
	}

	/// Throws an exception if the input does not contain a data object of the given type and under the given hierarchy path.
	template<class DataObjectClass>
	DataObjectClass* expectMutableLeafObject(const TypedDataObjectReference<DataObjectClass>& dataRef) {
		OVITO_ASSERT(dataRef);
		return expectMutableLeafObject<DataObjectClass>(*dataRef.dataClass(), dataRef.dataPath());
	}

	/// \brief Replaces the objects in this state with copies if they are shared between multiple pipeline flow states.
	///
	/// After calling this method, none of the objects in the flow state is referenced by anybody else.
	/// Thus, it becomes safe to modify the data objects including their subobjects.
	void makeAllMutableRecursive();

	/// Ensures that a DataObject from this flow state is not shared with others and is safe to modify.
	DataObject* makeMutable(const DataObject* obj, bool deepCopy = false);

	/// Ensures that a DataObject from this flow state is not shared with others and is safe to modify.
	template<class DataObjectClass>
	DataObjectClass* makeMutable(const DataObjectClass* obj, bool deepCopy = false) {
		return static_object_cast<DataObjectClass>(makeMutable(static_cast<const DataObject*>(obj), deepCopy));
	}

	/// \brief Returns true if this state object has no valid contents.
	bool isEmpty() const { return objects().empty(); }

	/// Returns the source frame number associated with this pipeline state.
	/// If the data does not originate from a pipeline with a FileSource, returns -1.
	int sourceFrame() const;

	/// Instantiates a new data object, passes the given parameters to its class constructor,
	/// assigns the given data source object, and finally inserts the data object into this pipeline flow state.
	template<class DataObjectType, class PipelineObjectClass, typename... Args>
	DataObjectType* createObject(const PipelineObjectClass* dataSource, Args&&... args) {
		OVITO_ASSERT(dataSource != nullptr);
		OORef<DataObjectType> obj = new DataObjectType(dataSource->dataset(), std::forward<Args>(args)...);
		obj->setDataSource(const_cast<PipelineObjectClass*>(dataSource));
		addObject(obj);
		return obj;
	}

	/// Instantiates a new data object, passes the given parameters to its class constructor,
	/// assign a unique identifier to the object, assigns the given data source object, and
	/// finally inserts the data object into this pipeline flow state.
	template<class DataObjectType, class PipelineObjectClass, typename... Args>
	DataObjectType* createObject(const QString& baseName, const PipelineObjectClass* dataSource, Args&&... args) {
		DataObjectType* obj = createObject<DataObjectType, PipelineObjectClass, Args...>(dataSource, std::forward<Args>(args)...);
		OVITO_ASSERT(!baseName.isEmpty());
		obj->setIdentifier(generateUniqueIdentifier<DataObjectType>(baseName));
		return obj;
	}

	/// Builds a list of the global attributes stored in this pipeline state.
	QVariantMap buildAttributesMap() const;

	/// Looks up the value for the given global attribute.
	/// Returns a given default value if the attribute is not defined in this pipeline state.
	QVariant getAttributeValue(const QString& attrName, const QVariant& defaultValue = QVariant()) const;

	/// Looks up the value for the global attribute with the given base name and creator.
	/// Returns a given default value if the attribute is not defined in this pipeline state.
	QVariant getAttributeValue(const PipelineObject* dataSource, const QString& attrBaseName, const QVariant& defaultValue = QVariant()) const;

	/// Inserts a new global attribute into the pipeline state.
	AttributeDataObject* addAttribute(const QString& key, QVariant value, const PipelineObject* dataSource);

	/// Returns a new unique data object identifier that does not collide with the
	/// identifiers of any existing data object of the given type in the same data
	/// collection.
	QString generateUniqueIdentifier(const QString& baseName, const OvitoClass& dataObjectClass) const;

	/// Returns a new unique data object identifier that does not collide with the
	/// identifiers of any existing data object of the given type in the same data
	/// collection.
	template<class DataObjectClass>
	QString generateUniqueIdentifier(const QString& baseName) const {
		return generateUniqueIdentifier(baseName, DataObjectClass::OOClass());
	}

private:

	/// Part of the implementation of containsObjectRecursive().
	static bool containsObjectRecursiveImpl(const DataObject* dataObj, const DataObject::OOMetaClass& objectClass);

	/// Part of the implementation of getObjectsRecursive().
	static void getObjectsRecursiveImpl(ConstDataObjectPath& path, const DataObject::OOMetaClass& objectClass, std::vector<ConstDataObjectPath>& results);

	/// Implementation detail of getObject().
	static bool getObjectImpl(const DataObject::OOMetaClass& objectClass, QStringRef pathString, ConstDataObjectPath& path);

	/// Implementation detail of getLeafObject().
	static const DataObject* getLeafObjectImpl(const DataObject::OOMetaClass& objectClass, QStringRef pathString, const DataObject* parent);

	/// Implementation detail of makeAllMutableRecursive().
	static void makeAllMutableImpl(DataObject* parent, CloneHelper& cloneHelper);

private:

	/// Stores the list of data objects.
	DECLARE_MODIFIABLE_VECTOR_REFERENCE_FIELD(DataObject, objects, setObjects);
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace

Q_DECLARE_METATYPE(Ovito::ConstDataObjectPath);
Q_DECLARE_METATYPE(Ovito::DataObjectPath);
