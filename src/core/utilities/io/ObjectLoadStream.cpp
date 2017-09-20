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
#include <core/oo/OORef.h>
#include <core/app/PluginManager.h>
#include <core/dataset/DataSet.h>
#include "ObjectLoadStream.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(IO)

/******************************************************************************
* Opens the stream for reading.
******************************************************************************/
ObjectLoadStream::ObjectLoadStream(QDataStream& source) : LoadStream(source)
{
	qint64 oldPos = filePosition();

	// Jump to index at the end of the file.
	setFilePosition(source.device()->size() - 2 * (qint64)(sizeof(qint64) + sizeof(quint32)));

	// Read index of tables.
	qint64 classTableStart, objectTableStart;
	quint32 classCount, objectCount;
	*this >> classTableStart;
	*this >> classCount;
	*this >> objectTableStart;
	*this >> objectCount;

	// Jump to beginning of the class table.
	setFilePosition(classTableStart);
	expectChunk(0x200);
	_classes.resize(classCount);
	for(auto& classInfo : _classes) {

		// Read the runtime type from the stream.
		expectChunk(0x201);
		OvitoClassPtr clazz = OvitoClass::deserializeRTTI(*this);
		closeChunk();

		// Load the plugin containing the class.
		clazz->plugin()->loadPlugin();

		// Create the class info structure.
		classInfo = clazz->createClassInfoStructure();
		classInfo->clazz = clazz;

		// Let the metaclass read its specific information from the stream.
		expectChunk(0x202);
		clazz->loadClassInfo(*this, classInfo.get());
		closeChunk();
	}
	closeChunk();

	// Jump to start of object table.
	setFilePosition(objectTableStart);
	expectChunk(0x300);
	_objects.resize(objectCount);
	for(ObjectEntry& entry : _objects) {
		entry.object = nullptr;
		quint32 id;
		*this >> id;
		entry.classInfo = _classes[id].get();
		*this >> entry.fileOffset;
	}
	closeChunk();

	// Go back to previous position in file.
	setFilePosition(oldPos);
}

/******************************************************************************
* Loads an object with runtime type information from the stream.
* The method returns a pointer to the object but this object will be
* in an uninitialized state until it is loaded at a later time.
******************************************************************************/
OORef<OvitoObject> ObjectLoadStream::loadObjectInternal()
{
	quint32 id;
	*this >> id;
	if(id == 0) return nullptr;
	else {
		ObjectEntry& entry = _objects[id - 1];
		if(entry.object != nullptr) return entry.object;
		else {
			// When loading a RefTarget-derived class from the stream, we must already have a current DataSet as context.
			OVITO_ASSERT(_dataset != nullptr || entry.classInfo->clazz == &DataSet::OOClass() || !entry.classInfo->clazz->isDerivedFrom(RefTarget::OOClass()));

			// Create an instance of the object class.
			entry.object = entry.classInfo->clazz->createInstance(_dataset);
			
			// When deserializing a DataSet, use it as the context for all subsequently deserialized objects.
			if(entry.classInfo->clazz == &DataSet::OOClass()) {
				OVITO_ASSERT(_dataset == nullptr);
				setDataset(static_object_cast<DataSet>(entry.object.get()));
			}
			else {
				OVITO_ASSERT(!entry.classInfo->clazz->isDerivedFrom(RefTarget::OOClass()) || _dataset != nullptr);
			}

			_objectsToLoad.push_back(id - 1);
			return entry.object;
		}
	}
}

/******************************************************************************
* Closes the stream.
******************************************************************************/
void ObjectLoadStream::close()
{
	// This prevents re-entrance in case of an exception.
	if(!_currentObject) {

		for(int i = 0; i < _objectsToLoad.size(); i++) {
			quint32 index = _objectsToLoad[i];
			_currentObject = &_objects[index];
			OVITO_CHECK_POINTER(_currentObject);
			OVITO_CHECK_OBJECT_POINTER(_currentObject->object);

			// Seek to object data.
			setFilePosition(_currentObject->fileOffset);

			// Load class contents.
			try {
				try {
					// Make the object being loaded a child of this stream object.
					// This is to let the OvitoObject::isBeingLoaded() function detect that
					// the object is being loaded from this stream.
					OVITO_ASSERT(_currentObject->object->parent() == nullptr);
					_currentObject->object->setParent(this);
					OVITO_ASSERT(_currentObject->object->isBeingLoaded());

					// Let the object load its data fields.
					_currentObject->object->loadFromStream(*this);

					OVITO_ASSERT(_currentObject->object->parent() == this);
					_currentObject->object->setParent(nullptr);
				}
				catch(...) {
					// Clean up.
					if(_currentObject && _currentObject->object) {
						OVITO_CHECK_OBJECT_POINTER(_currentObject->object);
						_currentObject->object->setParent(nullptr);
					}
					throw;
				}
			}
			catch(Exception& ex) {
				throw ex.appendDetailMessage(tr("Object of class type %1 failed to load.").arg(_currentObject->object->getOOClass().name()));
			}
		}

		// Now that all references are in place call post-processing function on each loaded object.
		for(const ObjectEntry& entry : _objects) {
			if(entry.object)
				entry.object->loadFromStreamComplete();
		}
	}
	LoadStream::close();
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
