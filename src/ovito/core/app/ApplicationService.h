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
