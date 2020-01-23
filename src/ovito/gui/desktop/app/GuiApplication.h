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


#include <ovito/gui/desktop/GUI.h>
#include <ovito/core/utilities/Exception.h>
#include <ovito/core/app/StandaloneApplication.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui)

/**
 * \brief The main application with a graphical user interface.
 */
class OVITO_GUI_EXPORT GuiApplication : public StandaloneApplication
{
public:

	/// Create the global instance of the right QCoreApplication derived class.
	virtual void createQtApplication(int& argc, char** argv) override;

	/// Handler function for exceptions.
	virtual void reportError(const Exception& exception, bool blocking) override;

protected:

	/// Defines the program's command line parameters.
	virtual void registerCommandLineParameters(QCommandLineParser& parser) override;

	/// Interprets the command line parameters provided to the application.
	virtual bool processCommandLineParameters() override;

	/// Prepares application to start running.
	virtual bool startupApplication() override;

	/// Is called at program startup once the event loop is running.
	virtual void postStartupInitialization() override;

	/// Creates the global FileManager class instance.
	virtual FileManager* createFileManager() override;

	/// Handles events sent to the Qt application object.
	virtual bool eventFilter(QObject* watched, QEvent* event) override;

private Q_SLOTS:

	/// Displays an error message box. This slot is called by reportError().
	void showErrorMessages();

private:

	/// Initializes the graphical user interface of the application.
	void initializeGUI();

	/// List of errors to be displayed by showErrorMessages().
	std::deque<Exception> _errorList;

	Q_OBJECT
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
