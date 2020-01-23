////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2019 Alexander Stukowski
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
#include "PropertyStorage.h"
#include "PropertyObject.h"

#include <boost/range/adaptor/strided.hpp>

namespace Ovito { namespace StdObj {

namespace detail {

// Base class that stores a pointer to an underlying PropertyStorage object.
template<class PointerType>
class PropertyAccessBase
{
public:

	/// \brief Returns the number of elements in the property array.
	size_t size() const { 
		OVITO_ASSERT(this->_storage);
		return this->_storage->size(); 
	}

	/// \brief Returns the number of vector components per element.
	size_t componentCount() const { 
		OVITO_ASSERT(this->_storage);
		return this->_storage->componentCount(); 
	}

	/// \brief Returns the number of bytes per element.
	size_t stride() const { 
		OVITO_ASSERT(this->_storage);
		return this->_storage->stride(); 
	}

	/// \brief Returns the number of bytes per vector component.
	size_t dataTypeSize() const { 
		OVITO_ASSERT(this->_storage);
		return this->_storage->dataTypeSize(); 
	}

	/// \brief Returns whether this accessor object points to a valid PropertyStorage. 
	explicit operator bool() const noexcept {
		return (bool)this->_storage;
	}

	/// \brief Returns the pointer to the internal PropertyStorage object.
	const PointerType& storage() const {
		return this->_storage;
	}

	/// \brief Detaches the accessor object from the underlying PropertyStorage.
	void reset() {
		this->_storage = nullptr;
	}

protected:

	/// Constructor that creates an invalid access object not associated with any PropertyStorage.
	PropertyAccessBase() {}

	/// Constructor that associates the access object with a PropertyStorage (may be null).
	PropertyAccessBase(PointerType storage) : _storage(std::move(storage)) {}

#ifdef OVITO_DEBUG
	/// Destructor sets the internal storage pointer to null to easier detect invalid memory access.
	~PropertyAccessBase() { reset(); }
#endif

	/// Pointer to the PropertyStorage object holding the data elements.
	PointerType _storage{nullptr};
};

// Base class that allows read access to the data elements of the underlying PropertyStorage.
template<typename T, class PointerType>
class ReadOnlyPropertyAccessBase : public PropertyAccessBase<PointerType>
{
public:

	using iterator = const T*;
	using const_iterator = const T*;

	/// \brief Returns the value of the i-th element from the array.
	const T& get(size_t i) const {
		OVITO_ASSERT(i < this->size());
		return *(this->cbegin() + i);
	}

	/// \brief Indexed access to the elements of the array.
	const T& operator[](size_t i) const {
		return this->get(i);
	}

	/// \brief Returns a range of const iterators over the elements stored in this array.
	boost::iterator_range<const T*> crange() const {
		return boost::make_iterator_range(cbegin(), cend());
	}

	/// \brief Returns a pointer to the first element of the property array.
	const T* begin() const {
		return cbegin();
	}

	/// \brief Returns a pointer pointing to the end of the property array.
	const T* end() const {
		return cend();
	}

	/// \brief Returns a pointer to the first element of the property array.
	const T* cbegin() const {
		OVITO_ASSERT(this->_storage);
		OVITO_ASSERT(this->_storage->dataType() == PropertyStoragePrimitiveDataType<T>::value);
		OVITO_ASSERT(this->stride() == sizeof(T));
		return reinterpret_cast<const T*>(this->_storage->cbuffer());
	}

	/// \brief Returns a pointer pointing to the end of the property array.
	const T* cend() const {
		return cbegin() + this->size();
	}

protected:

	/// Constructor that creates an invalid access object not associated with any PropertyStorage.
	ReadOnlyPropertyAccessBase() {}

	/// Constructor that associates the access object with a PropertyStorage (may be null).
	ReadOnlyPropertyAccessBase(PointerType storage) : PropertyAccessBase<PointerType>(std::move(storage)) {
		OVITO_ASSERT(!this->_storage || this->_storage->stride() == sizeof(T));
		OVITO_ASSERT(!this->_storage || this->_storage->dataType() == PropertyStoragePrimitiveDataType<T>::value);
	}
};

// Base class that allows read access to the individual components of vector elements of the underlying PropertyStorage.
template<typename T, class PointerType>
class ReadOnlyPropertyAccessBaseTable : public PropertyAccessBase<PointerType>
{
public:

	using iterator = const T*;
	using const_iterator = const T*;

	/// \brief Returns the value of the i-th element from the array.
	const T& get(size_t i, size_t j) const {
		OVITO_ASSERT(i < this->size());
		OVITO_ASSERT(j < this->componentCount());
		return *(this->cbegin() + (i * this->componentCount()) + j);
	}

