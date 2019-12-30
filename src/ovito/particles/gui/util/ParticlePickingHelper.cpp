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

#include <ovito/particles/gui/ParticlesGui.h>
#include <ovito/core/viewport/Viewport.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/dataset/scene/PipelineSceneNode.h>
#include <ovito/core/dataset/animation/AnimationSettings.h>
#include <ovito/gui/viewport/ViewportWindow.h>
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/particles/objects/ParticlesVis.h>
#include "ParticlePickingHelper.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Util)

/******************************************************************************
* Finds the particle under the mouse cursor.
******************************************************************************/
bool ParticlePickingHelper::pickParticle(ViewportWindow* vpwin, const QPoint& clickPoint, PickResult& result)
{
	ViewportPickResult vpPickResult = vpwin->pick(clickPoint);
	// Check if user has clicked on something.
	if(vpPickResult.isValid()) {

		// Check if that was a particle.
		ParticlePickInfo* pickInfo = dynamic_object_cast<ParticlePickInfo>(vpPickResult.pickInfo());
		if(pickInfo) {
			if(const ParticlesObject* particles = pickInfo->pipelineState().getObject<ParticlesObject>()) {
				ConstPropertyAccess<Point3> posProperty = particles->expectProperty(ParticlesObject::PositionProperty);
				size_t particleIndex = pickInfo->particleIndexFromSubObjectID(vpPickResult.subobjectId());
				if(posProperty && particleIndex < posProperty.size()) {
					// Save reference to the selected particle.
					TimeInterval iv;
					result.objNode = vpPickResult.pipelineNode();
					result.particleIndex = particleIndex;
					result.localPos = posProperty[result.particleIndex];
					result.worldPos = result.objNode->getWorldTransform(vpwin->viewport()->dataset()->animationSettings()->time(), iv) * result.localPos;

					// Determine particle ID.
					ConstPropertyAccess<qlonglong> identifierProperty = particles->getProperty(ParticlesObject::IdentifierProperty);
					if(identifierProperty && result.particleIndex < identifierProperty.size()) {
						result.particleId = identifierProperty[result.particleIndex];
					}
					else result.particleId = -1;

					return true;
				}
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
	const ParticlesObject* particles = flowState.getObject<ParticlesObject>();
	if(!particles) return;

	// If particle selection is based on ID, find particle with the given ID.
	size_t particleIndex = pickRecord.particleIndex;
	if(pickRecord.particleId >= 0) {
		if(ConstPropertyAccess<qlonglong> identifierProperty = particles->getProperty(ParticlesObject::IdentifierProperty)) {
			if(particleIndex >= identifierProperty.size() || identifierProperty[particleIndex] != pickRecord.particleId) {
				auto iter = boost::find(identifierProperty, pickRecord.particleId);
				if(iter == identifierProperty.cend()) return;
				particleIndex = (iter - identifierProperty.cbegin());
			}
		}
	}

	// Get the particle vis element, which is attached to the position property object.
	ParticlesVis* particleVis = particles->visElement<ParticlesVis>();
	if(!particleVis)
		return;

	// Set up transformation.
	TimeInterval iv;
	const AffineTransformation& nodeTM = pickRecord.objNode->getWorldTransform(vp->dataset()->animationSettings()->time(), iv);
	renderer->setWorldTransform(nodeTM);

	// Render highlight marker.
	particleVis->highlightParticle(particleIndex, particles, renderer);
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
