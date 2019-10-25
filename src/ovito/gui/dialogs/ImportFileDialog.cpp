////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2014 Alexander Stukowski
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

#include <ovito/gui/GUI.h>
#include "ImportFileDialog.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/******************************************************************************
* Constructs the dialog window.
******************************************************************************/
ImportFileDialog::ImportFileDialog(const QVector<const FileImporterClass*>& importerTypes, DataSet* dataset, QWidget* parent, const QString& caption, const QString& dialogClass) :
	HistoryFileDialog(dialogClass, parent, caption), _importerTypes(importerTypes)
{
	// Build filter string.
	for(auto importerClass : _importerTypes) {
		_filterStrings << QString("%1 (%2)").arg(importerClass->fileFilterDescription(), importerClass->fileFilter());
	}
	if(_filterStrings.isEmpty()) {
		dataset->throwException(tr("There are no importer plugins installed."));
	}

	_filterStrings.prepend(tr("<Auto-detect file format> (*)"));

	setNameFilters(_filterStrings);
	setAcceptMode(QFileDialog::AcceptOpen);
	setFileMode(QFileDialog::ExistingFile);

	selectNameFilter(_filterStrings.front());
}

/******************************************************************************
* Returns the file to import after the dialog has been closed with "OK".
******************************************************************************/
QString ImportFileDialog::fileToImport() const
{
	if(_selectedFile.isEmpty()) {
		QStringList filesToImport = selectedFiles();
		if(filesToImport.isEmpty()) return QString();
		return filesToImport.front();
	}
	else return _selectedFile;
}

/******************************************************************************
* Returns the selected importer type or NULL if auto-detection is requested.
******************************************************************************/
const FileImporterClass* ImportFileDialog::selectedFileImporterType() const
{
	int importFilterIndex = _filterStrings.indexOf(_selectedFilter.isEmpty() ? selectedNameFilter() : _selectedFilter) - 1;
	OVITO_ASSERT(importFilterIndex >= -1 && importFilterIndex < _importerTypes.size());

	if(importFilterIndex >= 0)
		return _importerTypes[importFilterIndex];
	else
		return nullptr;
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
