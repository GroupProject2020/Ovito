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
 * This dialog lets the user select a remote file to be imported.
 */
class OVITO_GUI_EXPORT ImportRemoteFileDialog : public QDialog
{
	Q_OBJECT

public:

	/// \brief Constructs the dialog window.
	ImportRemoteFileDialog(const QVector<const FileImporterClass*>& importerTypes, DataSet* dataset, QWidget* parent = nullptr, const QString& caption = QString());

	/// \brief Sets the current URL in the dialog.
	void selectFile(const QUrl& url);

	/// \brief Returns the file to import after the dialog has been closed with "OK".
	QUrl fileToImport() const;

	/// \brief Returns the selected importer type or NULL if auto-detection is requested.
	const FileImporterClass* selectedFileImporterType() const;

	virtual QSize sizeHint() const override {
		return QDialog::sizeHint().expandedTo(QSize(500, 0));
	}

protected Q_SLOTS:

	/// This is called when the user has pressed the OK button of the dialog.
	/// Validates and saves all input made by the user and closes the dialog box.
	void onOk();

private:

	QVector<const FileImporterClass*> _importerTypes;

	QComboBox* _urlEdit;
	QComboBox* _formatSelector;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace


