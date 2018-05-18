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
#include <core/utilities/concurrent/PromiseWatcher.h>
#include <core/utilities/io/FileManager.h>
#include <core/app/Application.h>
#include <core/utilities/io/ssh/SshConnection.h>
#include <core/utilities/io/ssh/ScpChannel.h>
#include <core/utilities/io/ssh/LsChannel.h>
#include <core/utilities/io/ssh/CatChannel.h>
#include "RemoteFileJob.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(IO) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

using namespace Ovito::Ssh;

/// List SFTP jobs that are waiting to be executed.
QQueue<RemoteFileJob*> RemoteFileJob::_queuedJobs;

/// Tracks of how many jobs are currently active.
int RemoteFileJob::_numActiveJobs = 0;

/// The maximum number of simultaneous jobs at a time.
constexpr int MaximumNumberOfSimulateousJobs = 2;

/******************************************************************************
* Constructor.
******************************************************************************/
RemoteFileJob::RemoteFileJob(QUrl url, PromiseStatePtr promiseState) :
		_url(std::move(url)), _promiseState(std::move(promiseState))
{
	// Run all event handlers of this class in the main thread.
	moveToThread(QCoreApplication::instance()->thread());

	// Start download process in the main thread.
	QMetaObject::invokeMethod(this, "start", Qt::AutoConnection);
}

