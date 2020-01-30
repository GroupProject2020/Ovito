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
#include <ovito/core/utilities/concurrent/TaskManager.h>
#include <ovito/core/dataset/DataSetContainer.h>
#include <ovito/core/app/Application.h>
#include "WasmFileManager.h"

#ifdef Q_OS_WASM
    #include <emscripten.h>
    #include <emscripten/html5.h>
#endif

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(IO)

/******************************************************************************
* Makes a file available on this computer.
******************************************************************************/
SharedFuture<FileHandle> WasmFileManager::fetchUrl(TaskManager& taskManager, const QUrl& url)
{
	if(url.scheme() == QStringLiteral("imported")) {
		QUrl normalizedUrl = normalizeUrl(url);
		QMutexLocker lock(&mutex());

		// Check if requested URL is in the file cache.
        const auto& cacheEntry = _importedFiles.find(normalizedUrl);
		if(cacheEntry == _importedFiles.end())
            return Future<FileHandle>::createFailed(Exception(tr("Requested file does not exist in imported file set:\n%1").arg(url.fileName()), taskManager.datasetContainer()));

        // Return a file handle referring to the file data buffer previously loaded into application memory.
        return FileHandle(url, cacheEntry->second);
	}
    return FileManager::fetchUrl(taskManager, url);
}

/******************************************************************************
* Lists all files in a remote directory.
******************************************************************************/
Future<QStringList> WasmFileManager::listDirectoryContents(TaskManager& taskManager, const QUrl& url)
{
	if(url.scheme() == QStringLiteral("imported")) {
		QUrl normalizedUrl = normalizeUrl(url);
		QMutexLocker lock(&mutex());

        QStringList fileList;
        for(const auto& importEntry : _importedFiles) {
            if(importEntry.first.host() == url.host() && importEntry.first.path().startsWith(url.path()))
                fileList.push_back(importEntry.first.fileName());
        }
        return std::move(fileList);
	}
    return FileManager::listDirectoryContents(taskManager, url);
}

#ifdef Q_OS_WASM

/******************************************************************************
* Internal callback method. JavaScript will call this function when the 
* imported file data is ready.
******************************************************************************/
void WasmFileManager::importedFileDataReady(char* content, size_t contentSize, const char* fileName, int fileImportId)
{
    // Copy file data into a QByteArray and free buffer that was allocated on the JavaScript side.
    QByteArray fileContent(content, contentSize);
    ::free(content);

    // Look up the callback registered for the import operation.
    QMutexLocker locker(&mutex());
    auto callbackIter = _importOperationCallbacks.find(fileImportId);
    if(callbackIter == _importOperationCallbacks.end())
        return;
    auto callback = std::move(callbackIter->second);
    _importOperationCallbacks.erase(callbackIter);

    // Generate a unique ID for the file to avoid name clashes if 
    // the user imports several different files having the same filename.
    // Generate the URL under which the imported file will be accessible in the application.
    QUrl url;
    url.setScheme(QStringLiteral("imported"));
    url.setHost(QString::number(QDateTime::currentMSecsSinceEpoch()));
    url.setPath(QStringLiteral("/") + QString::fromUtf8(fileName), QUrl::DecodedMode);

    // Store the file content in the cache for subsequent access by other parts of the program.
    _importedFiles[url] = std::move(fileContent);
    locker.unlock();

    // Notify callback function that the import operation has been completed.
    callback(url);
}

/******************************************************************************
* Internal callback method. JavaScript will call this function when the file 
* import operation has been canceled by the user.
******************************************************************************/
void WasmFileManager::importedFileDataCanceled(int fileImportId)
{
    // Look up the callback registered for the import operation.
    QMutexLocker locker(&mutex());
    auto callbackIter = _importOperationCallbacks.find(fileImportId);
    if(callbackIter == _importOperationCallbacks.end())
        return;
    auto callback = std::move(callbackIter->second);
    _importOperationCallbacks.erase(callbackIter);
    locker.unlock();

    // Notify callback function that the operation has been canceled by the user.
    callback(QUrl());
}

