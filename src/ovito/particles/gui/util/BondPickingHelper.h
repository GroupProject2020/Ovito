////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2018 Alexander Stukowski
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
	bool pickBond(ViewportWindowInterface* vpwin, const QPoint& clickPoint, PickResult& result);
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
