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
#include "PropertyAccess.h"

#include <cstring>

namespace Ovito { namespace StdObj {

// Export a couple of commonly used class template instantiations. 
template class ConstPropertyAccess<int>;
template class ConstPropertyAccess<qlonglong>;
template class ConstPropertyAccess<FloatType>;
template class ConstPropertyAccess<Point3>;
template class ConstPropertyAccess<Vector3>;
template class ConstPropertyAccess<Color>;
template class ConstPropertyAccess<Vector3I>;
template class ConstPropertyAccess<std::array<qlonglong,2>>;
template class ConstPropertyAccess<int, true>;
template class ConstPropertyAccess<qlonglong, true>;
template class ConstPropertyAccess<FloatType, true>;
template class ConstPropertyAccess<void, true>;
template class PropertyAccess<int>;
template class PropertyAccess<qlonglong>;
template class PropertyAccess<FloatType>;
template class PropertyAccess<Point3>;
template class PropertyAccess<Vector3>;
template class PropertyAccess<Color>;
template class PropertyAccess<Vector3I>;
template class PropertyAccess<std::array<qlonglong,2>>;
template class PropertyAccess<int, true>;
template class PropertyAccess<qlonglong, true>;
template class PropertyAccess<FloatType, true>;
template class PropertyAccess<void, true>;

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
		auto src = reinterpret_cast<const FloatType*>(cbuffer());
		auto dst = reinterpret_cast<FloatType*>(buffer());
		for(size_t i = 0; i < s; ++i, ++src) {
			if(!mask.test(i)) *dst++ = *src;
		}
		resize(dst - reinterpret_cast<FloatType*>(buffer()), true);
	}
	else if(dataType() == PropertyStorage::Int && stride() == sizeof(int)) {
		// Single integer
		auto src = reinterpret_cast<const int*>(cbuffer());
		auto dst = reinterpret_cast<int*>(buffer());
		for(size_t i = 0; i < s; ++i, ++src) {
			if(!mask.test(i)) *dst++ = *src;
		}
		resize(dst - reinterpret_cast<int*>(buffer()), true);
	}
	else if(dataType() == PropertyStorage::Int64 && stride() == sizeof(qlonglong)) {
		// Single 64-bit integer
		auto src = reinterpret_cast<const qlonglong*>(cbuffer());
		auto dst = reinterpret_cast<qlonglong*>(buffer());
		for(size_t i = 0; i < s; ++i, ++src) {
			if(!mask.test(i)) *dst++ = *src;
		}
		resize(dst - reinterpret_cast<qlonglong*>(buffer()), true);
	}
	else if(dataType() == PropertyStorage::Float && stride() == sizeof(Point3)) {
		// Triple float (may actually be four floats when SSE instructions are enabled).
		auto src = reinterpret_cast<const Point3*>(cbuffer());
		auto dst = reinterpret_cast<Point3*>(buffer());
		for(size_t i = 0; i < s; ++i, ++src) {
			if(!mask.test(i)) *dst++ = *src;
		}
		resize(dst - reinterpret_cast<Point3*>(buffer()), true);
	}
	else if(dataType() == PropertyStorage::Float && stride() == sizeof(Color)) {
		// Triple float
		auto src = reinterpret_cast<const Color*>(cbuffer());
		auto dst = reinterpret_cast<Color*>(buffer());
		for(size_t i = 0; i < s; ++i, ++src) {
			if(!mask.test(i)) *dst++ = *src;
		}
		resize(dst - reinterpret_cast<Color*>(buffer()), true);
	}
	else if(dataType() == PropertyStorage::Int && stride() == sizeof(Point3I)) {
		// Triple int.
		auto src = reinterpret_cast<const Point3I*>(cbuffer());
		auto dst = reinterpret_cast<Point3I*>(buffer());
		for(size_t i = 0; i < s; ++i, ++src) {
			if(!mask.test(i)) *dst++ = *src;
		}
		resize(dst - reinterpret_cast<Point3I*>(buffer()), true);
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
		auto src = reinterpret_cast<const FloatType*>(cbuffer());
		auto dst = reinterpret_cast<FloatType*>(copy->buffer());
		for(size_t i = 0; i < s; ++i, ++src) {
			if(!mask.test(i)) *dst++ = *src;
		}
		OVITO_ASSERT(dst == reinterpret_cast<FloatType*>(copy->buffer()) + newSize);
	}
	else if(dataType() == PropertyStorage::Int && stride() == sizeof(int)) {
		// Single integer
		auto src = reinterpret_cast<const int*>(cbuffer());
		auto dst = reinterpret_cast<int*>(copy->buffer());
		for(size_t i = 0; i < s; ++i, ++src) {
			if(!mask.test(i)) *dst++ = *src;
		}
		OVITO_ASSERT(dst == reinterpret_cast<int*>(copy->buffer()) + newSize);
	}
	else if(dataType() == PropertyStorage::Int64 && stride() == sizeof(qlonglong)) {
		// Single 64-bit integer
		auto src = reinterpret_cast<const qlonglong*>(cbuffer());
		auto dst = reinterpret_cast<qlonglong*>(copy->buffer());
		for(size_t i = 0; i < s; ++i, ++src) {
			if(!mask.test(i)) *dst++ = *src;
		}
		OVITO_ASSERT(dst == reinterpret_cast<qlonglong*>(copy->buffer()) + newSize);
	}
	else if(dataType() == PropertyStorage::Float && stride() == sizeof(Point3)) {
		// Triple float (may actually be four floats when SSE instructions are enabled).
		auto src = reinterpret_cast<const Point3*>(cbuffer());
		auto dst = reinterpret_cast<Point3*>(copy->buffer());
		for(size_t i = 0; i < s; ++i, ++src) {
			if(!mask.test(i)) *dst++ = *src;
		}
		OVITO_ASSERT(dst == reinterpret_cast<Point3*>(copy->buffer()) + newSize);
	}
	else if(dataType() == PropertyStorage::Float && stride() == sizeof(Color)) {
		// Triple float
		auto src = reinterpret_cast<const Color*>(cbuffer());
		auto dst = reinterpret_cast<Color*>(copy->buffer());
		for(size_t i = 0; i < s; ++i, ++src) {
			if(!mask.test(i)) *dst++ = *src;
		}
		OVITO_ASSERT(dst == reinterpret_cast<Color*>(copy->buffer()) + newSize);
	}
	else if(dataType() == PropertyStorage::Int && stride() == sizeof(Point3I)) {
		// Triple int.
		auto src = reinterpret_cast<const Point3I*>(cbuffer());
		auto dst = reinterpret_cast<Point3I*>(copy->buffer());
		for(size_t i = 0; i < s; ++i, ++src) {
			if(!mask.test(i)) *dst++ = *src;
		}
		OVITO_ASSERT(dst == reinterpret_cast<Point3I*>(copy->buffer()) + newSize);
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
void PropertyStorage::mappedCopyFrom(const PropertyStorage& source, const std::vector<size_t>& mapping)
{
	OVITO_ASSERT(source.size() == mapping.size());
	OVITO_ASSERT(stride() == source.stride());
	OVITO_ASSERT(&source != this);

	// Optimize copying operation for the most common property types.
	if(stride() == sizeof(FloatType)) {
		// Single float
		auto src = reinterpret_cast<const FloatType*>(source.cbuffer());
		auto dst = reinterpret_cast<FloatType*>(buffer());
		for(size_t idx : mapping) {
			OVITO_ASSERT(idx < this->size());
			dst[idx] = *src++;
		}
	}
	else if(stride() == sizeof(int)) {
		// Single integer
		auto src = reinterpret_cast<const int*>(source.cbuffer());
		auto dst = reinterpret_cast<int*>(buffer());
		for(size_t idx : mapping) {
			OVITO_ASSERT(idx < this->size());
			dst[idx] = *src++;
		}
	}
	else if(stride() == sizeof(qlonglong)) {
		// Single 64-bit integer
		auto src = reinterpret_cast<const qlonglong*>(source.cbuffer());
		auto dst = reinterpret_cast<qlonglong*>(buffer());
		for(size_t idx : mapping) {
			OVITO_ASSERT(idx < this->size());
			dst[idx] = *src++;
		}
	}
	else if(stride() == sizeof(Point3)) {
		// Triple float (may actually be four floats when SSE instructions are enabled).
		auto src = reinterpret_cast<const Point3*>(source.cbuffer());
		auto dst = reinterpret_cast<Point3*>(buffer());
		for(size_t idx : mapping) {
			OVITO_ASSERT(idx < this->size());
			dst[idx] = *src++;
		}
	}
	else if(stride() == sizeof(Color)) {
		// Triple float
		auto src = reinterpret_cast<const Color*>(source.cbuffer());
		auto dst = reinterpret_cast<Color*>(buffer());
		for(size_t idx : mapping) {
			OVITO_ASSERT(idx < this->size());
			dst[idx] = *src++;
		}
	}
	else if(stride() == sizeof(Point3I)) {
		// Triple int
		auto src = reinterpret_cast<const Point3I*>(source.cbuffer());
		auto dst = reinterpret_cast<Point3I*>(buffer());
		for(size_t idx : mapping) {
			OVITO_ASSERT(idx < this->size());
			dst[idx] = *src++;
		}
	}
	else {
		// General case:
		const uint8_t* src = source.cbuffer();
		uint8_t* dst = buffer();
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
	OVITO_ASSERT(&destination != this);

	// Optimize copying operation for the most common property types.
	if(stride() == sizeof(FloatType)) {
		// Single float
		auto src = reinterpret_cast<const FloatType*>(cbuffer());
		auto dst = reinterpret_cast<FloatType*>(destination.buffer());
		for(size_t idx : mapping) {
			OVITO_ASSERT(idx < size());
			*dst++ = src[idx];
		}
	}
	else if(stride() == sizeof(int)) {
		// Single integer
		auto src = reinterpret_cast<const int*>(cbuffer());
		auto dst = reinterpret_cast<int*>(destination.buffer());
		for(size_t idx : mapping) {
			OVITO_ASSERT(idx < size());
			*dst++ = src[idx];
		}
	}
	else if(stride() == sizeof(qlonglong)) {
		// Single 64-bit integer
		auto src = reinterpret_cast<const qlonglong*>(cbuffer());
		auto dst = reinterpret_cast<qlonglong*>(destination.buffer());
		for(size_t idx : mapping) {
			OVITO_ASSERT(idx < size());
			*dst++ = src[idx];
		}
	}
	else if(stride() == sizeof(Point3)) {
		// Triple float (may actually be four floats when SSE instructions are enabled).
		auto src = reinterpret_cast<const Point3*>(cbuffer());
		auto dst = reinterpret_cast<Point3*>(destination.buffer());
		for(size_t idx : mapping) {
			OVITO_ASSERT(idx < size());
			*dst++ = src[idx];
		}
	}
	else if(stride() == sizeof(Color)) {
		// Triple float
		auto src = reinterpret_cast<const Color*>(cbuffer());
		auto dst = reinterpret_cast<Color*>(destination.buffer());
		for(size_t idx : mapping) {
			OVITO_ASSERT(idx < size());
			*dst++ = src[idx];
		}
	}
	else if(stride() == sizeof(Point3I)) {
		// Triple int
		auto src = reinterpret_cast<const Point3I*>(cbuffer());
		auto dst = reinterpret_cast<Point3I*>(destination.buffer());
		for(size_t idx : mapping) {
			OVITO_ASSERT(idx < size());
			*dst++ = src[idx];
		}
	}
	else {
		// General case:
		const uint8_t* src = cbuffer();
		uint8_t* dst = destination.buffer();
		size_t stride = this->stride();
		for(size_t idx : mapping) {
			OVITO_ASSERT(idx < size());
			std::memcpy(dst, src + stride * idx, stride);
			dst += stride;
		}
	}
}

/******************************************************************************
* Copies the data elements from the given source array into this array. 
* Array size, component count and data type of source and destination must match exactly.
******************************************************************************/
void PropertyStorage::copyFrom(const PropertyStorage& source)
{
	OVITO_ASSERT(this->dataType() == source.dataType());
	OVITO_ASSERT(this->stride() == source.stride());
	OVITO_ASSERT(this->size() == source.size());
	if(&source != this)
		std::memcpy(buffer(), source.cbuffer(), this->stride() * this->size());
}

/******************************************************************************
* Copies a range of data elements from the given source array into this array. 
* Component count and data type of source and destination must be compatible.
******************************************************************************/
void PropertyStorage::copyRangeFrom(const PropertyStorage& source, size_t sourceIndex, size_t destIndex, size_t count)
{
	OVITO_ASSERT(this->dataType() == source.dataType());
	OVITO_ASSERT(this->stride() == source.stride());
	OVITO_ASSERT(sourceIndex + count <= source.size());
	OVITO_ASSERT(destIndex + count <= this->size());
	std::memcpy(buffer() + destIndex * this->stride(), source.cbuffer() + sourceIndex * source.stride(), this->stride() * count);
}

}	// End of namespace
}	// End of namespace
