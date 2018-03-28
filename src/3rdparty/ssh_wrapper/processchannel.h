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

#include "sshchannel.h"

namespace Ovito { namespace Ssh {

class ProcessChannel : public SshChannel
{
    Q_OBJECT

public:

    /// Constructor.
    explicit ProcessChannel(SshConnection* connection, const QString& command);

    /// Destructor.
    ~ProcessChannel();

    /// Opens the QIODevice. Same as openChannel().
    virtual bool open(OpenMode ignored) override;

    /// Closes the QIODevice.
    virtual void close() override;

    /// Opens the SSH channel and starts the session.
    void openChannel();

    /// Closes the SSH channel.
    void closeChannel();

    /// Returns the exit code returned by the remote process.
    int exitCode() const { return _exitCode; }

Q_SIGNALS:

    void opened();
    void closed();
    void error();
    void finished(int exit_code);

protected:

    /// Schedules an I/O operation.
    virtual void queueCheckIO() override;

private:

    enum State {
        StateClosed,
        StateClosing,
        StateWaitSession,
        StateOpening,
        StateExec,
        StateOpen,
        StateError,
        StateSessionError
    };

    class StderrChannel : public SshChannel
    {
    public:

        /// Constructor.
        StderrChannel(ProcessChannel* parent) : SshChannel(parent->connection(), parent, true) {}

        /// Closes the channel.
        virtual void close() override {
            static_cast<ProcessChannel*>(parent())->close();
        }

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
    StderrChannel* stderr() const { return _stderr; }

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
};


} // End of namespace
} // End of namespace
