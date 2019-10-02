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
#include "DataObject.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Scene)

/// Utility class that is used to reference a particular data object in a DataCollection
/// as a path through the hierarchy of nested data objects.
class OVITO_CORE_EXPORT ConstDataObjectPath : public QVarLengthArray<const DataObject*, 3>
{
public:

	/// Inherit constructors from base class.
	using QVarLengthArray::QVarLengthArray;

	/// Converts the path to a string representation.
	QString toString() const;

	/// Produces a string representation of the object path that is suitable for the user interface.
	QString toHumanReadableString() const;
};

/// Utility class that is used to reference a particular data object in a DataCollection
/// as a path through the hierarchy of nested data objects.
class OVITO_CORE_EXPORT DataObjectPath : public QVarLengthArray<DataObject*, 3>
{
public:

	/// Inherit constructors from base class.
	using QVarLengthArray::QVarLengthArray;

	/// A path to a mutable object can be implicitly converted to a path to a constant object.
	operator const ConstDataObjectPath&() const {
		return *reinterpret_cast<const ConstDataObjectPath*>(this);
	}

	/// Converts the path to a string representation.
	QString toString() const { return static_cast<const ConstDataObjectPath&>(*this).toString(); }

	/// Produces a string representation of the object path that is suitable for the user interface.
	QString toHumanReadableString() const { return static_cast<const ConstDataObjectPath&>(*this).toHumanReadableString(); }
};

/**
 * \brief A reference to a DataObject in a PipelineFlowState.
 */
class OVITO_CORE_EXPORT DataObjectReference
{
public:

	/// \brief Default constructor. Creates a null reference.
	DataObjectReference() = default;

	/// \brief Constructs a reference to a data object.
	DataObjectReference(const DataObject::OOMetaClass* dataClass, const QString& dataPath = QString(), const QString& dataTitle = QString()) :
		_dataClass(dataClass), _dataPath(dataPath), _dataTitle(dataTitle) {}

	/// \brief Constructs a reference to a data object from a data object path.
	DataObjectReference(const ConstDataObjectPath& path) : DataObjectReference(path.empty() ? nullptr : &path.back()->getOOMetaClass(), path.toString(), path.toHumanReadableString()) {}

	/// Returns the DataObject subclass being referenced.
	const DataObject::OOMetaClass* dataClass() const { return _dataClass; }

	/// Returns the identifier and path of the data object being referenced.
	const QString& dataPath() const { return _dataPath; }

	/// Returns the title of the data object used in the user interface.
	const QString& dataTitle() const { return _dataTitle; }

	/// \brief Compares two references for equality.
	bool operator==(const DataObjectReference& other) const {
		return dataClass() == other.dataClass() && dataPath() == other.dataPath();
	}

	/// \brief Compares two references for inequality.
	bool operator!=(const DataObjectReference& other) const { return !(*this == other); }

	/// \brief Returns whether this reference points to any data object.
	explicit operator bool() const {
		return dataClass() != nullptr;
	}

private:

	/// The DataObject subclass being referenced.
	const DataObject::OOMetaClass* _dataClass = nullptr;

	/// The identifier and path of the data object being referenced.
	QString _dataPath;

	/// The title of the data object used in the user interface (optional).
	QString _dataTitle;

	friend OVITO_CORE_EXPORT SaveStream& operator<<(SaveStream& stream, const DataObjectReference& r);
	friend OVITO_CORE_EXPORT LoadStream& operator>>(LoadStream& stream, DataObjectReference& r);
};

/// Writes a DataObjectReference to an output stream.
/// \relates DataObjectReference
inline OVITO_CORE_EXPORT SaveStream& operator<<(SaveStream& stream, const DataObjectReference& r)
{
	stream.beginChunk(0x02);
	stream << r.dataClass();
	stream << r.dataPath();
	stream << r.dataTitle();
	stream.endChunk();
	return stream;
}

/// Reads a DataObjectReference from an input stream.
/// \relates DataObjectReference
inline OVITO_CORE_EXPORT LoadStream& operator>>(LoadStream& stream, DataObjectReference& r)
{
	stream.expectChunk(0x02);
	stream >> r._dataClass;
	stream >> r._dataPath;
	stream >> r._dataTitle;
	if(!r._dataClass)
		r._dataPath.clear();
	stream.closeChunk();
	return stream;
}

/**
 * A reference to a DataObject subclass.
 */
template<class DataObjectType>
class TypedDataObjectReference : public DataObjectReference
{
public:

	/// \brief Default constructor. Creates a null reference.
	TypedDataObjectReference() = default;

	/// \brief Conversion copy constructor.
	TypedDataObjectReference(const DataObjectReference& other) : DataObjectReference(other) {
		OVITO_ASSERT(!DataObjectReference::dataClass() || DataObjectReference::dataClass()->isDerivedFrom(DataObjectType::OOClass()));
	}

	/// \brief Conversion move constructor.
	TypedDataObjectReference(DataObjectReference&& other) : DataObjectReference(std::move(other)) {
		OVITO_ASSERT(!DataObjectReference::dataClass() || DataObjectReference::dataClass()->isDerivedFrom(DataObjectType::OOClass()));
	}

	/// \brief Constructs a reference to a data object.
	TypedDataObjectReference(const typename DataObjectType::OOMetaClass* dataClass, const QString& dataPath = QString(), const QString& dataTitle = QString()) : DataObjectReference(dataClass, dataPath, dataTitle) {}

	/// Returns the DataObject subclass being referenced.
	const typename DataObjectType::OOMetaClass* dataClass() const {
		return static_cast<const typename DataObjectType::OOMetaClass*>(DataObjectReference::dataClass());
	}

	friend SaveStream& operator<<(SaveStream& stream, const TypedDataObjectReference& r) {
		return stream << static_cast<const DataObjectReference&>(r);
	}

	friend LoadStream& operator>>(LoadStream& stream, TypedDataObjectReference& r) {
		return stream >> static_cast<DataObjectReference&>(r);
	}
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace

Q_DECLARE_METATYPE(Ovito::DataObjectReference);
