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


#include <core/Core.h>
#include <core/utilities/concurrent/Future.h>

#include <QQueue>
#include <QTemporaryFile>

namespace Ovito { namespace Ssh {
	class SshConnection;
	class SftpChannel;
}}

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(IO) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/**
 * \brief Base class for background jobs that access remote files and directories via SFTP.
 */
class SftpJob : public QObject
{
	Q_OBJECT

public:

	/// Constructor.
	SftpJob(const QUrl& url, const PromiseStatePtr& promiseState);

	/// Destructor.
	virtual ~SftpJob() {
		OVITO_ASSERT(_sftpChannel == nullptr);
		OVITO_ASSERT(_connection == nullptr);
	}

	/// Returns the URL being accessed.
	const QUrl& url() const { return _url; }

	/// Changes the URL being accessed.
	void setUrl(const QUrl& url) { _url = url; }

protected:

	/// Opens the SSH connection.
	Q_INVOKABLE void start();

	/// Closes the SSH connection.
	virtual void shutdown(bool success);

protected Q_SLOTS:

	/// Handles SSH connection errors.
	void onSshConnectionError();

	/// Handles SSH authentication errors.
	void onSshConnectionAuthenticationFailed();

	/// Handles SSH connection cancelation by user.
	void onSshConnectionCanceled();

	/// Is called when the SSH connection has been established.
    void onSshConnectionEstablished();

    /// Is called when an SFTP error occurs.
    void onSftpChannelError(const QString& reason);

    /// Is called when the SFTP channel has been created.
	virtual void onSftpChannelInitialized() = 0;

protected:

	/// The URL of the file or directory.
	QUrl _url;

	/// The SSH connection.
	Ovito::Ssh::SshConnection* _connection = nullptr;

	/// The SFTP channel.
    Ovito::Ssh::SftpChannel* _sftpChannel = nullptr;

    /// The associated future interface of the job.
    PromiseStatePtr _promiseState;

	/// This is for listening to signals from the promise object.
	PromiseWatcher* _promiseWatcher = nullptr;

    /// Indicates whether this SFTP job is currently active.
    bool _isActive = false;

    /// Queue of SFTP jobs that are waiting to be executed.
    static QQueue<SftpJob*> _queuedJobs;

    /// Keeps track of how many SFTP jobs are currently active.
    static int _numActiveJobs;
};

/**
 * \brief A background jobs that downloads a remote file via SFTP.
 */
class SftpDownloadJob : public SftpJob
{
	Q_OBJECT

public:

	/// Constructor.
	SftpDownloadJob(const QUrl& url, Promise<QString>&& promise) :
		SftpJob(url, promise.sharedState()), _promise(std::move(promise)) {}

protected:

	/// Closes the SSH connection.
	virtual void shutdown(bool success) override;

    /// Is called when the SFTP channel has been created.
	virtual void onSftpChannelInitialized() override;

protected Q_SLOTS:

	/// Is called after the file has been downloaded.
	void onSftpJobFinished();

	/// Is called when the file info for the requested file arrived.
//	void onFileInfoAvailable(QSsh::SftpJobId job, const QList<QSsh::SftpFileInfo>& fileInfoList);

private:

    /// The local copy of the file.
    QScopedPointer<QTemporaryFile> _localFile;

	/// The promise through which the result of this download job is returned.
	Promise<QString> _promise;
};

/**
 * \brief A background jobs that lists the files in a remote directory, which is accessed via SFTP.
 */
class SftpListDirectoryJob : public SftpJob
{
	Q_OBJECT

public:

	/// Constructor.
	SftpListDirectoryJob(const QUrl& url, Promise<QStringList>&& promise) :
		SftpJob(url, promise.sharedState()), _promise(std::move(promise)) {}

protected:

    /// Is called when the SFTP channel has been created.
	virtual void onSftpChannelInitialized() override;

protected Q_SLOTS:

	/// Is called after the file has been downloaded.
//	void onSftpJobFinished(QSsh::SftpJobId jobId, const QString& errorMessage);

	/// Is called when the file info for the requested file arrived.
//	void onFileInfoAvailable(QSsh::SftpJobId job, const QList<QSsh::SftpFileInfo>& fileInfoList);

private:

    /// The list of files.
    QStringList _fileList;

    /// The SFTP job.
    //QSsh::SftpJobId _listingJob;

	/// The promise through which the result of this download job is returned.
	Promise<QStringList> _promise;	
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
