///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2018) Alexander Stukowski
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


#include <ovito/particles/gui/ParticlesGui.h>
#include <ovito/core/dataset/scene/PipelineSceneNode.h>
#include <ovito/core/viewport/Viewport.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Util)

/**
 * \brief Utility class that supports the picking of bonds in the viewports.
 */
class OVITO_PARTICLESGUI_EXPORT BondPickingHelper
{
public:

	struct PickResult {

		/// The index of the picked bond.
		size_t bondIndex;

		/// The scene node that contains the picked bond.
		OORef<PipelineSceneNode> sceneNode;
	};

	/// \brief Finds the bond under the mouse cursor.
	/// \param vpwin The viewport window to perform hit testing in.
	/// \param clickPoint The position of the mouse cursor in the viewport.
	/// \param time The animation at which hit testing is performed.
	/// \param result The output structure that receives information about the picked bond.
	/// \return \c true if a bond has been picked; \c false otherwise.
	bool pickBond(ViewportWindow* vpwin, const QPoint& clickPoint, PickResult& result);
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
