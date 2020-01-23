////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2013 Alexander Stukowski
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


#include <ovito/gui/desktop/GUI.h>
#include <ovito/core/dataset/io/FileImporter.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/**
 * This dialog asks the user for a username/password for a remote server.
 */
class OVITO_GUI_EXPORT RemoteAuthenticationDialog : public QDialog
{
	Q_OBJECT

public:

	/// \brief Constructs the dialog window.
	RemoteAuthenticationDialog(QWidget* parent, const QString& title, const QString& labelText);

	/// \brief Sets the username shown in the dialog.
	void setUsername(const QString& username) { _usernameEdit->setText(username); }

	/// \brief Sets the password shown in the dialog.
	void setPassword(const QString& password) { _passwordEdit->setText(password); }

	/// \brief Returns the username entered by the user.
	QString username() const { return _usernameEdit->text(); }

	/// \brief Returns the password entered by the user.
	QString password() const { return _passwordEdit->text(); }

	/// \brief Displays the dialog.
	virtual int exec() override;

private:

	QLineEdit* _usernameEdit;
	QLineEdit* _passwordEdit;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace


