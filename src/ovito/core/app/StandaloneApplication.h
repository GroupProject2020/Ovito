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
#include <ovito/core/app/ApplicationService.h>
#include "Application.h"

namespace Ovito {

/**
 * \brief The application object used when running as a standalone application.
 */
class OVITO_CORE_EXPORT StandaloneApplication : public Application
{
	Q_OBJECT

public:

	/// \brief Returns the one and only instance of this class.
	inline static StandaloneApplication* instance() {
		return qobject_cast<StandaloneApplication*>(Application::instance());
	}

	/// \brief Initializes the application.
	/// \param argc The number of command line arguments.
	/// \param argv The command line arguments.
	/// \return \c true if the application was initialized successfully;
	///         \c false if an error occurred and the program should be terminated.
	///
	/// This is called on program startup.
	bool initialize(int& argc, char** argv);

	/// \brief Enters the main event loop.
	/// \return The program exit code.
	///
	/// If the application has been started in console mode then this method does nothing.
	int runApplication();

	/// \brief Releases everything.
	///
	/// This is called before the application exits.
	void shutdown();

	/// \brief Returns the command line options passed to the program.
	const QCommandLineParser& cmdLineParser() const { return _cmdLineParser; }

	/// Returns the list of application services created at application startup.
	const std::vector<OORef<ApplicationService>>& applicationServices() const { return _applicationServices; }

protected Q_SLOTS:

	/// Is called at program startup once the event loop is running.
	virtual void postStartupInitialization();

protected:

	/// Defines the program's command line parameters.
	virtual void registerCommandLineParameters(QCommandLineParser& parser);

	/// Interprets the command line parameters provided to the application.
	virtual bool processCommandLineParameters();

	/// Prepares application at startup.
	virtual bool startupApplication() = 0;

protected:

	/// The parser for the command line options passed to the program.
	QCommandLineParser _cmdLineParser;

	/// The service objects created at application startup.
	std::vector<OORef<ApplicationService>> _applicationServices;
};

}	// End of namespace