	/// \brief Returns a pointer to the beginning of the property array.
	const T* cbegin() const {
		return reinterpret_cast<const T*>(this->_storage->cbuffer());
	}

	/// \brief Returns a pointer to the end of the property array.
	const T* cend() const {
		return this->cbegin() + (this->size() * this->elementCount());
	}

	/// \brief Returns a range of iterators over the i-th vector component of all elements stored in this array.
	auto componentRange(size_t componentIndex) const {
		OVITO_ASSERT(this->componentCount() > componentIndex);
		const T* begin = cbegin() + componentIndex;
		return boost::adaptors::stride(boost::make_iterator_range(begin, begin + (this->size() * this->componentCount())), this->componentCount());
	}

protected:

	/// Constructor that creates an invalid access object not associated with any PropertyStorage.
	ReadOnlyPropertyAccessBaseTable() {}

	/// Constructor that associates the access object with a PropertyStorage (may be null).
	ReadOnlyPropertyAccessBaseTable(PointerType storage) : PropertyAccessBase<PointerType>(std::move(storage)) {
		OVITO_ASSERT(!this->_storage || this->_storage->stride() == sizeof(T) * this->_storage->componentCount());
		OVITO_ASSERT(!this->_storage || this->_storage->dataType() == qMetaTypeId<T>());
		OVITO_ASSERT(!this->_storage || this->_storage->dataTypeSize() == sizeof(T));
	}
};

// Base class that allows read access to the raw data of the underlying PropertyStorage.
template<class PointerType>
class ReadOnlyPropertyAccessBaseTable<void, PointerType> : public PropertyAccessBase<PointerType>
{
public:

	/// \brief Returns the j-th component of the i-th element in the array.
	template<typename U>
	U get(size_t i, size_t j) const {
		OVITO_ASSERT(this->_storage);
		switch(this->storage()->dataType()) {
		case PropertyStorage::Float:
			return static_cast<U>(*reinterpret_cast<const FloatType*>(this->cdata(j) + i * this->stride()));
		case PropertyStorage::Int:
			return static_cast<U>(*reinterpret_cast<const int*>(this->cdata(j) + i * this->stride()));
		case PropertyStorage::Int64:
			return static_cast<U>(*reinterpret_cast<const qlonglong*>(this->cdata(j) + i * this->stride()));
		default:
			OVITO_ASSERT(false);
			throw Exception(QStringLiteral("Cannot read value from property '%1', because it has a non-standard data type.").arg(this->_storage->name()));
		}
	}

	/// \brief Returns a pointer to the raw data of the property array.
	const uint8_t* cdata(size_t component = 0) const {
		OVITO_ASSERT(this->_storage);
		return this->_storage->cbuffer() + (component * this->dataTypeSize());
	}

	/// \brief Returns a pointer to the raw data of the property array.
	const uint8_t* cdata(size_t index, size_t component) const {
		OVITO_ASSERT(this->_storage);
		OVITO_ASSERT(index < this->size());
		OVITO_ASSERT(component < this->componentCount());
		return this->_storage->cbuffer() + (index * this->stride()) + (component * this->dataTypeSize());
	}

protected:

	// Inherit constructors from base class.
	using PropertyAccessBase<PointerType>::PropertyAccessBase;
};

// Base class that allows read/write access to the data elements of the underlying PropertyStorage.
template<typename T, class PointerType>
class ReadWritePropertyAccessBase : public ReadOnlyPropertyAccessBase<T, PointerType>
{
public:

	using iterator = T*;
	using const_iterator = const T*;

	/// \brief Sets the value of the i-th element in the array.
	void set(size_t i, const T& v) {
		OVITO_ASSERT(i < this->size());
		*(this->begin() + i) = v;
	}

	/// \brief Indexed access to the elements of the array.
	T& operator[](size_t i) {
		OVITO_ASSERT(i < this->size());
		return *(this->begin() + i);
	}

	/// \brief Indexed access to the elements of the array.
	const T& operator[](size_t i) const {
		OVITO_ASSERT(i < this->size());
		return *(this->cbegin() + i);
	}

	/// \brief Returns a pointer to the first element of the property array.
	T* begin() const {
		OVITO_ASSERT(this->_storage);
		return reinterpret_cast<T*>(this->_storage->buffer());
	}

	/// \brief Returns a pointer pointing to the end of the property array.
	T* end() const {
		return this->begin() + this->size();
	}

	/// \brief Returns a range of iterators over the elements stored in this array.
	boost::iterator_range<T*> range() {
		return boost::make_iterator_range(begin(), end());
	}

