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

#include <ovito/gui/web/GUIWeb.h>
#include <ovito/gui/web/mainwin/MainWindow.h>
#include <ovito/core/app/Application.h>
#include <ovito/core/dataset/io/FileImporter.h>
#include <ovito/core/utilities/io/FileManager.h>
#include "WasmDataSetContainer.h"

namespace Ovito {

IMPLEMENT_OVITO_CLASS(WasmDataSetContainer);

/******************************************************************************
* Initializes the dataset manager.
******************************************************************************/
WasmDataSetContainer::WasmDataSetContainer(MainWindow* mainWindow) :
	_mainWindow(mainWindow)
{
}

/******************************************************************************
* Imports a given file into the scene.
******************************************************************************/
bool WasmDataSetContainer::importFile(const QUrl& url, const FileImporterClass* importerType)
{
	OVITO_ASSERT(currentSet() != nullptr);

	if(!url.isValid())
		throw Exception(tr("Failed to import file. URL is not valid: %1").arg(url.toString()), currentSet());

	OORef<FileImporter> importer;
	if(!importerType) {

		// Detect file format.
		Future<OORef<FileImporter>> importerFuture = FileImporter::autodetectFileFormat(currentSet(), url);
		if(!taskManager().waitForFuture(importerFuture))
			return false;

		importer = importerFuture.result();
		if(!importer)
			currentSet()->throwException(tr("Could not detect the format of the file to be imported. The format might not be supported."));
	}
	else {
		importer = static_object_cast<FileImporter>(importerType->createInstance(currentSet()));
		if(!importer)
			currentSet()->throwException(tr("Failed to import file. Could not initialize file reader."));
	}

	// Load user-defined default settings for the importer.
	importer->loadUserDefaults();

	// Specify how the file's data should be inserted into the current scene.
	FileImporter::ImportMode importMode = FileImporter::AddToScene;

	return importer->importFile({url}, importMode, true);
}

}	// End of namespace
