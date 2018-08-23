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
#include <core/dataset/data/DataObject.h>
#include <core/dataset/data/AttributeDataObject.h>
#include <core/oo/CloneHelper.h>

#include <boost/optional.hpp>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Scene)

/******************************************************************************
* Tries to convert one of the to data objects stored in this flow state to
* the given object type.
******************************************************************************/
OORef<DataObject> PipelineFlowState::convertObject(const OvitoClass& objectClass, TimePoint time) const
{
	for(DataObject* o : objects()) {
		if(OORef<DataObject> obj = o->convertTo(objectClass, time))
			return obj;
	}
	return {};
}

/******************************************************************************
* Returns true if the given object is part of this pipeline flow state.
* The method ignores the revision number of the object.
******************************************************************************/
bool PipelineFlowState::contains(DataObject* obj) const
{
	for(DataObject* o : objects())
		if(o == obj) return true;
	return false;
}

/******************************************************************************
* Adds an additional data object to this state.
******************************************************************************/
void PipelineFlowState::addObject(DataObject* obj)
{
	OVITO_CHECK_OBJECT_POINTER(obj);
	OVITO_ASSERT_MSG(!contains(obj), "PipelineFlowState::addObject", "Cannot add the same data object more than once.");
	_objects.emplace_back(obj);
}

/******************************************************************************
* Inserts an additional data object into this state.
******************************************************************************/
void PipelineFlowState::insertObject(int index, DataObject* obj)
{
	OVITO_CHECK_OBJECT_POINTER(obj);
	OVITO_ASSERT_MSG(!contains(obj), "PipelineFlowState::insertObject", "Cannot insert the same data object more than once.");
	OVITO_ASSERT(index >= 0 && index <= _objects.size());
	_objects.emplace(_objects.begin() + index, obj);
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
bool PipelineFlowState::replaceObject(DataObject* oldObj, DataObject* newObj)
{
	OVITO_CHECK_OBJECT_POINTER(oldObj);
	for(auto iter = _objects.begin(); iter != _objects.end(); ++iter) {
		if(iter->get() == oldObj) {
			if(newObj)
				*iter = newObj;
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
void PipelineFlowState::cloneObjectsIfNeeded(bool deepCopy)
{
	boost::optional<CloneHelper> cloneHelper;
	for(auto& ref : _objects) {
		OVITO_ASSERT(ref->numberOfStrongReferences() >= 1);
		if(ref->numberOfStrongReferences() > 1) {
			if(!cloneHelper) cloneHelper.emplace();
			ref = cloneHelper->cloneObject(ref.get(), deepCopy);
		}
		OVITO_ASSERT(ref->numberOfStrongReferences() == 1);
	}
}

/******************************************************************************
* Finds an object of the given type and with the given identifier in the list 
* of data objects stored in this flow state.
******************************************************************************/
DataObject* PipelineFlowState::findObject(const DataObject::OOMetaClass& objectClass, const QString& identifier, PipelineObject* dataSource) const
{
	// Look for the data object with the given ID, or with the given ID followed 
	// an enumeration index that was appended by PipelineOutputHelper::generateUniqueIdentifier().
	for(DataObject* obj : objects()) {
		if(objectClass.isMember(obj) && (!dataSource || obj->dataSource() == dataSource)) {
			if(obj->identifier() == identifier || obj->identifier().startsWith(identifier + QChar('.')))
				return obj;
		}
	}
	return nullptr;
}

/******************************************************************************
* Builds a list of the global attributes stored in this pipeline state.
******************************************************************************/
QVariantMap PipelineFlowState::buildAttributesMap() const
{
	QVariantMap attributes;
	for(DataObject* obj : objects()) {
		if(AttributeDataObject* attribute = dynamic_object_cast<AttributeDataObject>(obj)) {
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
	for(DataObject* obj : objects()) {
		if(AttributeDataObject* attribute = dynamic_object_cast<AttributeDataObject>(obj)) {
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
QVariant PipelineFlowState::getAttributeValue(const QString& attrBaseName, PipelineObject* dataSource, const QVariant& defaultValue) const
{
	if(AttributeDataObject* attribute = findObject<AttributeDataObject>(attrBaseName, dataSource))
		return attribute->value();
	else
		return defaultValue;
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
