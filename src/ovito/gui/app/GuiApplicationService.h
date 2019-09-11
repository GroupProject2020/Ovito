///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2017) Alexander Stukowski
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


#include <ovito/gui/GUI.h>
#include <ovito/core/app/ApplicationService.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(PluginSystem)

/**
 * \brief Abstract base class for plugin services that can perform actions on application startup.
 */
class OVITO_GUI_EXPORT GuiApplicationService : public ApplicationService
{
	Q_OBJECT
	OVITO_CLASS(GuiApplicationService)

public:

	/// \brief Is called when a new main window is created.
	virtual void registerActions(ActionManager& actionManager) {}

	/// \brief Is called when the main menu is created.
	virtual void addActionsToMenu(ActionManager& actionManager, QMenuBar* menuBar) {}
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace


