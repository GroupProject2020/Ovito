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


#include <plugins/stdobj/StdObj.h>
#include <core/dataset/data/DataVis.h>

namespace Ovito { namespace StdObj {
	
/**
 * \brief A visual element that renders a SimulationCellObject as a wireframe box.
 */
class OVITO_STDOBJ_EXPORT SimulationCellVis : public DataVis
{
	Q_OBJECT
	OVITO_CLASS(SimulationCellVis)
	Q_CLASSINFO("DisplayName", "Simulation cell");

public:

	/// \brief Constructor.
	Q_INVOKABLE SimulationCellVis(DataSet* dataset);

	/// \brief Lets the visualization element render the data object.
	virtual void render(TimePoint time, const std::vector<const DataObject*>& objectStack, const PipelineFlowState& flowState, SceneRenderer* renderer, const PipelineSceneNode* contextNode) override;

	/// \brief Computes the bounding box of the object.
	virtual Box3 boundingBox(TimePoint time, const std::vector<const DataObject*>& objectStack, const PipelineSceneNode* contextNode, const PipelineFlowState& flowState, TimeInterval& validityInterval) override;

	/// \brief Indicates whether this object should be surrounded by a selection marker in the viewports when it is selected.
	virtual bool showSelectionMarker() override { return false; }

protected:

	/// Renders the given simulation using wireframe mode.
	void renderWireframe(TimePoint time, const SimulationCellObject* cell, const PipelineFlowState& flowState, SceneRenderer* renderer, const PipelineSceneNode* contextNode);

	/// Renders the given simulation using solid shading mode.
	void renderSolid(TimePoint time, const SimulationCellObject* cell, const PipelineFlowState& flowState, SceneRenderer* renderer, const PipelineSceneNode* contextNode);

protected:

	/// Controls the line width used to render the simulation cell.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, cellLineWidth, setCellLineWidth);

	/// Controls whether the simulation cell is visible.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, renderCellEnabled, setRenderCellEnabled);

	/// Controls the rendering color of the simulation cell.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(Color, cellColor, setCellColor, PROPERTY_FIELD_MEMORIZE);
};

}	// End of namespace
}	// End of namespace
