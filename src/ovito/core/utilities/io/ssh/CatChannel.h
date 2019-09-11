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

#include <ovito/core/Core.h>
#include "ProcessChannel.h"

namespace Ovito { namespace Ssh {

class CatChannel : public ProcessChannel
{
    Q_OBJECT

public:

    /// Constructor.
    explicit CatChannel(SshConnection* connection, const QString& location);

    /// Sets the destination buffer for the received file data.
    void setDestinationBuffer(char* buffer) {
        _dataBuffer = buffer;
        processData();
    }

Q_SIGNALS:

    /// This signal is generated before transmission of a file begins.
    void receivingFile(qint64 fileSize);

    /// This signal is generated during data transmission.
    void receivedData(qint64 totalReceivedBytes);

    /// This signal is generated after a file has been fully transmitted.
    void receivedFileComplete();

private Q_SLOTS:

    /// Is called whenever data arrives from the remote process.
    void processData();

private:

    char* _dataBuffer = nullptr;
    qint64 _bytesReceived = 0;
    qint64 _fileSize = -1;
};


} // End of namespace
} // End of namespace
