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
    connect(_connection, &SshConnection::doProcessState, this, &SftpChannel::processState);
    connect(_connection, &SshConnection::doCleanup, this, &SftpChannel::cleanup);
}

/******************************************************************************
* Initializes the SFTP channel.
******************************************************************************/
void SftpChannel::initialize()
{
    // Switch SSH session to blocking mode. SFTP functions do not support non-blocking mode.
    ::ssh_set_blocking(_connection->_session, 1);
    
    // Create the SFTP session.
    _sftp = ::sftp_new(_connection->_session);
    if(!_sftp) {
        Q_EMIT channelError(tr("Failed to create SFTP session."));
        return;
    }

    // Initialize the SFTP session.
    if(::sftp_init(_sftp) < 0) {
        Q_EMIT channelError(tr("Failed to initialize SFTP session."));
        return;
    }
    
    Q_EMIT initialized();    
}

/******************************************************************************
* Closes the SFTP transfer.
******************************************************************************/
void SftpChannel::cleanup()
{
    // Close SFTP file.
    if(_sftpFile) {
        ::sftp_close(_sftpFile);
        _sftpFile = nullptr;
    }
    // Close SFTP session.
    if(_sftp) {
        ::sftp_free(_sftp);
        _sftp = nullptr;
    }
    _destinationFile = nullptr;
}

/******************************************************************************
* Downloads a file from the remote server to a local directory.
******************************************************************************/
qint64 SftpChannel::downloadFile(const QString& remotePath, QFile* destination)
{
    Q_ASSERT(_sftp);
    Q_ASSERT(!_sftpFile);
    Q_ASSERT(!_destinationFile);

    _sftpFile = ::sftp_open(_sftp, qPrintable(remotePath), O_RDONLY, 0);
    if(!_sftpFile) {
        Q_EMIT channelError(_connection->errorMessage());
        return -1;
    }

    // Determine size of remote file.
    sftp_attributes file_attr = ::sftp_fstat(_sftpFile);	
    if(!file_attr) {
        Q_EMIT channelError(tr("Failed to determine size of remote file: %1").arg(_connection->errorMessage()));
        cleanup();
        return -1;
    }
    qint64 fileSize = file_attr->size;
    ::sftp_attributes_free(file_attr);

    ::sftp_file_set_nonblocking(_sftpFile);

    _receivedBytes = 0;
    _destinationFile = destination;
    _buffer.resize(0x10000);

    if(!_destinationFile->isOpen()) {
        if(!_destinationFile->open(QIODevice::WriteOnly)) {
            Q_EMIT channelError(tr("Failed to open local file for writing: %1").arg(_destinationFile->errorString()));
            cleanup();
            return -1;
        }
    }

    _asyncReadRequest = ::sftp_async_read_begin(_sftpFile, _buffer.size());
    if(_asyncReadRequest < 0) {
        Q_EMIT channelError(_connection->errorMessage());
        cleanup();
        return -1;
    }

    return fileSize;
}

/******************************************************************************
* Is called when the SSH connection has received an update.
******************************************************************************/
void SftpChannel::processState()
{
    if(_asyncReadRequest <= 0) return;

    auto nbytes = ::sftp_async_read(_sftpFile, _buffer.data(), _buffer.size(), _asyncReadRequest);
    if(nbytes == SSH_AGAIN) {
        // Try again at a later time.
        return;
    }
    else if(nbytes == SSH_ERROR) {
        _asyncReadRequest = 0;
        Q_EMIT channelError(_connection->errorMessage());
        cleanup();
    }
    else if(nbytes <= 0) {
        _asyncReadRequest = 0;
        Q_EMIT finished();
    }
    else {
        _asyncReadRequest = 0;
         qDebug() << "SftpChannel::processState() SSH_OK nbytes=" << nbytes << ":" << this;
        if(_destinationFile->write(_buffer.data(), nbytes) != nbytes) {
            Q_EMIT channelError(tr("Failed to write received data to local file: %1").arg(_destinationFile->errorString()));
            cleanup();
        }
        else {
            _receivedBytes += nbytes;
            Q_EMIT progress(_receivedBytes);
            _asyncReadRequest = ::sftp_async_read_begin(_sftpFile, _buffer.size());
            if(_asyncReadRequest < 0) {
                Q_EMIT channelError(_connection->errorMessage());
                cleanup();
            }
        }
    }
}

/******************************************************************************
* Retrieves the list of files in the given directory.
******************************************************************************/
void SftpChannel::listDirectory(const QString& remotePath)
{
    Q_ASSERT(_sftp);
    Q_ASSERT(!_sftpFile);

    sftp_dir dir = ::sftp_opendir(_sftp, qPrintable(remotePath));
    if(!dir) {
        Q_EMIT channelError(_connection->errorMessage());
        return;
    }

    _directoryListing.clear();
    while(sftp_attributes attributes = ::sftp_readdir(_sftp, dir)) {
        if(attributes->type == 1)
            _directoryListing.push_back(QString::fromUtf8(attributes->name));
        ::sftp_attributes_free(attributes);
    }

    if(!::sftp_dir_eof(dir)) {
        ::sftp_closedir(dir);
        Q_EMIT channelError(tr("Failed to list remote directory: %1").arg(_connection->errorMessage()));
        return;
    }

    ::sftp_closedir(dir);

    Q_EMIT finished();
}

} // End of namespace
} // End of namespace
