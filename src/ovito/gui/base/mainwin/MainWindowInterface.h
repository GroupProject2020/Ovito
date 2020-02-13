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


#include <ovito/gui/base/GUIBase.h>

namespace Ovito {

/**
 * \brief The main window of the application.
 *
 * Note that is is possible to open multiple main windows per
 * application instance to edit multiple datasets simultaneously.
 */
class OVITO_GUIBASE_EXPORT MainWindowInterface
{
public:

	/// Constructor.
	explicit MainWindowInterface() {}

	/// Sets the window's viewport input manager.
	void setViewportInputManager(ViewportInputManager* manager) { _viewportInputManager = manager; }

	/// Returns the window's viewport input manager.
	ViewportInputManager* viewportInputManager() const { return _viewportInputManager; }
	
	/// Displays a message string in the window's status bar.
	virtual void showStatusBarMessage(const QString& message, int timeout = 0) {}

	/// Hides any messages currently displayed in the window's status bar.
	virtual void clearStatusBarMessage() {}

private:

	/// The associated viewport input manager.
	ViewportInputManager* _viewportInputManager = nullptr;
};

}	// End of namespace
