///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2019) Alexander Stukowski
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


#include <ovito/particles/Particles.h>
#include "GSDImporter.h"
#include "gsd.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Import) OVITO_BEGIN_INLINE_NAMESPACE(Formats)

template<typename T> gsd_type gsdDataType() { OVITO_ASSERT(false); }
template<> gsd_type gsdDataType<uint8_t>() { return GSD_TYPE_UINT8; }
template<> gsd_type gsdDataType<uint16_t>() { return GSD_TYPE_UINT16; }
template<> gsd_type gsdDataType<uint32_t>() { return GSD_TYPE_UINT32; }
template<> gsd_type gsdDataType<uint64_t>() { return GSD_TYPE_UINT64; }
template<> gsd_type gsdDataType<int8_t>() { return GSD_TYPE_INT8; }
template<> gsd_type gsdDataType<int16_t>() { return GSD_TYPE_INT16; }
template<> gsd_type gsdDataType<int32_t>() { return GSD_TYPE_INT32; }
template<> gsd_type gsdDataType<int64_t>() { return GSD_TYPE_INT64; }
template<> gsd_type gsdDataType<float>() { return GSD_TYPE_FLOAT; }
template<> gsd_type gsdDataType<double>() { return GSD_TYPE_DOUBLE; }


/**
 * \brief A thin wrapper class around the GSD (General Simulation Data) routines
 *        used by the GSDImporter class.
 */
class GSDFile
{
public:

	/// Constructor.
	GSDFile(const char* filename, const gsd_open_flag flags = GSD_OPEN_READONLY) {
		switch(::gsd_open(&_handle, filename, flags)) {
			case 0: break; // Success
			case -1: throw Exception(GSDImporter::tr("Failed to open GSD file for reading. I/O error."));
			case -2: throw Exception(GSDImporter::tr("Failed to open GSD file for reading. Not a GSD file."));
			case -3: throw Exception(GSDImporter::tr("Failed to open GSD file for reading. Invalid GSD file version."));
			case -4: throw Exception(GSDImporter::tr("Failed to open GSD file for reading. Corrupt file."));
			case -5: throw Exception(GSDImporter::tr("Failed to open GSD file for reading. Unable to allocate memory."));
			default: throw Exception(GSDImporter::tr("Failed to open GSD file for reading. Unknown error."));
		}
	}

	/// Destructor.
	~GSDFile() {
		::gsd_close(&_handle);
	}

	// Returns the schema name of the GSD file.
	const char* schemaName() const {
		return _handle.header.schema;
	}

	/// Returns the number of frames in the GSD file.
	uint64_t numerOfFrames() {
		return ::gsd_get_nframes(&_handle);
	}

	/// Returns whether a chunk with the given name exists.
	bool hasChunk(const char* chunkName, uint64_t frame) {
		if(::gsd_find_chunk(&_handle, frame, chunkName) != nullptr) return true;
		// Automatically fall back to frame 0 if chunk doesn't exist for the requested simulation frame.
		if(frame != 0 && ::gsd_find_chunk(&_handle, 0, chunkName) != nullptr) return true;
		return false;
	}

	/// Searches for chunk names starting with the given prefix string.
	const char* findMatchingChunkName(const char* match, const char* prev) {
		return ::gsd_find_matching_chunk_name(&_handle, match, prev);
	}

	/// Determines the corresponding OVITO data type of a GSD chunk and the number of vector components.
	std::pair<int, size_t> getChunkDataTypeAndComponentCount(const char* chunkName) {
		auto chunk = ::gsd_find_chunk(&_handle, 0, chunkName);
		if(!chunk) throw Exception(GSDImporter::tr("GSD file I/O error. Chunk %1 does not exist.").arg(chunkName));
		switch(chunk->type) {
			case GSD_TYPE_INT8:
			case GSD_TYPE_UINT8:
			case GSD_TYPE_INT16:
			case GSD_TYPE_UINT16:
			case GSD_TYPE_INT32:
				return { PropertyStorage::Int, chunk->M };
			case GSD_TYPE_UINT32: // Note: We map unsigned int32 to the signed int64 in OVITO to avoid overflows.
			case GSD_TYPE_INT64:
			case GSD_TYPE_UINT64:
				return { PropertyStorage::Int64, chunk->M };
			case GSD_TYPE_FLOAT:
			case GSD_TYPE_DOUBLE:
				return { PropertyStorage::Float, chunk->M };
			default:
				throw Exception(GSDImporter::tr("GSD file I/O error. Unknown chunk data type."));
		}
	}

