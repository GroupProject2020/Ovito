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

#include "processchannel.h"
#include "sshconnection.h"

#include <QDebug>
#include <QTimer>

namespace Ovito { namespace Ssh {

/******************************************************************************
* Constructor.
******************************************************************************/
ProcessChannel::ProcessChannel(SshConnection* connection, const QString& command) : SshChannel(connection, connection), 
    _command(command),
    _stderr(new StderrChannel(this))
{
    connect(connection, &SshConnection::error,          this, &ProcessChannel::handleSessionError);
    connect(connection, &SshConnection::doProcessState, this, &ProcessChannel::processState);
    connect(connection, &SshConnection::doCleanup,      this, &ProcessChannel::closeChannel);
}

/******************************************************************************
* Destructor.
******************************************************************************/
ProcessChannel::~ProcessChannel()
{
    qDebug() << "ProcessChannel::~ProcessChannel()";
    closeChannel();
}

/******************************************************************************
* Opens the QIODevice.
******************************************************************************/
bool ProcessChannel::open(OpenMode)
{
    openChannel();
    return true;
}

/******************************************************************************
* If the process has been successfully opened this function calls sendEof()
* otherwise closeChannel() is called.
******************************************************************************/
void ProcessChannel::close()
{
    if(state() == StateOpen)
        sendEof();
    else
        closeChannel();
}

/******************************************************************************
* Opens the SSH channel and starts the session.
******************************************************************************/
void ProcessChannel::openChannel()
{
    if(state() == StateClosed) {
        setState(StateWaitSession, true);
    }
}

/******************************************************************************
* Closes the SSH channel.
******************************************************************************/
void ProcessChannel::closeChannel()
{
    qDebug() << "ProcessChannel::closeChannel()";
    if(state() != StateClosed && state() != StateClosing) {
        qDebug() << "Closing channel";

        // Prevent recursion
        setState(StateClosing, false);

        Q_EMIT readChannelFinished();

        if(_channel) {
            if(::ssh_channel_is_open(_channel))
                ::ssh_channel_close(_channel);

            ::ssh_channel_free(_channel);
            _channel = nullptr;
        }

        QIODevice::close();
        static_cast<QIODevice*>(stderr())->close();

        _readBuffer.clear();
        _writeBuffer.clear();
        _stderr->_readBuffer.clear();
        _stderr->_writeBuffer.clear();

        setState(StateClosed, false);
    }
}

/******************************************************************************
* Part of the state machine implementation.
******************************************************************************/
void ProcessChannel::setState(State state, bool processState)
{
    if(_state != state) {
        _state = state;

        switch(state) {
        case StateClosed:       Q_EMIT closed();        break;
        case StateClosing:                              break;
        case StateWaitSession:                          break;
        case StateOpening:                              break;
        case StateExec:                                 break;
        case StateOpen:         Q_EMIT opened();        break;
        case StateError:        Q_EMIT error();         break;
        case StateSessionError: Q_EMIT error();         break;
        }
    }

    if(processState)
        queueCheckIO();
}

/******************************************************************************
* Schedules an I/O operation.
******************************************************************************/
void ProcessChannel::queueCheckIO()
{
    QTimer::singleShot(0, this, &ProcessChannel::processState);
}

/******************************************************************************
* State machine implementation.
******************************************************************************/
void ProcessChannel::processState()
{
    switch(state()) {

    case StateClosed:
    case StateClosing:
    case StateError:
    case StateSessionError:
        return;

    case StateWaitSession:
        if(connection()->isConnected())
            setState(StateOpening, true);
        return;

    case StateOpening:
        if(!_channel) {
            _channel = ::ssh_channel_new(connection()->_session);
            if(!_channel) {
                qCritical() << "Failed to create SSH channel.";
                return;
            }
            stderr()->_channel = channel();
        }

        switch(auto rc = ::ssh_channel_open_session(channel())) {
        case SSH_AGAIN:
            connection()->enableWritableSocketNotifier();
            return;

        case SSH_ERROR:
            setState(StateError, false);
            return;

        case SSH_OK:
            setState(StateExec, true);
            return;

        default:
            qWarning() << "Unknown result code" << rc << "received from ssh_channel_open_session()";
            return;
        }

    case StateExec: {
        switch(auto rc = ::ssh_channel_request_exec(channel(), qPrintable(_command))) {
        case SSH_AGAIN:
            connection()->enableWritableSocketNotifier();
            return;

        case SSH_ERROR:
            setState(StateError, false);
            return;

        case SSH_OK:
            // Set Unbuffered to disable QIODevice buffers.
            QIODevice::open(ReadWrite | Unbuffered);
            stderr()->open(ReadWrite | Unbuffered);
            setState(StateOpen, true);
            return;

        default:
            qWarning() << "Unknown result code" << rc << "received from ssh_channel_request_exec()";
            return;
        }
        }

    case StateOpen:
        // Send/received data.
        checkIO();
        stderr()->checkIO();
        // Check if end of transmission from remote side has been reached.
        if(state() == StateOpen && ::ssh_channel_poll(channel(), false) == SSH_EOF && ::ssh_channel_poll(channel(), true) == SSH_EOF) {
            // EOF state affects atEnd() and canReadLine() behavior,
            // so emit readyRead signal so that users can do something about it.
            if(!_readBuffer.isEmpty()) {
                Q_EMIT readyRead();
            }
            if(!stderr()->_readBuffer.isEmpty()) {
                Q_EMIT stderr()->readyRead();
            }
            _exitCode = ::ssh_channel_get_exit_status(channel());
            closeChannel();
            Q_EMIT finished(_exitCode);
        }
        return;        
    }

    Q_ASSERT_X(false, __func__, "Case was not handled properly");
}

} // End of namespace
} // End of namespace
