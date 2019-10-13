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


#include <ovito/particles/Particles.h>
#include <ovito/core/dataset/data/DataVis.h>
#include <ovito/core/rendering/ArrowPrimitive.h>
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
	virtual void render(TimePoint time, const std::vector<const DataObject*>& objectStack, const PipelineFlowState& flowState, SceneRenderer* renderer, const PipelineSceneNode* contextNode) override;

	/// \brief Computes the display bounding box of the data object.
	virtual Box3 boundingBox(TimePoint time, const std::vector<const DataObject*>& objectStack, const PipelineSceneNode* contextNode, const PipelineFlowState& flowState, TimeInterval& validityInterval) override;

public:

    Q_PROPERTY(Ovito::ArrowPrimitive::ShadingMode shadingMode READ shadingMode WRITE setShadingMode);

private:

	/// Clips a trajectory line at the periodic box boundaries.
	static void clipTrajectoryLine(const Point3& v1, const Point3& v2, const SimulationCell& simulationCell, const std::function<void(const Point3&, const Point3&)>& segmentCallback);

	/// Controls the display width of trajectory lines.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, lineWidth, setLineWidth, PROPERTY_FIELD_MEMORIZE);

	/// Controls the color of the trajectory lines.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(Color, lineColor, setLineColor, PROPERTY_FIELD_MEMORIZE);

	/// Controls the whether the trajectory lines are rendered only up to the current animation time.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, showUpToCurrentTime, setShowUpToCurrentTime);

	/// Controls the whether the displayed trajectory lines are wrapped at periodic boundaries of the simulation cell.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, wrappedLines, setWrappedLines);

	/// Controls the shading mode for lines.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(ArrowPrimitive::ShadingMode, shadingMode, setShadingMode, PROPERTY_FIELD_MEMORIZE);
};

}	// End of namespace
}	// End of namespace