	/// Reads a single unsigned 64-bit integer from the GSD file, or returns a default value if the chunk is not present in the file.
	template<typename T>
	T readOptionalScalar(const char* chunkName, uint64_t frame, T defaultValue) {
		auto chunk = ::gsd_find_chunk(&_handle, frame, chunkName);
		// Automatically fall back to frame 0 if chunk doesn't exist for the requested simulation frame.
		if(!chunk && frame != 0) chunk = ::gsd_find_chunk(&_handle, 0, chunkName);
		if(chunk) {
			if(chunk->N != 1 || chunk->M != 1)
				throw Exception(GSDImporter::tr("GSD file I/O error: Chunk '%1' does not contain a scalar value.").arg(chunkName));
			if(chunk->type != gsdDataType<T>())
				throw Exception(GSDImporter::tr("GSD file I/O error: Data type of chunk '%1' is not %2 but %3.").arg(chunkName).arg(gsdDataType<T>()).arg(chunk->type));
			OVITO_ASSERT(::gsd_sizeof_type(gsdDataType<T>()) == sizeof(defaultValue));
			switch(::gsd_read_chunk(&_handle, &defaultValue, chunk)) {
				case 0: break; // Success
				case -1: throw Exception(GSDImporter::tr("GSD file I/O error."));
				case -2: throw Exception(GSDImporter::tr("GSD file I/O error: Invalid input."));
				case -3: throw Exception(GSDImporter::tr("GSD file I/O error: Invalid file data."));
				default: throw Exception(GSDImporter::tr("GSD file I/O error."));
			}
		}
		return defaultValue;
	}

