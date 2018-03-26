///////////////////////////////////////////////////////////////////////////////
// 
//  Copyright (2016) Alexander Stukowski
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
#include <core/utilities/io/FileManager.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(IO)

/**
 * \brief The file manager provides transparent access to remote files.
 */
class GuiFileManager : public FileManager
{
	Q_OBJECT

public:

	/// \brief Asks the user for the login password for a SSH server.
	/// \return True on success, false if user has canceled the operation.
	virtual bool askUserForPassword(const QString& hostname, const QString& username, QString& password) override;

	/// \brief Asks the user for the passphrase for a private SSH key.
	/// \return True on success, false if user has canceled the operation.
	virtual bool askUserForKeyPassphrase(const QString& hostname, const QString& prompt, QString& passphrase) override;

	/// \brief Informs the user about an unknown SSH host.
	virtual bool detectedUnknownSshServer(const QString& hostname, const QString& unknownHostMessage, const QString& hostPublicKeyHash) override;	
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace


