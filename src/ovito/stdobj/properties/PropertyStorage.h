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

namespace Ovito { namespace StdObj {

/**
 * \brief Memory storage used for e.g. particle and bond properties.
 */
class OVITO_STDOBJ_EXPORT PropertyStorage : public std::enable_shared_from_this<PropertyStorage>
{
public:

	/// \brief The most commonly used data types. Note that, at least in principle,
	///        the PropertyStorage class supports any data type registered with the Qt meta type system.
	enum StandardDataType {
#ifndef Q_CC_MSVC
		Int = qMetaTypeId<int>(),
		Int64 = qMetaTypeId<qlonglong>(),
		Float = qMetaTypeId<FloatType>()
#else // MSVC compiler doesn't treat qMetaTypeId() function as constexpr. Workaround:
		Int = QMetaType::Int,
		Int64 = QMetaType::LongLong,
#ifdef FLOATTYPE_FLOAT
		Float = QMetaType::Float
#else
		Float = QMetaType::Double
#endif
#endif
	};

	/// \brief The standard property types defined by all property classes.
	enum GenericStandardType {
		GenericUserProperty = 0,	//< This is reserved for user-defined properties.
		GenericSelectionProperty = 1,
		GenericColorProperty = 2,
		GenericTypeProperty = 3,
		GenericIdentifierProperty = 4,
		//Begin of modification
		GenericTransparencyProperty = 5,
		//End of modification
		
		// This is value at which type IDs of specific standard properties start:
		FirstSpecificProperty = 1000
	};

public:

	/// Helper method for implementing copy-on-write semantics.
	/// Checks if the property storage referred to by the shared_ptr is exclusive owned.
	/// If yes, it is returned as is. Otherwise, a copy of the data storage is made,
	/// stored in the shared_ptr, and returned by the function.
	static const std::shared_ptr<PropertyStorage>& makeMutable(std::shared_ptr<PropertyStorage>& propertyPtr) {
		OVITO_ASSERT(propertyPtr);
		OVITO_ASSERT(propertyPtr.use_count() >= 1);
		if(propertyPtr.use_count() > 1)
			propertyPtr = std::make_shared<PropertyStorage>(*propertyPtr);
		OVITO_ASSERT(propertyPtr.use_count() == 1);
		return propertyPtr;
	}

public:

	/// \brief Default constructor that creates an empty, uninitialized storage.
	PropertyStorage() = default;

	/// \brief Constructor that creates a property storage.
	PropertyStorage(size_t elementCount, int dataType, size_t componentCount, size_t stride, const QString& name, bool initializeMemory, int type = 0, QStringList componentNames = QStringList());

	/// \brief Copy constructor.
	PropertyStorage(const PropertyStorage& other);

	/// \brief Move constructor.
	PropertyStorage(PropertyStorage&& other) = default;

	/// \brief Returns the type of this property.
	int type() const { return _type; }

	/// \brief Changes the type of this property. Note that this method is only for internal use.
	///        Normally, you should not change the type of a property once it was created.
	void setType(int newType) { _type = newType; }

	/// \brief Gets the property's name.
	/// \return The name of property.
	const QString& name() const { return _name; }

	/// \brief Sets the property's name if this is a user-defined property.
	/// \param name The new name string.
	void setName(const QString& name) { _name = name; }

	/// \brief Returns the number of stored elements.
	size_t size() const { return _numElements; }

	/// \brief Resizes the property storage.
	/// \param newSize The new number of elements.
	/// \param preserveData Controls whether the existing data is preserved.
	///                     This also determines whether newly allocated memory is initialized to zero.
	void resize(size_t newSize, bool preserveData);

	/// \brief Grows the number of data elements while preserving the exiting data.
	/// Newly added elements are *not* initialized to zero by this method.
	/// \return True if the memory buffer was reallocated, because the current capacity was insufficient
	/// to accommodate the new elements.
	bool grow(size_t numAdditionalElements) {
		size_t newSize = _numElements + numAdditionalElements;
		OVITO_ASSERT(newSize >= _numElements);
		bool needToGrow;
		if((needToGrow = (newSize > _capacity)))
			growCapacity(newSize);
		_numElements = newSize;
		return needToGrow;
	}

	/// \brief Reduces the number of data elements while preserving the exiting data.
	/// Note: This method never reallocates the memory buffer. Thus, the capacity of the array remains unchanged and the
	/// memory of the truncated elements is not released by the method.
	void truncate(size_t numElementsToRemove) {
		OVITO_ASSERT(numElementsToRemove <= _numElements);
		_numElements -= numElementsToRemove;
	}

	/// \brief Returns the data type of the property.
	/// \return The identifier of the data type used for the elements stored in
	///         this property storage according to the Qt meta type system.
	int dataType() const { return _dataType; }