	/// Reads a single chunk from the GSD file and returns the data as a QVariant.
	QVariant readVariant(const char* chunkName, uint64_t frame) {
		auto chunk = ::gsd_find_chunk(&_handle, frame, chunkName);
		// Automatically fall back to frame 0 if chunk doesn't exist for the requested simulation frame.
		if(!chunk && frame != 0) chunk = ::gsd_find_chunk(&_handle, 0, chunkName);
		if(!chunk)
			throw Exception(GSDImporter::tr("GSD file I/O error: Chunk '%1' does not exist at frame %2 (or the initial frame).").arg(chunkName).arg(frame));
		int errCode = -1;
		QVariant result;
		if(chunk->type == GSD_TYPE_INT8 && chunk->M == 1) {
			// Special handling for char arrays, which need to be converted to a string object.
			QByteArray buffer(chunk->N, '\0');
			errCode = ::gsd_read_chunk(&_handle, buffer.data(), chunk);
			result = QString::fromUtf8(buffer);
		}
		else {
			if(chunk->N == 1 && chunk->M == 1) {
				if(chunk->type == GSD_TYPE_INT8) {
					int8_t value;
					errCode = ::gsd_read_chunk(&_handle, &value, chunk);
					result = QVariant::fromValue((int)value);
				}
				else if(chunk->type == GSD_TYPE_UINT8) {
					uint8_t value;
					errCode = ::gsd_read_chunk(&_handle, &value, chunk);
					result = QVariant::fromValue((uint)value);
				}
				else if(chunk->type == GSD_TYPE_INT16) {
					int16_t value;
					errCode = ::gsd_read_chunk(&_handle, &value, chunk);
					result = QVariant::fromValue((int)value);
				}
				else if(chunk->type == GSD_TYPE_UINT16) {
					uint16_t value;
					errCode = ::gsd_read_chunk(&_handle, &value, chunk);
					result = QVariant::fromValue((uint)value);
				}
				else if(chunk->type == GSD_TYPE_INT32) {
					int32_t value;
					errCode = ::gsd_read_chunk(&_handle, &value, chunk);
					result = QVariant::fromValue(value);
				}
				else if(chunk->type == GSD_TYPE_UINT32) {
					uint32_t value;
					errCode = ::gsd_read_chunk(&_handle, &value, chunk);
					result = QVariant::fromValue(value);
				}
				else if(chunk->type == GSD_TYPE_INT64) {
					int64_t value;
					errCode = ::gsd_read_chunk(&_handle, &value, chunk);
					result = QVariant::fromValue(value);
				}
				else if(chunk->type == GSD_TYPE_UINT64) {
					uint64_t value;
					errCode = ::gsd_read_chunk(&_handle, &value, chunk);
					result = QVariant::fromValue(value);
				}
				else if(chunk->type == GSD_TYPE_FLOAT) {
					float value;
					errCode = ::gsd_read_chunk(&_handle, &value, chunk);
					result = QVariant::fromValue((double)value);
				}
				else if(chunk->type == GSD_TYPE_DOUBLE) {
					double value;
					errCode = ::gsd_read_chunk(&_handle, &value, chunk);
					result = QVariant::fromValue(value);
				}
			}
			else {
				QVariantList list;
				if(chunk->type == GSD_TYPE_INT8) {
					std::vector<int8_t> buffer(chunk->N * chunk->M);
					errCode = ::gsd_read_chunk(&_handle, buffer.data(), chunk);
					std::copy(buffer.begin(), buffer.end(), std::back_inserter(list));
				}
				else if(chunk->type == GSD_TYPE_UINT8) {
					std::vector<uint8_t> buffer(chunk->N * chunk->M);
					errCode = ::gsd_read_chunk(&_handle, buffer.data(), chunk);
					std::copy(buffer.begin(), buffer.end(), std::back_inserter(list));
				}
				else if(chunk->type == GSD_TYPE_INT16) {
					std::vector<int16_t> buffer(chunk->N * chunk->M);
					errCode = ::gsd_read_chunk(&_handle, buffer.data(), chunk);
					std::copy(buffer.begin(), buffer.end(), std::back_inserter(list));
				}
				else if(chunk->type == GSD_TYPE_UINT16) {
					std::vector<uint16_t> buffer(chunk->N * chunk->M);
					errCode = ::gsd_read_chunk(&_handle, buffer.data(), chunk);
					std::copy(buffer.begin(), buffer.end(), std::back_inserter(list));
				}
				else if(chunk->type == GSD_TYPE_INT32) {
					std::vector<int32_t> buffer(chunk->N * chunk->M);
					errCode = ::gsd_read_chunk(&_handle, buffer.data(), chunk);
					std::copy(buffer.begin(), buffer.end(), std::back_inserter(list));
				}
				else if(chunk->type == GSD_TYPE_UINT32) {
					std::vector<uint32_t> buffer(chunk->N * chunk->M);
					errCode = ::gsd_read_chunk(&_handle, buffer.data(), chunk);
					std::copy(buffer.begin(), buffer.end(), std::back_inserter(list));
				}
				else if(chunk->type == GSD_TYPE_INT64) {
					std::vector<int64_t> buffer(chunk->N * chunk->M);
					errCode = ::gsd_read_chunk(&_handle, buffer.data(), chunk);
					std::transform(buffer.begin(), buffer.end(), std::back_inserter(list), [](int64_t v) { return QVariant::fromValue((qlonglong)v); });
				}
				else if(chunk->type == GSD_TYPE_UINT64) {
					std::vector<uint64_t> buffer(chunk->N * chunk->M);
					errCode = ::gsd_read_chunk(&_handle, buffer.data(), chunk);
					std::transform(buffer.begin(), buffer.end(), std::back_inserter(list), [](uint64_t v) { return QVariant::fromValue((qulonglong)v); });
				}
				else if(chunk->type == GSD_TYPE_FLOAT) {
					std::vector<float> buffer(chunk->N * chunk->M);
					errCode = ::gsd_read_chunk(&_handle, buffer.data(), chunk);
					std::copy(buffer.begin(), buffer.end(), std::back_inserter(list));
				}
				else if(chunk->type == GSD_TYPE_DOUBLE) {
					std::vector<double> buffer(chunk->N * chunk->M);
					errCode = ::gsd_read_chunk(&_handle, buffer.data(), chunk);
					std::copy(buffer.begin(), buffer.end(), std::back_inserter(list));
				}
				result = QVariant::fromValue(list);
			}
		}
		switch(errCode) {
			case 0: break; // Success
			case -1: throw Exception(GSDImporter::tr("GSD file I/O error."));
			case -2: throw Exception(GSDImporter::tr("GSD file I/O error: Invalid input."));
			case -3: throw Exception(GSDImporter::tr("GSD file I/O error: Invalid file data."));
			default: throw Exception(GSDImporter::tr("GSD file I/O error."));
		}
		return result;
	}

