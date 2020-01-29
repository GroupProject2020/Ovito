////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2020 Alexander Stukowski
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

#include <ovito/core/Core.h>
#include <ovito/core/utilities/io/FileManager.h>
#include "CompressedTextReader.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(IO)

/******************************************************************************
* Opens the given I/O device for reading.
******************************************************************************/
CompressedTextReader::CompressedTextReader(const FileHandle& input) :
#ifdef OVITO_ZLIB_SUPPORT
	_device(input.createIODevice()), _uncompressor(_device.get(), 6, 0x100000)
#else
	_device(input.createIODevice())
#endif
{
	// Try to find out what the filename is.
	if(!input.sourceUrl().isEmpty())
		_filename = input.sourceUrl().fileName();
	else if(QFileDevice* fileDevice = qobject_cast<QFileDevice*>(_device.get()))
		_filename = fileDevice->fileName();

	// Check if file is compressed (i.e. filename ends with .gz).
	if(_filename.endsWith(".gz", Qt::CaseInsensitive)) {
#ifdef OVITO_ZLIB_SUPPORT
		// Open compressed file for reading.
		_uncompressor.setStreamFormat(GzipIODevice::GzipFormat);
		if(!_uncompressor.open(QIODevice::ReadOnly))
			throw Exception(tr("Failed to open input file: %1").arg(_uncompressor.errorString()));
		_stream = &_uncompressor;
#else
		throw Exception(tr("Cannot open file '%1' for reading. This version of OVITO was built without I/O support for gzip compressed files."));
#endif
	}
	else {
		// Open uncompressed file for reading.
		if(!_device->open(QIODevice::ReadOnly))
			throw Exception(tr("Failed to open input file: %1").arg(_device->errorString()));
		_stream = _device.get();
	}
}

/******************************************************************************
* Reads in the next line.
******************************************************************************/
const char* CompressedTextReader::readLine(int maxSize)
{
	_lineNumber++;

	if(_stream->atEnd())
		throw Exception(tr("File parsing error. Unexpected end of file after line %1.").arg(_lineNumber));

	qint64 readBytes = 0;
	if(!maxSize) {
		if(_line.size() <= 1) {
			_line.resize(1024);
		}
		readBytes = _stream->readLine(_line.data(), _line.size());

		if(readBytes == _line.size() - 1 && _line[readBytes - 1] != '\n') {
			qint64 readResult;
			do {
				_line.resize(_line.size() + 16384);
				readResult = _stream->readLine(_line.data() + readBytes, _line.size() - readBytes);
				if(readResult > 0 || readBytes == 0)
					readBytes += readResult;
			}
			while(readResult == Q_INT64_C(16384) && _line[readBytes - 1] != '\n');
		}
	}
	else {
        if(maxSize > (int)_line.size()) {
			_line.resize(maxSize + 1);
		}
		readBytes = _stream->readLine(_line.data(), _line.size());
	}

	if(readBytes <= 0)
		_line[0] = '\0';
	else {
		_line[readBytes] = '\0';
		_byteOffset += readBytes;
	}

	return _line.data();
}

/******************************************************************************
* Maps a part of the input file to memory.
******************************************************************************/
std::pair<const char*, const char*> CompressedTextReader::mmap(qint64 offset, qint64 size)
{
	OVITO_ASSERT(_mmapPointer == nullptr);
	if(!isCompressed()) {
		if(QFileDevice* fileDevice = qobject_cast<QFileDevice*>(&device()))
			_mmapPointer = fileDevice->map(underlyingByteOffset(), size);
	}
	return std::make_pair(
			reinterpret_cast<const char*>(_mmapPointer),
			reinterpret_cast<const char*>(_mmapPointer) + size);
}

/******************************************************************************
* Unmaps the file from memory.
******************************************************************************/
void CompressedTextReader::munmap()
{
	OVITO_ASSERT(_mmapPointer != nullptr);
	if(QFileDevice* fileDevice = qobject_cast<QFileDevice*>(&device()))
		fileDevice->unmap(_mmapPointer);
	_mmapPointer = nullptr;
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