/******************************************************************************
* Global user file data ready callback and C helper function. JavaScript will
* call this function when the file data is ready.
******************************************************************************/
extern "C" EMSCRIPTEN_KEEPALIVE void ovitoFileDataReady(char* content, size_t contentSize, const char* fileName, int fileImportId)
{
    // Forward call to WasmFileManager class method:
    static_cast<WasmFileManager*>(Application::instance()->fileManager())->importedFileDataReady(content, contentSize, fileName, fileImportId);
}

/******************************************************************************
* Global callback function called by the JavaScript side if the user has 
* canceled the file import operation. 
******************************************************************************/
extern "C" EMSCRIPTEN_KEEPALIVE void ovitoFileDataCanceled(int fileImportId)
{
    // Forward call to WasmFileManager class method:
    static_cast<WasmFileManager*>(Application::instance()->fileManager())->importedFileDataCanceled(fileImportId);
}

/******************************************************************************
* Opens a file dialog in the browser allowing the user to import a file from 
* the local computer into the application. 
******************************************************************************/
void WasmFileManager::importFileIntoMemory(MainWindow* mainWindow, const QString& acceptedFileTypes, std::function<void(const QUrl&)> callback)
{
    QByteArray acceptedFileTypesUtf8 = acceptedFileTypes.toUtf8();

    // Generate a unique ID for this import operation.
    static int fileImportId = 0;

    // Store away the callback function, which will get called upon success of the import operation.
    WasmFileManager* this_ = static_cast<WasmFileManager*>(Application::instance()->fileManager());
    QMutexLocker locker(&this_->mutex());
    this_->_importOperationCallbacks[fileImportId] = std::move(callback);

    EM_ASM_({
        const accept = UTF8ToString($0);
        const fileId = $1;

        // Create file input element, which will display the native file dialog.
        var fileElement = document.createElement("input");
        fileElement.type = "file";
        fileElement.style = "display:none";
        fileElement.accept = accept;
        fileElement.onchange = function(event) {
            const files = event.target.files;

            // Notify the C++ side in case the user has not selected any file.
            // Note: This doesn't work yet, because the browser won't invoke onchange() if the user
            // presses the cancel button of the file selection dialog.
            if(files.length == 0) {
                ccall("ovitoFileDataCanceled", null, ["number"], [fileId]);
            }

            // Read selected file(s).
            for(var i = 0; i < files.length; i++) {
                const file = files[i];
                var reader = new FileReader();
                reader.onload = function() {
                    const name = file.name;
                    var contentArray = new Uint8Array(reader.result);
                    const contentSize = reader.result.byteLength;

                    // Copy the file content to the C++ heap.
                    // Note: this could be simplified by passing the content as an
                    // "array" type to ccall and then let it copy to C++ memory.
                    // However, this built-in solution does not handle files larger
                    // than ~15M (Chrome). Instead, allocate memory manually and
                    // pass a pointer to the C++ side (which will free() it when done).

                    // TODO: consider slice()ing the file to read it piecewise and
                    // then assembling it in a QByteArray on the C++ side.

                    const heapPointer = _malloc(contentSize);
                    const heapBytes = new Uint8Array(Module.HEAPU8.buffer, heapPointer, contentSize);
                    heapBytes.set(contentArray);

                    // Null out the first data copy to enable GC
                    reader = null;
                    contentArray = null;

                    // Call the C++ file data ready callback.
                    ccall("ovitoFileDataReady", null,
                        ["number", "number", "string", "number"], [heapPointer, contentSize, name, fileId]);
                };
                reader.readAsArrayBuffer(file);
            }

            // Clean up document.
            document.body.removeChild(fileElement);

        }; // onchange callback

        // Trigger file dialog open.
        document.body.appendChild(fileElement);
        fileElement.click();

    }, acceptedFileTypesUtf8.constData(), fileImportId++);
}

#else  // !Q_OS_WASM

