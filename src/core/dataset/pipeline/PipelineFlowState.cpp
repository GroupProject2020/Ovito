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

#include <core/Core.h>
#include <core/dataset/pipeline/PipelineFlowState.h>
#include <core/dataset/pipeline/PipelineObject.h>
#include <core/dataset/data/DataObject.h>
#include <core/dataset/data/AttributeDataObject.h>
#include <core/dataset/DataSet.h>
#include <core/oo/CloneHelper.h>

#include <boost/optional.hpp>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Scene)

/******************************************************************************
* Returns true if the given object is part of this pipeline flow state.
* The method ignores the revision number of the object.
******************************************************************************/
bool PipelineFlowState::contains(const DataObject* obj) const
{
	for(const DataObject* o : objects())
		if(o == obj) return true;
	return false;
}

/******************************************************************************
* Adds an additional data object to this state.
******************************************************************************/
void PipelineFlowState::addObject(const DataObject* obj)
{
	OVITO_CHECK_OBJECT_POINTER(obj);

	OVITO_ASSERT_MSG(!contains(obj), "PipelineFlowState::addObject", "Cannot add the same data object more than once.");
	_objects.emplace_back(const_cast<DataObject*>(obj));
}

/******************************************************************************
* Inserts an additional data object into this state.
******************************************************************************/
void PipelineFlowState::insertObject(int index, const DataObject* obj)
{
	OVITO_CHECK_OBJECT_POINTER(obj);

	// Undo recording should never be active during pipeline evaluation.
	OVITO_ASSERT(!obj->dataset()->undoStack().isRecording());

	OVITO_ASSERT_MSG(!contains(obj), "PipelineFlowState::insertObject", "Cannot insert the same data object more than once.");
	OVITO_ASSERT(index >= 0 && index <= _objects.size());
	_objects.emplace(_objects.begin() + index, const_cast<DataObject*>(obj));
}

/******************************************************************************
* Replaces a data object with a new one.
******************************************************************************/
void PipelineFlowState::removeObjectByIndex(int index)
{
	OVITO_ASSERT(index >= 0 && index < _objects.size());
	_objects.erase(_objects.begin() + index);
}

/******************************************************************************
* Replaces a data object with a new one.
******************************************************************************/
bool PipelineFlowState::replaceObject(const DataObject* oldObj, const DataObject* newObj)
{
	OVITO_CHECK_OBJECT_POINTER(oldObj);
	for(auto iter = _objects.begin(); iter != _objects.end(); ++iter) {
		if(iter->get() == oldObj) {
			if(newObj)
				*iter = const_cast<DataObject*>(newObj);
			else
				_objects.erase(iter);
			return true;
		}
	}
	OVITO_ASSERT_MSG(false, "PipelineFlowState::replaceObject", "Old data object not found.");
	return false;
}

/******************************************************************************
* Replaces objects with copies if there are multiple references.
* After calling this method, none of the objects in the flow state is referenced by anybody else.
* Thus, it becomes safe to modify the data objects.
******************************************************************************/
void PipelineFlowState::makeAllMutableRecursive()
{
	boost::optional<CloneHelper> cloneHelper;
	for(auto& ref : _objects) {
		OVITO_ASSERT(ref->numberOfStrongReferences() >= 1);
		if(ref->numberOfStrongReferences() > 1) {
			if(!cloneHelper) cloneHelper.emplace();
			ref = cloneHelper->cloneObject(ref.get(), false);
			makeAllMutableImpl(ref, *cloneHelper);
		}
		OVITO_ASSERT(ref->numberOfStrongReferences() == 1);
	}
}

/******************************************************************************
* Implementation detail of makeAllMutableRecursive().
******************************************************************************/
void PipelineFlowState::makeAllMutableImpl(DataObject* parent, CloneHelper& cloneHelper)
{
	parent->visitSubObjects([&](const DataObject* subObject) {
		OVITO_ASSERT(subObject->numberOfStrongReferences() >= 1);
		if(subObject->numberOfStrongReferences() > 1) {
			OORef<DataObject> clone = cloneHelper.cloneObject(subObject, false);
			parent->replaceReferencesTo(subObject, clone);
			subObject = clone;
		}
		makeAllMutableImpl(const_cast<DataObject*>(subObject), cloneHelper);
		return false;
	});
}

/******************************************************************************
* Finds an object of the given type in the list of data objects stored in this 
* flow state.
******************************************************************************/
const DataObject* PipelineFlowState::getObject(const DataObject::OOMetaClass& objectClass) const 
{
	for(const DataObject* obj : objects()) {
		if(objectClass.isMember(obj))
			return obj;
	}
	return nullptr;
}

