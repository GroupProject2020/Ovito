////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2020 Alexander Stukowski
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
#include <ovito/core/utilities/io/FileManager.h>

namespace Ovito {

/**
 * \brief The file manager provides transparent access to remote files.
 */
class GuiFileManager : public FileManager
{
	Q_OBJECT

public:

#ifdef OVITO_SSH_CLIENT
	/// \brief Asks the user for the login password for a SSH server.
	/// \return True on success, false if user has canceled the operation.
	virtual bool askUserForPassword(const QString& hostname, const QString& username, QString& password) override;

	/// \brief Asks the user for the answer to a keyboard-interactive question sent by the SSH server.
	/// \return True on success, false if user has canceled the operation.
	virtual bool askUserForKbiResponse(const QString& hostname, const QString& username, const QString& instruction, const QString& question, bool showAnswer, QString& answer) override;

	/// \brief Asks the user for the passphrase for a private SSH key.
	/// \return True on success, false if user has canceled the operation.
	virtual bool askUserForKeyPassphrase(const QString& hostname, const QString& prompt, QString& passphrase) override;

	/// \brief Informs the user about an unknown SSH host.
	virtual bool detectedUnknownSshServer(const QString& hostname, const QString& unknownHostMessage, const QString& hostPublicKeyHash) override;
#endif
};

}	// End of namespace


