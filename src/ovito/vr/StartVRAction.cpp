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

#include <ovito/gui/desktop/GUI.h>
#include <ovito/gui/desktop/actions/ActionManager.h>
#include <ovito/gui/desktop/mainwin/MainWindow.h>
#include <ovito/core/app/StandaloneApplication.h>
#include "StartVRAction.h"
#include "VRWindow.h"

namespace VRPlugin {

IMPLEMENT_OVITO_CLASS(StartVRAction);

/******************************************************************************
* Is called when a new main window is created.
******************************************************************************/
void StartVRAction::registerActions(ActionManager& actionManager)
{
	// Register an action, which allows the user to run a Python script file.
	QAction* startVRAction = actionManager.createCommandAction("StartVR", tr("Start VR module..."));

	connect(startVRAction, &QAction::triggered, [&actionManager]() {
		try {
			VRWindow* window = new VRWindow(actionManager.mainWindow(), &actionManager.mainWindow()->datasetContainer());
			window->show();
		}
		catch(const Exception& ex) {
			ex.reportError();
		}
	});
}

/******************************************************************************
* Is called when the main menu is created.
******************************************************************************/
void StartVRAction::addActionsToMenu(ActionManager& actionManager, QMenuBar* menuBar)
{
	QAction* startVRAction = actionManager.findAction("StartVR");
	if(!startVRAction) return;

	QMenu* vrMenu = menuBar->addMenu(tr("&Virtual Reality"));
	vrMenu->setObjectName(QStringLiteral("VRMenu"));
	vrMenu->addAction(startVRAction);
}

/******************************************************************************
* Registers plugin-specific command line options.
******************************************************************************/
void StartVRAction::registerCommandLineOptions(QCommandLineParser& cmdLineParser)
{
	// Register the --vr command line option.
	cmdLineParser.addOption(QCommandLineOption("vr", tr("Invokes the virtual reality module.")));
}

/******************************************************************************
* Is called after the application has been completely initialized.
******************************************************************************/
void StartVRAction::applicationStarted()
{
	// Handle the --vr command line option.
	if(StandaloneApplication::instance()->cmdLineParser().isSet("vr")) {

		// Trigger the 'Start VR' command action, which has been registered by registerActions() above.
		GuiDataSetContainer* container = qobject_cast<GuiDataSetContainer*>(StandaloneApplication::instance()->datasetContainer());
		container->mainWindow()->actionManager()->findAction("StartVR")->trigger();
	}
}

} // End of namespace
