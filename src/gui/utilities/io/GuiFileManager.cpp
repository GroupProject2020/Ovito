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

#include <gui/GUI.h>
#include <core/app/Application.h>
#include "GuiFileManager.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(IO)

/******************************************************************************
* Asks the user for the login password for a SSH server.
******************************************************************************/
bool GuiFileManager::askUserForPassword(const QString& hostname, const QString& username, QString& password)
{
	if(Application::instance()->guiMode()) {
		bool ok;
		password = QInputDialog::getText(nullptr, tr("SSH Password Authentication"), 
			tr("OVITO is trying to connect to remote host '%1'.\n\nPlease enter the password for user '%2':").arg(hostname).arg(username),
			QLineEdit::Password, password, &ok);
		return ok;
	}
	else {
		return FileManager::askUserForPassword(hostname, username, password);
	}
}

/******************************************************************************
* Asks the user for the passphrase for a private SSH key.
******************************************************************************/
bool GuiFileManager::askUserForKeyPassphrase(const QString& hostname, const QString& prompt, QString& passphrase)
{
	if(Application::instance()->guiMode()) {
		bool ok;
		passphrase = QInputDialog::getText(nullptr, tr("SSH Remote Connection"), 
			tr("OVITO is trying to connect to remote host '%1'.\n\n%2").arg(hostname).arg(prompt),
			QLineEdit::Password, passphrase, &ok);
		return ok;
	}
	else {
		return FileManager::askUserForKeyPassphrase(hostname, prompt, passphrase);
	}
}

/******************************************************************************
* Informs the user about an unknown SSH host.
******************************************************************************/
bool GuiFileManager::detectedUnknownSshServer(const QString& hostname, const QString& unknownHostMessage, const QString& hostPublicKeyHash)
{
	if(Application::instance()->guiMode()) {
		return QMessageBox::question(nullptr, tr("SSH Unknown Remote Host"),
			tr("OVITO is trying to connect to remote host '%1'.\n%2 Key fingerprint is %3\n\nAre you sure you want to continue connecting?")
			.arg(hostname).arg(unknownHostMessage).arg(hostPublicKeyHash),
			QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes;
	}
	else {
		return FileManager::detectedUnknownSshServer(hostname, unknownHostMessage, hostPublicKeyHash);
	}
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
