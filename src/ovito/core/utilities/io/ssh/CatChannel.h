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
