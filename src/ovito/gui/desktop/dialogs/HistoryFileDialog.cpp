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
#include "HistoryFileDialog.h"

#define MAX_DIRECTORY_HISTORY_SIZE	5

namespace Ovito {

/******************************************************************************
* Constructs the dialog window.
******************************************************************************/
HistoryFileDialog::HistoryFileDialog(const QString& dialogClass, QWidget* parent, const QString& caption, const QString& directory, const QString& filter) :
	QFileDialog(parent, caption, directory, filter), _dialogClass(dialogClass)
{
	connect(this, &QFileDialog::fileSelected, this, &HistoryFileDialog::onFileSelected);

	// The user can request the Qt file dialog instead of the native dialog by settings the corresponding
	// option in the application settings.
	// The native dialogs of some platforms don't provide the directory history function but may be faster
	// than the Qt implementation.
	QSettings settings;
	if(settings.value("file/use_qt_dialog", false).toBool())
		setOption(QFileDialog::DontUseNativeDialog);

	QStringList history = loadDirHistory();
	if(history.isEmpty() == false) {
		if(directory.isEmpty()) {
			setDirectory(history.front());
		}
		setHistory(history);
	}
}

/******************************************************************************
* This is called when the user has pressed the OK button of the dialog.
******************************************************************************/
void HistoryFileDialog::onFileSelected(const QString& file)
{
	if(file.isEmpty()) return;
	QString currentDir = QFileInfo(file).absolutePath();

	QStringList history = loadDirHistory();
	int index = history.indexOf(currentDir);
	if(index >= 0)
		history.move(index, 0);
	else {
		history.push_front(currentDir);
		if(history.size() > MAX_DIRECTORY_HISTORY_SIZE)
			history.erase(history.begin() + MAX_DIRECTORY_HISTORY_SIZE, history.end());
	}
	saveDirHistory(history);
}

/******************************************************************************
* Loads the list of most recently visited directories from the settings store.
******************************************************************************/
QStringList HistoryFileDialog::loadDirHistory() const
{
	QSettings settings;
	settings.beginGroup("filedialog/" + _dialogClass);
	return settings.value("history").toStringList();
}

/******************************************************************************
* Saves the list of most recently visited directories to the settings store.
******************************************************************************/
void HistoryFileDialog::saveDirHistory(const QStringList& list) const
{
	QSettings settings;
	settings.beginGroup("filedialog/" + _dialogClass);
	settings.setValue("history", QVariant::fromValue(list));
}

}	// End of namespace
