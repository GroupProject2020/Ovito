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

#include <ovito/gui/desktop/GUI.h>
#include "RemoteAuthenticationDialog.h"

namespace Ovito {

/******************************************************************************
* Constructs the dialog window.
******************************************************************************/
RemoteAuthenticationDialog::RemoteAuthenticationDialog(QWidget* parent, const QString& title, const QString& labelText) : QDialog(parent)
{
	setWindowTitle(title);

	QVBoxLayout* layout1 = new QVBoxLayout(this);
	layout1->setSpacing(2);

	QLabel* label = new QLabel(labelText);
	//label->setWordWrap(true);
	layout1->addWidget(label);
	layout1->addSpacing(10);

	layout1->addWidget(new QLabel(tr("Login:")));
	_usernameEdit = new QLineEdit(this);
	layout1->addWidget(_usernameEdit);
	layout1->addSpacing(10);

	layout1->addWidget(new QLabel(tr("Password:")));
	_passwordEdit = new QLineEdit(this);
	_passwordEdit->setEchoMode(QLineEdit::Password);
	layout1->addWidget(_passwordEdit);
	layout1->addSpacing(10);

	QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
	connect(buttonBox, &QDialogButtonBox::accepted, this, &RemoteAuthenticationDialog::accept);
	connect(buttonBox, &QDialogButtonBox::rejected, this, &RemoteAuthenticationDialog::reject);
	layout1->addWidget(buttonBox);
}

/******************************************************************************
* Displays the dialog.
******************************************************************************/
int RemoteAuthenticationDialog::exec()
{
	if(_usernameEdit->text().isEmpty()) {

		if(qEnvironmentVariableIsSet("USER"))
			_usernameEdit->setText(QString::fromLocal8Bit(qgetenv("USER")));
		else if(qEnvironmentVariableIsSet("USERNAME"))
			_usernameEdit->setText(QString::fromLocal8Bit(qgetenv("USERNAME")));

		_usernameEdit->setFocus();
	}
	else
		_passwordEdit->setFocus();

	return QDialog::exec();
}

}	// End of namespace
