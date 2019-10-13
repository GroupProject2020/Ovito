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

#include <ovito/core/Core.h>
#include "ScpChannel.h"

namespace Ovito { namespace Ssh {

/******************************************************************************
* Constructor.
******************************************************************************/
ScpChannel::ScpChannel(SshConnection* connection, const QString& location) :
    ProcessChannel(connection, QStringLiteral("scp -f \"%1\"").arg(location))
{
    connect(this, &QIODevice::readyRead, this, &ScpChannel::processData);

    connect(this, &ProcessChannel::opened, this, [this]() {
        setState(StateConnected);
        write("", 1);
    });
}

/******************************************************************************
* Is called whenever data arrives from the remote process.
******************************************************************************/
void ScpChannel::processData()
{
    if(state() == StateConnected) {
        if(canReadLine()) {
            const QByteArray line = readLine();
            if(line.size() == 0) {
                setErrorString(tr("Received empty response line from SCP remote process."));
                Q_EMIT error();
                return;
            }

            if(line[0] == 'C') {
                qlonglong fs;
                if(sscanf(line, "C%*d %lld", &fs) != 1 || fs < 0) {
                    setErrorString(tr("Received invalid C line from SCP remote process: %1").arg(QString::fromLocal8Bit(line)));
                    Q_EMIT error();
                    return;
                }
                _fileSize = fs;
                _bytesReceived = 0;

                // Accept SCP request to start transmission of file data.
                write("", 1);
                Q_EMIT receivingFile(_fileSize);
                setState(StateReceivingFile);
            }
            else if(line[0] == 'D' || line[0] == 'E') {
                setErrorString(tr("Received unexpected D/E line from SCP remote process."));
                Q_EMIT error();
            }
            else if(line[0] == 0x01 || line[0] == 0x02) {
                QString msg = QString::fromLocal8Bit(line.mid(1)).trimmed();
                setErrorString(tr("SCP error: %1").arg(msg));
                qDebug() << "Server reported error:" << msg;
                Q_EMIT error();
            }
            else {
                qWarning() << "Received unknown response line from SCP remote process:" << line;
                setErrorString(tr("Received unknown response line from SCP remote process."));
                Q_EMIT error();
            }
        }
    }
    else if(state() == StateReceivingFile && _dataBuffer != nullptr) {
        qint64 avail = std::min(bytesAvailable(), _fileSize - _bytesReceived);
        qint64 nread = read(_dataBuffer + _bytesReceived, avail);
        if(nread < 0) {
            qDebug() << "Read a negative number of bytes from remote stream.";
            Q_EMIT error();
            return;
        }
        _bytesReceived += nread;
        if(nread > 0)
            Q_EMIT receivedData(_bytesReceived);
        if(_bytesReceived == _fileSize) {
            write("", 1);
            setState(StateFileComplete);
        }
    }
    else if(state() == StateFileComplete) {
        if(canReadLine()) {
            const QByteArray line = readLine();
            if(line.size() == 0) {
                setErrorString(tr("Received empty response line from SCP remote process."));
                Q_EMIT error();
                return;
            }

            if(line[0] == 0x00) {
                setState(StateConnected);
                Q_EMIT receivedFileComplete();
            }
            else if(line[0] == 0x01) {
                QString msg = QString::fromLocal8Bit(line.mid(1)).trimmed();
                setErrorString(tr("SCP error: %1").arg(msg));
                Q_EMIT error();
            }
            else {
                qWarning() << "Received unexpected response line from SCP remote process:" << line;
                setErrorString(tr("Received unexpected response line from SCP remote process."));
                Q_EMIT error();
            }
        }
    }
}

} // End of namespace
} // End of namespace
