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
#include <core/utilities/concurrent/Future.h>
#include <core/utilities/concurrent/TaskManager.h>
#include <core/dataset/DataSetContainer.h>
#include <3rdparty/ssh_wrapper/sshconnection.h>
#include "FileManager.h"
#include "SftpJob.h"

#include <QTemporaryFile>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(IO)

using namespace Ovito::Ssh;

/******************************************************************************
* Constructor.
******************************************************************************/
FileManager::FileManager()
{
}

/******************************************************************************
* Destructor.
******************************************************************************/
FileManager::~FileManager()
{
    for(SshConnection* connection : _unacquiredConnections) {
        disconnect(connection, 0, this, 0);
        delete connection;
    }
    Q_ASSERT(_acquiredConnections.empty());
}

/******************************************************************************
* Makes a file available on this computer.
******************************************************************************/
SharedFuture<QString> FileManager::fetchUrl(TaskManager& taskManager, const QUrl& url)
{
	if(url.isLocalFile()) {
		// Nothing to do to fetch local files. Simply return a finished Future object.

		// But first check if the file exists.
		QString filePath = url.toLocalFile();
		if(QFileInfo(url.toLocalFile()).exists() == false)
			return Future<QString>::createFailed(Exception(tr("File does not exist:\n%1").arg(filePath), &taskManager.datasetContainer()));

		return filePath;
	}
	else if(url.scheme() == QStringLiteral("sftp")) {
		QUrl normalizedUrl = normalizeUrl(url);
		QMutexLocker lock(&_mutex);

		// Check if requested URL is already in the cache.
		if(auto cacheEntry = _cachedFiles.object(normalizedUrl)) {
			return cacheEntry->fileName();
		}

		// Check if requested URL is already being loaded.
		auto inProgressEntry = _pendingFiles.find(normalizedUrl);
		if(inProgressEntry != _pendingFiles.end()) {
			return inProgressEntry->second;
		}

		// Start the background download job.
		Promise<QString> promise = Promise<QString>::createSynchronous(&taskManager, false, true);
		SharedFuture<QString> future(promise.future());
		_pendingFiles.emplace(normalizedUrl, future);
		new SftpDownloadJob(url, std::move(promise));
		return future;
	}
	else {
		return Future<QString>::createFailed(Exception(tr("URL scheme not supported. The program supports only the sftp:// scheme and local file paths."), &taskManager.datasetContainer()));
	}
}

/******************************************************************************
* Lists all files in a remote directory.
******************************************************************************/
Future<QStringList> FileManager::listDirectoryContents(TaskManager& taskManager, const QUrl& url)
{
	if(url.scheme() == QStringLiteral("sftp")) {
		Promise<QStringList> promise = Promise<QStringList>::createSynchronous(&taskManager, false, false);
		Future<QStringList> future = promise.future();
		new SftpListDirectoryJob(url, std::move(promise));
		return future;
	}
	else {
		return Future<QStringList>::createFailed(Exception(tr("URL scheme not supported. The program supports only the sftp:// scheme and local file paths."), &taskManager.datasetContainer()));
	}
}

/******************************************************************************
* Removes a cached remote file so that it will be downloaded again next
* time it is requested.
******************************************************************************/
void FileManager::removeFromCache(const QUrl& url)
{
	QMutexLocker lock(&_mutex);
	_cachedFiles.remove(normalizeUrl(url));
}

/******************************************************************************
* Is called when a remote file has been fetched.
******************************************************************************/
void FileManager::fileFetched(QUrl url, QTemporaryFile* localFile)
{
	QUrl normalizedUrl = normalizeUrl(url);
	QMutexLocker lock(&_mutex);

	auto inProgressEntry = _pendingFiles.find(normalizedUrl);
	if(inProgressEntry != _pendingFiles.end())
		_pendingFiles.erase(inProgressEntry);
	else
		OVITO_ASSERT(false);

	if(localFile) {
		// Store downloaded file in local cache.
		OVITO_ASSERT(localFile->thread() == this->thread());
		localFile->setParent(this);
		if(!_cachedFiles.insert(normalizedUrl, localFile, 0))
			throw Exception(tr("Failed to insert downloaded file into file cache."));
	}
}

/******************************************************************************
* Constructs a URL from a path entered by the user.
******************************************************************************/
QUrl FileManager::urlFromUserInput(const QString& path)
{
	if(path.startsWith(QStringLiteral("sftp://")))
		return QUrl(path);
	else
		return QUrl::fromLocalFile(path);
}

/******************************************************************************
* Create a new SSH connection or returns an existing connection having the same parameters.
******************************************************************************/
SshConnection* FileManager::acquireSshConnection(const SshConnectionParameters& sshParams)
{
    OVITO_ASSERT(QThread::currentThread() == QCoreApplication::instance()->thread());

    // Check in-use connections:
    for(SshConnection* connection : _acquiredConnections) {
        if(connection->connectionParameters() != sshParams)
            continue;

        _acquiredConnections.append(connection);
        return connection;
    }

    // Check cached open connections:
    for(SshConnection* connection : _unacquiredConnections) {
        if(!connection->isConnected() || connection->connectionParameters() != sshParams)
            continue;

        _unacquiredConnections.removeOne(connection);
        _acquiredConnections.append(connection);
        return connection;
    }

    // Create a new connection:
    SshConnection* const connection = new SshConnection(sshParams);
    connect(connection, &SshConnection::disconnected, this, &FileManager::cleanupSshConnection);
    connect(connection, &SshConnection::unknownHost, this, &FileManager::cleanupSshConnection);
    _acquiredConnections.append(connection);

    return connection;
}

/******************************************************************************
* Releases an SSH connection after it is no longer used.
******************************************************************************/
void FileManager::releaseSshConnection(SshConnection* connection)
{
    OVITO_ASSERT(QThread::currentThread() == QCoreApplication::instance()->thread());

    bool wasAquired = _acquiredConnections.removeOne(connection);
    OVITO_ASSERT(wasAquired);
    if(_acquiredConnections.contains(connection))
        return;

    if(!connection->isConnected()) {
        disconnect(connection, 0, this, 0);
        connection->deleteLater();
    }
    else {
        connection->disconnectFromHost(); // Clean up after neglectful clients.
        Q_ASSERT(!_unacquiredConnections.contains(connection));
        _unacquiredConnections.append(connection);
    }
}

/******************************************************************************
*  Is called whenever an SSH connection is closed.
******************************************************************************/
void FileManager::cleanupSshConnection()
{
    SshConnection* currentConnection = qobject_cast<SshConnection*>(sender());
    if(!currentConnection)
        return;

    if(_unacquiredConnections.removeOne(currentConnection)) {
        disconnect(currentConnection, 0, this, 0);
        currentConnection->deleteLater();
    }
}

/******************************************************************************
*  Is called whenever a SSH connection to an yet unknown server is being established.
******************************************************************************/
void FileManager::unknownSshServer()
{
    SshConnection* currentConnection = qobject_cast<SshConnection*>(sender());
    if(!currentConnection)
        return;

	
} 

/******************************************************************************
* Shows a dialog which asks the user for the login credentials.
******************************************************************************/
bool FileManager::askUserForCredentials(QUrl& url)
{
	std::string username;
	std::string password;
	std::cout << "Please enter the SSH username for the remote machine '" << qPrintable(url.host()) << "': " << std::flush;
	std::cin >> username;
	std::cout << "Please enter the SSH password (set echo off beforehand!): " << std::flush;
	std::cin >> password;
	url.setUserName(QString::fromStdString(username));
	url.setPassword(QString::fromStdString(password));
	return true;
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
