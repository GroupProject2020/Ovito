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


#include <plugins/mesh/Mesh.h>
#include <core/dataset/data/DataVis.h>
#include <core/dataset/animation/controller/Controller.h>
#include <core/dataset/animation/AnimationSettings.h>

namespace Ovito { namespace Mesh {

/**
 * \brief A visualization element for rendering TriMeshObject data objects.
 */
class OVITO_MESH_EXPORT TriMeshVis : public DataVis
{
	Q_OBJECT
	OVITO_CLASS(TriMeshVis)
	Q_CLASSINFO("DisplayName", "Triangle mesh");
	
public:

	/// \brief Constructor.
	Q_INVOKABLE TriMeshVis(DataSet* dataset);

	/// \brief Lets the vis element render a data object.
	virtual void render(TimePoint time, const std::vector<DataObject*>& objectStack, const PipelineFlowState& flowState, SceneRenderer* renderer, PipelineSceneNode* contextNode) override;

	/// \brief Computes the bounding box of the object.
	virtual Box3 boundingBox(TimePoint time, const std::vector<DataObject*>& objectStack, PipelineSceneNode* contextNode, const PipelineFlowState& flowState, TimeInterval& validityInterval) override;

	/// Returns the transparency parameter.
	FloatType transparency() const { return transparencyController()->currentFloatValue(); }

	/// Sets the transparency parameter.
	void setTransparency(FloatType t) { transparencyController()->setCurrentFloatValue(t); }

private:

	/// Controls the display color of the mesh.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(Color, color, setColor, PROPERTY_FIELD_MEMORIZE);

	/// Controls the transparency of the mesh.
	DECLARE_MODIFIABLE_REFERENCE_FIELD(Controller, transparencyController, setTransparencyController);
};

}	// End of namespace
}	// End of namespace
