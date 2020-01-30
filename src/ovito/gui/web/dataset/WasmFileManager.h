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

#pragma once


#include <ovito/core/Core.h>
#include <ovito/core/utilities/io/FileManager.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(IO)

/**
 * \brief The file manager provides transparent access to remote files.
 */
class WasmFileManager : public FileManager
{
	Q_OBJECT

public:

	/// \brief Makes a file available locally.
	/// \return A Future that will provide access to the file contents after it has been fetched from the remote location.
	virtual SharedFuture<FileHandle> fetchUrl(TaskManager& taskManager, const QUrl& url) override;

	/// \brief Lists all files in a remote directory.
	/// \return A Future that will provide the list of file names.
	virtual Future<QStringList> listDirectoryContents(TaskManager& taskManager, const QUrl& url) override;

	/// \brief Opens a file dialog in the browser allowing the user to import a file from the local computer into the application. 
	static void importFileIntoMemory(MainWindow* mainWindow, const QString& acceptedFileTypes, std::function<void(const QUrl&)> callback);

public:

	/// Internal callback method. JavaScript will call this function when the imported file data is ready.
	void importedFileDataReady(char* content, size_t contentSize, const char* fileName, int fileImportId);

	/// Internal callback method. JavaScript will call this function when the file import operation has been canceled by the user.
	void importedFileDataCanceled(int fileImportId);

private:

	/// In-memory cache for files that have been imported into the application through the web browser interface.
	std::map<QUrl, QByteArray> _importedFiles;

	/// Callback functions for file import operations in progress.
	std::map<int, std::function<void(const QUrl&)>> _importOperationCallbacks;

};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
