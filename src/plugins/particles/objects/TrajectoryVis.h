///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2015) Alexander Stukowski
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


#include <plugins/particles/Particles.h>
#include <core/dataset/data/DataVis.h>
#include <core/rendering/ArrowPrimitive.h>
#include "TrajectoryObject.h"

namespace Ovito { namespace Particles {

/**
 * \brief A visualization element for rendering particle trajectory lines.
 */
class OVITO_PARTICLES_EXPORT TrajectoryVis : public DataVis
{
	Q_OBJECT
	OVITO_CLASS(TrajectoryVis)
	Q_CLASSINFO("DisplayName", "Trajectory lines");

public:

	/// \brief Constructor.
	Q_INVOKABLE TrajectoryVis(DataSet* dataset);

	/// \brief Renders the associated data object.
	virtual void render(TimePoint time, const std::vector<DataObject*>& objectStack, const PipelineFlowState& flowState, SceneRenderer* renderer, PipelineSceneNode* contextNode) override;

	/// \brief Computes the display bounding box of the data object.
	virtual Box3 boundingBox(TimePoint time, const std::vector<DataObject*>& objectStack, PipelineSceneNode* contextNode, const PipelineFlowState& flowState, TimeInterval& validityInterval) override;

public:

    Q_PROPERTY(Ovito::ArrowPrimitive::ShadingMode shadingMode READ shadingMode WRITE setShadingMode);

protected:

	/// Controls the display width of trajectory lines.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, lineWidth, setLineWidth, PROPERTY_FIELD_MEMORIZE);

	/// Controls the color of the trajectory lines.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(Color, lineColor, setLineColor, PROPERTY_FIELD_MEMORIZE);

	/// Controls the whether the trajectory lines are rendered only up to the current animation time.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, showUpToCurrentTime, setShowUpToCurrentTime);

	/// Controls the shading mode for lines.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(ArrowPrimitive::ShadingMode, shadingMode, setShadingMode, PROPERTY_FIELD_MEMORIZE);
};

}	// End of namespace
}	// End of namespace


