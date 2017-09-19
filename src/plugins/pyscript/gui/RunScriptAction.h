///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2016) Alexander Stukowski
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


#include <plugins/pyscript/PyScript.h>
#include <gui/app/GuiApplicationService.h>

namespace PyScript {

using namespace Ovito;

/**
 * \brief An application service that is automatically invoked on application startup
 *        and that installs new actions in the graphical user interface.
 */
class RunScriptAction : public GuiApplicationService
{
	Q_OBJECT
	OVITO_CLASS(RunScriptAction)

public:

	/// \brief Default constructor.
	Q_INVOKABLE RunScriptAction() {}

	/// \brief Is called when a new main window is created.
	virtual void registerActions(ActionManager& actionManager) override;
};

}	// End of namespace
