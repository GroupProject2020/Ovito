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

#include <QObject>
#include <QFile>
#include <QByteArray>

#include <libssh/sftp.h>

namespace Ovito { namespace Ssh {

class SshConnection;

class SftpChannel : public QObject
{
    Q_OBJECT

public:

    /// Constructor.
    SftpChannel(SshConnection* parent);

    /// Destructor.
    ~SftpChannel() { cleanup(); }

    /// Initializes the SFTP channel.
    void initialize();

    /// Downloads a file from the remote server to a local directory.
    qint64 downloadFile(const QString& remotePath, QFile* destination);

    /// Retrieves the list of files in the given directory.
    void listDirectory(const QString& remotePath);

    /// Returns the list of files in the directory after a successful call to listDirectory().  
    const QStringList& directoryListing() const { return _directoryListing; }

Q_SIGNALS:

    /// This signal is generated when the channel is fully initialized.
    void initialized();

    /// This signal is generated when an error has occurred.
    void channelError(const QString& reason);

    /// This signal is generated when new data was received from the remote host.
    void progress(qint64 nbytes);

    /// This signal is generated when the file transfer has been completed.
    void finished();

private Q_SLOTS:

    /// Closes the SFTP transfer.
    void cleanup();

    /// Is called when the SSH connection has received an update.
    void processState();

private:

    /// The underlying SSH connection.
    SshConnection* _connection;

    /// The SFTP session handle.
    sftp_session _sftp = nullptr;

    /// The SFTP file handle.
    sftp_file _sftpFile = nullptr;

    /// The local destination file.
    QFile* _destinationFile = nullptr;

    /// The receive buffer.
    QByteArray _buffer;

    /// The libssh asynchronous file read request.
    int _asyncReadRequest = 0;

    /// The number of bytes received so far. 
    qint64 _receivedBytes = 0;

    /// The list of files in the requested directory.
    QStringList _directoryListing;
};

} // End of namespace
} // End of namespace
