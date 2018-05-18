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
#include <core/utilities/concurrent/SharedFuture.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(IO)

namespace Ssh {
	// These classes are defined elsewhere:
	class SshConnection;
	struct SshConnectionParameters;
}

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

	/// \brief Makes a file available on this computer.
	/// \return A Future that will provide the local file name of the downloaded file.
	SharedFuture<QString> fetchUrl(TaskManager& taskManager, const QUrl& url);

	/// \brief Removes a cached remote file so that it will be downloaded again next time it is requested.
	void removeFromCache(const QUrl& url);

	/// \brief Lists all files in a remote directory.
	/// \return A Future that will provide the list of file names.
	Future<QStringList> listDirectoryContents(TaskManager& taskManager, const QUrl& url);

	/// \brief Constructs a URL from a path entered by the user.
	QUrl urlFromUserInput(const QString& path);

    /// Create a new SSH connection or returns an existing connection having the same parameters.
    Ssh::SshConnection* acquireSshConnection(const Ssh::SshConnectionParameters& sshParams);

    /// Releases an SSH connection after it is no longer used.
    void releaseSshConnection(Ssh::SshConnection* connection);

protected:

	/// \brief Asks the user for the login password for a SSH server.
	/// \return True on success, false if user has canceled the operation.
	virtual bool askUserForPassword(const QString& hostname, const QString& username, QString& password);

	/// \brief Asks the user for the passphrase for a private SSH key.
	/// \return True on success, false if user has canceled the operation.
	virtual bool askUserForKeyPassphrase(const QString& hostname, const QString& prompt, QString& passphrase);

	/// \brief Informs the user about an unknown SSH host.
	virtual bool detectedUnknownSshServer(const QString& hostname, const QString& unknownHostMessage, const QString& hostPublicKeyHash);

private Q_SLOTS:
    
    /// Is called whenever an SSH connection is closed.
    void cleanupSshConnection();

	/// Is called whenever a SSH connection to an yet unknown server is being established.
	void unknownSshServer();

	/// Is called whenever a SSH connection to a server requires password authentication.
	void needSshPassword();

	/// Is called when an authentication attempt for a SSH connection failed.
	void sshAuthenticationFailed(int auth);

	/// Is called whenever a private SSH key requires a passphrase.
	void needSshPassphrase(const QString& prompt);

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
	std::map<QUrl, WeakSharedFuture<QString>> _pendingFiles;

	/// Cache holding the remote files that have already been downloaded.
	QCache<QUrl, QTemporaryFile> _cachedFiles{std::numeric_limits<int>::max()};

	/// The mutex to synchronize access to above data structures.
	QMutex _mutex{QMutex::Recursive};

	/// Holds open SSH connections, which are currently active.
    QList<Ssh::SshConnection*> _acquiredConnections;	

	/// Holds SSH connections, which are still open but not in use.
    QList<Ssh::SshConnection*> _unacquiredConnections;

	friend class DownloadRemoteFileJob;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
