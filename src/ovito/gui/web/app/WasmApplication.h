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

#pragma once


#include <ovito/gui/web/GUIWeb.h>
#include <ovito/core/app/ApplicationService.h>
#include <ovito/core/app/StandaloneApplication.h>

#include <QQmlApplicationEngine>

namespace Ovito {

/**
 * \brief The application object used when running on the WebAssembly platform.
 */
class OVITO_GUIWEB_EXPORT WasmApplication : public StandaloneApplication
{
	Q_OBJECT

public:

	/// Constructor.
	WasmApplication() {
		// Always enable GUI mode when running in the web browser.
		_consoleMode = false;
		_headlessMode = false;
	}

	/// Handler function for exceptions.
	virtual void reportError(const Exception& exception, bool blocking) override;

	/// Returns a pointer to the main dataset container.
	WasmDataSetContainer* datasetContainer() const;

protected:

	/// Defines the program's command line parameters.
	virtual void registerCommandLineParameters(QCommandLineParser& parser) override;

	/// Prepares application to start running.
	virtual bool startupApplication() override;

	/// Is called at program startup once the event loop is running.
	virtual void postStartupInitialization() override;

private:

	/// List of errors to be displayed by showErrorMessages().
	std::deque<Exception> _errorList;

	/// The global Qml engine.
	QQmlApplicationEngine* _qmlEngine = nullptr;
};

}	// End of namespace
