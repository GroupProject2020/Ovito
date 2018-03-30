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


#include <core/Core.h>
#include "GzipIODevice.h"
#include <zlib.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Util)

using ZlibByte = Bytef;
using ZlibSize = uInt;

OVITO_STATIC_ASSERT((std::is_same<ZlibByte, unsigned char>::value));

struct ZLibState
{
    z_stream _zlibStream;

    /// Constructor.
    ZLibState() {
        // Use default zlib memory management.
        _zlibStream.zalloc = Z_NULL;
        _zlibStream.zfree = Z_NULL;
        _zlibStream.opaque = Z_NULL;
    }
};

/// Flushes the zlib stream.
void GzipIODevice::flushZlib(int flushMode)
{
    // No input.
    _zlibStruct->_zlibStream.next_in = 0;
    _zlibStruct->_zlibStream.avail_in = 0;
    int status;
    do {
        _zlibStruct->_zlibStream.next_out = _buffer.get();
        _zlibStruct->_zlibStream.avail_out = _bufferSize;
        status = ::deflate(&_zlibStruct->_zlibStream, flushMode);
        if(status != Z_OK && status != Z_STREAM_END) {
            _state = Error;
            setZlibError(tr("Internal zlib error when compressing: "), status);
            return;
        }

        ZlibSize outputSize = _bufferSize - _zlibStruct->_zlibStream.avail_out;

        // Try to write data from the buffer to to the underlying device, return on failure.
        if(!writeBytes(outputSize))
            return;

        // If the mode is Z_FNISH we must loop until we get Z_STREAM_END,
        // else we loop as long as zlib is able to fill the output buffer.
    } 
    while((flushMode == Z_FINISH && status != Z_STREAM_END) || (flushMode != Z_FINISH && _zlibStruct->_zlibStream.avail_out == 0));

    if(flushMode == Z_FINISH)
        OVITO_ASSERT(status == Z_STREAM_END);
    else
        OVITO_ASSERT(status == Z_OK);
}

// Writes outputSize bytes from buffer to the inderlying device.
bool GzipIODevice::writeBytes(qint64 outputSize)
{
    ZlibSize totalBytesWritten = 0;
    // Loop until all bytes are written to the underlying device.
    do {
        const qint64 bytesWritten = _device->write(reinterpret_cast<char*>(_buffer.get()), outputSize);
        if(bytesWritten == -1) {
            setErrorString(tr("Error writing to underlying I/O device: %1").arg(_device->errorString()));
            return false;
        }
        totalBytesWritten += bytesWritten;
    } 
    while(totalBytesWritten != outputSize);

    // Put up a flag so that the device will be flushed on close.
    _state = BytesWritten;
    return true;
}

// Sets the error string to errorMessage + zlib error string for zlibErrorCode
void GzipIODevice::setZlibError(const QString& errorMessage, int zlibErrorCode)
{
    // Watch out, zlibErrorString may be null.
    const char* const zlibErrorString = ::zError(zlibErrorCode);
    QString errorString;
    if(zlibErrorString)
        errorString = errorMessage + zlibErrorString;
    else
        errorString = tr("%1 - Unknown error (code %2)").arg(errorMessage).arg(zlibErrorCode);

    setErrorString(errorString);
}

/// Constructor
GzipIODevice::GzipIODevice(QIODevice* device, int compressionLevel, int bufferSize) :
    _device(device), 
    _compressionLevel(compressionLevel),
    _zlibStruct(new ZLibState()),
    _bufferSize(bufferSize), 
    _buffer(std::make_unique<ZlibByte[]>(bufferSize))
{
}

/// Destructor.
GzipIODevice::~GzipIODevice()
{
    close();
    delete _zlibStruct;
}

bool GzipIODevice::seek(qint64 pos)
{
    if(isWritable())
		return false;

	OpenMode mode = openMode();
	close();
    if(_device->isOpen()) {
    	if(!_device->reset())
    		return false;
    }
	if(!open(mode))
		return false;

	char buffer[0x10000];
	while(pos > 0) {
		qint64 s = read(buffer, std::min(pos, (qint64)sizeof(buffer)));
		if(s <= 0)
			return false;
		pos -= s;
	}

	return true;
}

