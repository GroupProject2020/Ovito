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
#include "LsChannel.h"

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
