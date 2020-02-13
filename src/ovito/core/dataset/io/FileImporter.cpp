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

#include <ovito/core/Core.h>
#include <ovito/core/app/PluginManager.h>
#include <ovito/core/utilities/io/FileManager.h>
#include <ovito/core/utilities/concurrent/TaskManager.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/dataset/DataSetContainer.h>
#include <ovito/core/dataset/io/FileSourceImporter.h>
#include <ovito/core/app/Application.h>
#include "FileImporter.h"

namespace Ovito {

IMPLEMENT_OVITO_CLASS(FileImporter);

/******************************************************************************
* Tries to detect the format of the given file.
******************************************************************************/
Future<OORef<FileImporter>> FileImporter::autodetectFileFormat(DataSet* dataset, const QUrl& url)
{
	if(!url.isValid())
		dataset->throwException(tr("Invalid path or URL."));

	// Resolve filename if it contains a wildcard.
	return FileSourceImporter::findWildcardMatches(url, dataset).then(dataset->executor(), [dataset](std::vector<QUrl>&& urls) {
		if(urls.empty())
			dataset->throwException(tr("There are no files in the directory matching the filename pattern."));

		// Download file so we can determine its format.
		return Application::instance()->fileManager()->fetchUrl(dataset->taskManager(), urls.front())
			.then(dataset->executor(), [dataset, url = urls.front()](const FileHandle& file) {
				// Detect file format.
				return autodetectFileFormat(dataset, file);
			});
	});
}

/******************************************************************************
* Tries to detect the format of the given file.
******************************************************************************/
OORef<FileImporter> FileImporter::autodetectFileFormat(DataSet* dataset, const FileHandle& file)
{
	for(const FileImporterClass* importerClass : PluginManager::instance().metaclassMembers<FileImporter>()) {
		try {
			if(importerClass->checkFileFormat(file)) {
				return static_object_cast<FileImporter>(importerClass->createInstance(dataset));
			}
		}
		catch(const Exception&) {
			// Ignore errors that occur during file format detection.
		}
	}
	return nullptr;
}

/******************************************************************************
* Helper function that is called by sub-classes prior to file parsing in order to
* activate the default "C" locale.
******************************************************************************/
void FileImporter::activateCLocale()
{
	std::setlocale(LC_ALL, "C");
}

}	// End of namespace
