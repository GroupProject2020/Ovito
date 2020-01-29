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
#include <ovito/gui/base/viewport/ViewportInputManager.h>
#include <ovito/core/dataset/DataSetContainer.h>
#include "MainWindow.h"

/******************************************************************************
* Global user file data ready callback and C helper function. JavaScript will
* call this function when the file data is ready; the helper then forwards
* the call to the current handler function. This means there can be only one
* file open in proress at a given time.
******************************************************************************/
//extern "C" EMSCRIPTEN_KEEPALIVE void fileDataReadyCallback(char *content, size_t contentSize, const char *fileName)
//{

//}

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui)

/******************************************************************************
* Constructor.
******************************************************************************/
MainWindow::MainWindow() : _datasetContainer(this)
{
	// Create the object that manages the input modes of the viewports.
	setViewportInputManager(new ViewportInputManager(this, this, datasetContainer()));
}

/******************************************************************************
* Destructor.
******************************************************************************/
MainWindow::~MainWindow()
{
	datasetContainer().setCurrentSet(nullptr);
}

/******************************************************************************
* Lets the user select a file on the local computer to be imported into the scene.
******************************************************************************/
void MainWindow::importDataFile()
{
	try {
#ifdef Q_OS_WASM
//		if(::g_qtFileDataReadyCallback)
//			qWarning() << "Warning: Concurrent file imports are not supported. Cancelling earlier import operation.";

		// Call fileDataReadyCallback() to make sure the emscripten linker does not
        // optimize it away, which may happen if the function is called from JavaScript
        // only. Set g_qtFileDataReadyCallback to null to make it a no-op.
//        ::g_qtFileDataReadyCallback = nullptr;
 //       fileDataReadyCallback(nullptr, 0, nullptr);
#else
//		QFileDialog()
#endif
//		datasetContainer().importFile(fileUrl);
	}
	catch(const Exception& ex) {
		ex.reportError();
	}
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
