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

#pragma once


#include <ovito/core/Core.h>
#include <ovito/core/utilities/Exception.h>

namespace Ovito {

/**
 * \brief The main application.
 */
class OVITO_CORE_EXPORT Application : public QObject
{
	Q_OBJECT

public:

	/// \brief Returns the one and only instance of this class.
	inline static Application* instance() { return _instance; }

	/// \brief Constructor.
	Application();

	/// \brief Destructor.
	virtual ~Application();

	/// \brief Initializes the application.
	/// \return \c true if the application was initialized successfully;
	///         \c false if an error occurred and the program should be terminated.
	bool initialize();

	/// \brief Handler method for Qt error messages.
	///
	/// This can be used to set a debugger breakpoint for the OVITO_ASSERT macros.
	static void qtMessageOutput(QtMsgType type, const QMessageLogContext& context, const QString& msg);

	/// \brief Returns whether the application has been started in graphical mode.
	/// \return \c true if the application should use a graphical user interface;
	///         \c false if the application has been started in the non-graphical console mode.
	bool guiMode() const { return !_consoleMode; }

	/// \brief Returns whether the application has been started in console mode.
	/// \return \c true if the application has been started in the non-graphical console mode;
	///         \c false if the application should use a graphical user interface.
	bool consoleMode() const { return _consoleMode; }

	/// \brief Returns whether the application runs in headless mode (without an X server on Linux and no OpenGL support).
	bool headlessMode() const { return _headlessMode; }

	/// \brief When in console mode, this specifies the exit code that will be returned by the application on shutdown.
	void setExitCode(int code) { _exitCode = code; }

	/// \brief Returns a pointer to the main dataset container.
	/// \return The dataset container of the first main window when running in GUI mode;
	///         or the global dataset container when running in console mode.
	DataSetContainer* datasetContainer() const;

	/// Returns the global FileManager class instance.
	FileManager* fileManager() const { return _fileManager.get(); }

	/// Returns the number of parallel threads to be used by the application when doing computations.
	int idealThreadCount() const { return _idealThreadCount; }

	/// Sets the number of parallel threads to be used by the application when doing computations.
	void setIdealThreadCount(int count) { _idealThreadCount = std::max(1, count); }

	/// Returns the major version number of the application.
	static int applicationVersionMajor();

	/// Returns the minor version number of the application.
	static int applicationVersionMinor();

	/// Returns the revision version number of the application.
	static int applicationVersionRevision();

	/// Returns the complete version string of the application release.
	static QString applicationVersionString();

	/// Returns the human-readable name of the application.
	static QString applicationName();

	/// Create the global instance of the right QCoreApplication derived class.
	virtual void createQtApplication(int& argc, char** argv);

	/// Handler function for exceptions.
	virtual void reportError(const Exception& exception, bool blocking);

	/// The possibles types of contexts in which the program's actions are performed.
	enum class ExecutionContext {
		Interactive,	///< Actions are currently performed by the interactive user.
		Scripting		///< Actions are currently performed by a script.
	};

	/// \brief Returns type of context in which the program's actions are currently performed.
	/// \note It is only safe to call this method from the main thread.
	ExecutionContext executionContext() const {
		OVITO_ASSERT(QThread::currentThread() == QCoreApplication::instance()->thread());
		return _executionContext;
	}

	/// Notifies the application that script execution has started or stopped.
	/// This is an internal method that should only be called by script engines.
	void switchExecutionContext(ExecutionContext context) {
		OVITO_ASSERT(QThread::currentThread() == QCoreApplication::instance()->thread());
		_executionContext = context;
	}

protected:

	/// Creates the global FileManager class instance.
	virtual FileManager* createFileManager();

	/// Indicates that the application is running in console mode.
	bool _consoleMode;

	/// Indicates that the application is running in headless mode (without OpenGL support).
	bool _headlessMode;

	/// Indicates that a script engine is executing code right now.
	/// If false, the program is running in interactive mode and all actions are performed by the human user.
	ExecutionContext _executionContext = ExecutionContext::Interactive;

	/// In console mode, this is the exit code returned by the application on shutdown.
	int _exitCode;

	/// The main dataset container.
	QPointer<DataSetContainer> _datasetContainer;

	/// The number of parallel threads to be used by the application when doing computations.
	int _idealThreadCount;

	/// The global file manager instance.
	std::unique_ptr<FileManager> _fileManager;

	/// The default message handler method of Qt.
	static QtMessageHandler defaultQtMessageHandler;

	/// The one and only instance of this class.
	static Application* _instance;
};

}	// End of namespace