	/// \brief Sets all array elements to the given uniform value.
	void fill(const T& value) {
		OVITO_ASSERT(this->storage());
		this->_storage->template fill<T>(value);
	}

	/// \brief Sets all array elements for which the corresponding entries in the 
	///        selection array are non-zero to the given uniform value.
	void fillSelected(const T& value, const PropertyStorage* selectionProperty) {
		OVITO_ASSERT(this->storage());
		this->_storage->template fillSelected<T>(value, selectionProperty);
	}

	/// Copies the data from the given source array to this array. 
	/// The array size and data type of source and destination must match.
	template<class PointerType2>
	void copyFrom(const ReadOnlyPropertyAccessBase<T, PointerType2>& source) {
		OVITO_ASSERT(this->storage());
		OVITO_ASSERT(source.storage());
		this->_storage->copyFrom(*source.storage());
	}

protected:

	// Inherit constructors from base class.
	using ReadOnlyPropertyAccessBase<T, PointerType>::ReadOnlyPropertyAccessBase;
};

// Base class that allows read/write access to the individual components of the vector elements of the underlying PropertyStorage.
template<typename T, class PointerType>
class ReadWritePropertyAccessBaseTable : public ReadOnlyPropertyAccessBaseTable<T, PointerType>
{
public:

	using iterator = T*;
	using const_iterator = const T*;

	/// \brief Returns a pointer to the first element of the property array.
	T* begin() const {
		OVITO_ASSERT(this->_storage);
		return reinterpret_cast<T*>(this->_storage->buffer());
	}

	/// \brief Returns a pointer pointing to the end of the property array.
	T* end() const {
		return this->begin() + (this->size() * this->elementCount());
	}

	/// \brief Returns a range of iterators over the i-th vector component of all elements stored in this array.
	auto componentRange(size_t componentIndex) {
		OVITO_ASSERT(this->_storage);
		OVITO_ASSERT(this->_storage->componentCount() > componentIndex);
		T* begin = this->begin() + componentIndex;
		return boost::adaptors::stride(boost::make_iterator_range(begin, begin + (this->size() * this->componentCount())), this->componentCount());
	}

	/// \brief Sets the j-th component of the i-th element of the array to a new value.
	void set(size_t i, size_t j, const T& value) {
		OVITO_ASSERT(this->_storage);
		*(begin() + i * this->componentCount() + j) = value;
	}

protected:

	// Inherit constructors from base class.
	using ReadOnlyPropertyAccessBaseTable<T, PointerType>::ReadOnlyPropertyAccessBaseTable;
};

// Base class that allows read/write access to the raw data of the underlying PropertyStorage.
template<class PointerType>
class ReadWritePropertyAccessBaseTable<void, PointerType> : public ReadOnlyPropertyAccessBaseTable<void, PointerType>
{
public:

	/// \brief Sets the j-th component of the i-th element of the array to a new value.
	template<typename U>
	void set(size_t i, size_t j, const U& value) {
		OVITO_ASSERT(this->_storage);
		switch(this->_storage->dataType()) {
		case PropertyStorage::Float:
			*reinterpret_cast<FloatType*>(this->data(j) + i * this->stride()) = value;
			break;
		case PropertyStorage::Int:
			*reinterpret_cast<int*>(this->data(j) + i * this->stride()) = value;
			break;
		case PropertyStorage::Int64:
			*reinterpret_cast<qlonglong*>(this->data(j) + i * this->stride()) = value;
			break;
		default:
			OVITO_ASSERT(false);
			throw Exception(QStringLiteral("Cannot assign value to property '%1', because it has a non-standard data type.").arg(this->_storage->name()));
		}
	}

	/// \brief Returns a pointer to the raw data of the property array.
	uint8_t* data(size_t component = 0) {
		OVITO_ASSERT(this->_storage);
		return this->_storage->buffer() + (component * this->dataTypeSize());
	}

	/// \brief Returns a pointer to the raw data of the property array.
	uint8_t* data(size_t index, size_t component) {
		OVITO_ASSERT(this->_storage);
		OVITO_ASSERT(index < this->size());
		OVITO_ASSERT(component < this->componentCount());
		return this->_storage->buffer() + (index * this->stride()) + (component * this->dataTypeSize());
	}

protected:

