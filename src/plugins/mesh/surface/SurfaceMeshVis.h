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

#pragma once


#include <plugins/mesh/Mesh.h>
#include <plugins/stdobj/simcell/SimulationCell.h>
#include <core/dataset/data/TransformingDataVis.h>
#include <core/utilities/mesh/TriMesh.h>
#include <core/utilities/mesh/HalfEdgeMesh.h>
#include <core/utilities/concurrent/Task.h>
#include <core/dataset/animation/controller/Controller.h>

namespace Ovito { namespace Mesh {

/**
 * \brief A visualization element for rendering SurfaceMesh data objects.
 */
class OVITO_MESH_EXPORT SurfaceMeshVis : public TransformingDataVis
{
	Q_OBJECT
	OVITO_CLASS(SurfaceMeshVis)
	Q_CLASSINFO("DisplayName", "Surface mesh");

public:

	/// \brief Constructor.
	Q_INVOKABLE SurfaceMeshVis(DataSet* dataset);

	/// Lets the visualization element render the data object.
	virtual void render(TimePoint time, const std::vector<DataObject*>& objectStack, const PipelineFlowState& flowState, SceneRenderer* renderer, PipelineSceneNode* contextNode) override;

	/// Computes the bounding box of the object.
	virtual Box3 boundingBox(TimePoint time, const std::vector<DataObject*>& objectStack, PipelineSceneNode* contextNode, const PipelineFlowState& flowState, TimeInterval& validityInterval) override;

	/// Returns the transparency of the surface mesh.
	FloatType surfaceTransparency() const { return surfaceTransparencyController() ? surfaceTransparencyController()->currentFloatValue() : 0.0f; }

	/// Sets the transparency of the surface mesh.
	void setSurfaceTransparency(FloatType transparency) { if(surfaceTransparencyController()) surfaceTransparencyController()->setCurrentFloatValue(transparency); }

	/// Returns the transparency of the surface cap mesh.
	FloatType capTransparency() const { return capTransparencyController() ? capTransparencyController()->currentFloatValue() : 0.0f; }

	/// Sets the transparency of the surface cap mesh.
	void setCapTransparency(FloatType transparency) { if(capTransparencyController()) capTransparencyController()->setCurrentFloatValue(transparency); }

	/// Generates the final triangle mesh, which will be rendered.
	static bool buildSurfaceMesh(const HalfEdgeMesh<>& input, const SimulationCell& cell, bool reverseOrientation, const QVector<Plane3>& cuttingPlanes, TriMesh& output, PromiseState* progress = nullptr);

	/// Generates the triangle mesh for the PBC cap.
	static void buildCapMesh(const HalfEdgeMesh<>& input, const SimulationCell& cell, bool isCompletelySolid, bool reverseOrientation, const QVector<Plane3>& cuttingPlanes, TriMesh& output, PromiseState* progress = nullptr);

protected:

	/// Lets the vis element transform a data object in preparation for rendering.
	virtual Future<PipelineFlowState> transformDataImpl(TimePoint time, DataObject* dataObject, PipelineFlowState&& flowState, const PipelineFlowState& cachedState, PipelineSceneNode* contextNode) override;

	/// Is called when the value of a property of this object has changed.
	virtual void propertyChanged(const PropertyFieldDescriptor& field) override;

protected:
	
	/// Computation engine that builds the rendering mesh.
	class PrepareSurfaceEngine : public AsynchronousTask<TriMesh, TriMesh>
	{
	public:

		/// Constructor.
		PrepareSurfaceEngine(std::shared_ptr<HalfEdgeMesh<>> mesh, const SimulationCell& simCell, bool isCompletelySolid, bool reverseOrientation, const QVector<Plane3>& cuttingPlanes, bool smoothShading) :
			_inputMesh(std::move(mesh)), _simCell(simCell), _isCompletelySolid(isCompletelySolid), _reverseOrientation(reverseOrientation), _cuttingPlanes(cuttingPlanes), _smoothShading(smoothShading) {}

		/// Computes the results.
		virtual void perform() override;

	private:

		std::shared_ptr<HalfEdgeMesh<>> _inputMesh;
		SimulationCell _simCell;
		bool _isCompletelySolid;
		bool _reverseOrientation;
		bool _smoothShading;
		QVector<Plane3> _cuttingPlanes;
	};

private:

	/// Splits a triangle face at a periodic boundary.
	static bool splitFace(TriMesh& output, TriMeshFace& face, int oldVertexCount, std::vector<Point3>& newVertices, std::map<std::pair<int,int>,std::pair<int,int>>& newVertexLookupMap, const SimulationCell& cell, size_t dim);

	/// Traces the closed contour of the surface-boundary intersection.
	static std::vector<Point2> traceContour(HalfEdgeMesh<>::Edge* firstEdge, const std::vector<Point3>& reducedPos, const SimulationCell& cell, size_t dim);

	/// Clips a 2d contour at a periodic boundary.
	static void clipContour(std::vector<Point2>& input, std::array<bool,2> periodic, std::vector<std::vector<Point2>>& openContours, std::vector<std::vector<Point2>>& closedContours);

	/// Computes the intersection point of a 2d contour segment crossing a periodic boundary.
	static void computeContourIntersection(size_t dim, FloatType t, Point2& base, Vector2& delta, int crossDir, std::vector<std::vector<Point2>>& contours);

	/// Determines if the 2D box corner (0,0) is inside the closed region described by the 2d polygon.
	static bool isCornerInside2DRegion(const std::vector<std::vector<Point2>>& contours);

	/// Controls the display color of the surface mesh.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(Color, surfaceColor, setSurfaceColor, PROPERTY_FIELD_MEMORIZE);

	/// Controls the display color of the cap mesh.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(Color, capColor, setCapColor, PROPERTY_FIELD_MEMORIZE);

	/// Controls whether the cap mesh is rendered.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(bool, showCap, setShowCap, PROPERTY_FIELD_MEMORIZE);

	/// Controls whether the surface mesh is rendered using smooth shading.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, smoothShading, setSmoothShading);

	/// Controls whether the mesh' orientation is flipped.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, reverseOrientation, setReverseOrientation);

	/// Controls the transparency of the surface mesh.
	DECLARE_MODIFIABLE_REFERENCE_FIELD(Controller, surfaceTransparencyController, setSurfaceTransparencyController);

	/// Controls the transparency of the surface cap mesh.
	DECLARE_MODIFIABLE_REFERENCE_FIELD(Controller, capTransparencyController, setCapTransparencyController);
};

}	// End of namespace
}	// End of namespace
