///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2017) Alexander Stukowski
//
//  This file is part of OVITO (Open Visualization Tool).
//
//  OVITO is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  OVITO is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////

#include <gui/GUI.h>
#include <gui/actions/ActionManager.h>
#include <gui/mainwin/MainWindow.h>
#include <core/app/StandaloneApplication.h>
#include "StartVRAction.h"
#include "VRWindow.h"

namespace VRPlugin {

IMPLEMENT_OVITO_OBJECT(StartVRAction, GuiAutoStartObject);

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


};