/*!
    Opens the GzipIODevice in \a mode. Only ReadOnly and WriteOnly is supported.
    This functon will return false if you try to open in other modes.

    If the underlying device is not opened, this function will open it in a suitable mode. If this happens
    the device will also be closed when close() is called.

    If the underlying device is already opened, its openmode must be compatable with \a mode.

    Returns true on success, false on error.
*/
bool GzipIODevice::open(OpenMode mode)
{
    if(isOpen()) {
        qWarning("GzipIODevice::open: device already open");
        return false;
    }

    // Check for correct mode: ReadOnly xor WriteOnly
    const bool read = (bool)(mode & ReadOnly);
    const bool write = (bool)(mode & WriteOnly);
    const bool both = (read && write);
    const bool neither = !(read || write);
    if(both || neither) {
        qWarning("GzipIODevice::open: GzipIODevice can only be opened in the ReadOnly or WriteOnly modes");
        return false;
    }

    // If the underlying device is open, check that is it opened in a compatible mode.
    if(_device->isOpen()) {
        _manageDevice = false;
        const OpenMode deviceMode = _device->openMode();
        if(read && !(deviceMode & ReadOnly)) {
            qWarning("GzipIODevice::open: underlying device must be open in one of the ReadOnly or WriteOnly modes");
            return false;
        } 
        else if(write && !(deviceMode & WriteOnly)) {
            qWarning("GzipIODevice::open: underlying device must be open in one of the ReadOnly or WriteOnly modes");
            return false;
        }

    // If the underlying device is closed, open it.
    } 
    else {
        _manageDevice = true;
        if(_device->open(mode) == false) {
            setErrorString(tr("Error opening underlying device: %1").arg(_device->errorString()));
            return false;
        }
    }

    // Initialize zlib for deflating or inflating.

    // The second argument to inflate/deflateInit2 is the windowBits parameter,
    // which also controls what kind of compression stream headers to use.
    // The default value for this is 15. Passing a value greater than 15
    // enables gzip headers and then subtracts 16 form the windowBits value.
    // (So passing 31 gives gzip headers and 15 windowBits). Passing a negative
    // value selects no headers hand then negates the windowBits argument.
    int windowBits;
    switch(streamFormat()) {
    case GzipFormat:
        windowBits = 31;
        break;
    case RawZipFormat:
        windowBits = -15;
        break;
    default:
        windowBits = 15;
    }

    int status;
    if(read) {
        _state = NotReadFirstByte;
        _zlibStruct->_zlibStream.avail_in = 0;
        _zlibStruct->_zlibStream.next_in = 0;
        if(streamFormat() == ZlibFormat) {
            status = ::inflateInit(&_zlibStruct->_zlibStream);
        } 
        else {
            status = ::inflateInit2(&_zlibStruct->_zlibStream, windowBits);
        }
    } 
    else {
        _state = NoBytesWritten;
        if(streamFormat() == ZlibFormat)
            status = ::deflateInit(&_zlibStruct->_zlibStream, _compressionLevel);
        else
            status = ::deflateInit2(&_zlibStruct->_zlibStream, _compressionLevel, Z_DEFLATED, windowBits, 8, Z_DEFAULT_STRATEGY);
    }

    // Handle error.
    if(status != Z_OK) {
        setZlibError(tr("Internal zlib error: "), status);
        return false;
    }
    return QIODevice::open(mode);
}

/// Closes the GzipIODevice, and also the underlying device if it was opened by GzipIODevice.
void GzipIODevice::close()
{
    if(isOpen() == false)
        return;

    // Flush and close the zlib stream.
    if(openMode() & ReadOnly) {
        _state = NotReadFirstByte;
        ::inflateEnd(&_zlibStruct->_zlibStream);
    } 
    else {
        if(_state == BytesWritten) { // Only flush if we have written anything.
            _state = NoBytesWritten;
            flushZlib(Z_FINISH);
        }
        ::deflateEnd(&_zlibStruct->_zlibStream);
    }

    // Close the underlying device if we are managing it.
    if(_manageDevice)
        _device->close();

    _zlibStruct->_zlibStream.next_in = 0;
    _zlibStruct->_zlibStream.avail_in = 0;
    _zlibStruct->_zlibStream.next_out = NULL;
    _zlibStruct->_zlibStream.avail_out = 0;
    _state = Closed;

    QIODevice::close();
}

/*!
    Flushes the internal buffer.

    Each time you call flush, all data written to the GzipIODevice is compressed and written to the
    underlying device. Calling this function can reduce the compression ratio. The underlying device
    is not flushed.

    Calling this function when GzipIODevice is in ReadOnly mode has no effect.
*/
void GzipIODevice::flush()
{
    if(isOpen() == false || openMode() & ReadOnly)
        return;

    flushZlib(Z_SYNC_FLUSH);
}

