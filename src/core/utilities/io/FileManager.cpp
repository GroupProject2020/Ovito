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
#include <core/utilities/io/ssh/SshConnection.h>
#include "FileManager.h"
#include "RemoteFileJob.h"

#include <QTemporaryFile>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(IO)

using namespace Ovito::Ssh;

/******************************************************************************
* Destructor.
******************************************************************************/
FileManager::~FileManager()
{
    for(SshConnection* connection : _unacquiredConnections) {
        disconnect(connection, nullptr, this, nullptr);
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
		if(!QFileInfo(url.toLocalFile()).exists())
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
			SharedFuture<QString> future = inProgressEntry->second.lock();
			if(future.isValid())
				return future;
			else
				_pendingFiles.erase(inProgressEntry);
		}

		// Start the background download job.
		Promise<QString> promise = taskManager.createSynchronousPromise<QString>(false);
		SharedFuture<QString> future(promise.future());
		_pendingFiles.emplace(normalizedUrl, future);
		new DownloadRemoteFileJob(url, std::move(promise));
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
		Promise<QStringList> promise = taskManager.createSynchronousPromise<QStringList>(false);
		Future<QStringList> future = promise.future();
		new ListRemoteDirectoryJob(url, std::move(promise));
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
	QUrl normalizedUrl = normalizeUrl(std::move(url));
	QMutexLocker lock(&_mutex);

	auto inProgressEntry = _pendingFiles.find(normalizedUrl);
	if(inProgressEntry != _pendingFiles.end())
		_pendingFiles.erase(inProgressEntry);

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
    connect(connection, &SshConnection::unknownHost, this, &FileManager::unknownSshServer);
	connect(connection, &SshConnection::needPassword, this, &FileManager::needSshPassword);
	connect(connection, &SshConnection::needKbiAnswers, this, &FileManager::needKbiAnswers);
	connect(connection, &SshConnection::authFailed, this, &FileManager::sshAuthenticationFailed);
	connect(connection, &SshConnection::needPassphrase, this, &FileManager::needSshPassphrase);
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
        disconnect(connection, nullptr, this, nullptr);
        connection->deleteLater();
    }
    else {
        Q_ASSERT(!_unacquiredConnections.contains(connection));
        _unacquiredConnections.append(connection);
    }
}

/******************************************************************************
*  Is called whenever an SSH connection is closed.
******************************************************************************/
void FileManager::cleanupSshConnection()
{
    SshConnection* connection = qobject_cast<SshConnection*>(sender());
    if(!connection)
        return;

    if(_unacquiredConnections.removeOne(connection)) {
        disconnect(connection, nullptr, this, nullptr);
        connection->deleteLater();
    }
}

/******************************************************************************
* Is called whenever a SSH connection to an yet unknown server is being established.
******************************************************************************/
void FileManager::unknownSshServer()
{
    SshConnection* connection = qobject_cast<SshConnection*>(sender());
    if(!connection)
        return;

	if(detectedUnknownSshServer(connection->hostname(), connection->unknownHostMessage(), connection->hostPublicKeyHash())) {
		if(connection->markCurrentHostKnown())
			return;
	}
	connection->cancel();
} 

/******************************************************************************
* Informs the user about an unknown SSH host.
******************************************************************************/
bool FileManager::detectedUnknownSshServer(const QString& hostname, const QString& unknownHostMessage, const QString& hostPublicKeyHash)
{
	std::cout << "OVITO is connecting to remote host '" << qPrintable(hostname) << "' via SSH." << std::endl;
	std::cout << qPrintable(unknownHostMessage) << std::endl;
	std::cout << "Host key fingerprint is " << qPrintable(hostPublicKeyHash) << std::endl;
	std::cout << "Are you sure you want to continue connecting (yes/no)? " << std::flush;
	std::string reply;
	std::cin >> reply;
	return reply == "yes";
}

/******************************************************************************
* Is called when an authentication attempt for a SSH connection failed.
******************************************************************************/
void FileManager::sshAuthenticationFailed(int auth)
{
    SshConnection* connection = qobject_cast<SshConnection*>(sender());
    if(!connection)
        return;
	
	SshConnection::AuthMethods supported = connection->supportedAuthMethods();
	if(auth & SshConnection::UseAuthPassword && supported & SshConnection::AuthMethodPassword) {
        connection->usePasswordAuth(true);
	}
	else if(auth & SshConnection::UseAuthKbi && supported & SshConnection::AuthMethodKbi) {
        connection->useKbiAuth(true);
	}
}

/******************************************************************************
* Is called whenever a SSH connection to a server requires password authentication.
******************************************************************************/
void FileManager::needSshPassword()
{
    SshConnection* connection = qobject_cast<SshConnection*>(sender());
    if(!connection)
        return;

	QString password = connection->password();
	if(askUserForPassword(connection->hostname(), connection->username(), password)) {		
		connection->setPassword(password);
	}
	else {
		connection->cancel();
	}
}

/******************************************************************************
* Is called whenever a SSH connection to a server requires keyboard interactive authentication.
******************************************************************************/
void FileManager::needKbiAnswers()
{
    SshConnection* connection = qobject_cast<SshConnection*>(sender());
    if(!connection)
        return;

	QStringList answers;
	for(const SshConnection::KbiQuestion& question : connection->kbiQuestions()) {
		QString answer;
		if(askUserForKbiResponse(connection->hostname(), connection->username(), question.instruction, question.question, question.showAnswer, answer)) {
			answers << answer;
		}
		else {
			connection->cancel();
			return;
		}
	}
	connection->setKbiAnswers(std::move(answers));
}

/******************************************************************************
* Asks the user for the login password for a SSH server.
******************************************************************************/
bool FileManager::askUserForPassword(const QString& hostname, const QString& username, QString& password)
{
	std::string pw;
	std::cout << "Please enter the password for user '" << qPrintable(username) << "' ";
	std::cout << "on SSH remote host '" << qPrintable(hostname) << "' (set echo off beforehand!): " << std::flush;	
	std::cin >> pw;
	password = QString::fromStdString(pw);
	return true;
}

/******************************************************************************
* Asks the user for the answer to a keyboard-interactive question sent by the SSH server.
******************************************************************************/
bool FileManager::askUserForKbiResponse(const QString& hostname, const QString& username, const QString& instruction, const QString& question, bool showAnswer, QString& answer)
{
	std::cout << "SSH keyboard interactive authentication";
	if(!showAnswer)
		std::cout << " (set echo off beforehand!)";
	std::cout << " - " << qPrintable(question) << std::flush;
	std::string pw;
	std::cin >> pw;
	answer = QString::fromStdString(pw);
	return true;
}

/******************************************************************************
* Is called whenever a private SSH key requires a passphrase.
******************************************************************************/
void FileManager::needSshPassphrase(const QString& prompt)
{
    SshConnection* connection = qobject_cast<SshConnection*>(sender());
    if(!connection)
        return;

	QString passphrase;
	if(askUserForKeyPassphrase(connection->hostname(), prompt, passphrase)) {
		connection->setPassphrase(passphrase);
	}
}

/******************************************************************************
* Asks the user for the passphrase for a private SSH key.
******************************************************************************/
bool FileManager::askUserForKeyPassphrase(const QString& hostname, const QString& prompt, QString& passphrase)
{
	std::string pp;
	std::cout << qPrintable(prompt) << std::flush;	
	std::cin >> pp;
	passphrase = QString::fromStdString(pp);
	return true;
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
