////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2013 Alexander Stukowski
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
#include <ovito/core/dataset/pipeline/PipelineFlowState.h>
#include <ovito/core/dataset/scene/PipelineSceneNode.h>
#include <ovito/core/viewport/Viewport.h>
#include <ovito/core/rendering/LinePrimitive.h>
#include <ovito/core/rendering/ParticlePrimitive.h>
#include <ovito/gui/base/rendering/ViewportSceneRenderer.h>

namespace Ovito { namespace Particles {

/**
 * \brief Utility class that supports the picking of particles in the viewports.
 */
class OVITO_PARTICLESGUI_EXPORT ParticlePickingHelper
{
public:

	struct PickResult {

		/// The position of the picked particle in local coordinates.
		Point3 localPos;

		/// The position of the picked particle in world coordinates.
		Point3 worldPos;

		/// The radius of the picked particle.
		FloatType radius;

		/// The index of the picked particle.
		size_t particleIndex;

		/// The identifier of the picked particle.
		qlonglong particleId;

		/// The scene node that contains the picked particle.
		OORef<PipelineSceneNode> objNode;
	};

	/// \brief Finds the particle under the mouse cursor.
	/// \param vpwin The viewport window to perform hit testing in.
	/// \param clickPoint The position of the mouse cursor in the viewport.
	/// \param time The animation at which hit testing is performed.
	/// \param result The output structure that receives information about the picked particle.
	/// \return \c true if a particle has been picked; \c false otherwise.
	bool pickParticle(ViewportWindowInterface* vpwin, const QPoint& clickPoint, PickResult& result);

	/// \brief Renders the particle selection overlay in a viewport.
	/// \param vp The viewport into which the selection marker should be rendered.
	/// \param renderer The renderer for the viewport.
	/// \param pickRecord Specifies the particle for which the selection marker should be rendered.
	void renderSelectionMarker(Viewport* vp, SceneRenderer* renderer, const PickResult& pickRecord);
};

}	// End of namespace
}	// End of namespace
