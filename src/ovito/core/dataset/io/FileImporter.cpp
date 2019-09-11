///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2014) Alexander Stukowski
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

#include <ovito/core/Core.h>
#include <ovito/core/app/PluginManager.h>
#include <ovito/core/utilities/io/FileManager.h>
#include <ovito/core/utilities/concurrent/TaskManager.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/dataset/DataSetContainer.h>
#include <ovito/core/dataset/io/FileSourceImporter.h>
#include <ovito/core/app/Application.h>
#include "FileImporter.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(DataIO)

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
			.then(dataset->executor(), [dataset, url = urls.front()](const QString& filename) {
				// Detect file format.
				return autodetectFileFormat(dataset, filename, url);
			});
	});
}

/******************************************************************************
* Tries to detect the format of the given file.
******************************************************************************/
OORef<FileImporter> FileImporter::autodetectFileFormat(DataSet* dataset, const QString& localFile, const QUrl& sourceLocation)
{
	for(const FileImporterClass* importerClass : PluginManager::instance().metaclassMembers<FileImporter>()) {
		try {
			QFile file(localFile);
			if(importerClass->checkFileFormat(file, sourceLocation)) {
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

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
