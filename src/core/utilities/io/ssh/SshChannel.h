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

#pragma once

#include <core/Core.h>
#include <libssh/libssh.h>

namespace Ovito { namespace Ssh {

class SshConnection;

class SshChannel : public QIODevice
{
    Q_OBJECT

public:

    /// Constructor.
    explicit SshChannel(SshConnection* connection, QObject* parent, bool isStderr = false);

    /// Returns true if the current read and write position is at the end of the device.
    virtual bool atEnd() const override;

    /// Returns the number of bytes that are available for reading. 
    virtual qint64 bytesAvailable() const override;

    /// This function returns the number of bytes waiting to be written.
    virtual qint64 bytesToWrite() const override;

    /// Returns whether this device is sequential.
    virtual bool isSequential() const override { return true; }

    /// Returns whether a complete line of data can be read from the device.
    virtual bool canReadLine() const override;

    /// Sends EOF to the channel once write buffer has been written to the channel.
    void sendEof();

    /// Gets the error message from libssh.
    QString errorMessage() const;

protected:

    /// Reads bytes from the device into data, and returns the number of bytes read.
    virtual qint64 readData(char* data, qint64 maxlen) override;

    /// Writes bytes of data to the device. 
    virtual qint64 writeData(const char* data, qint64 len) override;

    /// Performs data I/O.
    void checkIO();

    /// Requests a I/O operation.
    virtual void queueCheckIO() = 0;

    /// Returns the underlying SSH connection.
    SshConnection* connection() const { return _connection; }

    /// Returns the libssh channel.
    const ssh_channel& channel() const { return _channel; }

protected:

    enum EofState {
        EofNotSent,
        EofQueued,
        EofSent
    };
    
    /// The underlying SSH connection.
    SshConnection* _connection;

    /// The libssh channel.
    ssh_channel _channel = nullptr;

    bool _isStderr;
    EofState _eofState = EofNotSent;

    int _bufferSize = 1024 * 16;
    int _writeSize = 1024 * 16;
    QByteArray _readBuffer;
    QByteArray _writeBuffer;

    bool _ioInProgress = false;
};

} // End of namespace
} // End of namespace
