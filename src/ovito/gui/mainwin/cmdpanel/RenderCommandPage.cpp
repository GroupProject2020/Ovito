////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2013 Alexander Stukowski
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
#include <ovito/core/app/PluginManager.h>
#include <ovito/gui/actions/ActionManager.h>
#include <ovito/gui/mainwin/MainWindow.h>
#include <ovito/core/rendering/RenderSettings.h>
#include "RenderCommandPage.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/******************************************************************************
* Initializes the command panel page.
******************************************************************************/
RenderCommandPage::RenderCommandPage(MainWindow* mainWindow, QWidget* parent) : QWidget(parent)
{
	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setContentsMargins(2,2,2,2);

	QToolBar* toolbar = new QToolBar(this);
	toolbar->setStyleSheet("QToolBar { padding: 0px; margin: 0px; border: 0px none black; }");
	toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	layout->addWidget(toolbar);
	toolbar->addAction(mainWindow->actionManager()->getAction(ACTION_RENDER_ACTIVE_VIEWPORT));

	// Create the properties panel.
	propertiesPanel = new PropertiesPanel(this, mainWindow);
	propertiesPanel->setFrameStyle(QFrame::NoFrame | QFrame::Plain);
	layout->addWidget(propertiesPanel, 1);

	connect(&mainWindow->datasetContainer(), &DataSetContainer::dataSetChanged, this, &RenderCommandPage::onDataSetChanged);
}

/******************************************************************************
* This is called when a new dataset has been loaded.
******************************************************************************/
void RenderCommandPage::onDataSetChanged(DataSet* newDataSet)
{
	disconnect(_renderSettingsReplacedConnection);
	if(newDataSet) {
		_renderSettingsReplacedConnection = connect(newDataSet, &DataSet::renderSettingsReplaced, this, &RenderCommandPage::onRenderSettingsReplaced);
		onRenderSettingsReplaced(newDataSet->renderSettings());
	}
	else {
		onRenderSettingsReplaced(nullptr);
	}
}

/******************************************************************************
* This is called when new render settings have been loaded.
******************************************************************************/
void RenderCommandPage::onRenderSettingsReplaced(RenderSettings* newRenderSettings)
{
	propertiesPanel->setEditObject(newRenderSettings);
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
