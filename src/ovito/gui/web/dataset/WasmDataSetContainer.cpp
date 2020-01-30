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
#include <ovito/core/app/PluginManager.h>
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
	// Prepare scene for display whenever a new dataset becomes active.
	if(Application::instance()->guiMode()) {
		connect(this, &DataSetContainer::dataSetChanged, this, [this](DataSet* dataset) {
			if(dataset) {
				_sceneReadyScheduled = true;
				Q_EMIT scenePreparationBegin();
				dataset->whenSceneReady().finally(executor(), [this]() {
					_sceneReadyScheduled = false;
					sceneBecameReady();
				});
			}
		});
	}
}

/******************************************************************************
* Is called when a RefTarget referenced by this object has generated an event.
******************************************************************************/
bool WasmDataSetContainer::referenceEvent(RefTarget* source, const ReferenceEvent& event)
{
	if(source == currentSet()) {
		if(Application::instance()->guiMode()) {
			if(event.type() == ReferenceEvent::TargetChanged) {
				// Update viewports as soon as the scene becomes ready.
				if(!_sceneReadyScheduled) {
					_sceneReadyScheduled = true;
					Q_EMIT scenePreparationBegin();
					currentSet()->whenSceneReady().finally(executor(), [this]() {
						_sceneReadyScheduled = false;
						sceneBecameReady();
					});
				}
			}
			else if(event.type() == ReferenceEvent::PreliminaryStateAvailable) {
				// Update viewports when a new preliminiary state from one of the data pipelines
				// becomes available (unless we are playing an animation).
				if(currentSet()->animationSettings()->isPlaybackActive() == false)
					currentSet()->viewportConfig()->updateViewports();
			}
		}
	}
	return DataSetContainer::referenceEvent(source, event);
}

/******************************************************************************
* Is called when scene of the current dataset is ready to be displayed.
******************************************************************************/
void WasmDataSetContainer::sceneBecameReady()
{
	if(currentSet())
		currentSet()->viewportConfig()->updateViewports();
	Q_EMIT scenePreparationEnd();
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
		if(!importer) {
			QString fileFormatList;
			for(const FileImporterClass* importerClass : PluginManager::instance().metaclassMembers<FileImporter>()) {
				fileFormatList += QStringLiteral("<li>%1</li>").arg(importerClass->fileFilterDescription().toHtmlEscaped());
			}
			currentSet()->throwException(tr("<p>Could not detect the format of the imported file. This version of OVITO supports the following formats:</p><p><ul>%1</ul></p>").arg(fileFormatList));
		}
	}
	else {
		importer = static_object_cast<FileImporter>(importerType->createInstance(currentSet()));
		if(!importer)
			currentSet()->throwException(tr("Failed to import file. Could not initialize file reader."));
	}

	// Load user-defined default settings for the importer.
	importer->loadUserDefaults();

	// Specify how the file's data should be inserted into the current scene.
	FileImporter::ImportMode importMode = FileImporter::ResetScene;

	return importer->importFile({url}, importMode, true);
}

}	// End of namespace
