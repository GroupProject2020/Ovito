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
#include <core/oo/PropertyFieldDescriptor.h>
#include <core/oo/RefMaker.h>
#include <core/oo/OvitoObject.h>
#include <core/dataset/DataSet.h>
#include "RefMakerClass.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem)

/******************************************************************************
* Is called by the system after construction of the meta-class instance.
******************************************************************************/
void RefMakerClass::initialize()
{
	OvitoClass::initialize();

	// Collect all property fields of the class hierarchy in one array.
	for(const RefMakerClass* clazz = this; clazz != &RefMaker::OOClass(); clazz = static_cast<const RefMakerClass*>(clazz->superClass())) {
		for(const PropertyFieldDescriptor* field = clazz->_firstPropertyField; field != nullptr; field = field->next()) {
			_propertyFields.push_back(field);
		}
	}
}
	
/******************************************************************************
* Searches for a property field defined in this class or one of its super classes.
******************************************************************************/
const PropertyFieldDescriptor* RefMakerClass::findPropertyField(const char* identifier, bool searchSuperClasses) const
{
	if(!searchSuperClasses) {
		for(const PropertyFieldDescriptor* field = _firstPropertyField; field; field = field->next())
			if(qstrcmp(field->identifier(), identifier) == 0) return field;
	}
	else {
		for(const PropertyFieldDescriptor* field : _propertyFields) {
			if(qstrcmp(field->identifier(), identifier) == 0) return field;
		}
	}
	return nullptr;
}

/******************************************************************************
* This method is called by the ObjectSaveStream class when saving one or more 
* object instances of a class belonging to this metaclass. 
******************************************************************************/
void RefMakerClass::saveClassInfo(SaveStream& stream) const
{
	OvitoClass::saveClassInfo(stream);
	
	// Serialize the list of property fields registered for this RefMaker-derived class.
	for(const PropertyFieldDescriptor* field : propertyFields()) {
		stream.beginChunk(0x01);
		stream << QByteArray::fromRawData(field->identifier(), qstrlen(field->identifier()));
		OvitoClass::serializeRTTI(stream, field->definingClass());
		stream << field->flags();
		stream << field->isReferenceField();
		if(field->isReferenceField()) {
			OvitoClass::serializeRTTI(stream, field->targetClass());
		}
		stream.endChunk();
	}

	// Property list terminator:
	stream.beginChunk(0x0);
	stream.endChunk();
}

/******************************************************************************
* This method is called by the ObjectLoadStream class when loading one or more object instances
* of a class belonging to this metaclass.
******************************************************************************/
void RefMakerClass::loadClassInfo(LoadStream& stream, OvitoClass::SerializedClassInfo* classInfo) const
{
	OvitoClass::loadClassInfo(stream, classInfo);
	
	for(;;) {
		quint32 chunkId = stream.openChunk();
		if(chunkId == 0x0) {
			stream.closeChunk();
			break;	// End of list
		}
		if(chunkId != 0x1)
			throw Exception(RefMaker::tr("File format is invalid. Failed to load property fields of class %1.").arg(classInfo->clazz->name()));

		SerializedClassInfo::PropertyFieldInfo fieldInfo;
		stream >> fieldInfo.identifier;
		OvitoClassPtr definingClass = OvitoClass::deserializeRTTI(stream);
		OVITO_ASSERT(definingClass->isDerivedFrom(RefMaker::OOClass()));
		fieldInfo.definingClass = static_cast<const RefMakerClass*>(definingClass);
		if(classInfo->clazz->isDerivedFrom(*fieldInfo.definingClass) == false) {
			qDebug() << "WARNING:" << classInfo->clazz->name() << "is not derived from" << fieldInfo.definingClass->name();
			throw Exception(RefMaker::tr("The class hierarchy stored in the file differs from the class hierarchy of the program."));
		}
		stream >> fieldInfo.flags;
		stream >> fieldInfo.isReferenceField;
		fieldInfo.targetClass = fieldInfo.isReferenceField ? OvitoClass::deserializeRTTI(stream) : nullptr;
		stream.closeChunk();

		fieldInfo.field = fieldInfo.definingClass->findPropertyField(fieldInfo.identifier.constData(), true);
		if(fieldInfo.field) {
			if(fieldInfo.field->isReferenceField() != fieldInfo.isReferenceField ||
					fieldInfo.field->isVector() != ((fieldInfo.flags & PROPERTY_FIELD_VECTOR) != 0) ||
					(fieldInfo.isReferenceField && fieldInfo.targetClass->isDerivedFrom(*fieldInfo.field->targetClass()) == false))
				throw Exception(RefMaker::tr("File format error: The type of the property field '%1' in class %2 has changed.").arg(fieldInfo.identifier, fieldInfo.definingClass->name()));
		}

		static_cast<RefMakerClass::SerializedClassInfo*>(classInfo)->propertyFields.push_back(std::move(fieldInfo));
	}
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