	/// Reads an one-dimensional array from the GSD file if the data chunk is present.
	template<typename T, size_t N>
	void readOptional1DArray(const char* chunkName, uint64_t frame, std::array<T,N>& a) {
		auto chunk = ::gsd_find_chunk(&_handle, frame, chunkName);
		// Automatically fall back to frame 0 if chunk doesn't exist for the requested simulation frame.
		if(!chunk && frame != 0) chunk = ::gsd_find_chunk(&_handle, 0, chunkName);
		if(chunk) {
			if(chunk->N != a.size() || chunk->M != 1)
				throw Exception(GSDImporter::tr("GSD file I/O error: Chunk '%1' does not contain a 1-dimensional array of the expected size.").arg(chunkName));
			if(chunk->type != gsdDataType<T>())
				throw Exception(GSDImporter::tr("GSD file I/O error: Data type of chunk '%1' is not %2 but %3.").arg(chunkName).arg(gsdDataType<T>()).arg(chunk->type));
			OVITO_ASSERT(::gsd_sizeof_type(gsdDataType<T>()) == sizeof(a[0]));
			switch(::gsd_read_chunk(&_handle, a.data(), chunk)) {
				case 0: break; // Success
				case -1: throw Exception(GSDImporter::tr("GSD file I/O error."));
				case -2: throw Exception(GSDImporter::tr("GSD file I/O error: Invalid input."));
				case -3: throw Exception(GSDImporter::tr("GSD file I/O error: Invalid file data."));
				default: throw Exception(GSDImporter::tr("GSD file I/O error."));
			}
		}
	}

	/// Reads an array of strings from the GSD file.
	QStringList readStringTable(const char* chunkName, uint64_t frame) {
		QStringList result;
		auto chunk = ::gsd_find_chunk(&_handle, frame, chunkName);
		// Automatically fall back to frame 0 if chunk doesn't exist for the requested simulation frame.
		if(!chunk && frame != 0) chunk = ::gsd_find_chunk(&_handle, 0, chunkName);
		if(chunk) {
			if(chunk->type != GSD_TYPE_INT8 && chunk->type != GSD_TYPE_UINT8)
				throw Exception(GSDImporter::tr("GSD file I/O error: Data type of chunk '%1' is not GSD_TYPE_UINT8 but %2.").arg(chunkName).arg(chunk->type));
			std::vector<char> buffer(chunk->N * chunk->M);
			switch(::gsd_read_chunk(&_handle, buffer.data(), chunk)) {
				case 0: break; // Success
				case -1: throw Exception(GSDImporter::tr("GSD file I/O error."));
				case -2: throw Exception(GSDImporter::tr("GSD file I/O error: Invalid input."));
				case -3: throw Exception(GSDImporter::tr("GSD file I/O error: Invalid file data."));
				default: throw Exception(GSDImporter::tr("GSD file I/O error."));
			}
			for(uint64_t i = 0; i < chunk->N; i++) {
				// Null-terminate string, just to be save.
				buffer[(i+1)*chunk->M - 1] = '\0';
				result.push_back(QString::fromUtf8(buffer.data() + i*chunk->M));
			}
		}
		return result;
	}

