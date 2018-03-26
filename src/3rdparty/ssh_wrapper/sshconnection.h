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

#include <QFlags>
#include <QObject>
#include <QString>
#include <QSocketNotifier>

#include <libssh/libssh.h>

namespace Ovito { namespace Ssh {

struct SshConnectionParameters
{
    QString host;
    QString userName;
    QString password;
    unsigned int port = 0;

    bool operator==(const SshConnectionParameters& other) const {
        return host == other.host && userName == other.userName && port == other.port;
    }
    bool operator!=(const SshConnectionParameters& other) const {
        return !(*this == other);
    }
};

class SshConnection : public QObject
{
    Q_OBJECT

public:

    Q_ENUMS(HostState)
    enum HostState {
        HostKnown                   = SSH_SERVER_KNOWN_OK,
        HostUnknown                 = SSH_SERVER_NOT_KNOWN,
        HostKeyChanged              = SSH_SERVER_KNOWN_CHANGED,
        HostKeyTypeChanged          = SSH_SERVER_FOUND_OTHER,
        HostKnownHostsFileMissing   = SSH_SERVER_FILE_NOT_FOUND
    };

    Q_FLAGS(UseAuthFlag)
    enum UseAuthFlag
    {
        UseAuthEmpty                = 0,    ///< Auth method not chosen
        UseAuthNone                 = 1<<0, ///< SSH None authentication method
        UseAuthAutoPubKey           = 1<<1, ///< Keys from ~/.ssh and ssh-agent
        UseAuthPassword             = 1<<2, ///< SSH Password auth method
        UseAuthKbi                  = 1<<3  ///< SSH KBI auth method
    };
    Q_DECLARE_FLAGS(UseAuths, UseAuthFlag)    

public:

    /// Constructor.
    explicit SshConnection(const SshConnectionParameters& serverInfo, QObject* parent = nullptr);

    /// Destructor.
    virtual ~SshConnection();

    /// Returns the error message string after the error() signal was emitted.
    QString errorMessage() const;

    /// Returns the SSH connection parameters.
    const SshConnectionParameters& connectionParameters() const { return _connectionParams; }

    /// Indicates whether this connection is successfully opened.
    bool isConnected() const { return _state == StateOpened; }

    /// Sets the password for use in password authentication.
    void setPassword(QString password);

    /// Returns the know/unknown status of the current remote host.
    HostState unknownHostType() const { return _unknownHostType; }

    /// Generates a message string to show to the user why the current host is unknown.
    QString unknownHostMessage();

    /// Returns the publish key of the current remote host.    
    QString hostPublicKeyHash();

    /// This turns the current remote host into a known host by adding it to the knows_hosts file. 
    bool markCurrentHostKnown();

public Q_SLOTS:

    /// Opens the connection to the host.
    void connectToHost();

    /// Closes the connection to the host.
    void disconnectFromHost();

Q_SIGNALS:

    void unknownHost();
    void chooseAuth();
    void needPassword();        ///< Use setPassword() to set password
    void needKbiAnswers();      ///< Use setKbiAnswers() set answers
    void authFailed(int auth);  ///< One authentication attempt has failed
    void allAuthsFailed();      ///< All authentication attempts have failed
    void connected();
    void disconnected();
    void error();
    void stateChanged();

private Q_SLOTS:

    /// Is called after the state has changed.
    void processStateGuard();

    /// Handles the signal from the QSocketNotifier.
    void handleSocketReadable();

    /// Handles the signal from the QSocketNotifier.
    void handleSocketWritable();

private:

    enum State {
        StateClosed = 0,
        StateClosing = 1,
        StateInit = 2,
        StateConnecting = 3,
        StateServerIsKnown = 4,
        StateUnknownHost = 5,
        StateAuthChoose = 6,
        StateAuthContinue = 7,
        StateAuthNone = 8,
        StateAuthAutoPubkey = 9,
        StateAuthPassword = 10,
        StateAuthNeedPassword = 11,
        StateAuthKbi = 12,
        StateAuthKbiQuestions = 13,
        StateAuthAllFailed = 14,
        StateOpened = 15,
        StateError = 16
    };

    /// Sets the internal state variable to a new value.
    void setState(State state, bool emitStateChangedSignal);

    /// The main state machine function.
    void processState();

    /// Sets an option of the libssh session object.
    bool setLibsshOption(enum ssh_options_e type, const void* value);

    /// Creates the notifier objects for the sockets.
    void createSocketNotifiers();

    /// Destroys the notifier objects for the sockets.
    void destroySocketNotifiers();

    /// Re-enables the writable socket notifier. 
    void enableWritableSocketNotifier();

    /// Chooses next authentication method to try.
    void tryNextAuth();

    /// Handles the server's reponse to an authentication attempt.
    void handleAuthResponse(int rc, UseAuthFlag auth);

    /// The SSH connection parameters.
    SshConnectionParameters _connectionParams;

    /// Indicates that the password for the connection has been set.
    bool _passwordSet = false;

    /// The passwort that has been set.
    QString _password;

    /// The libssh sesssion handle.
    ssh_session _session = 0;

    /// The current state of the connection.
    State _state = StateClosed;

    /// Indicates that a call to processState() is in progress.
    bool _processingState = false;

    QSocketNotifier* _readNotifier = nullptr;
    QSocketNotifier* _writeNotifier = nullptr;
    bool _enableWritableNofifier = false;

    /// The host known/unknown status.
    HostState _unknownHostType = HostUnknown;

    UseAuths _useAuths = UseAuths(UseAuthNone | UseAuthAutoPubKey | UseAuthPassword);
    UseAuths _failedAuths = UseAuthEmpty;
    UseAuthFlag _succeededAuth = UseAuthEmpty;
};

} // End of namespace
} // End of namespace
