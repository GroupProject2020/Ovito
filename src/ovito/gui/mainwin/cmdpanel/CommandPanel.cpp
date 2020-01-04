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
#include "CommandPanel.h"
#include "RenderCommandPage.h"
#include "ModifyCommandPage.h"
#include "OverlayCommandPage.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/******************************************************************************
* The constructor of the command panel class.
******************************************************************************/
CommandPanel::CommandPanel(MainWindow* mainWindow, QWidget* parent) : QWidget(parent)
{
	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setContentsMargins(0,0,0,0);

	// Create tab widget
	_tabWidget = new QTabWidget(this);
	layout->addWidget(_tabWidget, 1);

	// Create the tabs.
	_tabWidget->setDocumentMode(true);
	_tabWidget->addTab(_modifyPage = new ModifyCommandPage(mainWindow, _tabWidget), QIcon(":/gui/mainwin/command_panel/tab_modify.bw.svg"), QString());
	_tabWidget->addTab(_renderPage = new RenderCommandPage(mainWindow, _tabWidget), QIcon(":/gui/mainwin/command_panel/tab_render.bw.svg"), QString());
	_tabWidget->addTab(_overlayPage = new OverlayCommandPage(mainWindow, _tabWidget), QIcon(":/gui/mainwin/command_panel/tab_overlays.bw.svg"), QString());
	_tabWidget->setTabToolTip(0, tr("Pipelines"));
	_tabWidget->setTabToolTip(1, tr("Rendering"));
	_tabWidget->setTabToolTip(2, tr("Viewport layers"));
	setCurrentPage(MainWindow::MODIFY_PAGE);
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
