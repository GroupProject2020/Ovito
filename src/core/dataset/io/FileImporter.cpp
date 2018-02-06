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

#include <core/Core.h>
#include <core/app/PluginManager.h>
#include <core/utilities/io/FileManager.h>
#include <core/utilities/concurrent/TaskManager.h>
#include <core/dataset/DataSet.h>
#include <core/dataset/DataSetContainer.h>
#include <core/dataset/io/FileSourceImporter.h>
#include <core/app/Application.h>
#include "FileImporter.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(DataIO)

IMPLEMENT_OVITO_CLASS(FileImporter);

/******************************************************************************
* Tries to detect the format of the given file.
******************************************************************************/
OORef<FileImporter> FileImporter::autodetectFileFormat(DataSet* dataset, const QUrl& url)
{
	if(!url.isValid())
		dataset->throwException(tr("Invalid path or URL."));

	try {
		TaskManager& taskManager = dataset->container()->taskManager();

		// Resolve filename if it contains a wildcard.
		Future<QVector<FileSourceImporter::Frame>> framesFuture = FileSourceImporter::findWildcardMatches(url, taskManager);
		if(!taskManager.waitForTask(framesFuture))
			dataset->throwException(tr("Operation has been canceled by the user."));
		QVector<FileSourceImporter::Frame> frames = framesFuture.result();
		if(frames.empty())
			dataset->throwException(tr("There are no files in the directory matching the filename pattern."));

		// Download file so we can determine its format.
		SharedFuture<QString> fetchFileFuture = Application::instance()->fileManager()->fetchUrl(taskManager, frames.front().sourceFile);
		if(!taskManager.waitForTask(fetchFileFuture))
			dataset->throwException(tr("Operation has been canceled by the user."));

		// Detect file format.
		return autodetectFileFormat(dataset, fetchFileFuture.result(), frames.front().sourceFile.path());
	}
	catch(Exception& ex) {
		// Provide a context object for any errors that occur during file inspection.
		ex.setContext(dataset);
		throw;
	}
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
				return static_object_cast<FileImporter>(importerClass->createInstance(dataset));;
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
