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


#include <plugins/mesh/Mesh.h>
#include <plugins/stdobj/simcell/SimulationCell.h>
#include <plugins/mesh/surface/SurfaceMeshData.h>
#include <core/dataset/data/TransformingDataVis.h>
#include <core/dataset/animation/controller/Controller.h>
#include <core/utilities/mesh/TriMesh.h>
#include <core/utilities/concurrent/Task.h>

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
	virtual void render(TimePoint time, const std::vector<const DataObject*>& objectStack, const PipelineFlowState& flowState, SceneRenderer* renderer, const PipelineSceneNode* contextNode) override;

	/// Computes the bounding box of the object.
	virtual Box3 boundingBox(TimePoint time, const std::vector<const DataObject*>& objectStack, const PipelineSceneNode* contextNode, const PipelineFlowState& flowState, TimeInterval& validityInterval) override;

	/// Returns the transparency of the surface mesh.
	FloatType surfaceTransparency() const { return surfaceTransparencyController() ? surfaceTransparencyController()->currentFloatValue() : 0.0f; }

	/// Sets the transparency of the surface mesh.
	void setSurfaceTransparency(FloatType transparency) { if(surfaceTransparencyController()) surfaceTransparencyController()->setCurrentFloatValue(transparency); }

	/// Returns the transparency of the surface cap mesh.
	FloatType capTransparency() const { return capTransparencyController() ? capTransparencyController()->currentFloatValue() : 0.0f; }

	/// Sets the transparency of the surface cap mesh.
	void setCapTransparency(FloatType transparency) { if(capTransparencyController()) capTransparencyController()->setCurrentFloatValue(transparency); }

protected:

	/// Lets the vis element transform a data object in preparation for rendering.
	virtual Future<PipelineFlowState> transformDataImpl(TimePoint time, const DataObject* dataObject, PipelineFlowState&& flowState, const PipelineFlowState& cachedState, const PipelineSceneNode* contextNode) override;

	/// Is called when the value of a property of this object has changed.
	virtual void propertyChanged(const PropertyFieldDescriptor& field) override;

	/// Computation engine that builds the rendering mesh.
	class PrepareSurfaceEngine : public AsynchronousTask<TriMesh, TriMesh, std::vector<ColorA>, std::vector<size_t>>
	{
	public:

		/// Constructor.
		PrepareSurfaceEngine(const SurfaceMesh* mesh, bool reverseOrientation, QVector<Plane3> cuttingPlanes, bool smoothShading, bool generateCapPolygons = true) :
			_inputMesh(mesh),
			_reverseOrientation(reverseOrientation),
			_cuttingPlanes(std::move(cuttingPlanes)),
			_smoothShading(smoothShading),
			_generateCapPolygons(generateCapPolygons) {}

		/// Builds the non-periodic representation of the surface mesh.
		virtual void perform() override;

		/// Returns the input surface mesh.
		const SurfaceMeshData& inputMesh() const { return _inputMesh; }

	protected:

		/// This method can be overriden by subclasses to restrict the set of visible mesh faces,
		virtual void determineVisibleFaces() {}

		/// This method can be overriden by subclasses to assign colors to invidual mesh faces.
		virtual void determineFaceColors() {}

	private:

		/// Generates the triangle mesh from the periodic surface mesh, which will be rendered.
		bool buildSurfaceTriangleMesh();

		/// Generates the cap polygons where the surface mesh intersects the periodic domain boundaries.
		void buildCapTriangleMesh();

		/// Returns the periodic domain the surface mesh is embedded in.
		const SimulationCell& cell() const { return _inputMesh.cell(); }

	private:

		/// Splits a triangle face at a periodic boundary.
		bool splitFace(int faceIndex, int oldVertexCount, std::vector<Point3>& newVertices, std::map<std::pair<int,int>,std::tuple<int,int,FloatType>>& newVertexLookupMap, size_t dim);

		/// Traces the closed contour of the surface-boundary intersection.
		std::vector<Point2> traceContour(HalfEdgeMesh::edge_index firstEdge, const std::vector<Point3>& reducedPos, std::vector<bool>& visitedFaces, size_t dim) const;

		/// Clips a 2d contour at a periodic boundary.
		static void clipContour(std::vector<Point2>& input, std::array<bool,2> periodic, std::vector<std::vector<Point2>>& openContours, std::vector<std::vector<Point2>>& closedContours);

		/// Computes the intersection point of a 2d contour segment crossing a periodic boundary.
		static void computeContourIntersection(size_t dim, FloatType t, Point2& base, Vector2& delta, int crossDir, std::vector<std::vector<Point2>>& contours);

		/// Determines if the 2D box corner (0,0) is inside the closed region described by the 2d polygon.
		static bool isCornerInside2DRegion(const std::vector<std::vector<Point2>>& contours);

	protected:

		SurfaceMeshData _inputMesh;			///< The input surface mesh.
		bool _reverseOrientation;			///< Flag for inside-out display of the mesh.
		bool _smoothShading;				///< Flag for interpolated-normal shading
		bool _generateCapPolygons;			///< Controls the generation of cap polygons where the mesh intersection periodic cell boundaries.
		QVector<Plane3> _cuttingPlanes;		///< List of cutting planes at which the mesh should be truncated.

		TriMesh _surfaceMesh;					///< The output mesh generated by clipping the surface mesh at the cell boundaries.
		TriMesh _capPolygonsMesh;				///< The output mesh containing the generated cap polygons.
		boost::dynamic_bitset<> _faceSubset;	///< Bit array indicating which surface mesh faces are part of the render set.
		std::vector<ColorA> _materialColors;	///< The list of material colors for the output TriMesh.
		std::vector<size_t> _originalFaceMap;	///< Maps output mesh triangles to input mesh facets.
	};

	/// Creates the asynchronous task that builds the non-peridic representation of the input surface mesh.
	/// This method may be overriden by subclasses who want to implement custom behavior.
	virtual std::shared_ptr<PrepareSurfaceEngine> createSurfaceEngine(const SurfaceMesh* mesh) const;

	/// Create the viewport picking record for the surface mesh object.
	/// The default implementation returns null, because standard surface meshes do not support picking of
	/// mesh faces or vertices. Sub-classes can override this method to implement object-specific picking
	/// strategies.
	virtual OORef<ObjectPickInfo> createPickInfo(const SurfaceMesh* mesh, const RenderableSurfaceMesh* renderableMesh) const { return {}; }

private:

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

	/// Controls whether mesh faces facing away from the viewer are not rendered.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, cullFaces, setCullFaces);

	/// Controls the transparency of the surface mesh.
	DECLARE_MODIFIABLE_REFERENCE_FIELD(Controller, surfaceTransparencyController, setSurfaceTransparencyController);

	/// Controls the transparency of the surface cap mesh.
	DECLARE_MODIFIABLE_REFERENCE_FIELD(Controller, capTransparencyController, setCapTransparencyController);
};

}	// End of namespace
}	// End of namespace
