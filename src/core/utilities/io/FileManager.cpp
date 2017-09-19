///////////////////////////////////////////////////////////////////////////////
// 
//  Copyright (2013) Alexander Stukowski
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
#include "FileManager.h"
#include "SftpJob.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(IO)

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
		QMutexLocker lock(&_mutex);

		QUrl normalizedUrl = normalizeUrl(url);

		// Check if requested URL is already in the cache.
		auto cacheEntry = _cachedFiles.find(normalizedUrl);
		if(cacheEntry != _cachedFiles.end()) {
			return cacheEntry.value()->fileName();
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

	auto cacheEntry = _cachedFiles.find(normalizeUrl(url));
	if(cacheEntry != _cachedFiles.end()) {
		cacheEntry.value()->deleteLater();
		_cachedFiles.erase(cacheEntry);
	}
}

/******************************************************************************
* Is called when a remote file has been fetched.
******************************************************************************/
void FileManager::fileFetched(QUrl url, QTemporaryFile* localFile)
{
	QMutexLocker lock(&_mutex);

	QUrl normalizedUrl = normalizeUrl(url);

	auto inProgressEntry = _pendingFiles.find(normalizedUrl);
	if(inProgressEntry != _pendingFiles.end())
		_pendingFiles.erase(inProgressEntry);
	else
		OVITO_ASSERT(false);

	if(localFile) {
		// Store downloaded file in local cache.
		auto cacheEntry = _cachedFiles.find(normalizedUrl);
		if(cacheEntry != _cachedFiles.end())
			cacheEntry.value()->deleteLater();
		OVITO_ASSERT(localFile->thread() == this->thread());
		localFile->setParent(this);
		_cachedFiles[normalizedUrl] = localFile;
	}
}

/******************************************************************************
* Looks up login name and password for the given host in the credential cache.
******************************************************************************/
QPair<QString,QString> FileManager::findCredentials(const QString& host)
{
	QMutexLocker lock(&_mutex);
	auto loginInfo = _credentialCache.find(host);
	if(loginInfo != _credentialCache.end())
		return loginInfo.value();
	else
		return qMakePair(QString(), QString());
}

/******************************************************************************
* Saves the login name and password for the given host in the credential cache.
******************************************************************************/
void FileManager::cacheCredentials(const QString& host, const QString& username, const QString& password)
{
	QMutexLocker lock(&_mutex);
	_credentialCache.insert(host, qMakePair(username, password));
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