	template<typename T>
	void readFloatArray(const char* chunkName, uint64_t frame, T* buffer, size_t numElements, size_t componentCount = 1) {
		auto chunk = ::gsd_find_chunk(&_handle, frame, chunkName);
		// Automatically fall back to frame 0 if chunk doesn't exist for the requested simulation frame.
		if(!chunk && frame != 0) chunk = ::gsd_find_chunk(&_handle, 0, chunkName);
		if(!chunk)
			throw Exception(GSDImporter::tr("GSD file I/O error: Chunk '%1' does not exist at frame %2 (or the initial frame).").arg(chunkName).arg(frame));
		if(chunk->type != GSD_TYPE_FLOAT && chunk->type != GSD_TYPE_DOUBLE)
			throw Exception(GSDImporter::tr("GSD file I/O error: Data type of chunk '%1' is not GSD_TYPE_FLOAT but %2.").arg(chunkName).arg(chunk->type));
		if(chunk->N != numElements)
			throw Exception(GSDImporter::tr("GSD file I/O error: Number of elements in chunk '%1' does not match expected value.").arg(chunkName));
		if(chunk->M != componentCount)
			throw Exception(GSDImporter::tr("GSD file I/O error: Size of second dimension in chunk '%1' is %2 and does not match expected value %3.").arg(chunkName).arg(chunk->M).arg(componentCount));
		int errCode;
#ifdef FLOATTYPE_FLOAT
		if(chunk->type == GSD_TYPE_DOUBLE) {
			// Convert GSD data from double to float.
			std::vector<double> doubleBuffer(chunk->N * chunk->M);
			errCode = ::gsd_read_chunk(&_handle, doubleBuffer.data(), chunk);
			std::copy(doubleBuffer.begin(), doubleBuffer.end(), reinterpret_cast<float*>(buffer));
		}
		else {
			// No data type conversion needed.
			errCode = ::gsd_read_chunk(&_handle, buffer, chunk);
		}
#else
		if(chunk->type == GSD_TYPE_FLOAT) {
			// Convert GSD data from float to double.
			std::vector<float> floatBuffer(chunk->N * chunk->M);
			errCode = ::gsd_read_chunk(&_handle, floatBuffer.data(), chunk);
			std::copy(floatBuffer.begin(), floatBuffer.end(), reinterpret_cast<double*>(buffer));
		}
		else {
			// No data type conversion needed.
			errCode = ::gsd_read_chunk(&_handle, buffer, chunk);
		}
#endif
		switch(errCode) {
			case 0: break; // Success
			case -1: throw Exception(GSDImporter::tr("GSD file I/O error."));
			case -2: throw Exception(GSDImporter::tr("GSD file I/O error: Invalid input."));
			case -3: throw Exception(GSDImporter::tr("GSD file I/O error: Invalid file data."));
			default: throw Exception(GSDImporter::tr("GSD file I/O error."));
		}
	}

