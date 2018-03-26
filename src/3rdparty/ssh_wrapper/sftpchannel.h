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
    void downloadFile(const QString& remotePath, QFile* destination);

Q_SIGNALS:

    /// This signal is generated when the channel is fully initialized.
    void initialized();

    /// This signal is generated when an error has occurred.
    void channelError(const QString& reason);

    /// This signal is generated when the file transfer has been completed.
    void finished();

private Q_SLOTS:

    /// Closes the SFTP transfer.
    void cleanup();

private:

    /// The underlying SSH connection.
    SshConnection* _connection;

    /// The SFTP file handle.
    sftp_file _sftpFile = nullptr;

    /// The local destination file.
    QFile* _destinationFile = nullptr;

    /// The receive buffer.
    QByteArray _buffer;
};

} // End of namespace
} // End of namespace
