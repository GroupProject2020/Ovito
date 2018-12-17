///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2016) Alexander Stukowski
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


#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include <plugins/crystalanalysis/data/Microstructure.h>
#include <plugins/crystalanalysis/objects/microstructure/MicrostructureObject.h>
#include <plugins/crystalanalysis/objects/patterns/PatternCatalog.h>
#include <plugins/stdobj/simcell/SimulationCell.h>
#include <plugins/mesh/surface/RenderableSurfaceMesh.h>
#include <core/dataset/data/TransformingDataVis.h>
#include <core/rendering/SceneRenderer.h>
#include <core/utilities/mesh/TriMesh.h>
#include <core/utilities/concurrent/Task.h>
#include <core/dataset/animation/controller/Controller.h>

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

/**
 * \brief A visualization element for rendering SlipSurface data objects.
 */
class OVITO_CRYSTALANALYSIS_EXPORT SlipSurfaceVis : public TransformingDataVis
{
	Q_OBJECT
	OVITO_CLASS(SlipSurfaceVis)

	Q_CLASSINFO("DisplayName", "Slip surfaces");

public:

	/// \brief Constructor.
	Q_INVOKABLE SlipSurfaceVis(DataSet* dataset);

	/// Lets the visualization element render the data object.
	virtual void render(TimePoint time, const std::vector<const DataObject*>& objectStack, const PipelineFlowState& flowState, SceneRenderer* renderer, const PipelineSceneNode* contextNode) override;

	/// Computes the bounding box of the object.
	virtual Box3 boundingBox(TimePoint time, const std::vector<const DataObject*>& objectStack, const PipelineSceneNode* contextNode, const PipelineFlowState& flowState, TimeInterval& validityInterval) override;

	/// Returns the transparency of the surface mesh.
	FloatType surfaceTransparency() const { return surfaceTransparencyController() ? surfaceTransparencyController()->currentFloatValue() : 0.0f; }

	/// Sets the transparency of the surface mesh.
	void setSurfaceTransparency(FloatType transparency) { if(surfaceTransparencyController()) surfaceTransparencyController()->setCurrentFloatValue(transparency); }

	/// Generates the final triangle mesh, which will be rendered.
	static bool buildMesh(const Microstructure& input, const SimulationCell& cell, const QVector<Plane3>& cuttingPlanes, const QStringList& structureNames, TriMesh& output, std::vector<ColorA>& materialColors, std::vector<size_t>& originalFaceMap, PromiseState& promise);

protected:
	
	/// Lets the vis elementt transform a data object in preparation for rendering.
	virtual Future<PipelineFlowState> transformDataImpl(TimePoint time, const DataObject* dataObject, PipelineFlowState&& flowState, const PipelineFlowState& cachedState, const PipelineSceneNode* contextNode) override;
	
	/// Is called when the value of a property of this object has changed.
	virtual void propertyChanged(const PropertyFieldDescriptor& field) override;
	
protected:

	/// Computation engine that builds the render mesh.
	class PrepareMeshEngine : public AsynchronousTask<TriMesh, std::vector<ColorA>, std::vector<size_t>>
	{
	public:

		/// Constructor.
		PrepareMeshEngine(std::shared_ptr<const Microstructure> mesh, std::shared_ptr<const ClusterGraph> clusterGraph, const SimulationCell& simCell,
				QStringList structureNames, QVector<Plane3> cuttingPlanes, bool smoothShading) :
			_inputMesh(std::move(mesh)), _clusterGraph(std::move(clusterGraph)), _simCell(simCell),
			_structureNames(std::move(structureNames)), _cuttingPlanes(std::move(cuttingPlanes)), _smoothShading(smoothShading) {}

		/// Computes the results and stores them in this object for later retrieval.
		virtual void perform() override;

	private:

		const std::shared_ptr<const Microstructure> _inputMesh;
		const std::shared_ptr<const ClusterGraph> _clusterGraph;
		const SimulationCell _simCell;
		const QStringList _structureNames;
		const QVector<Plane3> _cuttingPlanes;
		bool _smoothShading;
	};

protected:

	/// Splits a triangle face at a periodic boundary.
	static bool splitFace(TriMesh& output, int faceIndex, int oldVertexCount, std::vector<Point3>& newVertices, std::map<std::pair<int,int>,std::pair<int,int>>& newVertexLookupMap, const SimulationCell& cell, size_t dim, std::vector<size_t>& originalFaceMap);

	/// Controls whether the mesh is rendered using smooth shading.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, smoothShading, setSmoothShading);

	/// Controls the transparency of the surface mesh.
	DECLARE_MODIFIABLE_REFERENCE_FIELD(Controller, surfaceTransparencyController, setSurfaceTransparencyController);	
};

/**
 * \brief This information record is attached to the slip surface mesh by the SlipSurfaceVis when rendering
 * them in the viewports. It facilitates the picking of slip surface facets with the mouse.
 */
class OVITO_CRYSTALANALYSIS_EXPORT SlipSurfacePickInfo : public ObjectPickInfo
{
	Q_OBJECT
	OVITO_CLASS(SlipSurfacePickInfo)

public:

	/// Constructor.
	SlipSurfacePickInfo(SlipSurfaceVis* visElement, const MicrostructureObject* microstructureObj, const RenderableSurfaceMesh* renderableMesh, const PatternCatalog* patternCatalog) :
		_visElement(visElement), _microstructureObj(microstructureObj), _renderableMesh(renderableMesh), _patternCatalog(patternCatalog) {}

	/// The data object containing the slip surfaces.
	const MicrostructureObject* microstructureObj() const { return _microstructureObj; }

	/// The renderable surface mesh for the slip surfaces.
	const RenderableSurfaceMesh* renderableMesh() const { return _renderableMesh; }

	/// Returns the vis element that rendered the slip surfaces.
	SlipSurfaceVis* visElement() const { return _visElement; }

	/// Returns the associated pattern catalog.
	const PatternCatalog* patternCatalog() const { return _patternCatalog; }

	/// \brief Given an sub-object ID returned by the Viewport::pick() method, looks up the
	/// corresponding slip surface facet.
	int slipFacetIndexFromSubObjectID(quint32 subobjID) const {
		if(renderableMesh() && subobjID < renderableMesh()->originalFaceMap().size())
			return renderableMesh()->originalFaceMap()[subobjID];
		else
			return -1;
	}

	/// Returns a human-readable string describing the picked object, which will be displayed in the status bar by OVITO.
	virtual QString infoString(PipelineSceneNode* objectNode, quint32 subobjectId) override;

private:

	/// The data object containing the slip surfaces.
	OORef<MicrostructureObject> _microstructureObj;

	/// The renderable surface mesh for the slip surfaces.
	OORef<RenderableSurfaceMesh> _renderableMesh;

	/// The vis element that rendered the slip surfaces.
	OORef<SlipSurfaceVis> _visElement;

	/// The data object containing the lattice structure.
	OORef<PatternCatalog> _patternCatalog;
};


}	// End of namespace
}	// End of namespace
}	// End of namespace
