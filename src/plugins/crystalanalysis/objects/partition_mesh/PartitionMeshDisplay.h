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


#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include <core/dataset/data/simcell/SimulationCell.h>
#include <core/dataset/data/DisplayObject.h>
#include <core/dataset/data/VersionedDataObjectRef.h>
#include <core/utilities/mesh/TriMesh.h>
#include <core/utilities/mesh/HalfEdgeMesh.h>
#include <core/utilities/concurrent/Task.h>
#include <core/rendering/MeshPrimitive.h>
#include <core/dataset/animation/controller/Controller.h>
#include "PartitionMesh.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

/**
 * \brief A display object for the PartitionMesh data object class.
 */
class OVITO_CRYSTALANALYSIS_EXPORT PartitionMeshDisplay : public DisplayObject
{
public:

	/// \brief Constructor.
	Q_INVOKABLE PartitionMeshDisplay(DataSet* dataset);

	/// \brief Lets the display object render the data object.
	virtual void render(TimePoint time, DataObject* dataObject, const PipelineFlowState& flowState, SceneRenderer* renderer, ObjectNode* contextNode) override;

	/// Indicates whether the display object wants to transform data objects before rendering. 
	virtual bool doesPerformDataTransformation() const override { return true; }
	
	/// Computes the bounding box of the object.
	virtual Box3 boundingBox(TimePoint time, DataObject* dataObject, ObjectNode* contextNode, const PipelineFlowState& flowState, TimeInterval& validityInterval) override;
	
	/// Returns the transparency of the surface mesh.
	FloatType surfaceTransparency() const { return surfaceTransparencyController() ? surfaceTransparencyController()->currentFloatValue() : 0.0f; }

	/// Sets the transparency of the surface mesh.
	void setSurfaceTransparency(FloatType transparency) { if(surfaceTransparencyController()) surfaceTransparencyController()->setCurrentFloatValue(transparency); }

	/// Returns the transparency of the cap polygons.
	FloatType capTransparency() const { return capTransparencyController() ? capTransparencyController()->currentFloatValue() : 0.0f; }

	/// Sets the transparency of the cap polygons.
	void setCapTransparency(FloatType transparency) { if(capTransparencyController()) capTransparencyController()->setCurrentFloatValue(transparency); }

	/// Generates the final triangle mesh, which will be rendered.
	static bool buildMesh(const PartitionMeshData& input, const SimulationCell& cell, const QVector<Plane3>& cuttingPlanes, TriMesh& output, PromiseState* promise);

protected:

	/// Lets the display object transform a data object in preparation for rendering.
	virtual Future<PipelineFlowState> transformDataImpl(TimePoint time, DataObject* dataObject, PipelineFlowState&& flowState, const PipelineFlowState& cachedState, ObjectNode* contextNode) override;

	/// Is called when the value of a property of this object has changed.
	virtual void propertyChanged(const PropertyFieldDescriptor& field) override;
	
protected:

	/// Computation engine that builds the render mesh.
	class PrepareMeshEngine :public AsynchronousTask<TriMesh, TriMesh>
	{
	public:

		/// Constructor.
		PrepareMeshEngine(std::shared_ptr<PartitionMeshData> mesh, const SimulationCell& simCell, int spaceFillingRegion, const QVector<Plane3>& cuttingPlanes, bool flipOrientation, bool smoothShading) :
			_inputMesh(std::move(mesh)), _simCell(simCell), _spaceFillingRegion(spaceFillingRegion), _cuttingPlanes(cuttingPlanes), _flipOrientation(flipOrientation), _smoothShading(smoothShading) {}

		/// Computes the results and stores them in this object for later retrieval.
		virtual void perform() override;

	private:

		std::shared_ptr<PartitionMeshData> _inputMesh;
		SimulationCell _simCell;
		int _spaceFillingRegion;
		bool _flipOrientation;
		QVector<Plane3> _cuttingPlanes;
		bool _smoothShading;
	};

protected:

	/// Splits a triangle face at a periodic boundary.
	static bool splitFace(TriMesh& output, int faceIndex, int oldVertexCount, std::vector<Point3>& newVertices, std::map<std::pair<int,int>,std::pair<int,int>>& newVertexLookupMap, const SimulationCell& cell, size_t dim);

	/// Controls the display color of the outer surface mesh.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(Color, surfaceColor, setSurfaceColor);

	/// Controls whether the cap polygons are rendered.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, showCap, setShowCap);

	/// Controls whether the mesh is rendered using smooth shading.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, smoothShading, setSmoothShading);

	/// Controls whether the orientation of mesh faces is flipped.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, flipOrientation, setFlipOrientation);

	/// Controls the transparency of the surface mesh.
	DECLARE_MODIFIABLE_REFERENCE_FIELD(Controller, surfaceTransparencyController, setSurfaceTransparencyController);

	/// Controls the transparency of the surface cap mesh.
	DECLARE_MODIFIABLE_REFERENCE_FIELD(Controller, capTransparencyController, setCapTransparencyController);

	/// The buffered geometry used to render the surface mesh.
	std::shared_ptr<MeshPrimitive> _surfaceBuffer;

	/// The buffered geometry used to render the surface cap.
	std::shared_ptr<MeshPrimitive> _capBuffer;

	/// This helper structure is used to detect any changes in the input data
	/// that require updating the geometry buffer.
	SceneObjectCacheHelper<
		ColorA,						// Surface color
		VersionedDataObjectRef		// Cluster graph
		> _geometryCacheHelper;

	/// The revision counter of this display object.
	/// The counter is increment every time the object's parameters change.
	unsigned int _revisionNumber = 0;
	
	Q_OBJECT
	OVITO_CLASS

	Q_CLASSINFO("DisplayName", "Microstructure");
};

}	// End of namespace
}	// End of namespace
}	// End of namespace


