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

#include <plugins/stdobj/StdObj.h>
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
	_stride(other._stride), 
	_componentCount(other._componentCount),
	_componentNames(other._componentNames),
	_data(new uint8_t[_numElements * _stride])
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
	std::unique_ptr<uint8_t[]> newBuffer(new uint8_t[newSize * _stride]);
	if(preserveData)
		std::memcpy(newBuffer.get(), _data.get(), _stride * std::min(_numElements, newSize));
	_data.swap(newBuffer);

	// Initialize new elements to zero.
	if(newSize > _numElements && preserveData) {
		std::memset(_data.get() + _numElements * _stride, 0, (newSize - _numElements) * _stride);
	}
	_numElements = newSize;
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
		auto src = constDataFloat();
		auto dst = dataFloat();
		for(size_t i = 0; i < s; ++i, ++src) {
			if(!mask.test(i)) *dst++ = *src;
		}
		resize(dst - dataFloat(), true);
	}
	else if(dataType() == PropertyStorage::Int && stride() == sizeof(int)) {
		// Single integer
		auto src = constDataInt();
		auto dst = dataInt();
		for(size_t i = 0; i < s; ++i, ++src) {
			if(!mask.test(i)) *dst++ = *src;
		}
		resize(dst - dataInt(), true);
	}
	else if(dataType() == PropertyStorage::Int64 && stride() == sizeof(qlonglong)*2) {
		// Pair 64-bit integer
		auto src = constDataInt64();
		auto dst = dataInt64();
		for(size_t i = 0; i < s; ++i, src += 2) {
			if(!mask.test(i)) {
				*dst++ = src[0];
				*dst++ = src[1];
			}
		}
		resize((dst - dataInt64())/2, true);
	}	
	else if(dataType() == PropertyStorage::Int64 && stride() == sizeof(qlonglong)) {
		// Single 64-bit integer
		auto src = constDataInt64();
		auto dst = dataInt64();
		for(size_t i = 0; i < s; ++i, ++src) {
			if(!mask.test(i)) *dst++ = *src;
		}
		resize(dst - dataInt64(), true);
	}
	else if(dataType() == PropertyStorage::Float && stride() == sizeof(Point3)) {
		// Triple float (may actually be four floats when SSE instructions are enabled).
		auto src = constDataPoint3();
		auto dst = dataPoint3();
		for(size_t i = 0; i < s; ++i, ++src) {
			if(!mask.test(i)) *dst++ = *src;
		}
		resize(dst - dataPoint3(), true);
	}
	else if(dataType() == PropertyStorage::Float && stride() == sizeof(Color)) {
		// Triple float
		auto src = constDataColor();
		auto dst = dataColor();
		for(size_t i = 0; i < s; ++i, ++src) {
			if(!mask.test(i)) *dst++ = *src;
		}
		resize(dst - dataColor(), true);
	}
	else if(dataType() == PropertyStorage::Int && stride() == sizeof(Point3I)) {
		// Triple int.
		auto src = constDataPoint3I();
		auto dst = dataPoint3I();
		for(size_t i = 0; i < s; ++i, ++src) {
			if(!mask.test(i)) *dst++ = *src;
		}
		resize(dst - dataPoint3I(), true);
	}
	else {
		// Generic case:
		const uint8_t* src = _data.get();
		uint8_t* dst = _data.get();
		for(size_t i = 0; i < s; i++, src += stride()) {
			if(!mask.test(i)) {
				memcpy(dst, src, stride());
				dst += stride();
			}
		}
		resize((dst - _data.get()) / stride(), true);
	}
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
		auto src = reinterpret_cast<const FloatType*>(source.constData());
		auto dst = reinterpret_cast<FloatType*>(data());
		for(size_t i = 0; i < source.size(); ++i, ++src) {
			OVITO_ASSERT(mapping[i] < this->size());
			dst[mapping[i]] = *src;
		}
	}
	else if(stride() == sizeof(int)) {
		// Single integer
		auto src = reinterpret_cast<const int*>(source.constData());
		auto dst = reinterpret_cast<int*>(data());
		for(size_t i = 0; i < source.size(); ++i, ++src) {
			OVITO_ASSERT(mapping[i] < this->size());
			dst[mapping[i]] = *src;
		}
	}
	else if(stride() == sizeof(qlonglong)) {
		// Single 64-bit integer
		auto src = reinterpret_cast<const qlonglong*>(source.constData());
		auto dst = reinterpret_cast<qlonglong*>(data());
		for(size_t i = 0; i < source.size(); ++i, ++src) {
			OVITO_ASSERT(mapping[i] < this->size());
			dst[mapping[i]] = *src;
		}
	}
	else if(stride() == sizeof(Point3)) {
		// Triple float (may actually be four floats when SSE instructions are enabled).
		auto src = reinterpret_cast<const Point3*>(source.constData());
		auto dst = reinterpret_cast<Point3*>(data());
		for(size_t i = 0; i < source.size(); ++i, ++src) {
			OVITO_ASSERT(mapping[i] < this->size());
			dst[mapping[i]] = *src;
		}
	}
	else if(stride() == sizeof(Color)) {
		// Triple float
		auto src = reinterpret_cast<const Color*>(source.constData());
		auto dst = reinterpret_cast<Color*>(data());
		for(size_t i = 0; i < source.size(); ++i, ++src) {
			OVITO_ASSERT(mapping[i] < this->size());
			dst[mapping[i]] = *src;
		}
	}
	else if(stride() == sizeof(Point3I)) {
		// Triple int
		auto src = reinterpret_cast<const Point3I*>(source.constData());
		auto dst = reinterpret_cast<Point3I*>(data());
		for(size_t i = 0; i < source.size(); ++i, ++src) {
			OVITO_ASSERT(mapping[i] < this->size());
			dst[mapping[i]] = *src;
		}
	}	
	else {
		// General case:
		const uint8_t* src = source._data.get();
		uint8_t* dst = _data.get();
		for(size_t i = 0; i < source.size(); i++, src += stride()) {
			OVITO_ASSERT(mapping[i] < this->size());
			memcpy(dst + stride() * mapping[i], src, stride());
		}
	}
}

}	// End of namespace
}	// End of namespace
