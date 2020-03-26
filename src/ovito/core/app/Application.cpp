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

#include <ovito/core/Core.h>
#include <ovito/core/dataset/UndoStack.h>
#include <ovito/core/dataset/DataSetContainer.h>
#include <ovito/core/utilities/io/FileManager.h>
#include "Application.h"

// Called from Application::initialize() to register the embedded Qt resource files
// when running a statically linked executable. Accordings to the Qt documentation this
// needs to happen from oustide of any C++ namespace.
static void registerQtResources()
{
#ifdef OVITO_BUILD_MONOLITHIC
	Q_INIT_RESOURCE(core);
	Q_INIT_RESOURCE(opengl);
	#if defined(OVITO_BUILD_GUI) || defined(OVITO_BUILD_WEBGUI)
		Q_INIT_RESOURCE(guibase);
		Q_INIT_RESOURCE(gui);
	#endif
#endif
}

namespace Ovito {

/// The one and only instance of this class.
Application* Application::_instance = nullptr;

/// Stores a pointer to the original Qt message handler function, which has been replaced with our own handler.
QtMessageHandler Application::defaultQtMessageHandler = nullptr;

/******************************************************************************
* Handler method for Qt error messages.
* This can be used to set a debugger breakpoint for the OVITO_ASSERT macros.
******************************************************************************/
void Application::qtMessageOutput(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
	// Forward message to default handler.
	if(defaultQtMessageHandler) defaultQtMessageHandler(type, context, msg);
	else std::cerr << qPrintable(msg) << std::endl;
}

/******************************************************************************
* Constructor.
******************************************************************************/
Application::Application() : _exitCode(0), _consoleMode(true), _headlessMode(true)
{
	// Set global application pointer.
	OVITO_ASSERT(_instance == nullptr);	// Only allowed to create one Application class instance.
	_instance = this;

	// Use all procesor cores by default.
	_idealThreadCount = std::max(1, QThread::idealThreadCount());
}

/******************************************************************************
* Destructor.
******************************************************************************/
Application::~Application()
{
	_instance = nullptr;
}

/******************************************************************************
* Returns the major version number of the application.
******************************************************************************/
int Application::applicationVersionMajor()
{
	// This compile-time constant is defined by the CMake build script.
	return OVITO_VERSION_MAJOR;
}

/******************************************************************************
* Returns the minor version number of the application.
******************************************************************************/
int Application::applicationVersionMinor()
{
	// This compile-time constant is defined by the CMake build script.
	return OVITO_VERSION_MINOR;
}

/******************************************************************************
* Returns the revision version number of the application.
******************************************************************************/
int Application::applicationVersionRevision()
{
	// This compile-time constant is defined by the CMake build script.
	return OVITO_VERSION_REVISION;
}

/******************************************************************************
* Returns the complete version string of the application release.
******************************************************************************/
QString Application::applicationVersionString()
{
	// This compile-time constant is defined by the CMake build script.
	return QStringLiteral(OVITO_VERSION_STRING);
}

/******************************************************************************
* Returns the human-readable name of the application.
******************************************************************************/
QString Application::applicationName()
{
	// This compile-time constant is defined by the CMake build script.
	return QStringLiteral(OVITO_APPLICATION_NAME);
}

/******************************************************************************
* This is called on program startup.
******************************************************************************/
bool Application::initialize()
{
	// Install custom Qt error message handler to catch fatal errors in debug mode.
	defaultQtMessageHandler = qInstallMessageHandler(qtMessageOutput);

	// Activate default "C" locale, which will be used to parse numbers in strings.
	std::setlocale(LC_ALL, "C");

	// Suppress console messages "qt.network.ssl: QSslSocket: cannot resolve ..."
	qputenv("QT_LOGGING_RULES", "qt.network.ssl.warning=false");

	// Register our floating-point data type with the Qt type system.
	qRegisterMetaType<FloatType>("FloatType");

	// Register generic object reference type with the Qt type system.
	qRegisterMetaType<OORef<OvitoObject>>("OORef<OvitoObject>");

	// Register Qt stream operators for basic types.
	qRegisterMetaTypeStreamOperators<Vector2>("Ovito::Vector2");
	qRegisterMetaTypeStreamOperators<Vector3>("Ovito::Vector3");
	qRegisterMetaTypeStreamOperators<Vector4>("Ovito::Vector4");
	qRegisterMetaTypeStreamOperators<Point2>("Ovito::Point2");
	qRegisterMetaTypeStreamOperators<Point3>("Ovito::Point3");
	qRegisterMetaTypeStreamOperators<AffineTransformation>("Ovito::AffineTransformation");
	qRegisterMetaTypeStreamOperators<Matrix3>("Ovito::Matrix3");
	qRegisterMetaTypeStreamOperators<Matrix4>("Ovito::Matrix4");
	qRegisterMetaTypeStreamOperators<Box2>("Ovito::Box2");
	qRegisterMetaTypeStreamOperators<Box3>("Ovito::Box3");
	qRegisterMetaTypeStreamOperators<Rotation>("Ovito::Rotation");
	qRegisterMetaTypeStreamOperators<Scaling>("Ovito::Scaling");
	qRegisterMetaTypeStreamOperators<Quaternion>("Ovito::Quaternion");
	qRegisterMetaTypeStreamOperators<Color>("Ovito::Color");
	qRegisterMetaTypeStreamOperators<ColorA>("Ovito::ColorA");

	// Register Qt conversion operators for custom types.
	QMetaType::registerConverter<QColor, Color>();
	QMetaType::registerConverter<Color, QColor>();
	QMetaType::registerConverter<QColor, ColorA>();
	QMetaType::registerConverter<ColorA, QColor>();

	// Register Qt resources.
	::registerQtResources();

	// Create global FileManager object.
	_fileManager.reset(createFileManager());

	return true;
}

/******************************************************************************
* Create the global instance of the right QCoreApplication derived class.
******************************************************************************/
void Application::createQtApplication(int& argc, char** argv)
{
	if(headlessMode()) {
#if defined(Q_OS_LINUX)
		// Determine font directory path.
		std::string applicationPath = argv[0];
		auto sepIndex = applicationPath.rfind('/');
		if(sepIndex != std::string::npos)
			applicationPath.resize(sepIndex + 1);
		std::string fontPath = applicationPath + "../share/ovito/fonts";
		if(!QDir(QString::fromStdString(fontPath)).exists())
			fontPath = "/usr/share/fonts";

		// On Linux, use the 'minimal' QPA platform plugin instead of the standard XCB plugin when no X server is available.
		// Still create a Qt GUI application object, because otherwise we cannot use (offscreen) font rendering functions.
		qputenv("QT_QPA_PLATFORM", "minimal");
		// Enable rudimentary font rendering support, which is implemented by the 'minimal' platform plugin:
		qputenv("QT_DEBUG_BACKINGSTORE", "1");
		qputenv("QT_QPA_FONTDIR", fontPath.c_str());

		new QGuiApplication(argc, argv);
#elif defined(Q_OS_MAC)
		new QGuiApplication(argc, argv);
#else
		new QCoreApplication(argc, argv);
#endif
	}
	else {
		new QGuiApplication(argc, argv);
	}
}

/******************************************************************************
* Returns a pointer to the main dataset container.
******************************************************************************/
DataSetContainer* Application::datasetContainer() const
{
	return _datasetContainer;
}

/******************************************************************************
* Creates the global FileManager class instance.
******************************************************************************/
FileManager* Application::createFileManager()
{
	return new FileManager();
}

/******************************************************************************
* Handler function for exceptions.
******************************************************************************/
void Application::reportError(const Exception& exception, bool /*blocking*/)
{
	for(int i = exception.messages().size() - 1; i >= 0; i--) {
		std::cerr << "ERROR: " << qPrintable(exception.messages()[i]) << std::endl;
	}
	std::cerr << std::flush;
}

#ifndef Q_OS_WASM
/******************************************************************************
* Returns the application-wide network manager object.
******************************************************************************/
QNetworkAccessManager* Application::networkAccessManager()
{
	if(!_networkAccessManager)
		_networkAccessManager = new QNetworkAccessManager(this);
	return _networkAccessManager;
}
#endif

}	// End of namespace
