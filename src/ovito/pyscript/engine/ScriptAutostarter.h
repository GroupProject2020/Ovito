////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2014 Alexander Stukowski
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


#include <ovito/pyscript/PyScript.h>
#include <ovito/core/app/ApplicationService.h>

namespace PyScript {

using namespace Ovito;

/**
 * \brief An application service that is automatically invoked on application startup
 *        and that will execute a script files passed on the command line.
 */
class ScriptAutostarter : public ApplicationService
{
	Q_OBJECT
	OVITO_CLASS(ScriptAutostarter)

public:

	/// \brief Default constructor.
	Q_INVOKABLE ScriptAutostarter() = default;

	/// \brief Destructor, which is called at program exit.
	virtual ~ScriptAutostarter();

	/// \brief Registers plugin-specific command line options.
	virtual void registerCommandLineOptions(QCommandLineParser& cmdLineParser) override;

	/// \brief Is called after the application has been completely initialized.
	virtual void applicationStarted() override;

private:

	/// Indicates that the application is running in standalone mode and is using the embedded Python interpreter.
	bool _isStandaloneApplication = false;
};

}	// End of namespace