	/// \brief Returns the number of bytes per value.
	/// \return Number of bytes used to store a single value of the data type
	///         specified by type().
	size_t dataTypeSize() const { return _dataTypeSize; }

	/// \brief Returns the number of bytes used per element.
	size_t stride() const { return _stride; }

	/// \brief Returns the number of vector components per element.
	/// \return The number of data values stored per element in this storage object.
	size_t componentCount() const { return _componentCount; }

	/// \brief Returns the human-readable names for the vector components if this is a vector property.
	/// \return The names of the vector components if this property contains more than one value per element.
	///         If this is only a single-valued property then an empty list is returned by this method.
	const QStringList& componentNames() const { return _componentNames; }

	/// \brief Sets the human-readable names for the vector components if this is a vector property.
	void setComponentNames(QStringList names) {
		OVITO_ASSERT(names.empty() || names.size() == componentCount());
		_componentNames = std::move(names);
	}

	/// \brief Returns a read-only pointer to the raw element data stored in this property array.
	const uint8_t* cbuffer() const {
		return _data.get();
	}

	/// \brief Returns a read-write pointer to the raw element data stored in this property array.
	uint8_t* buffer() {
		return _data.get();
	}

	/// \brief Sets all array elements to the given uniform value.
	template<typename T>
	void fill(const T value) {
		OVITO_ASSERT(stride() == sizeof(T));
		T* begin = reinterpret_cast<T*>(buffer());
		T* end = begin + this->size();
		std::fill(begin, end, value);
	}

	/// \brief Sets all array elements for which the corresponding entries in the 
	///        selection array are non-zero to the given uniform value.
	template<typename T>
	void fillSelected(const T value, const PropertyStorage& selectionProperty) {
		OVITO_ASSERT(selectionProperty.size() == this->size());
		OVITO_ASSERT(selectionProperty.dataType() == Int);
		OVITO_ASSERT(selectionProperty.componentCount() == 1);
		const int* selectionIter = reinterpret_cast<const int*>(selectionProperty.cbuffer());
		for(T* v = reinterpret_cast<T*>(buffer()), *end = v + this->size(); v != end; ++v)
			if(*selectionIter++) *v = value;
	}

	/// \brief Sets all array elements for which the corresponding entries in the 
	///        selection array are non-zero to the given uniform value.
	template<typename T>
	void fillSelected(const T& value, const PropertyStorage* selectionProperty) {
		if(selectionProperty)
			fillSelected(value, *selectionProperty);
		else
			fill(value);
	}

	/// \brief Sets all array elements for which the corresponding entries in the 
	///        selection array are non-zero to the given uniform value.
	template<typename T>
	void fillSelected(const T& value, const ConstPropertyPtr& selectionProperty) {
		fillSelected(value, selectionProperty.get());
	}

	// Set all property values to zeros.
	void fillZero() {
		std::memset(_data.get(), 0, this->size() * this->stride());
	}

	/// Reduces the size of the storage array, removing elements for which
	/// the corresponding bits in the bit array are set.
	void filterResize(const boost::dynamic_bitset<>& mask);

	/// Creates a copy of the array, not containing those elements for which
	/// the corresponding bits in the given bit array were set.
	std::shared_ptr<PropertyStorage> filterCopy(const boost::dynamic_bitset<>& mask) const;

	/// Copies the contents from the given source into this storage using a element mapping.
	void mappedCopyFrom(const PropertyStorage& source, const std::vector<size_t>& mapping);

	/// Copies the elements from this storage array into the given destination array using an index mapping.
	void mappedCopyTo(PropertyStorage& destination, const std::vector<size_t>& mapping) const;

	/// Copies the data elements from the given source array into this array. 
	/// Array size, component count and data type of source and destination must match exactly.
	void copyFrom(const PropertyStorage& source);

	/// Copies a range of data elements from the given source array into this array. 
	/// Component count and data type of source and destination must be compatible.
	void copyRangeFrom(const PropertyStorage& source, size_t sourceIndex, size_t destIndex, size_t count);

	/// Writes the object to an output stream.
	void saveToStream(SaveStream& stream, bool onlyMetadata) const;

	/// Reads the object from an input stream.
	void loadFromStream(LoadStream& stream);

	/// Copies the values in this property array to the given output iterator if it is compatible.
	/// Returns false if copying was not possible, because the data type of the array and the output iterator
	/// are not compatible.
	template<typename Iter>
	bool copyTo(Iter iter, size_t component = 0) const {
		size_t cmpntCount = componentCount();
		if(component >= cmpntCount) return false;
		if(size() == 0) return true;
		if(dataType() == PropertyStorage::Int) {
			for(auto v = reinterpret_cast<const int*>(cbuffer()) + component, v_end = v + size()*cmpntCount; v != v_end; v += cmpntCount)
				*iter++ = *v;
			return true;
		}
		else if(dataType() == PropertyStorage::Int64) {
			for(auto v = reinterpret_cast<const qlonglong*>(cbuffer()) + component, v_end = v + size()*cmpntCount; v != v_end; v += cmpntCount)
				*iter++ = *v;
			return true;
		}
		else if(dataType() == PropertyStorage::Float) {
			for(auto v = reinterpret_cast<const FloatType*>(cbuffer()) + component, v_end = v + size()*cmpntCount; v != v_end; v += cmpntCount)
				*iter++ = *v;
			return true;
		}
		return false;
	}

