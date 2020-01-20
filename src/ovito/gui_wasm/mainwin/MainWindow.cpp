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

#include <ovito/gui_wasm/GUI.h>
#include <ovito/core/dataset/DataSetContainer.h>
#include <ovito/core/viewport/ViewportConfiguration.h>
#include <ovito/core/viewport/ViewportWindowInterface.h>
#include <ovito/core/app/StandaloneApplication.h>
#include "MainWindow.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui)

/******************************************************************************
* The constructor of the main window class.
******************************************************************************/
MainWindow::MainWindow(QQuickWindow* quickWindow) : QObject(quickWindow), _datasetContainer(this)
{
}

/******************************************************************************
* Destructor.
******************************************************************************/
MainWindow::~MainWindow()
{
	_datasetContainer.setCurrentSet(nullptr);
}

/******************************************************************************
* Returns the main window in which the given dataset is opened.
******************************************************************************/
MainWindow* MainWindow::fromDataset(DataSet* dataset)
{
	if(WasmDataSetContainer* container = qobject_cast<WasmDataSetContainer*>(dataset->container()))
		return container->mainWindow();
	return nullptr;
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