/*!
    Returns 1 if there might be data available for reading, or 0 if there is no data available.

    There is unfortunately no way of knowing how much data there is available when dealing with compressed streams.

    Also, since the remaining compressed data might be a part of the meta-data that ends the compressed stream (and
    therefore will yield no uncompressed data), you cannot assume that a read after getting a 1 from this function will return data.
*/
qint64 GzipIODevice::bytesAvailable() const
{
    if((openMode() & ReadOnly) == false)
        return 0;

    qint64 numBytes = 0;

    switch(_state) {
        case NotReadFirstByte:
            numBytes = _device->bytesAvailable();
            break;
        case InStream:
            numBytes = 1;
            break;
        case EndOfStream:
        case Error:
        default:
            numBytes = 0;
            break;
    };

    numBytes += QIODevice::bytesAvailable();

    if(numBytes > 0)
        return 1;
    else
        return 0;
}

/*!
    Reads and decompresses data from the underlying device.
*/
qint64 GzipIODevice::readData(char* data, qint64 maxSize)
{
    if(_state == EndOfStream)
        return 0;

    if(_state == Error)
        return -1;

    // We will to try to fill the data buffer
    _zlibStruct->_zlibStream.next_out = reinterpret_cast<ZlibByte*>(data);
    _zlibStruct->_zlibStream.avail_out = maxSize;

    int status;
    do {
        // Read data if if the input buffer is empty. There could be data in the buffer
        // from a previous readData call.
        if(_zlibStruct->_zlibStream.avail_in == 0) {
            qint64 bytesAvalible = _device->read(reinterpret_cast<char*>(_buffer.get()), _bufferSize);
            _zlibStruct->_zlibStream.next_in = _buffer.get();
            _zlibStruct->_zlibStream.avail_in = bytesAvalible;

            if(bytesAvalible == -1) {
                _state = Error;
                setErrorString(tr("Error reading data from underlying device: %1").arg(_device->errorString()));
                return -1;
            }

            if(_state != InStream) {
                // If we are not in a stream and get 0 bytes, we are probably trying to read from an empty device.
                if(bytesAvalible == 0)
                    return 0;
                else if(bytesAvalible > 0)
                    _state = InStream;
            }
        }

        // Decompress.
        status = ::inflate(&_zlibStruct->_zlibStream, Z_SYNC_FLUSH);
        switch(status) {
            case Z_NEED_DICT:
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                _state = Error;
                setZlibError(tr("Internal zlib error when decompressing: "), status);
                return -1;
            case Z_BUF_ERROR: // No more input and zlib can not privide more output - Not an error, we can try to read again when we have more input.
                return 0;
        }
    // Loop until data buffer is full or we reach the end of the input stream.
    } 
    while(_zlibStruct->_zlibStream.avail_out != 0 && status != Z_STREAM_END);

    if(status == Z_STREAM_END) {
        _state = EndOfStream;

        // Unget any data left in the read buffer.
        for(int i = _zlibStruct->_zlibStream.avail_in;  i >= 0; --i)
            _device->ungetChar(*reinterpret_cast<char*>(_zlibStruct->_zlibStream.next_in + i));
    }

    const ZlibSize outputSize = maxSize - _zlibStruct->_zlibStream.avail_out;
	return outputSize;
}


/*!
    Compresses and writes data to the underlying device.
*/
qint64 GzipIODevice::writeData(const char* data, qint64 maxSize)
{
    if(maxSize < 1)
        return 0;
    _zlibStruct->_zlibStream.next_in = reinterpret_cast<ZlibByte*>(const_cast<char*>(data));
    _zlibStruct->_zlibStream.avail_in = maxSize;

    if(_state == Error)
        return -1;

    do {
        _zlibStruct->_zlibStream.next_out = _buffer.get();
        _zlibStruct->_zlibStream.avail_out = _bufferSize;
        const int status = ::deflate(&_zlibStruct->_zlibStream, Z_NO_FLUSH);
        if(status != Z_OK) {
            _state = Error;
            setZlibError(tr("Internal zlib error when compressing: "), status);
            return -1;
        }

        ZlibSize outputSize = _bufferSize - _zlibStruct->_zlibStream.avail_out;

        // Try to write data from the buffer to to the underlying device, return -1 on failure.
        if(writeBytes(outputSize) == false)
            return -1;

    } 
    while(_zlibStruct->_zlibStream.avail_out == 0); // run until output is not full.
    OVITO_ASSERT(_zlibStruct->_zlibStream.avail_in == 0);

    return maxSize;
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
