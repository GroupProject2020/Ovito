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
#include <ovito/gui/web/dataset/WasmFileManager.h>
#include <ovito/gui/web/viewport/ViewportWindow.h>
#include <ovito/core/utilities/io/FileManager.h>
#include <ovito/core/dataset/DataSetContainer.h>
#include <ovito/core/app/ApplicationService.h>
#include "WasmApplication.h"

#ifndef Q_OS_WASM
	#include <QApplication>
#endif

#if QT_FEATURE_static > 0
	#include <QtPlugin>
	// Explicitly import Qt plugins (needed for CMake builds)
	Q_IMPORT_PLUGIN(QtQuick2Plugin)       	// QtQuick
	Q_IMPORT_PLUGIN(QtQuick2WindowPlugin) 	// QtQuick.Window
	Q_IMPORT_PLUGIN(QtQuickLayoutsPlugin) 	// QtQuick.Layouts
	Q_IMPORT_PLUGIN(QtQuickTemplates2Plugin)// QtQuick.Templates
	Q_IMPORT_PLUGIN(QtQuickControls2Plugin) // QtQuick.Controls2
	Q_IMPORT_PLUGIN(QSvgIconPlugin) 		// SVG icon engine plugin
	#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
		Q_IMPORT_PLUGIN(QSvgPlugin)
		Q_IMPORT_PLUGIN(QWasmIntegrationPlugin)
	#endif
#endif

namespace Ovito {

/******************************************************************************
* Returns a pointer to the main dataset container.
******************************************************************************/
WasmDataSetContainer* WasmApplication::datasetContainer() const 
{
	return static_cast<WasmDataSetContainer*>(StandaloneApplication::datasetContainer()); 
}

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
* Create the global instance of the right QCoreApplication derived class.
******************************************************************************/
void WasmApplication::createQtApplication(int& argc, char** argv)
{
#ifdef Q_OS_WASM

	// Let the base class create a QtGui application object.
	StandaloneApplication::createQtApplication(argc, argv);

#else

	// On desktop platforms, enable high-resolution toolbar icons for high-dpi screens.
	QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
	QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

	// Create a QtWidget application object.
	new QApplication(argc, argv);

#endif

	// Specify the default OpenGL surface format.
	// When running in a web brwoser, try to obtain a WebGL 2.0 context if supported by the web browser.
	QSurfaceFormat::setDefaultFormat(OpenGLSceneRenderer::getDefaultSurfaceFormat());
}

/******************************************************************************
* Prepares application to start running.
******************************************************************************/
bool WasmApplication::startupApplication()
{
	// Make the C++ classes available as a Qt Quick items in QML. 
	qmlRegisterType<MainWindow>("org.ovito", 1, 0, "MainWindow");
	qmlRegisterType<ViewportsPanel>("org.ovito", 1, 0, "ViewportsPanel");
	qmlRegisterType<ViewportWindow>("org.ovito", 1, 0, "ViewportWindow");
	qmlRegisterUncreatableType<Viewport>("org.ovito", 1, 0, "Viewport", tr("Viewports cannot be created from QML."));
	qmlRegisterSingletonType<ViewportSettings>("org.ovito", 1, 0, "ViewportSettings", [](QQmlEngine* eng, QJSEngine*) -> QObject* {
		eng->setObjectOwnership(&ViewportSettings::getSettings(), QQmlEngine::CppOwnership);
		return &ViewportSettings::getSettings();
	});

	// Initialize the Qml engine.
	_qmlEngine = new QQmlApplicationEngine(this);
	_qmlEngine->load(QUrl(QStringLiteral("qrc:/gui/main.qml")));
	if(_qmlEngine->rootObjects().empty())
		return false;

	// Look up the main window in the Qt Quick scene.
	MainWindow* mainWin = _qmlEngine->rootObjects().front()->findChild<MainWindow*>();
	if(!mainWin) {
		qWarning() << "WARNING: No MainWindow instance found.";
		return false;
	}

	_datasetContainer = mainWin->datasetContainer();

	return true;
}

/******************************************************************************
* Creates the global FileManager class instance.
******************************************************************************/
FileManager* WasmApplication::createFileManager()
{
	return new WasmFileManager();
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

		// Import sample data.
		try {
//			datasetContainer()->importFile(Application::instance()->fileManager()->urlFromUserInput(":/gui/samples/test.data"));
//			datasetContainer()->importFile(Application::instance()->fileManager()->urlFromUserInput(":/gui/samples/animation.dump"));
			datasetContainer()->importFile(Application::instance()->fileManager()->urlFromUserInput(":/gui/samples/trajectory.xyz"));
		}
		catch(const Exception& ex) {
			ex.reportError();
		}
		newSet->undoStack().setClean();
	}

	StandaloneApplication::postStartupInitialization();
}

/******************************************************************************
* This is called on program shutdown.
******************************************************************************/
void WasmApplication::shutdown()
{
	// Release dataset and all contained objects.
	if(datasetContainer()) {
		datasetContainer()->setCurrentSet(nullptr);
		datasetContainer()->taskManager().cancelAllAndWait();
	}

	// Shut down QML engine.
	delete _qmlEngine;

	StandaloneApplication::shutdown();
}

/******************************************************************************
* Handler function for exceptions used in GUI mode.
******************************************************************************/
void WasmApplication::reportError(const Exception& exception, bool blocking)
{
	// Always display errors on the console.
	StandaloneApplication::reportError(exception, blocking);

	// If the exception has been thrown within the context of a DataSet or a DataSetContainer,
	// show the message box under the corresponding window.
	MainWindow* mainWindow;
	if(DataSet* dataset = qobject_cast<DataSet*>(exception.context())) {
		if(WasmDataSetContainer* container = qobject_cast<WasmDataSetContainer*>(dataset->container()))
			mainWindow = container->mainWindow();
		else
			mainWindow = nullptr;
	}
	else if(WasmDataSetContainer* datasetContainer = qobject_cast<WasmDataSetContainer*>(exception.context())) {
		mainWindow = datasetContainer->mainWindow();
	}
	else {
		mainWindow = qobject_cast<MainWindow*>(exception.context());
	}

	if(mainWindow) {
		// If the exception has additional message strings attached,
		// show them in the "Details" section of the popup dialog.
		QString detailedText;
		if(exception.messages().size() > 1) {
			for(int i = 1; i < exception.messages().size(); i++)
				detailedText += exception.messages()[i] + QStringLiteral("\n");
		}
		QMetaObject::invokeMethod(mainWindow, "showErrorMessage", Qt::QueuedConnection, Q_ARG(const QString&, exception.message()), Q_ARG(const QString&, detailedText));
	}
}

}	// End of namespace
