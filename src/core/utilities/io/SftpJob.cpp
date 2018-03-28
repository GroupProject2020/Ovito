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
#include <3rdparty/ssh_wrapper/scpchannel.h>
#include <3rdparty/ssh_wrapper/lschannel.h>
#include "SftpJob.h"

#include <QTimer>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(IO) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

using namespace Ovito::Ssh;

/// List SFTP jobs that are waiting to be executed.
QQueue<SftpJob*> SftpJob::_queuedJobs;

/// Tracks of how many jobs are currently active.
int SftpJob::_numActiveJobs = 0;

/// The maximum number of simultaneous jobs at a time.
constexpr int MaximumNumberOfSimulateousJobs = 2;

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
	connect(_promiseWatcher, &PromiseWatcher::canceled, this, &SftpJob::connectionCanceled);
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
	connect(_connection, &SshConnection::error, this, &SftpJob::connectionError);
	connect(_connection, &SshConnection::canceled, this, &SftpJob::connectionCanceled);
	connect(_connection, &SshConnection::allAuthsFailed, this, &SftpJob::authenticationFailed);
	if(_connection->isConnected()) {
		QTimer::singleShot(0, this, &SftpJob::connectionEstablished);
		return;
	}
	connect(_connection, &SshConnection::connected, this, &SftpJob::connectionEstablished);

	// Start to connect.
	_connection->connectToHost();
}

/******************************************************************************
* Closes the SSH connection.
******************************************************************************/
void SftpJob::shutdown(bool success)
{
	if(_promiseWatcher) {		
		_promiseWatcher->reset();
		disconnect(_promiseWatcher, 0, this, 0);
		_promiseWatcher->deleteLater();
	}
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

	// If there jobs waiting in the queue, execute next job.
	if(!_queuedJobs.isEmpty() && _numActiveJobs < MaximumNumberOfSimulateousJobs) {
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
void SftpJob::connectionError()
{
	_promiseState->setException(std::make_exception_ptr(
		Exception(tr("Cannot access URL\n\n%1\n\nSSH connection error: %2").arg(_url.toString(QUrl::RemovePassword | QUrl::PreferLocalFile | QUrl::PrettyDecoded)).
			arg(_connection->errorMessage()))));

	shutdown(false);
}

/******************************************************************************
* Handles SSH authentication errors.
******************************************************************************/
void SftpJob::authenticationFailed()
{
	_promiseState->setException(std::make_exception_ptr(
		Exception(tr("Cannot access URL\n\n%1\n\nSSH authentication failed").arg(_url.toString(QUrl::RemovePassword | QUrl::PreferLocalFile | QUrl::PrettyDecoded)))));

	shutdown(false);
}

/******************************************************************************
* Handles SSH connection cancelation by user.
******************************************************************************/
void SftpJob::connectionCanceled()
{
	// If use has canceled the SSH connection, 
	// cancel the file retrievel operation as well.
	_promiseState->cancel();
	shutdown(false);
}

/******************************************************************************
* Is called when the SSH connection has been established.
******************************************************************************/
void SftpDownloadJob::connectionEstablished()
{
	if(_promiseState->isCanceled()) {
		shutdown(false);
		return;
	}

	// Open the SCP channel.
	_promiseState->setProgressText(tr("Opening SCP channel to remote host %1").arg(_connection->hostname()));
	_scpChannel = new ScpChannel(_connection, _url.path());
	connect(_scpChannel, &ScpChannel::receivingFile, this, &SftpDownloadJob::receivingFile);
	connect(_scpChannel, &ScpChannel::receivedData, this, &SftpDownloadJob::receivedData);
	connect(_scpChannel, &ScpChannel::receivedFileComplete, this, &SftpDownloadJob::receivedFileComplete);
	connect(_scpChannel, &ScpChannel::error, this, &SftpDownloadJob::channelError);
	connect(_scpChannel, &ScpChannel::closed, this, &SftpDownloadJob::connectionCanceled);
	_scpChannel->openChannel();
}

/******************************************************************************
* Is called when the SCP channel failed.
******************************************************************************/
void SftpDownloadJob::channelError()
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
void SftpDownloadJob::shutdown(bool success)
{
	// Close SCP channel.
	if(_scpChannel) {
		disconnect(_scpChannel, 0, this, 0);
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
	SftpJob::shutdown(success);

	// Hand downloaded file over to FileManager cache.
	Application::instance()->fileManager()->fileFetched(_url, _localFile.take());
}

/******************************************************************************
* Is called when the remote host starts sending the file.
******************************************************************************/
void SftpDownloadJob::receivingFile(qint64 fileSize)
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
void SftpDownloadJob::receivedFileComplete() 
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
void SftpDownloadJob::receivedData(qint64 totalReceivedBytes)
{
	_promiseState->setProgressValue(std::min((qint64)std::numeric_limits<int>::max(), totalReceivedBytes / 1024));
	if(_promiseState->isCanceled()) {
		shutdown(false);
	}
}

/******************************************************************************
* Is called when the SSH connection has been established.
******************************************************************************/
void SftpListDirectoryJob::connectionEstablished()
{
	if(_promiseState->isCanceled()) {
		shutdown(false);
		return;
	}

	// Open the SCP channel.
	_promiseState->setProgressText(tr("Opening channel to remote host %1").arg(_connection->hostname()));
	_lsChannel = new LsChannel(_connection, _url.path());
	connect(_lsChannel, &LsChannel::error, this, &SftpListDirectoryJob::channelError);
	connect(_lsChannel, &LsChannel::receivingDirectory, this, &SftpListDirectoryJob::receivingDirectory);
	connect(_lsChannel, &LsChannel::receivedDirectoryComplete, this, &SftpListDirectoryJob::receivedDirectoryComplete);
	connect(_lsChannel, &LsChannel::closed, this, &SftpListDirectoryJob::connectionCanceled);
	_lsChannel->openChannel();
}

/******************************************************************************
* Is called before transmission of the directory listing begins.
******************************************************************************/
void SftpListDirectoryJob::receivingDirectory()
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
void SftpListDirectoryJob::channelError()
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
void SftpListDirectoryJob::receivedDirectoryComplete(const QStringList& listing)
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
void SftpListDirectoryJob::shutdown(bool success)
{
	if(_lsChannel) {
		disconnect(_lsChannel, 0, this, 0);
		_lsChannel->closeChannel();
		_lsChannel->deleteLater();
		_lsChannel = nullptr;
	}

	SftpJob::shutdown(success);
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
