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

#include <plugins/particles/gui/ParticlesGui.h>
#include <core/viewport/Viewport.h>
#include <core/dataset/scene/PipelineSceneNode.h>
#include <core/dataset/animation/AnimationSettings.h>
#include <gui/viewport/ViewportWindow.h>
#include <plugins/particles/objects/ParticleProperty.h>
#include <plugins/particles/objects/ParticlesVis.h>
#include "ParticlePickingHelper.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Util)

/******************************************************************************
* Finds the particle under the mouse cursor.
******************************************************************************/
bool ParticlePickingHelper::pickParticle(ViewportWindow* vpwin, const QPoint& clickPoint, PickResult& result)
{
	ViewportPickResult vpPickResult = vpwin->pick(clickPoint);
	// Check if user has clicked on something.
	if(vpPickResult) {

		// Check if that was a particle.
		ParticlePickInfo* pickInfo = dynamic_object_cast<ParticlePickInfo>(vpPickResult.pickInfo);
		if(pickInfo) {
			ParticleProperty* posProperty = ParticleProperty::findInState(pickInfo->pipelineState(), ParticleProperty::PositionProperty);
			int particleIndex = pickInfo->particleIndexFromSubObjectID(vpPickResult.subobjectId);
			if(posProperty && particleIndex >= 0) {
				// Save reference to the selected particle.
				TimeInterval iv;
				result.objNode = vpPickResult.objectNode;
				result.particleIndex = particleIndex;
				result.localPos = posProperty->getPoint3(result.particleIndex);
				result.worldPos = result.objNode->getWorldTransform(vpwin->viewport()->dataset()->animationSettings()->time(), iv) * result.localPos;

				// Determine particle ID.
				ParticleProperty* identifierProperty = ParticleProperty::findInState(pickInfo->pipelineState(), ParticleProperty::IdentifierProperty);
				if(identifierProperty && result.particleIndex < identifierProperty->size()) {
					result.particleId = identifierProperty->getInt64(result.particleIndex);
				}
				else result.particleId = -1;

				return true;
			}
		}
	}

	result.objNode = nullptr;
	return false;
}

/******************************************************************************
* Renders the particle selection overlay in a viewport.
******************************************************************************/
void ParticlePickingHelper::renderSelectionMarker(Viewport* vp, ViewportSceneRenderer* renderer, const PickResult& pickRecord)
{
	if(!pickRecord.objNode)
		return;

	if(!renderer->isInteractive() || renderer->isPicking())
		return;

	const PipelineFlowState& flowState = pickRecord.objNode->evaluatePipelinePreliminary(true);

	// If particle selection is based on ID, find particle with the given ID.
	size_t particleIndex = pickRecord.particleIndex;
	if(pickRecord.particleId >= 0) {
		ParticleProperty* identifierProperty = ParticleProperty::findInState(flowState, ParticleProperty::IdentifierProperty);
		if(identifierProperty) {
			auto begin = identifierProperty->constDataInt64();
			auto end = begin + identifierProperty->size();
			auto iter = std::find(begin, end, pickRecord.particleId);
			if(iter == end) return;
			particleIndex = (iter - begin);
		}
	}

	// Fetch properties of selected particle needed to render the overlay.
	ParticleProperty* posProperty = ParticleProperty::findInState(flowState, ParticleProperty::PositionProperty);
	if(!posProperty)
		return;

	// Get the particle vis element, which is attached to the position property object.
	ParticlesVis* particleVis = nullptr;
	for(DataVis* vis : posProperty->visElements()) {
		if((particleVis = dynamic_object_cast<ParticlesVis>(vis)))
			break;
	}
	if(!particleVis)
		return;
		
	// Set up transformation.
	TimeInterval iv;
	const AffineTransformation& nodeTM = pickRecord.objNode->getWorldTransform(vp->dataset()->animationSettings()->time(), iv);
	renderer->setWorldTransform(nodeTM);

	// Render highlight marker.
	particleVis->highlightParticle(particleIndex, flowState, renderer);
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
