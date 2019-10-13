////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2014 Alexander Stukowski
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


#include <ovito/core/Core.h>
#include <ovito/core/oo/OvitoObject.h>
#include <ovito/core/oo/OORef.h>
#include <ovito/core/utilities/io/LoadStream.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(IO)

/**
 * \brief An input stream that can deserialize an OvitoObject graph stored in a file.
 *
 * This class restores an object graph previously saved with the ObjectSaveStream class.
 *
 * \sa ObjectSaveStream
 */
class OVITO_CORE_EXPORT ObjectLoadStream : public LoadStream
{
	Q_OBJECT

public:

	/// \brief Initializes the ObjectLoadStream.
	/// \param source The Qt data stream from which the data is read. This stream must support random access.
	/// \throw Exception if the source stream does not support random access, or if an I/O error occurs.
	ObjectLoadStream(QDataStream& source);

	// Calls close() to close the ObjectLoadStream.
	virtual ~ObjectLoadStream() { ObjectLoadStream::close(); }

	/// \brief Closes the ObjectLoadStream, but not the underlying QDataStream passed to the constructor.
	virtual void close();

	/// \brief Loads an object from the stream.
	/// \note The returned object is not initialized yet when the function returns, and should not be accessed.
	///       The object's contents are loaded when close() is called.
	template<class T>
	OORef<T> loadObject() {
		OORef<OvitoObject> ptr = loadObjectInternal();
		OVITO_ASSERT(!ptr || ptr->getOOClass().isDerivedFrom(T::OOClass()));
		if(ptr && !ptr->getOOClass().isDerivedFrom(T::OOClass()))
			throw Exception(tr("Class hierarchy mismatch in file. The object class '%1' is not derived from '%2'.").arg(ptr->getOOClass().name()).arg(T::OOClass().name()));
		return static_object_cast<T>(ptr);
	}

	/// Returns the dataset to which objects loaded from the stream will be added to.
	DataSet* dataset() { return _dataset; }

	/// Sets the dataset to which objects loaded from the stream should be added to.
	void setDataset(DataSet* dataset) { _dataset = dataset; }

	/// Returns the class info for an object currently being deserialized from the stream.
	/// This method may only be called from within an OvitoObject::loadFromStream() method.
	const OvitoClass::SerializedClassInfo* getSerializedClassInfo() const {
		OVITO_ASSERT_MSG(_currentObject, "ObjectLoadStream::getSerializedClassInfo()", "No current object. Function may only called from within loadFromStream().");
		return _currentObject->classInfo;
	}

private:

	/// Loads an object with runtime type information from the stream.
	OORef<OvitoObject> loadObjectInternal();

	/// Data structure describing a single object instance loaded from the file.
	struct ObjectRecord {

		/// The object instance created from the serialized data.
		OORef<OvitoObject> object;

		/// The serialized class information.
		const OvitoClass::SerializedClassInfo* classInfo;

		/// The position at which the object data is stored in the file.
		qint64 fileOffset;
	};

	/// The list of classes stored in the file.
	std::vector<std::unique_ptr<OvitoClass::SerializedClassInfo>> _classes;

	/// List all the object instances stored in the file.
	std::vector<ObjectRecord> _objects;

	/// Objects that need to be loaded.
	std::vector<quint32> _objectsToLoad;

	/// The object currently being loaded from the stream.
	ObjectRecord* _currentObject = nullptr;

	/// The current dataset being loaded.
	DataSet* _dataset = nullptr;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace


