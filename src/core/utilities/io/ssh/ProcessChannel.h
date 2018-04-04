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
#include "SshChannel.h"

#include <libssh/callbacks.h>

#ifdef max
    #undef max
#endif
#ifdef min
    #undef min
#endif

namespace Ovito { namespace Ssh {

class ProcessChannel : public SshChannel
{
    Q_OBJECT

public:

    /// Constructor.
    explicit ProcessChannel(SshConnection* connection, QString command);

    /// Destructor.
    ~ProcessChannel();

    /// Opens the QIODevice. Same as openChannel().
    virtual bool open(OpenMode mode) override;

    /// Closes the QIODevice.
    virtual void close() override;

    /// Opens the SSH channel and starts the session.
    void openChannel();

    /// Closes the SSH channel.
    void closeChannel();

    /// Returns the exit code returned by the remote process.
    int exitCode() const { return _exitCode; }

    /// Returns the command executed on the remote host.
    const QString& command() const { return _command; }


Q_SIGNALS:

    void opened();
    void closed();
    void error();
    void finished(int exit_code);

protected:

    /// Schedules an I/O operation.
    virtual void queueCheckIO() override;

    virtual void timerEvent(QTimerEvent* event) override {
        SshChannel::timerEvent(event);
        processState();
    }

private:

    enum State {
        StateClosed = 0,
        StateClosing = 1,
        StateWaitSession = 2,
        StateOpening = 3,
        StateExec = 4,
        StateOpen = 5,
        StateError = 6,
        StateSessionError = 7
    };

    class StderrChannel : public SshChannel
    {
    public:

        /// Constructor.
        StderrChannel(ProcessChannel* parent) : SshChannel(parent->connection(), parent, true) {}

    protected:

        virtual bool open(OpenMode) override {
            // Set Unbuffered to disable QIODevice buffers.
            QIODevice::open(ReadWrite | Unbuffered);
            return true;
        }

        virtual void queueCheckIO() override {
            static_cast<ProcessChannel*>(parent())->queueCheckIO();
        }

        friend class ProcessChannel;
    };

    /// Part of the state machine implementation.
    void setState(State state, bool processState);

    /// Returns the current state of the channel.
    State state() const { return _state; }

    /// Returns the stderr channel.
    StderrChannel* stderrChannel() const { return _stderr; }

	/// Returns the underlying SSH connection.
    using SshChannel::connection;

    /// Callback function, which is called by libssh when data is available on the channel.
    static int channelDataCallback(ssh_session session, ssh_channel channel, void* data, uint32_t len, int is_stderr, void* userdata);

private Q_SLOTS:

    /// State machine implementation.
    void processState();

    /// Is called when the SSH session has signaled an error.
    void handleSessionError() {
        setState(StateSessionError, false);
    }

private:

    State _state = StateClosed;
    QString _command;
    StderrChannel* _stderr;
    int _exitCode = 0;
    struct ssh_channel_callbacks_struct _channelCallbacks;
    int _timerId = 0;
    bool _ioCheckQueued = false;
    bool _isConnectDelayed = false;

    static constexpr int SSH_CHANNEL_GRACE_PERIOD = 100;
};


} // End of namespace
} // End of namespace
