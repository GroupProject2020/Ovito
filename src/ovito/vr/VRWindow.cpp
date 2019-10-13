////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2017 Alexander Stukowski
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

#include <ovito/gui/GUI.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/dataset/DataSetContainer.h>
#include <ovito/core/viewport/Viewport.h>
#include <ovito/gui/properties/PropertiesPanel.h>
#include "VRWindow.h"

namespace VRPlugin {

/******************************************************************************
* Constructor.
******************************************************************************/
VRWindow::VRWindow(QWidget* parentWidget, GuiDataSetContainer* datasetContainer) : QMainWindow(parentWidget)
{
	// Use a default window size.
	resize(800, 600);

    // Set title.
	setWindowTitle(tr("Ovito - Virtual Reality Module"));

    // Create the widget for rendering.
	_glWidget = new VRRenderingWidget(this, datasetContainer->currentSet());
	setCentralWidget(_glWidget);

    // Create settings panel.
    PropertiesPanel* propPanel = new PropertiesPanel(this, datasetContainer->mainWindow());
    propPanel->setEditObject(_glWidget->settings());
	QDockWidget* dockWidget = new QDockWidget(tr("Settings"), this);
	dockWidget->setObjectName("SettingsPanel");
	dockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	dockWidget->setFeatures(QDockWidget::DockWidgetClosable);
	dockWidget->setWidget(propPanel);
	dockWidget->setTitleBarWidget(new QWidget());
	addDockWidget(Qt::LeftDockWidgetArea, dockWidget);

    // Close the VR window immediately when OVITO is closed or if another DataSet is loaded.
    connect(datasetContainer, &DataSetContainer::dataSetChanged, this, [this] {
        delete this;
    });

    // Delete window when it is being closed by the user.
    setAttribute(Qt::WA_DeleteOnClose);
}

};

