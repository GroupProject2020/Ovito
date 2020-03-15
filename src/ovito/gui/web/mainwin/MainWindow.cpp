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
#include <ovito/gui/web/dataset/WasmFileManager.h>
#include <ovito/gui/base/viewport/ViewportInputManager.h>
#include <ovito/core/dataset/DataSetContainer.h>
#include "MainWindow.h"
#include "ModifierListModel.h"

namespace Ovito {

/******************************************************************************
* Constructor.
******************************************************************************/
MainWindow::MainWindow() : _datasetContainer(this)
{
	// Create the object that manages the input modes of the viewports.
	setViewportInputManager(new ViewportInputManager(this, this, *datasetContainer()));

	// For timed display of texts in the status bar:
	connect(&_statusBarTimer, &QTimer::timeout, this, &MainWindow::clearStatusBarMessage);

	// Create list of available modifiers.
	_modifierListModel = new ModifierListModel(this);

	// Create list model for the items in the selected data pipeline.
	_pipelineListModel = new PipelineListModel(_datasetContainer, this);
}

/******************************************************************************
* Destructor.
******************************************************************************/
MainWindow::~MainWindow()
{
	datasetContainer()->setCurrentSet(nullptr);
}

/******************************************************************************
* Displays a message string in the window's status bar.
******************************************************************************/
void MainWindow::showStatusBarMessage(const QString& message, int timeout)
{
	if(message != _statusBarText) {
		_statusBarText = message;
		Q_EMIT statusBarTextChanged(_statusBarText);
		if(timeout > 0) {
			_statusBarTimer.start(timeout);
		}
		else {
			_statusBarTimer.stop();
		}
	}
}

/******************************************************************************
* Lets the user select a file on the local computer to be imported into the scene.
******************************************************************************/
void MainWindow::importDataFile()
{
	WasmFileManager::importFileIntoMemory(this, QStringLiteral("*"), [this](const QUrl& url) {
		try {
			if(url.isValid())
				datasetContainer()->importFile(url);
		}
		catch(const Exception& ex) {
			ex.reportError();
		}
	});
}

}	// End of namespace