/******************************************************************************
* Opens the SSH connection.
******************************************************************************/
void RemoteFileJob::start()
{
	if(!_isActive) {
		// Keep a counter of active jobs.
		// If there are too many jobs active simultaneously, queue them to be executed later.
		if(_numActiveJobs >= MaximumNumberOfSimulateousJobs) {
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
	connect(_promiseWatcher, &PromiseWatcher::canceled, this, &RemoteFileJob::connectionCanceled);
	_promiseWatcher->watch(_promiseState);

	SshConnectionParameters connectionParams;
	connectionParams.host = _url.host();
	connectionParams.userName = _url.userName();
	connectionParams.password = _url.password();
	connectionParams.port = _url.port(0);

	_promiseState->setProgressText(tr("Connecting to remote host %1").arg(connectionParams.host));

	// Open connection
	_connection = Application::instance()->fileManager()->acquireSshConnection(connectionParams);
	OVITO_CHECK_POINTER(_connection);

	// Listen for signals of the connection.
	connect(_connection, &SshConnection::error, this, &RemoteFileJob::connectionError);
	connect(_connection, &SshConnection::canceled, this, &RemoteFileJob::connectionCanceled);
	connect(_connection, &SshConnection::allAuthsFailed, this, &RemoteFileJob::authenticationFailed);
	if(_connection->isConnected()) {
		QTimer::singleShot(0, this, &RemoteFileJob::connectionEstablished);
		return;
	}
	connect(_connection, &SshConnection::connected, this, &RemoteFileJob::connectionEstablished);

	// Start to connect.
	_connection->connectToHost();
}

/******************************************************************************
* Closes the SSH connection.
******************************************************************************/
void RemoteFileJob::shutdown(bool success)
{
	if(_promiseWatcher) {
		_promiseWatcher->reset();
		disconnect(_promiseWatcher, nullptr, this, nullptr);
		_promiseWatcher->deleteLater();
	}
	if(_connection) {
		disconnect(_connection, nullptr, this, nullptr);
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

	// If there jobs waiting in the queue, execute next job.
	if(!_queuedJobs.isEmpty() && _numActiveJobs < MaximumNumberOfSimulateousJobs) {
		RemoteFileJob* waitingJob = _queuedJobs.dequeue();
		if(!waitingJob->_promiseState->isCanceled()) {
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
void RemoteFileJob::connectionError()
{
	_promiseState->setException(std::make_exception_ptr(
		Exception(tr("Cannot access URL\n\n%1\n\nSSH connection error: %2").arg(_url.toString(QUrl::RemovePassword | QUrl::PreferLocalFile | QUrl::PrettyDecoded)).
			arg(_connection->errorMessage()))));

	shutdown(false);
}

/******************************************************************************
* Handles SSH authentication errors.
******************************************************************************/
void RemoteFileJob::authenticationFailed()
{
	_promiseState->setException(std::make_exception_ptr(
		Exception(tr("Cannot access URL\n\n%1\n\nSSH authentication failed").arg(_url.toString(QUrl::RemovePassword | QUrl::PreferLocalFile | QUrl::PrettyDecoded)))));

	shutdown(false);
}

/******************************************************************************
* Handles SSH connection cancelation by user.
******************************************************************************/
void RemoteFileJob::connectionCanceled()
{
	// If use has canceled the SSH connection, 
	// cancel the file retrievel operation as well.
	_promiseState->cancel();
	shutdown(false);
}

/******************************************************************************
* Handles closed SSH channel.
******************************************************************************/
void DownloadRemoteFileJob::channelClosed()
{
	if(!_promiseState->isFinished()) {
		_promiseState->setException(std::make_exception_ptr(
			Exception(tr("Cannot access URL\n\n%1\n\nSSH channel closed: %2").arg(_url.toString(QUrl::RemovePassword | QUrl::PreferLocalFile | QUrl::PrettyDecoded)).
				arg(_scpChannel->errorMessage()))));
	}

	shutdown(false);
}

/******************************************************************************
* Is called when the SSH connection has been established.
******************************************************************************/
void DownloadRemoteFileJob::connectionEstablished()
{
	if(_promiseState->isCanceled()) {
		shutdown(false);
		return;
	}

	// Open the SCP channel.
	_promiseState->setProgressText(tr("Opening SCP channel to remote host %1").arg(_connection->hostname()));
	_scpChannel = new ScpChannel(_connection, _url.path());
	connect(_scpChannel, &ScpChannel::receivingFile, this, &DownloadRemoteFileJob::receivingFile);
	connect(_scpChannel, &ScpChannel::receivedData, this, &DownloadRemoteFileJob::receivedData);
	connect(_scpChannel, &ScpChannel::receivedFileComplete, this, &DownloadRemoteFileJob::receivedFileComplete);
	connect(_scpChannel, &ScpChannel::error, this, &DownloadRemoteFileJob::channelError);
	connect(_scpChannel, &ScpChannel::closed, this, &DownloadRemoteFileJob::channelClosed);
	_scpChannel->openChannel();
}

/******************************************************************************
* Is called when the SCP channel failed.
******************************************************************************/
void DownloadRemoteFileJob::channelError()
{
	_promiseState->setException(std::make_exception_ptr(
		Exception(tr("Cannot access remote URL\n\n%1\n\n%2")
			.arg(_url.toString(QUrl::RemovePassword | QUrl::PreferLocalFile | QUrl::PrettyDecoded))
			.arg(_scpChannel->errorMessage()))));
	
	shutdown(false);
}

/******************************************************************************
* Closes the SSH connection.
******************************************************************************/
void DownloadRemoteFileJob::shutdown(bool success)
{
	// Close file channel.
	if(_scpChannel) {
		disconnect(_scpChannel, nullptr, this, nullptr);
		_scpChannel->closeChannel();
		_scpChannel->deleteLater();
		_scpChannel = nullptr;
	}

	// Close local file and clean up.
	if(_localFile) {
		if(_fileMapping) {
			// Make sure the received data was successfully written to the temporary file.
			if(!_localFile->unmap(_fileMapping) || !_localFile->flush() || _localFile->error() != QFileDevice::NoError) {
				_promiseState->setException(std::make_exception_ptr(Exception(
					tr("Failed to write to local file %1: %2").arg(_localFile->fileName()).arg(_localFile->errorString()))));
				success = false;
			}
			_fileMapping = nullptr;
		}
		_localFile->close();
	}
	if(_localFile && success)
		_promise.setResults(_localFile->fileName());
	else
		_localFile.reset();

	// Close SSH connection.
	RemoteFileJob::shutdown(success);

	// Hand downloaded file over to FileManager cache.
	Application::instance()->fileManager()->fileFetched(_url, _localFile.take());
}

/******************************************************************************
* Is called when the remote host starts sending the file.
******************************************************************************/
void DownloadRemoteFileJob::receivingFile(qint64 fileSize)
{
	if(_promiseState->isCanceled()) {
		shutdown(false);
		return;
	}
	_promiseState->setProgressMaximum(std::min((qint64)std::numeric_limits<int>::max(), fileSize / 1024));
	_promiseState->setProgressText(tr("Fetching remote file %1").arg(_url.toString(QUrl::RemovePassword | QUrl::PreferLocalFile | QUrl::PrettyDecoded)));

	// Create the destination file.
	try {
		_localFile.reset(new QTemporaryFile());
		if(!_localFile->open() || !_localFile->resize(fileSize))
			throw Exception(tr("Failed to create temporary file: %1").arg(_localFile->errorString()));

		// Map the file to memory and let the SCP channel write the received data to the memory buffer.
		if(fileSize) {
			_fileMapping = _localFile->map(0, fileSize);
			if(!_fileMapping)
				throw Exception(tr("Failed to map temporary file to memory: %1").arg(_localFile->errorString()));
		}
		_scpChannel->setDestinationBuffer(reinterpret_cast<char*>(_fileMapping));
	}
    catch(Exception&) {
		_promiseState->captureException();
		shutdown(false);
	}
}

/******************************************************************************
* Is called after the file has been downloaded.
******************************************************************************/
void DownloadRemoteFileJob::receivedFileComplete() 
{
	if(_promiseState->isCanceled()) {
		shutdown(false);
		return;
	}
    shutdown(true);
}

/******************************************************************************
* Is called when the remote host sent some file data.
******************************************************************************/
void DownloadRemoteFileJob::receivedData(qint64 totalReceivedBytes)
{
	if(_promiseState->isCanceled()) {
		shutdown(false);
		return;
	}
	_promiseState->setProgressValue(std::min((qint64)std::numeric_limits<int>::max(), totalReceivedBytes / 1024));
}

/******************************************************************************
* Is called when the SSH connection has been established.
******************************************************************************/
void ListRemoteDirectoryJob::connectionEstablished()
{
	if(_promiseState->isCanceled()) {
		shutdown(false);
		return;
	}

	// Open the SCP channel.
	_promiseState->setProgressText(tr("Opening channel to remote host %1").arg(_connection->hostname()));
	_lsChannel = new LsChannel(_connection, _url.path());
	connect(_lsChannel, &LsChannel::error, this, &ListRemoteDirectoryJob::channelError);
	connect(_lsChannel, &LsChannel::receivingDirectory, this, &ListRemoteDirectoryJob::receivingDirectory);
	connect(_lsChannel, &LsChannel::receivedDirectoryComplete, this, &ListRemoteDirectoryJob::receivedDirectoryComplete);
	connect(_lsChannel, &LsChannel::closed, this, &ListRemoteDirectoryJob::channelClosed);
	_lsChannel->openChannel();
}

/******************************************************************************
* Is called before transmission of the directory listing begins.
******************************************************************************/
void ListRemoteDirectoryJob::receivingDirectory()
{
	if(_promiseState->isCanceled()) {
		shutdown(false);
		return;
	}

	// Set progress text.
	_promiseState->setProgressText(tr("Listing remote directory %1").arg(_url.toString(QUrl::RemovePassword | QUrl::PreferLocalFile | QUrl::PrettyDecoded)));
}

/******************************************************************************
* Is called when the SCP channel failed.
******************************************************************************/
void ListRemoteDirectoryJob::channelError()
{
	_promiseState->setException(std::make_exception_ptr(
		Exception(tr("Cannot access remote URL\n\n%1\n\n%2")
			.arg(_url.toString(QUrl::RemovePassword | QUrl::PreferLocalFile | QUrl::PrettyDecoded))
			.arg(_lsChannel->errorMessage()))));
	
	shutdown(false);
}

/******************************************************************************
* Is called after the directory listing has been fully transmitted.
******************************************************************************/
void ListRemoteDirectoryJob::receivedDirectoryComplete(const QStringList& listing)
{
	if(_promiseState->isCanceled()) {
		shutdown(false);
		return;
	}

	_promise.setResults(listing);
    shutdown(true);
}

/******************************************************************************
* Closes the SSH connection.
******************************************************************************/
void ListRemoteDirectoryJob::shutdown(bool success)
{
	if(_lsChannel) {
		disconnect(_lsChannel, nullptr, this, nullptr);
		_lsChannel->closeChannel();
		_lsChannel->deleteLater();
		_lsChannel = nullptr;
	}

	RemoteFileJob::shutdown(success);
}

/******************************************************************************
* Handles closed SSH channel.
******************************************************************************/
void ListRemoteDirectoryJob::channelClosed()
{
	if(!_promiseState->isFinished()) {
		_promiseState->setException(std::make_exception_ptr(
			Exception(tr("Cannot access URL\n\n%1\n\nSSH channel closed: %2").arg(_url.toString(QUrl::RemovePassword | QUrl::PreferLocalFile | QUrl::PrettyDecoded)).
				arg(_lsChannel->errorMessage()))));
	}

	shutdown(false);
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
