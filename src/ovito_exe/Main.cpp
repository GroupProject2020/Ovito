///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2019) Alexander Stukowski
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

#include <ovito/gui/GUI.h>
#include <ovito/gui/app/GuiApplication.h>

/**
 * This is the main entry point for the graphical desktop application.
 *
 * Note that most of the application logic is found in the Core and the Gui
 * modules of OVITO.
 */
int main(int argc, char** argv)
{
	// Initialize the application.
	Ovito::GuiApplication app;
	if(!app.initialize(argc, argv))
		return 1;

	// Enter event loop.
	int result = app.runApplication();

	// Shut application down.
	app.shutdown();

	return result;
}
