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


#include <ovito/core/Core.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(PluginSystem)

/**
 * \brief Abstract base class for services that want to perform actions on
 *        application startup.
 *
 * If you derive a subclass, a single instance of it will automatically be
 * created by the system and its virtual callback methods will be called at
 * appropriate times during the application life-cycle.
 *
 * For example, it is possible for a plugin to register additional command line
 * options with the central Application and react to them when they are used
 * by the user.
 */
class OVITO_CORE_EXPORT ApplicationService : public OvitoObject
{
	Q_OBJECT
	OVITO_CLASS(ApplicationService)

public:

	/// \brief Registers additional command line options.
	virtual void registerCommandLineOptions(QCommandLineParser& cmdLineParser) {}

	/// \brief Is called after the application has been completely initialized.
	virtual void applicationStarted() {}
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace


