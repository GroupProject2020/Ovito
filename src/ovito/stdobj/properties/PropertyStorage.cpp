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

#include <ovito/stdobj/StdObj.h>
#include "PropertyStorage.h"

#include <cstring>

namespace Ovito { namespace StdObj {

/******************************************************************************
* Constructor.
******************************************************************************/
PropertyStorage::PropertyStorage(size_t elementCount, int dataType, size_t componentCount, size_t stride, const QString& name, bool initializeMemory, int type, QStringList componentNames) :
	_dataType(dataType),
	_dataTypeSize(QMetaType::sizeOf(dataType)),
	_stride(stride),
	_componentCount(componentCount),
	_componentNames(std::move(componentNames)),
	_name(name),
	_type(type)
{
	OVITO_ASSERT(dataType == Int || dataType == Int64 || dataType == Float);
	OVITO_ASSERT(_dataTypeSize > 0);
	OVITO_ASSERT(_componentCount > 0);
	if(_stride == 0) _stride = _dataTypeSize * _componentCount;
	OVITO_ASSERT(_stride >= _dataTypeSize * _componentCount);
	OVITO_ASSERT((_stride % _dataTypeSize) == 0);
	if(componentCount > 1) {
		for(size_t i = _componentNames.size(); i < componentCount; i++)
			_componentNames << QString::number(i + 1);
	}
	resize(elementCount, initializeMemory);
}

/******************************************************************************
* Copy constructor.
******************************************************************************/
PropertyStorage::PropertyStorage(const PropertyStorage& other) :
	_type(other._type),
	_name(other._name),
	_dataType(other._dataType),
	_dataTypeSize(other._dataTypeSize),
	_numElements(other._numElements),
	_capacity(other._numElements),
	_stride(other._stride),
	_componentCount(other._componentCount),
	_componentNames(other._componentNames),
	_data(new uint8_t[other._numElements * other._stride])
{
	memcpy(_data.get(), other._data.get(), _numElements * _stride);
}

/******************************************************************************
* Saves the class' contents to the given stream.
******************************************************************************/
void PropertyStorage::saveToStream(SaveStream& stream, bool onlyMetadata) const
{
	stream.beginChunk(0x02);
	stream << _name;
	stream << _type;
	stream << QByteArray(QMetaType::typeName(_dataType));
	stream.writeSizeT(_dataTypeSize);
	stream.writeSizeT(_stride);
	stream.writeSizeT(_componentCount);
	stream << _componentNames;
	if(onlyMetadata) {
		stream.writeSizeT(0);
	}
	else {
		stream.writeSizeT(_numElements);
		stream.write(_data.get(), _stride * _numElements);
	}
	stream.endChunk();
}

/******************************************************************************
* Loads the class' contents from the given stream.
******************************************************************************/
void PropertyStorage::loadFromStream(LoadStream& stream)
{
	stream.expectChunk(0x02);
	stream >> _name;
	stream >> _type;
	QByteArray dataTypeName;
	stream >> dataTypeName;
	_dataType = QMetaType::type(dataTypeName.constData());
	OVITO_ASSERT_MSG(_dataType != 0, "PropertyStorage::loadFromStream()", QString("The metadata type '%1' seems to be no longer defined.").arg(QString(dataTypeName)).toLocal8Bit().constData());
	OVITO_ASSERT(dataTypeName == QMetaType::typeName(_dataType));
	stream.readSizeT(_dataTypeSize);
	stream.readSizeT(_stride);
	stream.readSizeT(_componentCount);
	stream >> _componentNames;
	stream.readSizeT(_numElements);
	_capacity = _numElements;
	_data.reset(new uint8_t[_numElements * _stride]);
	stream.read(_data.get(), _stride * _numElements);
	stream.closeChunk();

	// Do floating-point precision conversion from single to double precision.
	if(_dataType == qMetaTypeId<float>() && PropertyStorage::Float == qMetaTypeId<double>()) {
		OVITO_ASSERT(sizeof(FloatType) == sizeof(double));
		OVITO_ASSERT(_dataTypeSize == sizeof(float));
		_stride *= sizeof(double) / sizeof(float);
		_dataTypeSize = sizeof(double);
		_dataType = PropertyStorage::Float;
		std::unique_ptr<uint8_t[]> newBuffer(new uint8_t[_stride * _numElements]);
		double* dst = reinterpret_cast<double*>(newBuffer.get());
		const float* src = reinterpret_cast<const float*>(_data.get());
		for(size_t c = _numElements * _componentCount; c--; )
			*dst++ = (double)*src++;
		_data.swap(newBuffer);
	}

	// Do floating-point precision conversion from double to single precision.
	if(_dataType == qMetaTypeId<double>() && PropertyStorage::Float == qMetaTypeId<float>()) {
		OVITO_ASSERT(sizeof(FloatType) == sizeof(float));
		OVITO_ASSERT(_dataTypeSize == sizeof(double));
		_stride /= sizeof(double) / sizeof(float);
		_dataTypeSize = sizeof(float);
		_dataType = PropertyStorage::Float;
		std::unique_ptr<uint8_t[]> newBuffer(new uint8_t[_stride * _numElements]);
		float* dst = reinterpret_cast<float*>(newBuffer.get());
		const double* src = reinterpret_cast<const double*>(_data.get());
		for(size_t c = _numElements * _componentCount; c--; )
			*dst++ = (float)*src++;
		_data.swap(newBuffer);
	}
}

/******************************************************************************
* Resizes the array to the given size.
******************************************************************************/
void PropertyStorage::resize(size_t newSize, bool preserveData)
{
	if(newSize > _capacity || newSize < _capacity * 3 / 4 || !_data) {
		std::unique_ptr<uint8_t[]> newBuffer(new uint8_t[newSize * _stride]);
		if(preserveData)
			std::memcpy(newBuffer.get(), _data.get(), _stride * std::min(_numElements, newSize));
		_data.swap(newBuffer);
		_capacity = newSize;
	}
	// Initialize new elements to zero.
	if(newSize > _numElements && preserveData) {
		std::memset(_data.get() + _numElements * _stride, 0, (newSize - _numElements) * _stride);
	}
	_numElements = newSize;
}

/******************************************************************************
* Grows the storage buffer to accomodate at least the given number of data elements
******************************************************************************/
void PropertyStorage::growCapacity(size_t newSize)
{
	OVITO_ASSERT(newSize >= _numElements);
	OVITO_ASSERT(newSize > _capacity);
	size_t newCapacity = (newSize < 1024)
		? std::max(newSize * 2, (size_t)256)
		: (newSize * 3 / 2);
	std::unique_ptr<uint8_t[]> newBuffer(new uint8_t[newCapacity * _stride]);
	std::memcpy(newBuffer.get(), _data.get(), _stride * _numElements);
	_data.swap(newBuffer);
	_capacity = newCapacity;
}

/******************************************************************************
* Reduces the size of the storage array, removing elements for which
* the corresponding bits in the bit array are set.
******************************************************************************/
void PropertyStorage::filterResize(const boost::dynamic_bitset<>& mask)
{
	OVITO_ASSERT(size() == mask.size());
	size_t s = size();

	// Optimize filter operation for the most common property types.
	if(dataType() == PropertyStorage::Float && stride() == sizeof(FloatType)) {
		// Single float
		auto src = cdata<FloatType>();
		auto dst = data<FloatType>();
		for(size_t i = 0; i < s; ++i, ++src) {
			if(!mask.test(i)) *dst++ = *src;
		}
		resize(dst - data<FloatType>(), true);
	}
	else if(dataType() == PropertyStorage::Int && stride() == sizeof(int)) {
		// Single integer
		auto src = cdata<int>();
		auto dst = data<int>();
		for(size_t i = 0; i < s; ++i, ++src) {
			if(!mask.test(i)) *dst++ = *src;
		}
		resize(dst - data<int>(), true);
	}
	else if(dataType() == PropertyStorage::Int64 && stride() == sizeof(qlonglong)) {
		// Single 64-bit integer
		auto src = cdata<qlonglong>();
		auto dst = data<qlonglong>();
		for(size_t i = 0; i < s; ++i, ++src) {
			if(!mask.test(i)) *dst++ = *src;
		}
		resize(dst - data<qlonglong>(), true);
	}
	else if(dataType() == PropertyStorage::Float && stride() == sizeof(Point3)) {
		// Triple float (may actually be four floats when SSE instructions are enabled).
		auto src = cdata<Point3>();
		auto dst = data<Point3>();
		for(size_t i = 0; i < s; ++i, ++src) {
			if(!mask.test(i)) *dst++ = *src;
		}
		resize(dst - data<Point3>(), true);
	}
	else if(dataType() == PropertyStorage::Float && stride() == sizeof(Color)) {
		// Triple float
		auto src = cdata<Color>();
		auto dst = data<Color>();
		for(size_t i = 0; i < s; ++i, ++src) {
			if(!mask.test(i)) *dst++ = *src;
		}
		resize(dst - data<Color>(), true);
	}
	else if(dataType() == PropertyStorage::Int && stride() == sizeof(Point3I)) {
		// Triple int.
		auto src = cdata<Point3I>();
		auto dst = data<Point3I>();
		for(size_t i = 0; i < s; ++i, ++src) {
			if(!mask.test(i)) *dst++ = *src;
		}
		resize(dst - data<Point3I>(), true);
	}
	else {
		// Generic case:
		const uint8_t* src = _data.get();
		uint8_t* dst = _data.get();
		size_t stride = this->stride();
		for(size_t i = 0; i < s; i++, src += stride) {
			if(!mask.test(i)) {
				std::memcpy(dst, src, stride);
				dst += stride;
			}
		}
		resize((dst - _data.get()) / stride, true);
	}
}

/******************************************************************************
* Creates a copy of the array, not containing those elements for which
* the corresponding bits in the given bit array were set.
******************************************************************************/
std::shared_ptr<PropertyStorage> PropertyStorage::filterCopy(const boost::dynamic_bitset<>& mask) const
{
	OVITO_ASSERT(size() == mask.size());

	size_t s = size();
	size_t newSize = size() - mask.count();
	std::shared_ptr<PropertyStorage> copy = std::make_shared<PropertyStorage>(newSize, dataType(), componentCount(), stride(), name(), false, type(), componentNames());

	// Optimize filter operation for the most common property types.
	if(dataType() == PropertyStorage::Float && stride() == sizeof(FloatType)) {
		// Single float
		auto src = cdata<FloatType>();
		auto dst = copy->data<FloatType>();
		for(size_t i = 0; i < s; ++i, ++src) {
			if(!mask.test(i)) *dst++ = *src;
		}
		OVITO_ASSERT(dst == copy->data<FloatType>() + newSize);
	}
	else if(dataType() == PropertyStorage::Int && stride() == sizeof(int)) {
		// Single integer
		auto src = cdata<int>();
		auto dst = copy->data<int>();
		for(size_t i = 0; i < s; ++i, ++src) {
			if(!mask.test(i)) *dst++ = *src;
		}
		OVITO_ASSERT(dst == copy->data<int>() + newSize);
	}
	else if(dataType() == PropertyStorage::Int64 && stride() == sizeof(qlonglong)) {
		// Single 64-bit integer
		auto src = cdata<qlonglong>();
		auto dst = copy->data<qlonglong>();
		for(size_t i = 0; i < s; ++i, ++src) {
			if(!mask.test(i)) *dst++ = *src;
		}
		OVITO_ASSERT(dst == copy->data<qlonglong>() + newSize);
	}
	else if(dataType() == PropertyStorage::Float && stride() == sizeof(Point3)) {
		// Triple float (may actually be four floats when SSE instructions are enabled).
		auto src = cdata<Point3>();
		auto dst = copy->data<Point3>();
		for(size_t i = 0; i < s; ++i, ++src) {
			if(!mask.test(i)) *dst++ = *src;
		}
		OVITO_ASSERT(dst == copy->data<Point3>() + newSize);
	}
	else if(dataType() == PropertyStorage::Float && stride() == sizeof(Color)) {
		// Triple float
		auto src = cdata<Color>();
		auto dst = copy->data<Color>();
		for(size_t i = 0; i < s; ++i, ++src) {
			if(!mask.test(i)) *dst++ = *src;
		}
		OVITO_ASSERT(dst == copy->data<Color>() + newSize);
	}
	else if(dataType() == PropertyStorage::Int && stride() == sizeof(Point3I)) {
		// Triple int.
		auto src = cdata<Point3I>();
		auto dst = copy->data<Point3I>();
		for(size_t i = 0; i < s; ++i, ++src) {
			if(!mask.test(i)) *dst++ = *src;
		}
		OVITO_ASSERT(dst == copy->data<Point3I>() + newSize);
	}
	else {
		// Generic case:
		const uint8_t* src = _data.get();
		uint8_t* dst = copy->_data.get();
		size_t stride = this->stride();
		for(size_t i = 0; i < s; i++, src += stride) {
			if(!mask.test(i)) {
				std::memcpy(dst, src, stride);
				dst += stride;
			}
		}
		OVITO_ASSERT(dst == copy->_data.get() + newSize * stride);
	}
	return copy;
}

/******************************************************************************
* Copies the contents from the given source into this property storage using
* a mapping of indices.
******************************************************************************/
void PropertyStorage::mappedCopy(const PropertyStorage& source, const std::vector<size_t>& mapping)
{
	OVITO_ASSERT(source.size() == mapping.size());
	OVITO_ASSERT(stride() == source.stride());

	// Optimize copying operation for the most common property types.
	if(stride() == sizeof(FloatType)) {
		// Single float
		auto src = reinterpret_cast<const FloatType*>(source.cdata<void>());
		auto dst = reinterpret_cast<FloatType*>(data<void>());
		for(size_t idx : mapping) {
			OVITO_ASSERT(idx < this->size());
			dst[idx] = *src++;
		}
	}
	else if(stride() == sizeof(int)) {
		// Single integer
		auto src = reinterpret_cast<const int*>(source.cdata<void>());
		auto dst = reinterpret_cast<int*>(data<void>());
		for(size_t idx : mapping) {
			OVITO_ASSERT(idx < this->size());
			dst[idx] = *src++;
		}
	}
	else if(stride() == sizeof(qlonglong)) {
		// Single 64-bit integer
		auto src = reinterpret_cast<const qlonglong*>(source.cdata<void>());
		auto dst = reinterpret_cast<qlonglong*>(data<void>());
		for(size_t idx : mapping) {
			OVITO_ASSERT(idx < this->size());
			dst[idx] = *src++;
		}
	}
	else if(stride() == sizeof(Point3)) {
		// Triple float (may actually be four floats when SSE instructions are enabled).
		auto src = reinterpret_cast<const Point3*>(source.cdata<void>());
		auto dst = reinterpret_cast<Point3*>(data<void>());
		for(size_t idx : mapping) {
			OVITO_ASSERT(idx < this->size());
			dst[idx] = *src++;
		}
	}
	else if(stride() == sizeof(Color)) {
		// Triple float
		auto src = reinterpret_cast<const Color*>(source.cdata<void>());
		auto dst = reinterpret_cast<Color*>(data<void>());
		for(size_t idx : mapping) {
			OVITO_ASSERT(idx < this->size());
			dst[idx] = *src++;
		}
	}
	else if(stride() == sizeof(Point3I)) {
		// Triple int
		auto src = reinterpret_cast<const Point3I*>(source.cdata<void>());
		auto dst = reinterpret_cast<Point3I*>(data<void>());
		for(size_t idx : mapping) {
			OVITO_ASSERT(idx < this->size());
			dst[idx] = *src++;
		}
	}
	else {
		// General case:
		const uint8_t* src = source._data.get();
		uint8_t* dst = _data.get();
		size_t stride = this->stride();
		for(size_t i = 0; i < source.size(); i++, src += stride) {
			OVITO_ASSERT(mapping[i] < this->size());
			std::memcpy(dst + stride * mapping[i], src, stride);
		}
	}
}

/******************************************************************************
* Copies the elements from this storage array into the given destination array 
* using an index mapping.
******************************************************************************/
void PropertyStorage::mappedCopyTo(PropertyStorage& destination, const std::vector<size_t>& mapping) const
{
	OVITO_ASSERT(destination.size() == mapping.size());
	OVITO_ASSERT(this->stride() == destination.stride());

	// Optimize copying operation for the most common property types.
	if(stride() == sizeof(FloatType)) {
		// Single float
		auto src = reinterpret_cast<const FloatType*>(cdata<void>());
		auto dst = reinterpret_cast<FloatType*>(destination.data<void>());
		for(size_t idx : mapping) {
			OVITO_ASSERT(idx < size());
			*dst++ = src[idx];
		}
	}
	else if(stride() == sizeof(int)) {
		// Single integer
		auto src = reinterpret_cast<const int*>(cdata<void>());
		auto dst = reinterpret_cast<int*>(destination.data<void>());
		for(size_t idx : mapping) {
			OVITO_ASSERT(idx < size());
			*dst++ = src[idx];
		}
	}
	else if(stride() == sizeof(qlonglong)) {
		// Single 64-bit integer
		auto src = reinterpret_cast<const qlonglong*>(cdata<void>());
		auto dst = reinterpret_cast<qlonglong*>(destination.data<void>());
		for(size_t idx : mapping) {
			OVITO_ASSERT(idx < size());
			*dst++ = src[idx];
		}
	}
	else if(stride() == sizeof(Point3)) {
		// Triple float (may actually be four floats when SSE instructions are enabled).
		auto src = reinterpret_cast<const Point3*>(cdata<void>());
		auto dst = reinterpret_cast<Point3*>(destination.data<void>());
		for(size_t idx : mapping) {
			OVITO_ASSERT(idx < size());
			*dst++ = src[idx];
		}
	}
	else if(stride() == sizeof(Color)) {
		// Triple float
		auto src = reinterpret_cast<const Color*>(cdata<void>());
		auto dst = reinterpret_cast<Color*>(destination.data<void>());
		for(size_t idx : mapping) {
			OVITO_ASSERT(idx < size());
			*dst++ = src[idx];
		}
	}
	else if(stride() == sizeof(Point3I)) {
		// Triple int
		auto src = reinterpret_cast<const Point3I*>(cdata<void>());
		auto dst = reinterpret_cast<Point3I*>(destination.data<void>());
		for(size_t idx : mapping) {
			OVITO_ASSERT(idx < size());
			*dst++ = src[idx];
		}
	}
	else {
		// General case:
		const uint8_t* src = _data.get();
		uint8_t* dst = destination._data.get();
		size_t stride = this->stride();
		for(size_t idx : mapping) {
			OVITO_ASSERT(idx < size());
			std::memcpy(dst, src + stride * idx, stride);
			dst += stride;
		}
	}
}

}	// End of namespace
}	// End of namespace