/******************************************************************************
* Throws an exception if the input does not contain a data object of the given type.
******************************************************************************/
const DataObject* PipelineFlowState::expectObject(const DataObject::OOMetaClass& objectClass) const 
{
	if(const DataObject* obj = getObject(objectClass))
		return obj;
	else
		throw Exception(DataObject::tr("The dataset does not contain an object of type: %1").arg(objectClass.displayName()));
}

/******************************************************************************
* Throws an exception if the input does not contain any a data object of the 
* given type and under the given hierarchy path.
******************************************************************************/
const DataObject* PipelineFlowState::expectLeafObject(const DataObject::OOMetaClass& objectClass, const QString& pathString) const 
{
	const DataObject* obj = getLeafObject(objectClass, pathString);
	if(!obj) {
		if(pathString.isEmpty())
			throw Exception(DataObject::tr("The dataset does not contain an object of type: %1").arg(objectClass.displayName()));
		else
			throw Exception(DataObject::tr("The dataset does not contain an object named '%2' of type '%1'.").arg(objectClass.displayName()).arg(pathString));
	}
	return obj;
}

/******************************************************************************
* Ensures that a DataObject from this flow state is not shared with others and is safe to modify.
******************************************************************************/
DataObject* PipelineFlowState::makeMutable(const DataObject* obj, bool deepCopy)
{
	OVITO_ASSERT(obj != nullptr);
	OVITO_ASSERT(contains(obj));
	OVITO_ASSERT(obj->numberOfStrongReferences() >= 1);
	if(obj->numberOfStrongReferences() > 1) {
		OORef<DataObject> clone = CloneHelper().cloneObject(obj, deepCopy);
		if(replaceObject(obj, clone)) {
			OVITO_ASSERT(clone->numberOfStrongReferences() == 1);
			return clone;
		}
	}
	return const_cast<DataObject*>(obj);
}

/******************************************************************************
* Finds an object of the given type and with the given identifier in the list 
* of data objects stored in this flow state.
******************************************************************************/
const DataObject* PipelineFlowState::getObjectBy(const DataObject::OOMetaClass& objectClass, const PipelineObject* dataSource, const QString& identifier) const
{
	OVITO_ASSERT(!identifier.isEmpty());
	if(!dataSource) return nullptr;

	// Look for the data object with the given ID, or with the given ID followed 
	// an enumeration index that was appended by generateUniqueIdentifier().
	for(const DataObject* obj : objects()) {
		if(objectClass.isMember(obj) && obj->dataSource() == dataSource) {
			if(obj->identifier() == identifier || obj->identifier().startsWith(identifier + QChar('.')))
				return obj;
		}
	}
	return nullptr;
}

/******************************************************************************
* Part of the implementation of containsObjectRecursive()
******************************************************************************/
bool PipelineFlowState::containsObjectRecursiveImpl(const DataObject* dataObj, const DataObject::OOMetaClass& objectClass)
{
	if(objectClass.isMember(dataObj))
		return true;

	// Recursively visit the sub-objects of the object.
	return dataObj->visitSubObjects([&objectClass](const DataObject* subObject) {
		return containsObjectRecursiveImpl(subObject, objectClass);
	});
}

/******************************************************************************
* Part of the implementation of getObjectsRecursive()
******************************************************************************/
void PipelineFlowState::getObjectsRecursiveImpl(ConstDataObjectPath& path, const DataObject::OOMetaClass& objectClass, std::vector<ConstDataObjectPath>& results)
{
	if(objectClass.isMember(path.back()))
		results.push_back(path);

	// Recursively visit the sub-objects of the object.
	path.back()->visitSubObjects([&](const DataObject* subObject) {
		path.push_back(subObject);
		getObjectsRecursiveImpl(path, objectClass, results);
		path.pop_back();
		return false;
	});
}

/******************************************************************************
* Finds an object of the given type and under the hierarchy path in this flow state.
******************************************************************************/
ConstDataObjectPath PipelineFlowState::getObject(const DataObject::OOMetaClass& objectClass, const QString& pathString) const
{
	ConstDataObjectPath result;
	// Perform a recursive search for the requested object.
	for(const DataObject* obj : objects()) {
		result.push_back(obj);
		if(getObjectImpl(objectClass, &pathString, result))
			break;
		result.pop_back();
	}
	return result;
}

