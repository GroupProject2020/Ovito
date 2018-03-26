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
#include <core/utilities/io/FileManager.h>
#include <core/app/Application.h>
#include <3rdparty/ssh_wrapper/sshconnection.h>
#include "SftpJob.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(IO) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

using namespace Ovito::Ssh;

/// List SFTP jobs that are waiting to be executed.
QQueue<SftpJob*> SftpJob::_queuedJobs;

/// Keeps track of how many SFTP jobs are currently active.
int SftpJob::_numActiveJobs = 0;

enum { MaximumNumberOfSimulateousSftpJobs = 2 };

/******************************************************************************
* Constructor.
******************************************************************************/
SftpJob::SftpJob(const QUrl& url, const PromiseStatePtr& promiseState) :
		_url(url), _promiseState(promiseState)
{
	// Run all event handlers of this class in the main thread.
	moveToThread(QCoreApplication::instance()->thread());

	// Start download process in the main thread.
	QMetaObject::invokeMethod(this, "start", Qt::AutoConnection);
}

/******************************************************************************
* Opens the SSH connection.
******************************************************************************/
void SftpJob::start()
{
	if(!_isActive) {
		// Keep a counter of active jobs.
		// If there are too many jobs active simultaneously, queue them to be executed later.
		if(_numActiveJobs >= MaximumNumberOfSimulateousSftpJobs) {
			_queuedJobs.enqueue(this);
			return;
		}
		else {
			_numActiveJobs++;
			_isActive = true;
		}
	}

	// This background task started to run.
	_promiseState->setStarted();

	// Check if process has already been canceled.
	if(_promiseState->isCanceled()) {
		shutdown(false);
		return;
	}

	// Get notified if user has canceled the promise.
	_promiseWatcher = new PromiseWatcher(this);
	connect(_promiseWatcher, &PromiseWatcher::canceled, this, &SftpJob::onSshConnectionCanceled);
	_promiseWatcher->watch(_promiseState);

	SshConnectionParameters connectionParams;
	connectionParams.host = _url.host();
	connectionParams.userName = _url.userName();
	connectionParams.password = _url.password();
	connectionParams.port = _url.port(0);

	_promiseState->setProgressText(tr("Connecting to remote server %1").arg(connectionParams.host));

	// Open connection
	_connection = Application::instance()->fileManager()->acquireSshConnection(connectionParams);
	OVITO_CHECK_POINTER(_connection);

	// Listen for signals of the connection.
	connect(_connection, &SshConnection::error, this, &SftpJob::onSshConnectionError);
	connect(_connection, &SshConnection::canceled, this, &SftpJob::onSshConnectionCanceled);
	connect(_connection, &SshConnection::allAuthsFailed, this, &SftpJob::onSshConnectionAuthenticationFailed);
	if(_connection->isConnected()) {
		onSshConnectionEstablished();
		return;
	}
	connect(_connection, &SshConnection::connected, this, &SftpJob::onSshConnectionEstablished);

	// Start to connect.
	_connection->connectToHost();
}

/******************************************************************************
* Closes the SSH connection.
******************************************************************************/
void SftpJob::shutdown(bool success)
{
	if(_promiseWatcher) {		
		disconnect(_promiseWatcher, 0, this, 0);
		_promiseWatcher->deleteLater();
	}
#if 0
	if(_sftpChannel) {
		QObject::disconnect(_sftpChannel.data(), 0, this, 0);
		_sftpChannel->closeChannel();
		_sftpChannel.clear();
	}
#endif
	if(_connection) {
		disconnect(_connection, 0, this, 0);
		Application::instance()->fileManager()->releaseSshConnection(_connection);
		_connection = nullptr;
	}

	_promiseState->setFinished();

	// Update the counter of active jobs.
	if(_isActive) {
		_numActiveJobs--;
		_isActive = false;
	}

	// Schedule this object for deletion.
	deleteLater();

	// If there are now fewer active jobs, execute one of the waiting jobs.
	if(_numActiveJobs < MaximumNumberOfSimulateousSftpJobs && !_queuedJobs.isEmpty()) {
		SftpJob* waitingJob = _queuedJobs.dequeue();
		if(waitingJob->_promiseState->isCanceled() == false) {
			waitingJob->start();
		}
		else {
			// Skip canceled jobs.
			waitingJob->_promiseState->setStarted();
			waitingJob->shutdown(false);
		}
	}
}