	/// Calls a functor provided by the caller for every value of the given vector component.
	template<typename F>
	bool forEach(size_t component, F f) const {
		size_t cmpntCount = componentCount();
		size_t s = size();
		if(component >= cmpntCount) return false;
		if(s == 0) return true;
		if(dataType() == PropertyStorage::Int) {
			auto v = reinterpret_cast<const int*>(cbuffer()) + component;
			for(size_t i = 0; i < s; i++, v += cmpntCount)
				f(i, *v);
			return true;
		}
		else if(dataType() == PropertyStorage::Int64) {
			auto v = reinterpret_cast<const qlonglong*>(cbuffer()) + component;
			for(size_t i = 0; i < s; i++, v += cmpntCount)
				f(i, *v);
			return true;
		}
		else if(dataType() == PropertyStorage::Float) {
			auto v = reinterpret_cast<const FloatType*>(cbuffer()) + component;
			for(size_t i = 0; i < s; i++, v += cmpntCount)
				f(i, *v);
			return true;
		}
		return false;
	}

private:

	/// Grows the storage buffer to accomodate at least the given number of data elements.
	void growCapacity(size_t newSize);

	/// The type of this property.
	int _type = 0;

	/// The name of the property.
	QString _name;

	/// The data type of the property (a Qt metadata type identifier).
	int _dataType = QMetaType::Void;

	/// The number of bytes per data type value.
	size_t _dataTypeSize = 0;

	/// The number of elements in the property storage.
	size_t _numElements = 0;

	/// The capacity of the allocated buffer.
	size_t _capacity = 0;

	/// The number of bytes per element.
	size_t _stride = 0;

	/// The number of vector components per element.
	size_t _componentCount = 0;

	/// The names of the vector components if this property consists of more than one value per element.
	QStringList _componentNames;

	/// The internal memory buffer holding the data elements.
	std::unique_ptr<uint8_t[]> _data;
};

/// Typically, PropertyStorage objects are shallow copied. That's why we use a shared_ptr to hold on to them.
using PropertyPtr = std::shared_ptr<PropertyStorage>;

/// This pointer type is used to indicate that we only need read-only access to the property data.
using ConstPropertyPtr = std::shared_ptr<const PropertyStorage>;

/// Class template returning the Qt data type identifier for the components in the given C++ array structure.
template<typename T> struct PropertyStoragePrimitiveDataType { static constexpr int value = qMetaTypeId<T>(); };
#ifdef Q_CC_MSVC // MSVC compiler doesn't treat qMetaTypeId() function as constexpr. Workaround:
template<> struct PropertyStoragePrimitiveDataType<int> { static constexpr PropertyStorage::StandardDataType value = PropertyStorage::Int; };
template<> struct PropertyStoragePrimitiveDataType<qlonglong> { static constexpr PropertyStorage::StandardDataType value = PropertyStorage::Int64; };
template<> struct PropertyStoragePrimitiveDataType<FloatType> { static constexpr PropertyStorage::StandardDataType value = PropertyStorage::Float; };
#endif
template<typename T, std::size_t N> struct PropertyStoragePrimitiveDataType<std::array<T,N>> : public PropertyStoragePrimitiveDataType<T> {};
template<typename T> struct PropertyStoragePrimitiveDataType<Point_3<T>> : public PropertyStoragePrimitiveDataType<T> {};
template<typename T> struct PropertyStoragePrimitiveDataType<Vector_3<T>> : public PropertyStoragePrimitiveDataType<T> {};
template<typename T> struct PropertyStoragePrimitiveDataType<Point_2<T>> : public PropertyStoragePrimitiveDataType<T> {};
template<typename T> struct PropertyStoragePrimitiveDataType<Vector_2<T>> : public PropertyStoragePrimitiveDataType<T> {};
template<typename T> struct PropertyStoragePrimitiveDataType<Matrix_3<T>> : public PropertyStoragePrimitiveDataType<T> {};
template<typename T> struct PropertyStoragePrimitiveDataType<QuaternionT<T>> : public PropertyStoragePrimitiveDataType<T> {};
template<typename T> struct PropertyStoragePrimitiveDataType<ColorT<T>> : public PropertyStoragePrimitiveDataType<T> {};
template<typename T> struct PropertyStoragePrimitiveDataType<SymmetricTensor2T<T>> : public PropertyStoragePrimitiveDataType<T> {};

}	// End of namespace
}	// End of namespace
