////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2020 Alexander Stukowski
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
#include <ovito/core/utilities/concurrent/SharedFuture.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(IO)

namespace Ssh {
	// These classes are defined elsewhere:
	class SshConnection;
	struct SshConnectionParameters;
}

/**
 * \brief A handle to a file manages by the FileManager.
 */
class OVITO_CORE_EXPORT FileHandle 
{
public:

	/// Default constructor creating an invalid file handle.
	FileHandle() = default;

	/// Constructor for files located in the local file system.
	explicit FileHandle(const QUrl& sourceUrl, const QString& localFilePath) : _sourceUrl(sourceUrl), _localFilePath(localFilePath) {}

	/// Returns the URL denoting the source location of the data file.
	const QUrl& sourceUrl() const { return _sourceUrl; }

	/// Returns the path to the file in the local file system (may be empty). 
	const QString& localFilePath() const { return _localFilePath; }

	/// Create a QIODevice that permits reading data from the file referred to by this handle.
	std::unique_ptr<QIODevice> createIODevice() const {
		return std::make_unique<QFile>(localFilePath());
	}

	/// Returns a human-readable representation of the source location referred to by this file handle.
	QString toString() const { 
		return _sourceUrl.toString(QUrl::RemovePassword | QUrl::PreferLocalFile | QUrl::PrettyDecoded); 
	}

private:

	/// The URL denoting the data source.
	QUrl _sourceUrl;

	/// A path to the file in the local file system. 
	QString _localFilePath;
};

/**
 * \brief The file manager provides transparent access to remote files.
 */
class OVITO_CORE_EXPORT FileManager : public QObject
{
	Q_OBJECT

public:

	/// Constructor.
	FileManager() = default;

	/// Destructor.
	~FileManager();

	/// \brief Makes a file available locally.
	/// \return A Future that will provide access to the file contents after it has been fetched from the remote location.
	SharedFuture<FileHandle> fetchUrl(TaskManager& taskManager, const QUrl& url);

	/// \brief Removes a cached remote file so that it will be downloaded again next time it is requested.
	void removeFromCache(const QUrl& url);

	/// \brief Lists all files in a remote directory.
	/// \return A Future that will provide the list of file names.
	Future<QStringList> listDirectoryContents(TaskManager& taskManager, const QUrl& url);

	/// \brief Constructs a URL from a path entered by the user.
	QUrl urlFromUserInput(const QString& path);

#ifdef OVITO_SSH_CLIENT
    /// Create a new SSH connection or returns an existing connection having the same parameters.
    Ssh::SshConnection* acquireSshConnection(const Ssh::SshConnectionParameters& sshParams);

    /// Releases an SSH connection after it is no longer used.
    void releaseSshConnection(Ssh::SshConnection* connection);
#endif

protected:

#ifdef OVITO_SSH_CLIENT
	/// \brief Asks the user for the login password for a SSH server.
	/// \return True on success, false if user has canceled the operation.
	virtual bool askUserForPassword(const QString& hostname, const QString& username, QString& password);

	/// \brief Asks the user for the passphrase for a private SSH key.
	/// \return True on success, false if user has canceled the operation.
	virtual bool askUserForKeyPassphrase(const QString& hostname, const QString& prompt, QString& passphrase);

	/// \brief Asks the user for the answer to a keyboard-interactive question sent by the SSH server.
	/// \return True on success, false if user has canceled the operation.
	virtual bool askUserForKbiResponse(const QString& hostname, const QString& username, const QString& instruction, const QString& question, bool showAnswer, QString& answer);

	/// \brief Informs the user about an unknown SSH host.
	virtual bool detectedUnknownSshServer(const QString& hostname, const QString& unknownHostMessage, const QString& hostPublicKeyHash);
#endif

private Q_SLOTS:

#ifdef OVITO_SSH_CLIENT
    /// Is called whenever an SSH connection is closed.
    void cleanupSshConnection();

	/// Is called whenever a SSH connection to an yet unknown server is being established.
	void unknownSshServer();

	/// Is called whenever a SSH connection to a server requires password authentication.
	void needSshPassword();

	/// Is called whenever a SSH connection to a server requires keyboard interactive authentication.
	void needKbiAnswers();

	/// Is called when an authentication attempt for a SSH connection failed.
	void sshAuthenticationFailed(int auth);

	/// Is called whenever a private SSH key requires a passphrase.
	void needSshPassphrase(const QString& prompt);
#endif

private:

	/// Is called when a remote file has been fetched.
	void fileFetched(QUrl url, QTemporaryFile* localFile);

	/// Strips a URL from username and password information.
	static QUrl normalizeUrl(QUrl url) {
		url.setUserName({});
		url.setPassword({});
		return std::move(url);
	}

private:

	/// The remote files that are currently being fetched.
	std::map<QUrl, WeakSharedFuture<FileHandle>> _pendingFiles;

	/// Cache holding the remote files that have already been downloaded.
	QCache<QUrl, QTemporaryFile> _cachedFiles{std::numeric_limits<int>::max()};

	/// The mutex to synchronize access to above data structures.
	QMutex _mutex{QMutex::Recursive};

#ifdef OVITO_SSH_CLIENT
	/// Holds open SSH connections, which are currently active.
    QList<Ssh::SshConnection*> _acquiredConnections;

	/// Holds SSH connections, which are still open but not in use.
    QList<Ssh::SshConnection*> _unacquiredConnections;

	friend class DownloadRemoteFileJob;
#endif
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