/******************************************************************************
* Handles SSH connection errors.
******************************************************************************/
void SftpJob::onSshConnectionError()
{
	_promiseState->setException(std::make_exception_ptr(
		Exception(tr("Cannot access URL\n\n%1\n\nSSH connection error: %2").arg(_url.toString(QUrl::RemovePassword | QUrl::PreferLocalFile | QUrl::PrettyDecoded)).
			arg(_connection->errorMessage()))));

	shutdown(false);
}

/******************************************************************************
* Handles SSH authentication errors.
******************************************************************************/
void SftpJob::onSshConnectionAuthenticationFailed()
{
	_promiseState->setException(std::make_exception_ptr(
		Exception(tr("Cannot access URL\n\n%1\n\nSSH authentication failed").arg(_url.toString(QUrl::RemovePassword | QUrl::PreferLocalFile | QUrl::PrettyDecoded)))));

	shutdown(false);
}

/******************************************************************************
* Handles SSH connection cancelation by user.
******************************************************************************/
void SftpJob::onSshConnectionCanceled()
{
	_promiseState->cancel();
	shutdown(false);
}

/******************************************************************************
* Is called when the SSH connection has been established.
******************************************************************************/
void SftpJob::onSshConnectionEstablished()
{
	if(_promiseState->isCanceled()) {
		shutdown(false);
		return;
	}

	_promiseState->setProgressText(tr("Opening SFTP file transfer channel"));

#if 0
	_sftpChannel = _connection->createSftpChannel();
	connect(_sftpChannel.data(), &QSsh::SftpChannel::initialized, this, &SftpJob::onSftpChannelInitialized);
	connect(_sftpChannel.data(), &QSsh::SftpChannel::channelError, this, &SftpJob::onSftpChannelError);
	_sftpChannel->initialize();
#endif
}

/******************************************************************************
* Is called when the SFTP channel could not be created.
******************************************************************************/
void SftpJob::onSftpChannelError(const QString& reason)
{
	_promiseState->setException(std::make_exception_ptr(
		Exception(tr("Cannot access URL\n\n%1\n\nSFTP error: %2").arg(_url.toString(QUrl::RemovePassword | QUrl::PreferLocalFile | QUrl::PrettyDecoded)).arg(reason))));
	shutdown(false);
}

/******************************************************************************
* Closes the SSH connection.
******************************************************************************/
void SftpDownloadJob::shutdown(bool success)
{
	if(_timerId)
		killTimer(_timerId);

	if(_localFile && success)
		_promise.setResults(_localFile->fileName());
	else
		_localFile.reset();

	SftpJob::shutdown(success);

	Application::instance()->fileManager()->fileFetched(_url, _localFile.take());
}

/******************************************************************************
* Is called when the SFTP channel has been created.
******************************************************************************/
void SftpDownloadJob::onSftpChannelInitialized()
{
	if(_promiseState->isCanceled()) {
		shutdown(false);
		return;
	}
#if 0
	connect(_sftpChannel.data(), &QSsh::SftpChannel::finished, this, &SftpDownloadJob::onSftpJobFinished);
	connect(_sftpChannel.data(), &QSsh::SftpChannel::fileInfoAvailable, this, &SftpDownloadJob::onFileInfoAvailable);
	try {

		// Set progress text.
		_promiseState->setProgressText(tr("Fetching remote file %1").arg(_url.toString(QUrl::RemovePassword | QUrl::PreferLocalFile | QUrl::PrettyDecoded)));

		// Create temporary file.
		_localFile.reset(new QTemporaryFile());
		if(!_localFile->open())
			throw Exception(tr("Failed to create temporary file: %1").arg(_localFile->errorString()));
		_localFile->close();

		// Request file info.
		_sftpChannel->statFile(_url.path());

		// Start to download file.
		_downloadJob = _sftpChannel->downloadFile(_url.path(), _localFile->fileName(), QSsh::SftpOverwriteExisting);
		if(_downloadJob == QSsh::SftpInvalidJob)
			throw Exception(tr("Failed to download remote file %1.").arg(_url.toString(QUrl::RemovePassword | QUrl::PreferLocalFile | QUrl::PrettyDecoded)));

		// Start timer to monitor download progress.
		_timerId = startTimer(500);
	}
    catch(Exception&) {
		_promiseState->captureException();
		shutdown(false);
	}
#endif
}

