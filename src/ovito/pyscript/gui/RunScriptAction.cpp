////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2016 Alexander Stukowski
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

#include <ovito/pyscript/PyScript.h>
#include <ovito/core/dataset/DataSetContainer.h>
#include <ovito/core/utilities/concurrent/AsyncOperation.h>
#include <ovito/gui/utilities/concurrent/ProgressDialog.h>
#include <ovito/gui/actions/ActionManager.h>
#include <ovito/gui/dialogs/HistoryFileDialog.h>
#include <ovito/gui/mainwin/MainWindow.h>
#include <ovito/pyscript/engine/ScriptEngine.h>
#include "RunScriptAction.h"

namespace PyScript {

IMPLEMENT_OVITO_CLASS(RunScriptAction);

/******************************************************************************
* Is called when a new main window is created.
******************************************************************************/
void RunScriptAction::registerActions(ActionManager& actionManager)
{
	// Register an action, which allows the user to run a Python script file.
	QAction* runScriptFileAction = actionManager.createCommandAction(ACTION_SCRIPTING_RUN_FILE, tr("Run Python script..."), ":/gui/actions/file/scripting_manual.bw.svg");

	connect(runScriptFileAction, &QAction::triggered, [&actionManager]() {
		// Let the user select a script file on disk.
		HistoryFileDialog dlg("ScriptFile", actionManager.mainWindow(), tr("Run Script File"), QString(), tr("Python scripts (*.py)"));
		if(dlg.exec() != QDialog::Accepted)
			return;
		QString scriptFile = dlg.selectedFiles().front();
		DataSet* dataset = actionManager.mainWindow()->datasetContainer().currentSet();

		// Execute the script file.
		// Keep undo records so that script actions can be undone.
		dataset->undoStack().beginCompoundOperation(tr("Script actions"));
		try {
			// Show a progress dialog while script is running.
			ProgressDialog progressDialog(actionManager.mainWindow(), tr("Script execution"));
			OVITO_ASSERT(&progressDialog.taskManager() == &dataset->container()->taskManager());
			AsyncOperation scriptOperation(progressDialog.taskManager());

			// Execute the script file in a fresh and private namespace environment.
			ScriptEngine::executeFile(scriptFile, dataset, scriptOperation.task(), nullptr, false);
		}
		catch(const Exception& ex) {
			ex.reportError();
		}
		dataset->undoStack().endCompoundOperation();
	});
}

};