/******************************************************************************
* Throws an exception if the input does not contain any a data object of the 
* given type and under the given hierarchy path.
******************************************************************************/
ConstDataObjectPath PipelineFlowState::expectObject(const DataObject::OOMetaClass& objectClass, const QString& pathString) const 
{
	ConstDataObjectPath path = getObject(objectClass, pathString);
	if(path.empty()) {
		if(pathString.isEmpty())
			throw Exception(DataObject::tr("The dataset does not contain an object of type: %1").arg(objectClass.displayName()));
		else
			throw Exception(DataObject::tr("The dataset does not contain an object named '%2' of type '%1'.").arg(objectClass.displayName()).arg(pathString));
	}
	return path;
}

/******************************************************************************
* Throws an exception if the input does not contain any a data object of the 
* given type and under the given hierarchy path.
******************************************************************************/
DataObjectPath PipelineFlowState::expectMutableObject(const DataObject::OOMetaClass& objectClass, const QString& pathString) 
{
	DataObjectPath path = getMutableObject(objectClass, pathString);
	if(path.empty()) {
		if(pathString.isEmpty())
			throw Exception(DataObject::tr("The dataset does not contain an object of type: %1").arg(objectClass.displayName()));
		else
			throw Exception(DataObject::tr("The dataset does not contain an object named '%2' of type '%1'.").arg(objectClass.displayName()).arg(pathString));
	}
	return path;
}

/******************************************************************************
* Throws an exception if the input does not contain any a data object of the 
* given type and under the given hierarchy path.
******************************************************************************/
DataObject* PipelineFlowState::expectMutableLeafObject(const DataObject::OOMetaClass& objectClass, const QString& pathString) 
{
	DataObjectPath path = expectMutableObject(objectClass, pathString);
	OVITO_ASSERT(!path.empty());
	return path.back();
}

/******************************************************************************
* Implementation detail of getObject().
******************************************************************************/
bool PipelineFlowState::getObjectImpl(const DataObject::OOMetaClass& objectClass, QStringRef pathString, ConstDataObjectPath& path)
{
	const DataObject* object = path.back();
	if(pathString.isEmpty()) {
		if(!object->identifier().isEmpty()) return false;
		if(objectClass.isMember(object)) return true;
		return object->visitSubObjects([&](const DataObject* subObject) {
			path.push_back(subObject);
			if(getObjectImpl(objectClass, pathString, path))
				return true;
			path.pop_back();
			return false;
		});
	}
	else {
		int separatorPos = pathString.indexOf(QChar('/'));
		if(separatorPos == -1) {
			return objectClass.isMember(object) && object->identifier() == pathString;
		}
		else if(object->identifier() == pathString.left(separatorPos)) {
			QStringRef subPath = pathString.mid(separatorPos + 1);
			return object->visitSubObjects([&](const DataObject* subObject) {
				path.push_back(subObject);
				if(getObjectImpl(objectClass, subPath, path))
					return true;
				path.pop_back();
				return false;
			});
		}
		else return false;
	}
}

/******************************************************************************
* Finds an object of the given type and under the hierarchy path in this flow state.
******************************************************************************/
const DataObject* PipelineFlowState::getLeafObject(const DataObject::OOMetaClass& objectClass, const QString& pathString) const
{
	for(const DataObject* obj : objects()) {
		if(const DataObject* result = getLeafObjectImpl(objectClass, &pathString, obj))
			return result;
	}
	return nullptr;
}

/******************************************************************************
* Implementation detail of getLeafObject().
******************************************************************************/
const DataObject* PipelineFlowState::getLeafObjectImpl(const DataObject::OOMetaClass& objectClass, QStringRef pathString, const DataObject* parent)
{
	if(pathString.isEmpty()) {
		if(!parent->identifier().isEmpty()) return nullptr;
		if(objectClass.isMember(parent)) return parent;
		const DataObject* result = nullptr;
		parent->visitSubObjects([&](const DataObject* subObject) {
			result = getLeafObjectImpl(objectClass, pathString, subObject);
			return result != nullptr;
		});
		return result;
	}
	else {
		int separatorPos = pathString.indexOf(QChar('/'));
		if(separatorPos == -1) {
			if(objectClass.isMember(parent) && parent->identifier() == pathString)
				return parent;
		}
		else if(parent->identifier() == pathString.left(separatorPos)) {
			QStringRef subPath = pathString.mid(separatorPos + 1);
			const DataObject* result = nullptr;
			parent->visitSubObjects([&](const DataObject* subObject) {
				result = getLeafObjectImpl(objectClass, subPath, subObject);
				return result != nullptr;
			});
			return result;
		}
		return nullptr;
	}
}