#if 0
/******************************************************************************
* Is called after the file has been downloaded.
******************************************************************************/
void SftpDownloadJob::onSftpJobFinished(QSsh::SftpJobId jobId, const QString& errorMessage) 
{
	if(jobId != _downloadJob)
		return;

	if(_promiseState->isCanceled()) {
		shutdown(false);
		return;
	}
    if(!errorMessage.isEmpty()) {
		_promiseState->setException(std::make_exception_ptr(Exception(tr("Cannot access URL\n\n%1\n\nSFTP error: %2")
					.arg(_url.toString(QUrl::RemovePassword | QUrl::PreferLocalFile | QUrl::PrettyDecoded))
					.arg(errorMessage))));
		shutdown(false);
		return;
    }
    shutdown(true);
}

/******************************************************************************
* Is called when the file info for the requested file arrived.
******************************************************************************/
void SftpDownloadJob::onFileInfoAvailable(QSsh::SftpJobId job, const QList<QSsh::SftpFileInfo>& fileInfoList)
{
	if(fileInfoList.empty() == false ) {
		if(fileInfoList[0].sizeValid) {
			_promiseState->setProgressMaximum(fileInfoList[0].size / 1000);
		}
	}
}
#endif

/******************************************************************************
* Is invoked when the QObject's timer fires.
******************************************************************************/
void SftpDownloadJob::timerEvent(QTimerEvent* event)
{
	SftpJob::timerEvent(event);

	if(_localFile) {
		qint64 size = _localFile->size();
		if(size >= 0 && _promiseState->progressMaximum() > 0) {
			_promiseState->setProgressValue(size / 1000);
		}
    	if(_promiseState->isCanceled())
    		shutdown(false);
	}
}

/******************************************************************************
* Is called when the SFTP channel has been created.
******************************************************************************/
void SftpListDirectoryJob::onSftpChannelInitialized()
{
	if(_promiseState->isCanceled()) {
		shutdown(false);
		return;
	}

#if 0
	connect(_sftpChannel.data(), &QSsh::SftpChannel::finished, this, &SftpListDirectoryJob::onSftpJobFinished);
	connect(_sftpChannel.data(), &QSsh::SftpChannel::fileInfoAvailable, this, &SftpListDirectoryJob::onFileInfoAvailable);
	try {
		// Set progress text.
		_promiseState->setProgressText(tr("Listing remote directory %1").arg(_url.toString(QUrl::RemovePassword | QUrl::PreferLocalFile | QUrl::PrettyDecoded)));

		// Request file list.
		_listingJob = _sftpChannel->listDirectory(_url.path());
		if(_listingJob == QSsh::SftpInvalidJob)
			throw Exception(tr("Failed to list contents of remote directory %1.").arg(_url.toString(QUrl::RemovePassword | QUrl::PreferLocalFile | QUrl::PrettyDecoded)));
	}
    catch(Exception&) {
		_promiseState->captureException();
		shutdown(false);
	}
#endif
}

#if 0
/******************************************************************************
* Is called after the file has been downloaded.
******************************************************************************/
void SftpListDirectoryJob::onSftpJobFinished(QSsh::SftpJobId jobId, const QString& errorMessage) 
{
	if(jobId != _listingJob)
		return;

	if(_promiseState->isCanceled()) {
		shutdown(false);
		return;
	}
    if(!errorMessage.isEmpty()) {
    	_promiseState->setException(std::make_exception_ptr(Exception(tr("Cannot access URL\n\n%1\n\nSFTP error: %2")
					.arg(_url.toString(QUrl::RemovePassword | QUrl::PreferLocalFile | QUrl::PrettyDecoded))
					.arg(errorMessage))));
		shutdown(false);
		return;
    }

	_promise.setResults(_fileList);
    shutdown(true);
}

/******************************************************************************
* Is called when the file info for the requested file arrived.
******************************************************************************/
void SftpListDirectoryJob::onFileInfoAvailable(QSsh::SftpJobId job, const QList<QSsh::SftpFileInfo>& fileInfoList)
{
	for(const QSsh::SftpFileInfo& fileInfo : fileInfoList) {
		if(fileInfo.type == QSsh::FileTypeRegular)
			_fileList.push_back(fileInfo.name);
	}
}
#endif

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
