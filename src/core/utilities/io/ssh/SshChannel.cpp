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
#include "SshChannel.h"
#include "SshConnection.h"

namespace Ovito { namespace Ssh {

/******************************************************************************
* Constructor.
******************************************************************************/
SshChannel::SshChannel(SshConnection* connection, QObject* parent, bool isStderr) : 
    QIODevice(parent), _connection(connection), _isStderr(isStderr)
{
}

/******************************************************************************
* Returns true if the current read and write position is at the end of the device.
******************************************************************************/
bool SshChannel::atEnd() const
{
    return !_channel || isOpen() == false || (_readBuffer.isEmpty() 
        && (!::ssh_channel_is_open(_channel) || ::ssh_channel_poll(_channel, _isStderr) == SSH_EOF));
}

/******************************************************************************
* Returns the number of bytes that are available for reading. 
******************************************************************************/
qint64 SshChannel::bytesAvailable() const
{
    return _readBuffer.size() + QIODevice::bytesAvailable();
}

/******************************************************************************
* This function returns the number of bytes waiting to be written.
******************************************************************************/
qint64 SshChannel::bytesToWrite() const
{
    return _writeBuffer.size();
}

/******************************************************************************
* Returns whether a complete line of data can be read from the device.
******************************************************************************/
bool SshChannel::canReadLine() const
{
    return QIODevice::canReadLine() ||
           _readBuffer.contains('\n') ||
           _readBuffer.size() >= _bufferSize ||
            (!_readBuffer.isEmpty() && (!isOpen() || !_channel ||
                !::ssh_channel_is_open(_channel) ||
                ::ssh_channel_poll(_channel, _isStderr) == SSH_EOF));
}

/******************************************************************************
* Reads bytes from the device into data, and returns the number of bytes read.
******************************************************************************/
qint64 SshChannel::readData(char* data, qint64 maxlen)
{
    queueCheckIO();

    qint64 copy_len = maxlen;
    int data_size = _readBuffer.size();

    if(copy_len > data_size)
        copy_len = data_size;

    memcpy(data, _readBuffer.constData(), copy_len);
    _readBuffer.remove(0, copy_len);
    return copy_len;
}

/******************************************************************************
* Writes bytes of data to the device.
******************************************************************************/
qint64 SshChannel::writeData(const char* data, qint64 len)
{
    if(openMode() == ReadOnly) {
        qCritical() << "Cannot write to SSH channel because ReadOnly flag is set.";
        return 0;
    } 
    else if(_eofState != EofNotSent) {
        qCritical() << "Cannot write to SSH channel because EOF state is" << _eofState;
        return 0;
    } 
    else {
        _connection->enableWritableSocketNotifier();
        _writeBuffer.reserve(_writeSize);
        _writeBuffer.append(data, len);
        return len;
    }
}

/******************************************************************************
* Performs data I/O.
******************************************************************************/
void SshChannel::checkIO()
{
    if(!channel() || _ioInProgress) return;
    _ioInProgress = true;

    bool emit_ready_read = false;
    bool emit_bytes_written = false;

    int read_size = 0;
    int read_available = ::ssh_channel_poll(channel(), _isStderr);
    if(read_available > 0) {

        // Dont read more than buffer size specifies.
        int max_read = _bufferSize - _readBuffer.size();
        if(read_available > max_read) {
            read_available = max_read;
        }

        if(read_available > 0) {
            char data[read_available + 1];
            data[read_available] = 0;

            read_size = ::ssh_channel_read_nonblocking(channel(), data, read_available, _isStderr);
            if(read_size < 0) {
                qWarning() << "ssh_channel_read_nonblocking() returned negative value.";
                _ioInProgress = false;
                return;
            }

            _readBuffer.reserve(_bufferSize);
            _readBuffer.append(data, read_size);

            if(read_size > 0)
                emit_ready_read = true;
        }
    }

    int written = 0;
    int writable = 0;
    if(openMode() != ReadOnly) {

        writable = _writeBuffer.size();
        if (writable > _writeSize)
            writable = _writeSize;

        if(writable > 0) {
            written = ::ssh_channel_write(channel(), _writeBuffer.constData(), writable);
            OVITO_ASSERT(written >= 0);
            _writeBuffer.remove(0, written);

            if(written > 0)
                emit_bytes_written = true;
        }

        // Write more data once the socket is ready.
        if(_writeBuffer.size() > 0)
            _connection->enableWritableSocketNotifier();
    }

    // Send EOF once all data has been written to channel.
    if(_eofState == EofQueued && _writeBuffer.size() == 0) {
        ::ssh_channel_send_eof(channel());
        _eofState = EofSent;
    }

    // Emit signals here, so that somebody wont call closeChannel() while
    // when we are reading from it.
    if(emit_ready_read) {
        Q_EMIT readyRead();
    }
    if(emit_bytes_written) {
        Q_EMIT bytesWritten(written);
    }
    _ioInProgress = false;
}

/******************************************************************************
* Sends EOF to the channel once write buffer has been written to the channel.
******************************************************************************/
void SshChannel::sendEof()
{
    if(_eofState == EofNotSent) {
        _eofState = EofQueued;
    }
}

/******************************************************************************
* Gets the error message from libssh.
******************************************************************************/
QString SshChannel::errorMessage() const
{
    if(connection()->_state == SshConnection::StateError) {
        return connection()->errorMessage();
    }
    if(!QIODevice::errorString().isEmpty()) {
        return QIODevice::errorString();
    }
    if(connection()->_session && ::ssh_get_error_code(connection()->_session) != SSH_NO_ERROR) {
        QString msg(::ssh_get_error(connection()->_session));
        if(!msg.isEmpty()) return msg;        
    }
    if(_channel && ::ssh_get_error_code(_channel) != SSH_NO_ERROR) {
        QString msg(::ssh_get_error(_channel));
        if(!msg.isEmpty()) return msg;        
    }
    return {};
}

} // End of namespace
} // End of namespace
