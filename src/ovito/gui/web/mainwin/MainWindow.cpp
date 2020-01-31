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

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui)

/******************************************************************************
* Constructor.
******************************************************************************/
MainWindow::MainWindow() : _datasetContainer(this)
{
	// Create the object that manages the input modes of the viewports.
	setViewportInputManager(new ViewportInputManager(this, this, *datasetContainer()));
}

/******************************************************************************
* Destructor.
******************************************************************************/
MainWindow::~MainWindow()
{
	datasetContainer()->setCurrentSet(nullptr);
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

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
