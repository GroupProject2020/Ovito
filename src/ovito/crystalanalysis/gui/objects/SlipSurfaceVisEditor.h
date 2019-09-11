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

#pragma once


#include <ovito/crystalanalysis/CrystalAnalysis.h>
#include <ovito/gui/properties/PropertiesEditor.h>

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

/**
 * \brief A properties editor for the SlipSurface class.
 */
class SlipSurfaceVisEditor : public PropertiesEditor
{
	Q_OBJECT
	OVITO_CLASS(SlipSurfaceVisEditor)

public:

	/// Constructor.
	Q_INVOKABLE SlipSurfaceVisEditor() {}

protected:

	/// Creates the user interface controls for the editor.
	virtual void createUI(const RolloutInsertionParameters& rolloutParams) override;
};

}	// End of namespace
}	// End of namespace
}	// End of namespace
