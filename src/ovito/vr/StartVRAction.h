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


#include <ovito/gui/desktop/GUI.h>
#include <ovito/gui/desktop/app/GuiApplicationService.h>

namespace VRPlugin {

using namespace Ovito;

/**
 * \brief An auto-start object that is automatically invoked on application startup.
 */
class StartVRAction : public GuiApplicationService
{
	Q_OBJECT
	OVITO_CLASS(StartVRAction)

public:

	/// \brief Default constructor.
	Q_INVOKABLE StartVRAction() {}

	/// \brief Is called when a new main window is created.
	virtual void registerActions(ActionManager& actionManager) override;

	/// \brief Is called when the main menu is created.
	virtual void addActionsToMenu(ActionManager& actionManager, QMenuBar* menuBar) override;

	/// \brief Registers plugin-specific command line options.
	virtual void registerCommandLineOptions(QCommandLineParser& cmdLineParser) override;

	/// \brief Is called after the application has been completely initialized.
	virtual void applicationStarted() override;
};

}	// End of namespace
