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

#pragma once


#include <plugins/stdobj/StdObj.h>
#include <boost/range/iterator_range_core.hpp>


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

	/// \brief Returns a read-only pointer to the raw elements stored in this property object.
	const void* constData() const {
		return _data.get();
	}

	/// Returns a read-only pointer to the i-th element in the property storage.
	const void* constDataAt(size_t i) const {
		OVITO_ASSERT(i < size());
		return reinterpret_cast<const char*>(constData()) + (i * stride());
	}

	/// \brief Returns a read-only pointer to the first element stored in this object.
	template<typename T>
	const T* constDataGeneric() const {
		OVITO_ASSERT(dataType() == qMetaTypeId<T>());
		return reinterpret_cast<const T*>(constData());
	}

	/// \brief Returns a read-only pointer to the first integer element stored in this object.
	/// \note This method may only be used if this property is of data type int32.
	const int* constDataInt() const {
		OVITO_ASSERT(dataType() == Int);
		return reinterpret_cast<const int*>(constData());
	}

	/// \brief Returns a read-only pointer to the first integer element stored in this object.
	/// \note This method may only be used if this property is of data type integer.
	const qlonglong* constDataInt64() const {
		OVITO_ASSERT(dataType() == Int64);
		return reinterpret_cast<const qlonglong*>(constData());
	}

	/// \brief Returns a read-only pointer to the first float element in the property storage.
	/// \note This method may only be used if this property is of data type float.
	const FloatType* constDataFloat() const {
		OVITO_ASSERT(dataType() == Float);
		return reinterpret_cast<const FloatType*>(constData());
	}

	/// \brief Returns a read-only pointer to the first vector element in the property storage.
	/// \note This method may only be used if this property is of data type Vector3 or a FloatType channel with 3 components.
	const Vector3* constDataVector3() const {
		OVITO_ASSERT(dataType() == qMetaTypeId<Vector3>() || (dataType() == Float && componentCount() == 3));
		return reinterpret_cast<const Vector3*>(constData());
	}

	/// \brief Returns a read-only pointer to the first point element in the property storage.
	/// \note This method may only be used if this property is of data type Point3 or a FloatType channel with 3 components.
	const Point3* constDataPoint3() const {
		OVITO_ASSERT(dataType() == qMetaTypeId<Point3>() || (dataType() == Float && componentCount() == 3));
		return reinterpret_cast<const Point3*>(constData());
	}

	/// \brief Returns a read-only pointer to the first point element in the property storage.
	/// \note This method may only be used if this property is of data type Point2 or a FloatType channel with 2 components.
	const Point2* constDataPoint2() const {
		OVITO_ASSERT(dataType() == qMetaTypeId<Point2>() || (dataType() == Float && componentCount() == 2));
		return reinterpret_cast<const Point2*>(constData());
	}

	/// \brief Returns a read-only pointer to the first point element in the property storage.
	/// \note This method may only be used if this property is of data type Vector3I or an integer channel with 3 components.
	const Vector3I* constDataVector3I() const {
		OVITO_ASSERT(dataType() == qMetaTypeId<Vector3I>() || (dataType() == Int && componentCount() == 3));
		OVITO_STATIC_ASSERT(sizeof(Vector3I) == sizeof(int) * 3);
		return reinterpret_cast<const Vector3I*>(constData());
	}

	/// \brief Returns a read-only pointer to the first point element in the property storage.
	/// \note This method may only be used if this property is of data type Point3I or an integer channel with 3 components.
	const Point3I* constDataPoint3I() const {
		OVITO_ASSERT(dataType() == qMetaTypeId<Point3I>() || (dataType() == Int && componentCount() == 3));
		OVITO_STATIC_ASSERT(sizeof(Point3I) == sizeof(int) * 3);
		return reinterpret_cast<const Point3I*>(constData());
	}

	/// \brief Returns a read-only pointer to the first point element in the property storage.
	/// \note This method may only be used if this property is of data type Color or a FloatType channel with 3 components.
	const Color* constDataColor() const {
		OVITO_ASSERT(dataType() == qMetaTypeId<Color>() || (dataType() == Float && componentCount() == 3));
		return reinterpret_cast<const Color*>(constData());
	}

	/// \brief Returns a read-only pointer to the first symmetric tensor element in the property storage.
	/// \note This method may only be used if this property is of data type SymmetricTensor2 or a FloatType channel with 6 components.
	const SymmetricTensor2* constDataSymmetricTensor2() const {
		OVITO_ASSERT(dataType() == qMetaTypeId<SymmetricTensor2>() || (dataType() == Float && componentCount() == 6));
		return reinterpret_cast<const SymmetricTensor2*>(constData());
	}

	/// \brief Returns a read-only pointer to the first Matrix3 element in the property storage.
	/// \note This method may only be used if this property is of data type Matrix3 or a FloatType channel with 9 components.
	const Matrix3* constDataMatrix3() const {
		OVITO_ASSERT(dataType() == qMetaTypeId<Matrix3>() || (dataType() == Float && componentCount() == 9));
		return reinterpret_cast<const Matrix3*>(constData());
	}

	/// \brief Returns a read-only pointer to the first quaternion element in the property storage.
	/// \note This method may only be used if this property is of data type Quaternion or a FloatType channel with 4 components.
	const Quaternion* constDataQuaternion() const {
		OVITO_ASSERT(dataType() == qMetaTypeId<Quaternion>() || (dataType() == Float && componentCount() == 4));
		return reinterpret_cast<const Quaternion*>(constData());
	}

	/// \brief Returns a range of const iterators over the elements stored in this object.
	boost::iterator_range<const int*> constIntRange() const {
		OVITO_ASSERT(componentCount() == 1);
		return boost::make_iterator_range(constDataInt(), constDataInt() + size());
	}

	/// \brief Returns a range of const iterators over the elements stored in this object.
	boost::iterator_range<const qlonglong*> constInt64Range() const {
		OVITO_ASSERT(componentCount() == 1);
		return boost::make_iterator_range(constDataInt64(), constDataInt64() + size());
	}

	/// \brief Returns a range of const iterators over the elements stored in this object.
	boost::iterator_range<const FloatType*> constFloatRange() const {
		OVITO_ASSERT(componentCount() == 1);
		return boost::make_iterator_range(constDataFloat(), constDataFloat() + size());
	}

	/// \brief Returns a range of const iterators over the elements stored in this object.
	boost::iterator_range<const Point3*> constPoint3Range() const {
		return boost::make_iterator_range(constDataPoint3(), constDataPoint3() + size());
	}

	/// \brief Returns a range of const iterators over the elements stored in this object.
	boost::iterator_range<const Point2*> constPoint2Range() const {
		return boost::make_iterator_range(constDataPoint2(), constDataPoint2() + size());
	}

	/// \brief Returns a range of const iterators over the elements stored in this object.
	boost::iterator_range<const Vector3*> constVector3Range() const {
		return boost::make_iterator_range(constDataVector3(), constDataVector3() + size());
	}

	/// \brief Returns a range of const iterators over the elements stored in this object.
	boost::iterator_range<const Color*> constColorRange() const {
		return boost::make_iterator_range(constDataColor(), constDataColor() + size());
	}

	/// \brief Returns a range of const iterators over the elements stored in this object.
	boost::iterator_range<const Vector3I*> constVector3IRange() const {
		return boost::make_iterator_range(constDataVector3I(), constDataVector3I() + size());
	}

	/// \brief Returns a range of const iterators over the elements stored in this object.
	boost::iterator_range<const Point3I*> constPoint3IRange() const {
		return boost::make_iterator_range(constDataPoint3I(), constDataPoint3I() + size());
	}

	/// \brief Returns a range of const iterators over the elements stored in this object.
	boost::iterator_range<const SymmetricTensor2*> constSymmetricTensor2Range() const {
		return boost::make_iterator_range(constDataSymmetricTensor2(), constDataSymmetricTensor2() + size());
	}

	/// \brief Returns a range of const iterators over the elements stored in this object.
	boost::iterator_range<const Matrix3*> constMatrix3Range() const {
		return boost::make_iterator_range(constDataMatrix3(), constDataMatrix3() + size());
	}

	/// \brief Returns a range of const iterators over the elements stored in this object.
	boost::iterator_range<const Quaternion*> constQuaternionRange() const {
		return boost::make_iterator_range(constDataQuaternion(), constDataQuaternion() + size());
	}

	/// Returns a read-write pointer to the raw elements in the property storage.
	void* data() {
		return _data.get();
	}

	/// Returns a read-write pointer to the i-th element in the property storage.
	void* dataAt(size_t i) {
		OVITO_ASSERT(i < size());
		return reinterpret_cast<char*>(data()) + (i * stride());
	}

	/// \brief Returns a read-write pointer to the first element stored in this object.
	template<typename T>
	T* dataGeneric() {
		OVITO_ASSERT(dataType() == qMetaTypeId<T>());
		return reinterpret_cast<T*>(data());
	}

	/// \brief Returns a read-write pointer to the first integer element stored in this object.
	/// \note This method may only be used if this property is of data type integer.
	int* dataInt() {
		OVITO_ASSERT(dataType() == Int);
		return reinterpret_cast<int*>(data());
	}

	/// \brief Returns a read-write pointer to the first integer element stored in this object.
	/// \note This method may only be used if this property is of data type 64-bit integer.
	qlonglong* dataInt64() {
		OVITO_ASSERT(dataType() == Int64);
		return reinterpret_cast<qlonglong*>(data());
	}

	/// \brief Returns a read-only pointer to the first float element in the property storage.
	/// \note This method may only be used if this property is of data type float.
	FloatType* dataFloat() {
		OVITO_ASSERT(dataType() == Float);
		return reinterpret_cast<FloatType*>(data());
	}

	/// \brief Returns a read-write pointer to the first vector element in the property storage.
	/// \note This method may only be used if this property is of data type Vector3 or a FloatType channel with 3 components.
	Vector3* dataVector3() {
		OVITO_ASSERT(dataType() == qMetaTypeId<Vector3>() || (dataType() == Float && componentCount() == 3));
		return reinterpret_cast<Vector3*>(data());
	}

	/// \brief Returns a read-write pointer to the first point element in the property storage.
	/// \note This method may only be used if this property is of data type Vector3I or an integer channel with 3 components.
	Vector3I* dataVector3I() {
		OVITO_ASSERT(dataType() == qMetaTypeId<Vector3I>() || (dataType() == Int && componentCount() == 3));
		OVITO_STATIC_ASSERT(sizeof(Vector3I) == sizeof(int) * 3);
		return reinterpret_cast<Vector3I*>(data());
	}

	/// \brief Returns a read-write pointer to the first point element in the property storage.
	/// \note This method may only be used if this property is of data type Point3 or a FloatType channel with 3 components.
	Point3* dataPoint3() {
		OVITO_ASSERT(dataType() == qMetaTypeId<Point3>() || (dataType() == Float && componentCount() == 3));
		return reinterpret_cast<Point3*>(data());
	}

	/// \brief Returns a read-write pointer to the first point element in the property storage.
	/// \note This method may only be used if this property is of data type Point2 or a FloatType channel with 2 components.
	Point2* dataPoint2() {
		OVITO_ASSERT(dataType() == qMetaTypeId<Point2>() || (dataType() == Float && componentCount() == 2));
		return reinterpret_cast<Point2*>(data());
	}

	/// \brief Returns a read-write pointer to the first point element in the property storage.
	/// \note This method may only be used if this property is of data type Point3I or an integer channel with 3 components.
	Point3I* dataPoint3I() {
		OVITO_ASSERT(dataType() == qMetaTypeId<Point3I>() || (dataType() == Int && componentCount() == 3));
		OVITO_STATIC_ASSERT(sizeof(Point3I) == sizeof(int) * 3);
		return reinterpret_cast<Point3I*>(data());
	}

	/// \brief Returns a read-write pointer to the first point element in the property storage.
	/// \note This method may only be used if this property is of data type Color or a FloatType channel with 3 components.
	Color* dataColor() {
		OVITO_ASSERT(dataType() == qMetaTypeId<Color>() || (dataType() == Float && componentCount() == 3));
		return reinterpret_cast<Color*>(data());
	}

	/// \brief Returns a read-write pointer to the first symmetric tensor element in the property storage.
	/// \note This method may only be used if this property is of data type SymmetricTensor2 or a FloatType channel with 6 components.
	SymmetricTensor2* dataSymmetricTensor2() {
		OVITO_ASSERT(dataType() == qMetaTypeId<SymmetricTensor2>() || (dataType() == Float && componentCount() == 6));
		return reinterpret_cast<SymmetricTensor2*>(data());
	}

	/// \brief Returns a read-write pointer to the first Matrix3 element in the property storage.
	/// \note This method may only be used if this property is of data type Matrix3 or a FloatType channel with 9 components.
	Matrix3* dataMatrix3() {
		OVITO_ASSERT(dataType() == qMetaTypeId<Matrix3>() || (dataType() == Float && componentCount() == 9));
		return reinterpret_cast<Matrix3*>(data());
	}

	/// \brief Returns a read-write pointer to the first quaternion element in the property storage.
	/// \note This method may only be used if this property is of data type Quaternion or a FloatType channel with 4 components.
	Quaternion* dataQuaternion() {
		OVITO_ASSERT(dataType() == qMetaTypeId<Quaternion>() || (dataType() == Float && componentCount() == 4));
		return reinterpret_cast<Quaternion*>(data());
	}

	/// \brief Returns a range of iterators over the elements stored in this object.
	boost::iterator_range<int*> intRange() {
		OVITO_ASSERT(componentCount() == 1);
		return boost::make_iterator_range(dataInt(), dataInt() + size());
	}

	/// \brief Returns a range of iterators over the elements stored in this object.
	boost::iterator_range<qlonglong*> int64Range() {
		OVITO_ASSERT(componentCount() == 1);
		return boost::make_iterator_range(dataInt64(), dataInt64() + size());
	}

	/// \brief Returns a range of iterators over the elements stored in this object.
	boost::iterator_range<FloatType*> floatRange() {
		OVITO_ASSERT(componentCount() == 1);
		return boost::make_iterator_range(dataFloat(), dataFloat() + size());
	}

	/// \brief Returns a range of iterators over the elements stored in this object.
	boost::iterator_range<Point3*> point3Range() {
		return boost::make_iterator_range(dataPoint3(), dataPoint3() + size());
	}

	/// \brief Returns a range of iterators over the elements stored in this object.
	boost::iterator_range<Point2*> point2Range() {
		return boost::make_iterator_range(dataPoint2(), dataPoint2() + size());
	}

	/// \brief Returns a range of iterators over the elements stored in this object.
	boost::iterator_range<Vector3*> vector3Range() {
		return boost::make_iterator_range(dataVector3(), dataVector3() + size());
	}

	/// \brief Returns a range of const iterators over the elements stored in this object.
	boost::iterator_range<Color*> colorRange() {
		return boost::make_iterator_range(dataColor(), dataColor() + size());
	}

	/// \brief Returns a range of iterators over the elements stored in this object.
	boost::iterator_range<Vector3I*> vector3IRange() {
		return boost::make_iterator_range(dataVector3I(), dataVector3I() + size());
	}

	/// \brief Returns a range of iterators over the elements stored in this object.
	boost::iterator_range<Point3I*> point3IRange() {
		return boost::make_iterator_range(dataPoint3I(), dataPoint3I() + size());
	}

	/// \brief Returns a range of iterators over the elements stored in this object.
	boost::iterator_range<SymmetricTensor2*> symmetricTensor2Range() {
		return boost::make_iterator_range(dataSymmetricTensor2(), dataSymmetricTensor2() + size());
	}

	/// \brief Returns a range of iterators over the elements stored in this object.
	boost::iterator_range<Matrix3*> matrix3Range() {
		return boost::make_iterator_range(dataMatrix3(), dataMatrix3() + size());
	}

	/// \brief Returns a range of iterators over the elements stored in this object.
	boost::iterator_range<Quaternion*> quaternionRange() {
		return boost::make_iterator_range(dataQuaternion(), dataQuaternion() + size());
	}

	/// \brief Returns an element at the given index.
	template<typename T>
	const T& getValue(size_t index) const {
		OVITO_ASSERT(index < size());
		OVITO_ASSERT(componentCount() == 1);
		OVITO_ASSERT(dataTypeSize() == sizeof(T));
		OVITO_ASSERT(stride() == sizeof(T));
		return constDataGeneric<T>()[index];
	}

	/// \brief Returns an integer element at the given index (if this is an integer property).
	int getInt(size_t index) const {
		OVITO_ASSERT(index < size() && componentCount() == 1);
		return constDataInt()[index];
	}

	/// \brief Returns an integer element at the given index (if this is a 64-bit integer property).
	qlonglong getInt64(size_t index) const {
		OVITO_ASSERT(index < size() && componentCount() == 1);
		return constDataInt64()[index];
	}

	/// Returns a float element at the given index (if this is a float property).
	FloatType getFloat(size_t index) const {
		OVITO_ASSERT(index < size() && componentCount() == 1);
		return constDataFloat()[index];
	}

	/// Returns an integer element at the given index (if this is an integer property).
	int getIntComponent(size_t index, size_t componentIndex) const {
		OVITO_ASSERT(index < size() && componentIndex < componentCount());
		return constDataInt()[index*componentCount() + componentIndex];
	}

	/// Returns an integer element at the given index (if this is a 64-bit integer property).
	qlonglong getInt64Component(size_t index, size_t componentIndex) const {
		OVITO_ASSERT(index < size() && componentIndex < componentCount());
		return constDataInt64()[index*componentCount() + componentIndex];
	}

	/// Returns a float element at the given index (if this is a float property).
	FloatType getFloatComponent(size_t index, size_t componentIndex) const {
		OVITO_ASSERT(index < size() && componentIndex < componentCount());
		return constDataFloat()[index*componentCount() + componentIndex];
	}

	/// Returns a Vector3 element at the given index (if this is a vector property).
	const Vector3& getVector3(size_t index) const {
		OVITO_ASSERT(index < size());
		return constDataVector3()[index];
	}

	/// Returns a Point3 element at the given index (if this is a point property).
	const Point3& getPoint3(size_t index) const {
		OVITO_ASSERT(index < size());
		return constDataPoint3()[index];
	}

	/// Returns a Point2 element at the given index (if this is a point property).
	const Point2& getPoint2(size_t index) const {
		OVITO_ASSERT(index < size());
		return constDataPoint2()[index];
	}

	/// Returns a Vector3I element at the given index (if this is a point property).
	const Vector3I& getVector3I(size_t index) const {
		OVITO_ASSERT(index < size());
		return constDataVector3I()[index];
	}

	/// Returns a Point3I element at the given index (if this is a point property).
	const Point3I& getPoint3I(size_t index) const {
		OVITO_ASSERT(index < size());
		return constDataPoint3I()[index];
	}

	/// Returns a Color element at the given index (if this is a color property).
	const Color& getColor(size_t index) const {
		OVITO_ASSERT(index < size());
		return constDataColor()[index];
	}

	/// Returns a SymmetricTensor2 element.
	const SymmetricTensor2& getSymmetricTensor2(size_t index) const {
		OVITO_ASSERT(index < size());
		return constDataSymmetricTensor2()[index];
	}

	/// Returns a Matrix3 element.
	const Matrix3& getMatrix3(size_t index) const {
		OVITO_ASSERT(index < size());
		return constDataMatrix3()[index];
	}

	/// Returns a Quaternion element.
	const Quaternion& getQuaternion(size_t index) const {
		OVITO_ASSERT(index < size());
		return constDataQuaternion()[index];
	}

	/// Sets the value of an element at the given index.
	template<typename T>
	void setValue(size_t index, const T& newValue) {
		OVITO_ASSERT(index < size());
		dataGeneric<T>()[index] = newValue;
	}

	/// Sets the value of an integer element at the given index (if this is an integer property).
	void setInt(size_t index, int newValue) {
		OVITO_ASSERT(index < size());
		dataInt()[index] = newValue;
	}

	/// Sets the value of an integer element at the given index (if this is a 64-bit integer property).
	void setInt64(size_t index, qlonglong newValue) {
		OVITO_ASSERT(index < size());
		dataInt64()[index] = newValue;
	}

	/// Sets the value of a float element at the given index (if this is a float property).
	void setFloat(size_t index, FloatType newValue) {
		OVITO_ASSERT(index < size());
		dataFloat()[index] = newValue;
	}

	/// Sets the value of an integer element at the given index (if this is an integer property).
	void setIntComponent(size_t index, size_t componentIndex, int newValue) {
		OVITO_ASSERT(index < size() && componentIndex < componentCount());
		dataInt()[index*componentCount() + componentIndex] = newValue;
	}

	/// Sets the value of an integer element at the given index (if this is a 64-bit integer property).
	void setInt64Component(size_t index, size_t componentIndex, qlonglong newValue) {
		OVITO_ASSERT(index < size() && componentIndex < componentCount());
		dataInt64()[index*componentCount() + componentIndex] = newValue;
	}

	/// Sets the value of a float element at the given index (if this is a float property).
	void setFloatComponent(size_t index, size_t componentIndex, FloatType newValue) {
		OVITO_ASSERT(index < size() && componentIndex < componentCount());
		dataFloat()[index*componentCount() + componentIndex] = newValue;
	}

	/// Sets the value of a Vector3 element at the given index (if this is a vector property).
	void setVector3(size_t index, const Vector3& newValue) {
		OVITO_ASSERT(index < size());
		dataVector3()[index] = newValue;
	}

	/// Sets the value of a Point3 element at the given index (if this is a point property).
	void setPoint3(size_t index, const Point3& newValue) {
		OVITO_ASSERT(index < size());
		dataPoint3()[index] = newValue;
	}

	/// Sets the value of a Point2 element at the given index (if this is a point property).
	void setPoint2(size_t index, const Point2& newValue) {
		OVITO_ASSERT(index < size());
		dataPoint2()[index] = newValue;
	}

	/// Sets the value of a Vector3I element at the given index (if this is an integer vector property).
	void setVector3I(size_t index, const Vector3I& newValue) {
		OVITO_ASSERT(index < size());
		dataVector3I()[index] = newValue;
	}

	/// Sets the value of a Point3I element at the given index (if this is a point property).
	void setPoint3I(size_t index, const Point3I& newValue) {
		OVITO_ASSERT(index < size());
		dataPoint3I()[index] = newValue;
	}

	/// Sets the value of a Color element at the given index (if this is a color property).
	void setColor(size_t index, const Color& newValue) {
		OVITO_ASSERT(index < size());
		dataColor()[index] = newValue;
	}

	/// Sets the value of a SymmetricTensor2 element.
	void setSymmetricTensor2(size_t particleIndex, const SymmetricTensor2& newValue) {
		OVITO_ASSERT(particleIndex < size());
		dataSymmetricTensor2()[particleIndex] = newValue;
	}

	/// Sets the value of a Matrix3 element.
	void setMatrix3(size_t index, const Matrix3& newValue) {
		OVITO_ASSERT(index < size());
		dataMatrix3()[index] = newValue;
	}

	/// Sets the value of a Quaternion element.
	void setQuaternion(size_t index, const Quaternion& newValue) {
		OVITO_ASSERT(index < size());
		dataQuaternion()[index] = newValue;
	}

	/// Reduces the size of the storage array, removing elements for which
	/// the corresponding bits in the bit array are set.
	void filterResize(const boost::dynamic_bitset<>& mask);

	/// Creates a copy of the array, not containing those elements for which
	/// the corresponding bits in the given bit array were set.
	std::shared_ptr<PropertyStorage> filterCopy(const boost::dynamic_bitset<>& mask) const;

	/// Copies the contents from the given source into this storage using a mapping.
	void mappedCopy(const PropertyStorage& source, const std::vector<size_t>& mapping);

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
		if(dataType() == PropertyStorage::Int) {
			for(auto v = constDataInt() + component, v_end = v + size()*cmpntCount; v != v_end; v += cmpntCount)
				*iter++ = *v;
			return true;
		}
		else if(dataType() == PropertyStorage::Int64) {
			for(auto v = constDataInt64() + component, v_end = v + size()*cmpntCount; v != v_end; v += cmpntCount)
				*iter++ = *v;
			return true;
		}
		else if(dataType() == PropertyStorage::Float) {
			for(auto v = constDataFloat() + component, v_end = v + size()*cmpntCount; v != v_end; v += cmpntCount)
				*iter++ = *v;
			return true;
		}
		return false;
	}

	/// Calls a functor provided by the caller for every value of the given vector component.
	template<typename F>
	bool forEach(F f, size_t component) const {
		size_t cmpntCount = componentCount();
		size_t s = size();
		if(component >= cmpntCount) return false;
		if(dataType() == PropertyStorage::Int) {
			auto v = constDataInt() + component;
			for(size_t i = 0; i < s; i++, v += cmpntCount)
				f(i, *v);
			return true;
		}
		else if(dataType() == PropertyStorage::Int64) {
			auto v = constDataInt64() + component;
			for(size_t i = 0; i < s; i++, v += cmpntCount)
				f(i, *v);
			return true;
		}
		else if(dataType() == PropertyStorage::Float) {
			auto v = constDataFloat() + component;
			for(size_t i = 0; i < s; i++, v += cmpntCount)
				f(i, *v);
			return true;
		}
		return false;
	}

protected:

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

}	// End of namespace
}	// End of namespace