	template<typename IntType>
	void readIntArray(const char* chunkName, uint64_t frame, IntType* buffer, size_t numElements, size_t intsPerElement = 1) {
		auto chunk = ::gsd_find_chunk(&_handle, frame, chunkName);
		// Automatically fall back to frame 0 if chunk doesn't exist for the requested simulation frame.
		if(!chunk && frame != 0) chunk = ::gsd_find_chunk(&_handle, 0, chunkName);
		if(!chunk)
			throw Exception(GSDImporter::tr("GSD file I/O error: Chunk '%1' does not exist at frame %2 (or the initial frame).").arg(chunkName).arg(frame));
		if(chunk->type != GSD_TYPE_INT8 && chunk->type != GSD_TYPE_UINT8
			&& chunk->type != GSD_TYPE_INT16 && chunk->type != GSD_TYPE_UINT16
			&& chunk->type != GSD_TYPE_INT32 && chunk->type != GSD_TYPE_UINT32
			&& chunk->type != GSD_TYPE_INT64 && chunk->type != GSD_TYPE_UINT64)
			throw Exception(GSDImporter::tr("GSD file I/O error: Data type of chunk '%1' is not an integer type but %2.").arg(chunkName).arg(chunk->type));
		if(chunk->N != numElements)
			throw Exception(GSDImporter::tr("GSD file I/O error: Number of elements in chunk '%1' does not match expected value.").arg(chunkName));
		if(chunk->M != intsPerElement)
			throw Exception(GSDImporter::tr("GSD file I/O error: Size of second dimension in chunk '%1' is not %2.").arg(chunkName).arg(intsPerElement));
		int errCode;
		if(::gsd_sizeof_type(static_cast<gsd_type>(chunk->type)) == sizeof(IntType)) {
			// No data type conversion needed.
			errCode = ::gsd_read_chunk(&_handle, buffer, chunk);
		}
		else {
			// Perform data type conversion after loading the data into a temporary buffer.
			if(chunk->type == GSD_TYPE_INT8) {
				std::vector<int8_t> tempBuffer(chunk->N * chunk->M);
				errCode = ::gsd_read_chunk(&_handle, tempBuffer.data(), chunk);
				std::copy(tempBuffer.begin(), tempBuffer.end(), buffer);
			}
			else if(chunk->type == GSD_TYPE_UINT8) {
				std::vector<uint8_t> tempBuffer(chunk->N * chunk->M);
				errCode = ::gsd_read_chunk(&_handle, tempBuffer.data(), chunk);
				std::copy(tempBuffer.begin(), tempBuffer.end(), buffer);
			}
			else if(chunk->type == GSD_TYPE_INT16) {
				std::vector<int16_t> tempBuffer(chunk->N * chunk->M);
				errCode = ::gsd_read_chunk(&_handle, tempBuffer.data(), chunk);
				std::copy(tempBuffer.begin(), tempBuffer.end(), buffer);
			}
			else if(chunk->type == GSD_TYPE_UINT16) {
				std::vector<uint16_t> tempBuffer(chunk->N * chunk->M);
				errCode = ::gsd_read_chunk(&_handle, tempBuffer.data(), chunk);
				std::copy(tempBuffer.begin(), tempBuffer.end(), buffer);
			}
			else if(chunk->type == GSD_TYPE_INT32) {
				std::vector<int32_t> tempBuffer(chunk->N * chunk->M);
				errCode = ::gsd_read_chunk(&_handle, tempBuffer.data(), chunk);
				std::copy(tempBuffer.begin(), tempBuffer.end(), buffer);
			}
			else if(chunk->type == GSD_TYPE_UINT32) {
				std::vector<uint32_t> tempBuffer(chunk->N * chunk->M);
				errCode = ::gsd_read_chunk(&_handle, tempBuffer.data(), chunk);
				std::copy(tempBuffer.begin(), tempBuffer.end(), buffer);
			}
			else if(chunk->type == GSD_TYPE_INT64) {
				std::vector<int64_t> tempBuffer(chunk->N * chunk->M);
				errCode = ::gsd_read_chunk(&_handle, tempBuffer.data(), chunk);
				std::copy(tempBuffer.begin(), tempBuffer.end(), buffer);
			}
			else if(chunk->type == GSD_TYPE_UINT64) {
				std::vector<uint64_t> tempBuffer(chunk->N * chunk->M);
				errCode = ::gsd_read_chunk(&_handle, tempBuffer.data(), chunk);
				std::copy(tempBuffer.begin(), tempBuffer.end(), buffer);
			}
			else errCode = -1;
		}
		switch(errCode) {
			case 0: break; // Success
			case -1: throw Exception(GSDImporter::tr("GSD file I/O error."));
			case -2: throw Exception(GSDImporter::tr("GSD file I/O error: Invalid input."));
			case -3: throw Exception(GSDImporter::tr("GSD file I/O error: Invalid file data."));
			default: throw Exception(GSDImporter::tr("GSD file I/O error."));
		}
	}

private:

	gsd_handle _handle;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
