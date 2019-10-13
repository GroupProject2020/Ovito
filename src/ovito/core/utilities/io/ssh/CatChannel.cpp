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
#include "CatChannel.h"

namespace Ovito { namespace Ssh {

/******************************************************************************
* Constructor.
******************************************************************************/
CatChannel::CatChannel(SshConnection* connection, const QString& location) :
    ProcessChannel(connection, QStringLiteral("wc -c \"%1\" && cat \"%1\"").arg(location))
{
    qDebug() << "SSH command:" << command();
    connect(this, &QIODevice::readyRead, this, &CatChannel::processData);
}

/******************************************************************************
* Is called whenever data arrives from the remote process.
******************************************************************************/
void CatChannel::processData()
{
    if(_fileSize == -1) {
        if(canReadLine()) {
            const QByteArray line = readLine();
            if(line.size() < 2) {
                setErrorString(tr("Received invalid response line from remote host."));
                Q_EMIT error();
                return;
            }
            qDebug() << "Response line:" << line;

            qlonglong fs;
            if(sscanf(line, "%lld", &fs) != 1 || fs < 0) {
                setErrorString(tr("Received invalid response line from remote host: %1").arg(QString::fromLocal8Bit(line.left(100))));
                Q_EMIT error();
                return;
            }
            _fileSize = fs;
            _bytesReceived = 0;
            Q_EMIT receivingFile(_fileSize);
        }
    }
    if(_fileSize >= 0) {
        if(_dataBuffer == nullptr) {
            setErrorString(tr("Destination data buffer for has not been set."));
            Q_EMIT error();
            return;
        }
        qint64 avail = std::min(bytesAvailable(), _fileSize - _bytesReceived);
        qint64 nread = read(_dataBuffer + _bytesReceived, avail);
        if(nread < 0) {
            qDebug() << "Read negative number of bytes from remote stream.";
            Q_EMIT error();
            return;
        }
        _bytesReceived += nread;
        if(_bytesReceived == _fileSize) {
            Q_EMIT receivedFileComplete();
        }
        else {
            if(nread > 0)
                Q_EMIT receivedData(_bytesReceived);
        }
    }
}

} // End of namespace
} // End of namespace