	// Inherit constructors from base class.
	using ReadOnlyPropertyAccessBaseTable<void, PointerType>::ReadOnlyPropertyAccessBaseTable;
};

} // End of namespace detail.

/**
 * \brief Helper class that provides read access to the data elements of a PropertyStorage object.
 * 
 * The TableMode template parameter should be set to true if access to the individual components
 * of a vector property array is desired or if the number of vector components of the property is unknown at compile time. 
 * If TableMode is set to false, the data elements can only be access as a whole and the number of components must
 * be a compile-time constant.
 */
template<typename T, bool TableMode = false>
class ConstPropertyAccess : public std::conditional_t<TableMode, Ovito::StdObj::detail::ReadOnlyPropertyAccessBaseTable<T, const PropertyStorage*>, Ovito::StdObj::detail::ReadOnlyPropertyAccessBase<T, const PropertyStorage*>>
{
	using ParentType = std::conditional_t<TableMode, Ovito::StdObj::detail::ReadOnlyPropertyAccessBaseTable<T, const PropertyStorage*>, Ovito::StdObj::detail::ReadOnlyPropertyAccessBase<T, const PropertyStorage*>>;

public:

	/// Constructs an accessor object not associated yet with any PropertyStorage.
	ConstPropertyAccess() {}

	/// Constructs a read-only accessor for the data in a PropertyObject.
	ConstPropertyAccess(const PropertyObject* propertyObj) 
		: ParentType(propertyObj ? propertyObj->storage().get() : nullptr) {}

	/// Constructs a read-only accessor for the data in a PropertyStorage.
	ConstPropertyAccess(const PropertyPtr& property) 
		: ParentType(property.get()) {}

	/// Constructs a read-only accessor for the data in a PropertyStorage.
	ConstPropertyAccess(const ConstPropertyPtr& property) 
		: ParentType(property.get()) {}

	/// Constructs a read-only accessor for the data in a PropertyStorage.
	ConstPropertyAccess(const PropertyStorage& property) 
		: ParentType(&property) {}
};

/**
 * \brief Helper class that provides read access to the data elements in a PropertyStorage object
 *        and which keeps a strong reference to the PropertyStorage.
 */
template<typename T>
class ConstPropertyAccessAndRef : public Ovito::StdObj::detail::ReadOnlyPropertyAccessBase<T, ConstPropertyPtr>
{
public:

	/// Constructs an accessor object not associated yet with any PropertyStorage.
	ConstPropertyAccessAndRef() {}

	/// Constructs a read-only accessor for the data in a PropertyObject.
	ConstPropertyAccessAndRef(const PropertyObject* propertyObj) 
		: Ovito::StdObj::detail::ReadOnlyPropertyAccessBase<T, ConstPropertyPtr>(propertyObj ? propertyObj->storage() : nullptr) {}

	/// Constructs a read-only accessor for the data in a PropertyStorage.
	ConstPropertyAccessAndRef(ConstPropertyPtr property)
		: Ovito::StdObj::detail::ReadOnlyPropertyAccessBase<T, ConstPropertyPtr>(std::move(property)) {}
};

/**
 * \brief Helper class that provides read/write access to the data elements in a PropertyStorage object.
 * 
 * The TableMode template parameter should be set to true if access to the individual components
 * of a vector property array is desired or if the number of vector components of the property is unknown at compile time. 
 * If TableMode is set to false, the data elements can only be access as a whole and the number of components must
 * be a compile-time constant.
 * 
 * If the PropertyAccess object is initialized from a PropertyObject pointer, the property object's notifyTargetChanged()
 * method will automatically be called when the PropertyAccess object goes out of scope to inform the system about
 * a modification of the stored property values.
 */
template<typename T, bool TableMode = false>
class PropertyAccess : public std::conditional_t<TableMode, Ovito::StdObj::detail::ReadWritePropertyAccessBaseTable<T, PropertyStorage*>, Ovito::StdObj::detail::ReadWritePropertyAccessBase<T, PropertyStorage*>>
{
	using ParentType = std::conditional_t<TableMode, Ovito::StdObj::detail::ReadWritePropertyAccessBaseTable<T, PropertyStorage*>, Ovito::StdObj::detail::ReadWritePropertyAccessBase<T, PropertyStorage*>>;

public:

	/// Constructs an accessor object not associated yet with any PropertyStorage.
	PropertyAccess() {}

	/// Constructs a read/write accessor for the data in a PropertyObject.
	PropertyAccess(PropertyObject* propertyObj) : 
		ParentType(propertyObj ? propertyObj->modifiableStorage().get() : nullptr), 
		_propertyObject(propertyObj) {}

	/// Constructs a read/write accessor for the data in a PropertyStorage.
	PropertyAccess(const PropertyPtr& property) 
		: ParentType(property.get()) {}

	/// Constructs a read/write accessor for the data in a PropertyStorage.
	PropertyAccess(PropertyStorage* property) 
		: ParentType(property) {}

