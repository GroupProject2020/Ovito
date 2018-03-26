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

#include "sftpchannel.h"
#include "sshconnection.h"

#include <QDebug>
#include <fcntl.h>

namespace Ovito { namespace Ssh {

/******************************************************************************
* Constructor.
******************************************************************************/
SftpChannel::SftpChannel(SshConnection* parent) : QObject(parent), _connection(parent)
{
    connect(_connection, &SshConnection::doCleanup, this, &SftpChannel::cleanup);
}

/******************************************************************************
* Initializes the SFTP channel.
******************************************************************************/
void SftpChannel::initialize()
{
    if(!_connection->_sftp) {
        Q_EMIT channelError(tr("Failed to create SFTP session."));
        return;
    }

    if(::sftp_init(_connection->_sftp) < 0) {        
        Q_EMIT channelError(tr("Failed to initialize SFTP session: %1").arg(::sftp_get_error(_connection->_sftp)));
        return;
    }

    Q_EMIT initialized();    
}

/******************************************************************************
* Closes the SFTP transfer.
******************************************************************************/
void SftpChannel::cleanup()
{
    if(_sftpFile) {
        ::sftp_close(_sftpFile);
        _sftpFile = nullptr;
    }
    _destinationFile = nullptr;
}

/******************************************************************************
* Downloads a file from the remote server to a local directory.
******************************************************************************/
void SftpChannel::downloadFile(const QString& remotePath, QFile* destination)
{
    Q_ASSERT(!_sftpFile);
    Q_ASSERT(!_destinationFile);

    _sftpFile = ::sftp_open(_connection->_sftp, qPrintable(remotePath), O_RDONLY, 0);
    if(!_sftpFile) {
        Q_EMIT channelError(_connection->errorMessage());
        return;
    }
    ::sftp_file_set_nonblocking(_sftpFile);

    _destinationFile = destination;
    _buffer.

    async_request = ::sftp_async_read_begin(_sftpFile, sizeof(buffer));

    Q_EMIT finished();
}

} // End of namespace
} // End of namespace
