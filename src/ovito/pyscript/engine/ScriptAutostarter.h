///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2014) Alexander Stukowski
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
};

}	// End of namespace