/******************************************************************************
* Opens a file dialog in the browser allowing the user to import a file from 
* the local computer into the application. 
******************************************************************************/
void WasmFileManager::importFileIntoMemory(MainWindow* mainWindow, const QString& acceptedFileTypes, std::function<void(const QUrl&)> callback)
{
    // Use the FileDialog QML component to let the user select a file for import.

    QQmlComponent fileDialogComponent(qmlContext(mainWindow)->engine(), QUrl::fromLocalFile(":/gui/ui/ImportDialog.qml"), QQmlComponent::PreferSynchronous);
    if(fileDialogComponent.isError()) {
        qWarning() << "WasmFileManager::importFileIntoMemory():" << fileDialogComponent.errors();
        callback(QUrl());
        return;
    }

    QObject* importDialog = fileDialogComponent.create();
    if(!importDialog) {
        qWarning() << "WasmFileManager::importFileIntoMemory(): Creation of FileDialog component failed.";
        callback(QUrl());
        return;
    }

    // Generate a unique ID for this import operation.
    static int fileImportId = 1;

    // Store away the callback function, which will get called upon success of the import operation.
    WasmFileManager* this_ = static_cast<WasmFileManager*>(Application::instance()->fileManager());
    QMutexLocker locker(&this_->mutex());
    this_->_importOperationCallbacks[fileImportId] = std::move(callback);

    importDialog->setParent(mainWindow);
    importDialog->setProperty("importFileId", fileImportId++);

    QObject::connect(importDialog, SIGNAL(accepted()), this_, SLOT(importedFileDataReady()));
    QObject::connect(importDialog, SIGNAL(accepted()), importDialog, SLOT(deleteLater()));
    QObject::connect(importDialog, SIGNAL(rejected()), this_, SLOT(importedFileDataCanceled()));
    QObject::connect(importDialog, SIGNAL(rejected()), importDialog, SLOT(deleteLater()));
    
    QMetaObject::invokeMethod(importDialog, "open");
}

/******************************************************************************
* Internal callback method. JavaScript will call this function when the 
* imported file data is ready.
******************************************************************************/
void WasmFileManager::importedFileDataReady()
{
    QObject* importDialog = sender();
    int fileImportId = importDialog->property("importFileId").toInt();

    // Look up the callback registered for the import operation.
    QMutexLocker locker(&mutex());
    auto callbackIter = _importOperationCallbacks.find(fileImportId);
    if(callbackIter == _importOperationCallbacks.end())
        return;
    auto callback = std::move(callbackIter->second);
    _importOperationCallbacks.erase(callbackIter);

    // Read file data into a QByteArray.
    QUrl fileUrl = importDialog->property("fileUrl").toUrl();
    QFile file(fileUrl.toLocalFile());
    if(!file.open(QIODevice::ReadOnly)) {
        MainWindow* mainWindow = static_cast<MainWindow*>(importDialog->parent());
        mainWindow->showErrorMessage(tr("Could not read file '%1'.").arg(file.fileName()), file.errorString());
        callback(QUrl());
        return;
    }
    QByteArray fileContent = file.readAll();
    
    // Generate a unique ID for the file to avoid name clashes if 
    // the user imports several different files having the same filename.
    // Generate the URL under which the imported file will be accessible in the application.
    QUrl url;
    url.setScheme(QStringLiteral("imported"));
    url.setHost(QString::number(QDateTime::currentMSecsSinceEpoch()));
    url.setPath(QStringLiteral("/") + url.fileName(), QUrl::DecodedMode);

    // Store the file content in the cache for subsequent access by other parts of the program.
    _importedFiles[url] = std::move(fileContent);
    locker.unlock();

    // Notify callback function that the import operation has been completed.
    callback(url);
}

/******************************************************************************
* Internal callback method. JavaScript will call this function when the file 
* import operation has been canceled by the user.
******************************************************************************/
void WasmFileManager::importedFileDataCanceled()
{
    QObject* importDialog = sender();
    int fileImportId = importDialog->property("importFileId").toInt();

    // Look up the callback registered for the import operation.
    QMutexLocker locker(&mutex());
    auto callbackIter = _importOperationCallbacks.find(fileImportId);
    if(callbackIter == _importOperationCallbacks.end())
        return;
    auto callback = std::move(callbackIter->second);
    _importOperationCallbacks.erase(callbackIter);
    locker.unlock();

    // Notify callback function that the operation has been canceled by the user.
    callback(QUrl());
}

#endif

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
