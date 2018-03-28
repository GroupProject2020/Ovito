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

#include "lschannel.h"

#include <QDebug>
#include <QTimer>

namespace Ovito { namespace Ssh {

/******************************************************************************
* Constructor.
******************************************************************************/
LsChannel::LsChannel(SshConnection* connection, const QString& location) : 
    ProcessChannel(connection, QStringLiteral("ls -A -N -U -1 -p --color=never \"%1/\"").arg(location))
{
    connect(this, &QIODevice::readyRead, this, &LsChannel::processData);
    connect(this, &ProcessChannel::opened, this, &LsChannel::receivingDirectory);
    connect(this, &ProcessChannel::finished, this, [this](int exitCode) {
        if(exitCode == 0) {
            Q_EMIT receivedDirectoryComplete(_directoryListing);
        }
        else {
            setErrorString(tr("Failed to produce remote directory listing: 'ls' command returned exit code %1").arg(exitCode));
            Q_EMIT error();
        }
    });
}

/******************************************************************************
* Is called whenever data arrives from the remote process.
******************************************************************************/
void LsChannel::processData()
{
    while(canReadLine()) {
        QByteArray line = readLine();
        line.chop(1);   // Remote end of line character.
        if(line.size() == 0) continue;
        if(line.endsWith('/')) continue;    // Skip directory entries.
        _directoryListing.push_back(QString::fromLocal8Bit(line));
    }
}

} // End of namespace
} // End of namespace
