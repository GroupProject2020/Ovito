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
#include <ovito/gui/web/mainwin/MainWindow.h>
#include <ovito/gui/web/mainwin/ViewportsPanel.h>
#include <ovito/gui/web/viewport/ViewportWindow.h>
#include <ovito/core/utilities/io/FileManager.h>
#include <ovito/core/dataset/DataSetContainer.h>
#include <ovito/core/app/ApplicationService.h>
#include "WasmApplication.h"

#if QT_FEATURE_static > 0
	#include <QtPlugin>
	// Explicitly import Qt plugins (needed for CMake builds)
	Q_IMPORT_PLUGIN(QtQuick2Plugin)       	// QtQuick
	Q_IMPORT_PLUGIN(QtQuick2WindowPlugin) 	// QtQuick.Window
	Q_IMPORT_PLUGIN(QtQuickLayoutsPlugin) 	// QtQuick.Layouts
	Q_IMPORT_PLUGIN(QWasmIntegrationPlugin)	// Note: This plugin is no longer needed in Qt 5.14
#endif

namespace Ovito {

/******************************************************************************
* Defines the program's command line parameters.
******************************************************************************/
void WasmApplication::registerCommandLineParameters(QCommandLineParser& parser)
{
	StandaloneApplication::registerCommandLineParameters(parser);

	// Only needed for compatibility with the desktop application. 
	// The core module expects this command option to be defined. 
	parser.addOption(QCommandLineOption(QStringList{{"noviewports"}}, tr("Do not create any viewports (for debugging purposes only).")));
}

/******************************************************************************
* Prepares application to start running.
******************************************************************************/
bool WasmApplication::startupApplication()
{
	// Make the C++ classes available as a Qt Quick items in QML. 
	qmlRegisterType<MainWindow>("org.ovito", 1, 0, "MainWindow");
	qmlRegisterType<ViewportsPanel>("org.ovito", 1, 0, "ViewportsPanel");
	qmlRegisterType<ViewportWindow>();

	// Initialize the Qml engine.
	_qmlEngine = new QQmlApplicationEngine(this);
	_qmlEngine->load(QStringLiteral(":/gui/main.qml"));

	// Look up the main window in the Qt Quick scene.
	MainWindow* mainWin = qobject_cast<MainWindow*>(_qmlEngine->rootObjects().front());
	OVITO_ASSERT(mainWin);

	_datasetContainer = &mainWin->datasetContainer();

	return true;
}

/******************************************************************************
* Is called at program startup once the event loop is running.
******************************************************************************/
void WasmApplication::postStartupInitialization()
{
	// Create an empty dataset if nothing has been loaded.
	if(datasetContainer()->currentSet() == nullptr) {
		OORef<DataSet> newSet = new DataSet();
		newSet->loadUserDefaults();
		datasetContainer()->setCurrentSet(newSet);
	}

	StandaloneApplication::postStartupInitialization();
}

/******************************************************************************
* Handler function for exceptions used in GUI mode.
******************************************************************************/
void WasmApplication::reportError(const Exception& ex, bool blocking)
{
	// Always display errors in the terminal window.
	StandaloneApplication::reportError(ex, blocking);
}

}	// End of namespace