	/// When the PropertyAccess object goes out of scope, an automatic change message is sent by the
	/// the PropertyObject, assuming that its contents have been modified by the user of the PropertyAccess object.
	~PropertyAccess() {
		if(_propertyObject) _propertyObject->notifyTargetChanged();
	}

	/// Returns the PropertyObject that owns the PropertyStorage.
	PropertyObject* propertyObject() const { return _propertyObject; }

private:

	/// Pointer to the PropertyObject that owns the PropertyStorage.
	PropertyObject* _propertyObject = nullptr;
};

/**
 * \brief Helper class that provides read/write access to the data elements in a PropertyStorage object
 *        and which keeps a strong reference to the PropertyStorage.
 */
template<typename T, bool TableMode = false>
class PropertyAccessAndRef : public std::conditional_t<TableMode, Ovito::StdObj::detail::ReadWritePropertyAccessBaseTable<T, PropertyPtr>, Ovito::StdObj::detail::ReadWritePropertyAccessBase<T, PropertyPtr>>
{
	using ParentType = std::conditional_t<TableMode, Ovito::StdObj::detail::ReadWritePropertyAccessBaseTable<T, PropertyPtr>, Ovito::StdObj::detail::ReadWritePropertyAccessBase<T, PropertyPtr>>;

public:

	/// Constructs an accessor object not associated yet with any PropertyStorage.
	PropertyAccessAndRef() {}

	/// Constructs a read/write accessor for the data in a PropertyObject.
	PropertyAccessAndRef(PropertyObject* propertyObj) : 
		ParentType(propertyObj ? propertyObj->modifiableStorage() : nullptr) {}

	/// Constructs a read/write accessor for the data in a PropertyStorage.
	PropertyAccessAndRef(PropertyPtr property) 
		: ParentType(std::move(property)) {}

	/// \brief Helper method for implementing copy-on-write semantics.
	///        Makes sure that the property storage exclusive owned by this object.
	void makeMutable() { 
		PropertyStorage::makeMutable(this->_storage);
	}

	/// \brief Moves the internal PropertyPtr out of this object.
	PropertyPtr takeStorage() {
		return std::move(this->_storage);
	}
};

// Export a couple of commonly used class template instantiations. 
extern template class OVITO_STDOBJ_EXPORT ConstPropertyAccess<int>;
extern template class OVITO_STDOBJ_EXPORT ConstPropertyAccess<qlonglong>;
extern template class OVITO_STDOBJ_EXPORT ConstPropertyAccess<FloatType>;
extern template class OVITO_STDOBJ_EXPORT ConstPropertyAccess<Point3>;
extern template class OVITO_STDOBJ_EXPORT ConstPropertyAccess<Vector3>;
extern template class OVITO_STDOBJ_EXPORT ConstPropertyAccess<Color>;
extern template class OVITO_STDOBJ_EXPORT ConstPropertyAccess<Vector3I>;
extern template class OVITO_STDOBJ_EXPORT ConstPropertyAccess<std::array<qlonglong,2>>;
extern template class OVITO_STDOBJ_EXPORT ConstPropertyAccess<int, true>;
extern template class OVITO_STDOBJ_EXPORT ConstPropertyAccess<qlonglong, true>;
extern template class OVITO_STDOBJ_EXPORT ConstPropertyAccess<FloatType, true>;
extern template class OVITO_STDOBJ_EXPORT ConstPropertyAccess<void, true>;
extern template class OVITO_STDOBJ_EXPORT PropertyAccess<int>;
extern template class OVITO_STDOBJ_EXPORT PropertyAccess<qlonglong>;
extern template class OVITO_STDOBJ_EXPORT PropertyAccess<FloatType>;
extern template class OVITO_STDOBJ_EXPORT PropertyAccess<Point3>;
extern template class OVITO_STDOBJ_EXPORT PropertyAccess<Vector3>;
extern template class OVITO_STDOBJ_EXPORT PropertyAccess<Color>;
extern template class OVITO_STDOBJ_EXPORT PropertyAccess<Vector3I>;
extern template class OVITO_STDOBJ_EXPORT PropertyAccess<std::array<qlonglong,2>>;
extern template class OVITO_STDOBJ_EXPORT PropertyAccess<int, true>;
extern template class OVITO_STDOBJ_EXPORT PropertyAccess<qlonglong, true>;
extern template class OVITO_STDOBJ_EXPORT PropertyAccess<FloatType, true>;
extern template class OVITO_STDOBJ_EXPORT PropertyAccess<void, true>;

}	// End of namespace
}	// End of namespace
