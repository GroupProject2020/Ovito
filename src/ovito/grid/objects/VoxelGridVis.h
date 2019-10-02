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


#include <ovito/grid/Grid.h>
#include <ovito/core/dataset/data/DataVis.h>
#include <ovito/core/rendering/SceneRenderer.h>
#include <ovito/core/dataset/animation/controller/Controller.h>
#include <ovito/core/dataset/animation/AnimationSettings.h>

namespace Ovito { namespace Grid {

/**
 * \brief A visualization element for rendering VoxelGrid data objects.
 */
class OVITO_GRID_EXPORT VoxelGridVis : public DataVis
{
	Q_OBJECT
	OVITO_CLASS(VoxelGridVis)
	Q_CLASSINFO("DisplayName", "Voxel grid");

public:

	/// \brief Constructor.
	Q_INVOKABLE VoxelGridVis(DataSet* dataset);

	/// Lets the visualization element render the data object.
	virtual void render(TimePoint time, const std::vector<const DataObject*>& objectStack, const PipelineFlowState& flowState, SceneRenderer* renderer, const PipelineSceneNode* contextNode) override;

	/// Computes the bounding box of the object.
	virtual Box3 boundingBox(TimePoint time, const std::vector<const DataObject*>& objectStack, const PipelineSceneNode* contextNode, const PipelineFlowState& flowState, TimeInterval& validityInterval) override;

	/// Returns the transparency parameter.
	FloatType transparency() const { return transparencyController()->currentFloatValue(); }

	/// Sets the transparency parameter.
	void setTransparency(FloatType t) { transparencyController()->setCurrentFloatValue(t); }

private:

	/// Controls the transparency of the grid's faces.
	DECLARE_MODIFIABLE_REFERENCE_FIELD(Controller, transparencyController, setTransparencyController);

	/// Controls whether the grid lines should be highlighted.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, highlightGridLines, sethighlightGridLines);
};

}	// End of namespace
}	// End of namespace
