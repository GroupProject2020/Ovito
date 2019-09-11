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

#include <ovito/particles/gui/ParticlesGui.h>
#include <ovito/particles/objects/BondsVis.h>
#include <ovito/particles/objects/BondsObject.h>
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/core/viewport/Viewport.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/dataset/scene/PipelineSceneNode.h>
#include <ovito/core/dataset/animation/AnimationSettings.h>
#include <ovito/gui/viewport/ViewportWindow.h>
#include "BondPickingHelper.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Util)

/******************************************************************************
* Finds the bond under the mouse cursor.
******************************************************************************/
bool BondPickingHelper::pickBond(ViewportWindow* vpwin, const QPoint& clickPoint, PickResult& result)
{
	ViewportPickResult vpPickResult = vpwin->pick(clickPoint);
	// Check if user has clicked on something.
	if(vpPickResult.isValid()) {

		// Check if that was a bond.
		if(BondPickInfo* pickInfo = dynamic_object_cast<BondPickInfo>(vpPickResult.pickInfo())) {
			if(const ParticlesObject* particles = pickInfo->pipelineState().getObject<ParticlesObject>()) {
				if(particles->bonds()) {
					size_t bondIndex = vpPickResult.subobjectId() / 2;
					const PropertyObject* topologyProperty = particles->bonds()->getTopology();
					if(topologyProperty && topologyProperty->size() > bondIndex) {
						// Save reference to the selected bond.
						result.sceneNode = vpPickResult.pipelineNode();
						result.bondIndex = bondIndex;
						return true;
					}
				}
			}
		}
	}

	result.sceneNode = nullptr;
	return false;
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