/******************************************************************************
* Finds an object of the given type and under the hierarchy path in this flow state. 
* Duplicates it, and all its parent objects, if needed so that it can be safely 
* modified without unwanted side effects.
******************************************************************************/
DataObjectPath PipelineFlowState::getMutableObject(const DataObject::OOMetaClass& objectClass, const QString& pathString)
{
	// First, determine the full path to the object we are searching for.
	const ConstDataObjectPath path = getObject(objectClass, pathString);
	DataObjectPath resultPath;

	// If found, clone the object as well as all parents up the hierarchy. 
	if(!path.empty()) {
		resultPath.resize(path.size());
		resultPath.front() = makeMutable(path.front());
		for(int index = 1; index < path.size(); index++) {
			resultPath[index] = resultPath[index - 1]->makeMutable(path[index]);
		}
	}
	return resultPath;
}

/******************************************************************************
* Finds an object of the given type and under the hierarchy path in this flow state. 
* Duplicates it, and all its parent objects, if needed so that it can be safely 
* modified without unwanted side effects.
******************************************************************************/
DataObject* PipelineFlowState::getMutableLeafObject(const DataObject::OOMetaClass& objectClass, const QString& pathString)
{
	DataObjectPath path = getMutableObject(objectClass, pathString);
	return path.empty() ? nullptr : path.back();
}

/******************************************************************************
* Builds a list of the global attributes stored in this pipeline state.
******************************************************************************/
QVariantMap PipelineFlowState::buildAttributesMap() const
{
	QVariantMap attributes;
	for(const DataObject* obj : objects()) {
		if(const AttributeDataObject* attribute = dynamic_object_cast<AttributeDataObject>(obj)) {
			if(!attributes.contains(attribute->identifier())) {
				attributes.insert(attribute->identifier(), attribute->value());
			}
			else {
				for(int counter = 2; ; counter++) {
					QString uniqueName = attribute->identifier() + QChar('.') + QString::number(counter);
					if(!attributes.contains(uniqueName)) {
						attributes.insert(uniqueName, attribute->value());
						break;
					}
				}
			}
		}
	}
	return attributes;
}

/******************************************************************************
* Looks up the value for the given global attribute. 
* Returns a given default value if the attribute is not defined in this pipeline state. 
******************************************************************************/
QVariant PipelineFlowState::getAttributeValue(const QString& attrName, const QVariant& defaultValue) const
{
	for(const DataObject* obj : objects()) {
		if(const AttributeDataObject* attribute = dynamic_object_cast<AttributeDataObject>(obj)) {
			if(attribute->identifier() == attrName)
				return attribute->value();
		}
	}
	return defaultValue;
}

/******************************************************************************
* Looks up the value for the global attribute with the given base name and creator. 
* Returns a given default value if the attribute is not defined in this pipeline state. 
******************************************************************************/
QVariant PipelineFlowState::getAttributeValue(const PipelineObject* dataSource, const QString& attrBaseName, const QVariant& defaultValue) const
{
	if(const AttributeDataObject* attribute = getObjectBy<AttributeDataObject>(dataSource, attrBaseName))
		return attribute->value();
	else
		return defaultValue;
}

/******************************************************************************
* Inserts a new global attribute into the pipeline state.
******************************************************************************/
AttributeDataObject* PipelineFlowState::addAttribute(const QString& key, QVariant value, const PipelineObject* dataSource)
{
	return createObject<AttributeDataObject>(key, dataSource, std::move(value));
}

/******************************************************************************
* Returns a new unique data object identifier that does not collide with the 
* identifiers of any existing data object of the given type in the same data 
* collection.
******************************************************************************/
QString PipelineFlowState::generateUniqueIdentifier(const QString& baseName, const OvitoClass& dataObjectClass) const
{
	// This helper function checks if the given id is already used by another data object of the
	// given type in the this data collection.
	auto doesExist = [this,&dataObjectClass](const QString& id) {
		for(const DataObject* obj : objects()) {
			if(dataObjectClass.isMember(obj)) {
				if(obj->identifier() == id)
					return true;
			}
		}
		return false;
	};

	if(!doesExist(baseName)) {
		return baseName;
	}
	else {
		// Append consecutive indices to the base ID name.
		for(int i = 2; ; i++) {
			QString uniqueId = baseName + QChar('.') + QString::number(i);
			if(!doesExist(uniqueId)) {
				return uniqueId;
			}
		}
	}
	OVITO_ASSERT(false);
}

/******************************************************************************
* Returns the source frame number associated with this pipeline state.
* If the data does not originate from a pipeline with a FileSource, returns -1.
******************************************************************************/
int PipelineFlowState::sourceFrame() const
{
	return getAttributeValue(QStringLiteral("SourceFrame"), -1).toInt();
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
