////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2018 Alexander Stukowski
//
//  This file is part of OVITO (Open Visualization Tool).
//
//  OVITO is free software; you can redistribute it and/or modify it either under the
//  terms of the GNU General Public License version 3 as published by the Free Software
//  Foundation (the "GPL") or, at your option, under the terms of the MIT License.
//  If you do not alter this notice, a recipient may use your version of this
//  file under either the GPL or the MIT License.
//
//  You should have received a copy of the GPL along with this program in a
//  file LICENSE.GPL.txt.  You should have received a copy of the MIT License along
//  with this program in a file LICENSE.MIT.txt
//
//  This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND,
//  either express or implied. See the GPL or the MIT License for the specific language
//  governing rights and limitations.
//
////////////////////////////////////////////////////////////////////////////////////////

#pragma once


#include <ovito/core/Core.h>
#include <ovito/core/utilities/concurrent/Future.h>

#include <QQueue>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(IO) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

namespace Ssh {
	// These classes are defined elsewhere:
	class SshConnection;
	class ScpChannel;
	class LsChannel;
	class CatChannel;
}

/**
 * \brief Base class for background jobs that access remote files and directories via SSH.
 */
class RemoteFileJob : public QObject
{
	Q_OBJECT

public:

	/// Constructor.
	RemoteFileJob(QUrl url, TaskPtr promiseState);

	/// Returns the URL being accessed.
	const QUrl& url() const { return _url; }

protected:

	/// Opens the SSH connection.
	Q_INVOKABLE void start();

	/// Closes the SSH connection.
	virtual void shutdown(bool success);

protected Q_SLOTS:

	/// Handles SSH connection errors.
	void connectionError();

	/// Handles SSH authentication errors.
	void authenticationFailed();

	/// Handles SSH connection cancelation by user.
	void connectionCanceled();

	/// Is called when the SSH connection has been established.
    virtual void connectionEstablished() = 0;

protected:

	/// The URL of the file or directory.
	const QUrl _url;

	/// The SSH connection.
	Ovito::Ssh::SshConnection* _connection = nullptr;

    /// The associated future interface of the job.
    TaskPtr _promiseState;

	/// This is for listening to signals from the promise object.
	TaskWatcher* _promiseWatcher = nullptr;

    /// Indicates whether this job is currently active.
    bool _isActive = false;

    /// Queue of jobs that are waiting to be executed.
    static QQueue<RemoteFileJob*> _queuedJobs;

    /// Keeps track of how many jobs are currently active.
    static int _numActiveJobs;
};

/**
 * \brief A background jobs that downloads a file stored on a remote host to the local computer.
 */
class DownloadRemoteFileJob : public RemoteFileJob
{
	Q_OBJECT

public:

	/// Constructor.
	DownloadRemoteFileJob(QUrl url, Promise<QString>&& promise) :
		RemoteFileJob(std::move(url), promise.task()), _promise(std::move(promise)) {}

protected:

	/// Closes the SSH connection.
	virtual void shutdown(bool success) override;

	/// Is called when the SSH connection has been established.
    virtual void connectionEstablished() override;

protected Q_SLOTS:

    /// Is called when the remote host starts sending the file.
	void receivingFile(qint64 fileSize);

    /// Is called when the remote host sent some file data.
	void receivedData(qint64 totalReceivedBytes);

	/// Is called after the file has been downloaded.
	void receivedFileComplete();

    /// Is called when an SCP error occurs in the channel.
    void channelError();

	/// Handles SSH channel close.
	void channelClosed();

private:

	/// The SCP channel.
    Ovito::Ssh::ScpChannel* _scpChannel = nullptr;

    /// The local copy of the file.
    QScopedPointer<QTemporaryFile> _localFile;

	/// The memory-mapped destination file.
	uchar* _fileMapping = nullptr;

	/// The promise through which the result of this download job is returned.
	Promise<QString> _promise;
};

/**
 * \brief A background jobs that lists the files in a directory on a remote host.
 */
class ListRemoteDirectoryJob : public RemoteFileJob
{
	Q_OBJECT

public:

	/// Constructor.
	ListRemoteDirectoryJob(QUrl url, Promise<QStringList>&& promise) :
		RemoteFileJob(std::move(url), promise.task()), _promise(std::move(promise)) {}

protected:

	/// Closes the SSH connection.
	virtual void shutdown(bool success) override;

	/// Is called when the SSH connection has been established.
    virtual void connectionEstablished() override;

protected Q_SLOTS:

    /// Is called before transmission of the directory listing begins.
    void receivingDirectory();

    /// Is called after the directory listing has been fully transmitted.
    void receivedDirectoryComplete(const QStringList& listing);

    /// Is called when an error occurs in the SSH channel.
    void channelError();

	/// Handles SSH channel close.
	void channelClosed();

private:

	/// The listing channel.
    Ovito::Ssh::LsChannel* _lsChannel = nullptr;

	/// The promise through which the result of this job is returned.
	Promise<QStringList> _promise;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
