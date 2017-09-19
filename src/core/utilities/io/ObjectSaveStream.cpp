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
#include <core/oo/OvitoObject.h>
#include <core/oo/PropertyFieldDescriptor.h>
#include <core/dataset/DataSet.h>
#include "ObjectSaveStream.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(IO)

using namespace std;

/******************************************************************************
* The destructor closes the stream.
******************************************************************************/
ObjectSaveStream::~ObjectSaveStream()
{
	try {
		close();
	}
	catch(Exception& ex) {
		if(!ex.context()) ex.setContext(_dataset);
		ex.reportError();
	}
}

/******************************************************************************
* Saves an object with runtime type information to the stream.
******************************************************************************/
void ObjectSaveStream::saveObject(OvitoObject* object, bool excludeRecomputableData)
{
	if(object == nullptr) {
		*this << (quint32)0;
	}
	else {
		OVITO_CHECK_OBJECT_POINTER(object);
		OVITO_ASSERT(_objects.size() == _objectMap.size());
		quint32& id = _objectMap[object];
		if(id == 0) {
			_objects.emplace_back(object, excludeRecomputableData);
			id = (quint32)_objects.size();

			if(object->getOOClass() == DataSet::OOClass())
				_dataset = static_object_cast<DataSet>(object);

			OVITO_ASSERT(_dataset == nullptr || !object->getOOClass().isDerivedFrom(RefTarget::OOClass()) || static_object_cast<RefTarget>(object)->dataset() == _dataset);
		}
		else if(!excludeRecomputableData) {
			OVITO_ASSERT(_objects[id-1].first == object);
			_objects[id-1].second = false;
		}
		*this << id;
	}
}

/******************************************************************************
* Closes the stream.
******************************************************************************/
void ObjectSaveStream::close()
{
	if(!isOpen())
		return;

	try {
		QVector<qint64> objectOffsets;

		// Save all objects.
		beginChunk(0x100);
		for(size_t i = 0; i < _objects.size(); i++) {
			OvitoObject* obj = _objects[i].first;
			OVITO_CHECK_OBJECT_POINTER(obj);
			objectOffsets.push_back(filePosition());
			obj->saveToStream(*this, _objects[i].second);
		}
		endChunk();

		// Save RTTI.
		map<OvitoClassPtr, quint32> classes;
		qint64 beginOfRTTI = filePosition();
		beginChunk(0x200);
		for(const auto& obj : _objects) {
			OvitoClassPtr descriptor = &obj.first->getOOClass();
			if(classes.find(descriptor) == classes.end()) {
				classes.insert(make_pair(descriptor, (quint32)classes.size()));
				// Write the runtime type information to the stream.
				beginChunk(0x201);
				OvitoClass::serializeRTTI(*this, descriptor);
				endChunk();
				// Write the property fields to the stream if this is a RefMaker derived class.
				beginChunk(0x202);
				if(descriptor->isDerivedFrom(RefMaker::OOClass())) {
					for(const PropertyFieldDescriptor* field : static_cast<const RefMakerClass*>(descriptor)->propertyFields()) {
						beginChunk(0x01);
						*this << QByteArray::fromRawData(field->identifier(), qstrlen(field->identifier()));
						OvitoClass::serializeRTTI(*this, field->definingClass());
						*this << field->flags();
						*this << field->isReferenceField();
						if(field->isReferenceField()) {
							OvitoClass::serializeRTTI(*this, field->targetClass());
						}
						endChunk();
					}
				}
				// Write list terminator.
				beginChunk(0x00000000);
				endChunk();

				endChunk();
			}
		}
		endChunk();

		// Save object table.
		qint64 beginOfObjTable = filePosition();
		beginChunk(0x300);
		auto offsetIterator = objectOffsets.constBegin();
		for(const auto& obj : _objects) {
			*this << classes[&obj.first->getOOClass()];
			*this << *offsetIterator++;
		}
		endChunk();

		// Write index.
		*this << beginOfRTTI;
		*this << (quint32)classes.size();
		*this << beginOfObjTable;
		*this << (quint32)_objects.size();
	}
	catch(...) {
		SaveStream::close();
		throw;
	}
	SaveStream::close();
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
