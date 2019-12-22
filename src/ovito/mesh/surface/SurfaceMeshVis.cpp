////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2019 Alexander Stukowski
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

#include <ovito/mesh/Mesh.h>
#include <ovito/mesh/util/CapPolygonTessellator.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/core/rendering/SceneRenderer.h>
#include <ovito/core/rendering/MeshPrimitive.h>
#include <ovito/core/utilities/mesh/TriMesh.h>
#include <ovito/core/utilities/units/UnitsManager.h>
#include <ovito/core/dataset/animation/controller/Controller.h>
#include <ovito/core/dataset/data/VersionedDataObjectRef.h>
#include <ovito/core/dataset/DataSetContainer.h>
#include "SurfaceMeshVis.h"
#include "SurfaceMesh.h"
#include "RenderableSurfaceMesh.h"

namespace Ovito { namespace Mesh {

IMPLEMENT_OVITO_CLASS(SurfaceMeshVis);
DEFINE_PROPERTY_FIELD(SurfaceMeshVis, surfaceColor);
DEFINE_PROPERTY_FIELD(SurfaceMeshVis, capColor);
DEFINE_PROPERTY_FIELD(SurfaceMeshVis, showCap);
DEFINE_PROPERTY_FIELD(SurfaceMeshVis, smoothShading);
DEFINE_PROPERTY_FIELD(SurfaceMeshVis, reverseOrientation);
DEFINE_PROPERTY_FIELD(SurfaceMeshVis, highlightEdges);
DEFINE_REFERENCE_FIELD(SurfaceMeshVis, surfaceTransparencyController);
DEFINE_REFERENCE_FIELD(SurfaceMeshVis, capTransparencyController);
SET_PROPERTY_FIELD_LABEL(SurfaceMeshVis, surfaceColor, "Surface color");
SET_PROPERTY_FIELD_LABEL(SurfaceMeshVis, capColor, "Cap color");
SET_PROPERTY_FIELD_LABEL(SurfaceMeshVis, showCap, "Show cap polygons");
SET_PROPERTY_FIELD_LABEL(SurfaceMeshVis, smoothShading, "Smooth shading");
SET_PROPERTY_FIELD_LABEL(SurfaceMeshVis, surfaceTransparencyController, "Surface transparency");
SET_PROPERTY_FIELD_LABEL(SurfaceMeshVis, capTransparencyController, "Cap transparency");
SET_PROPERTY_FIELD_LABEL(SurfaceMeshVis, reverseOrientation, "Inside out");
SET_PROPERTY_FIELD_LABEL(SurfaceMeshVis, highlightEdges, "Highlight edges");
SET_PROPERTY_FIELD_UNITS_AND_RANGE(SurfaceMeshVis, surfaceTransparencyController, PercentParameterUnit, 0, 1);
SET_PROPERTY_FIELD_UNITS_AND_RANGE(SurfaceMeshVis, capTransparencyController, PercentParameterUnit, 0, 1);

IMPLEMENT_OVITO_CLASS(SurfaceMeshPickInfo);

/******************************************************************************
* Constructor.
******************************************************************************/
SurfaceMeshVis::SurfaceMeshVis(DataSet* dataset) : TransformingDataVis(dataset),
	_surfaceColor(1, 1, 1),
	_capColor(0.8, 0.8, 1.0),
	_showCap(true),
	_smoothShading(true),
	_reverseOrientation(false),
	_highlightEdges(false)
{
	setSurfaceTransparencyController(ControllerManager::createFloatController(dataset));
	setCapTransparencyController(ControllerManager::createFloatController(dataset));
}

/******************************************************************************
* Is called when the value of a property of this object has changed.
******************************************************************************/
void SurfaceMeshVis::propertyChanged(const PropertyFieldDescriptor& field)
{
	if(field == PROPERTY_FIELD(smoothShading) || field == PROPERTY_FIELD(reverseOrientation)) {
		// This kind of parameter change triggers a regeneration of the cached RenderableSurfaceMesh.
		invalidateTransformedObjects();
	}
	TransformingDataVis::propertyChanged(field);
}

/******************************************************************************
* Lets the vis element transform a data object in preparation for rendering.
******************************************************************************/
Future<PipelineFlowState> SurfaceMeshVis::transformDataImpl(const PipelineEvaluationRequest& request, const DataObject* dataObject, PipelineFlowState&& flowState, const PipelineFlowState& cachedState)
{
	// Get the input surface mesh.
	const SurfaceMesh* surfaceMesh = dynamic_object_cast<SurfaceMesh>(dataObject);
	if(!surfaceMesh)
		return std::move(flowState);

	// Make sure the surface mesh is ok.
	surfaceMesh->verifyMeshIntegrity();

	// Create compute engine.
	auto engine = createSurfaceEngine(surfaceMesh);

	// Submit engine for execution and post-process results.
	return dataset()->container()->taskManager().runTaskAsync(std::move(engine))
		.then(executor(), [this, flowState = std::move(flowState), dataObject](TriMesh&& surfaceMesh, TriMesh&& capPolygonsMesh, std::vector<ColorA>&& materialColors, std::vector<size_t>&& originalFaceMap, bool renderFacesTwoSided) mutable {
			UndoSuspender noUndo(this);

			// Output the computed mesh as a RenderableSurfaceMesh.
			OORef<RenderableSurfaceMesh> renderableMesh = new RenderableSurfaceMesh(this, dataObject, std::move(surfaceMesh), std::move(capPolygonsMesh), !renderFacesTwoSided);
            renderableMesh->setMaterialColors(std::move(materialColors));
            renderableMesh->setOriginalFaceMap(std::move(originalFaceMap));
			flowState.addObject(renderableMesh);
			return std::move(flowState);
		});
}

/******************************************************************************
* Computes the bounding box of the displayed data.
******************************************************************************/
Box3 SurfaceMeshVis::boundingBox(TimePoint time, const std::vector<const DataObject*>& objectStack, const PipelineSceneNode* contextNode, const PipelineFlowState& flowState, TimeInterval& validityInterval)
{
	Box3 bb;

	// Compute mesh bounding box.
	// Requires that the periodic SurfaceMesh has already been transformed into a non-periodic RenderableSurfaceMesh.
	if(const RenderableSurfaceMesh* meshObj = dynamic_object_cast<RenderableSurfaceMesh>(objectStack.back())) {
		bb.addBox(meshObj->surfaceMesh().boundingBox());
		bb.addBox(meshObj->capPolygonsMesh().boundingBox());
	}
	return bb;
}

/******************************************************************************
* Lets the visualization element render the data object.
******************************************************************************/
void SurfaceMeshVis::render(TimePoint time, const std::vector<const DataObject*>& objectStack, const PipelineFlowState& flowState, SceneRenderer* renderer, const PipelineSceneNode* contextNode)
{
	// Ignore render calls for the original SurfaceMesh.
	// We are only interested in the RenderableSurfaceMesh.
	if(dynamic_object_cast<SurfaceMesh>(objectStack.back()) != nullptr)
		return;

	if(renderer->isBoundingBoxPass()) {
		TimeInterval validityInterval;
		renderer->addToLocalBoundingBox(boundingBox(time, objectStack, contextNode, flowState, validityInterval));
		return;
	}

	// Get the rendering colors for the surface and cap meshes.
	FloatType surface_alpha = 1;
	FloatType cap_alpha = 1;
	TimeInterval iv;
	if(surfaceTransparencyController())
		surface_alpha = qBound(0.0, FloatType(1) - surfaceTransparencyController()->getFloatValue(time, iv), 1.0);
	if(capTransparencyController())
		cap_alpha = qBound(0.0, FloatType(1) - capTransparencyController()->getFloatValue(time, iv), 1.0);
	ColorA color_surface(surfaceColor(), surface_alpha);
	ColorA color_cap(capColor(), cap_alpha);

	// The key type used for caching the surface primitive:
	using SurfaceCacheKey = std::tuple<
		CompatibleRendererGroup,	// The scene renderer
		VersionedDataObjectRef,		// Mesh object
		ColorA,						// Surface color
		ColorA,						// Cap color
		bool						// Edge highlighting
	>;

    // The values stored in the vis cache.
    struct CacheValue {
        std::shared_ptr<MeshPrimitive> surfacePrimitive;
        std::shared_ptr<MeshPrimitive> capPrimitive;
        OORef<ObjectPickInfo> pickInfo;
    };

    // Get the renderable mesh.
    const RenderableSurfaceMesh* renderableMesh = dynamic_object_cast<RenderableSurfaceMesh>(objectStack.back());
    if(!renderableMesh) return;

	// Lookup the rendering primitive in the vis cache.
    auto& visCache = dataset()->visCache().get<CacheValue>(SurfaceCacheKey(renderer, objectStack.back(), color_surface, color_cap, highlightEdges()));

	// Check if we already have a valid rendering primitive that is up to date.
	if(!visCache.surfacePrimitive || !visCache.surfacePrimitive->isValid(renderer)) {
		visCache.surfacePrimitive = renderer->createMeshPrimitive();
		auto materialColors = renderableMesh->materialColors();
		for(ColorA& c : materialColors) {
			c.a() = surface_alpha;
		}
		visCache.surfacePrimitive->setMaterialColors(std::move(materialColors));
		visCache.surfacePrimitive->setMesh(renderableMesh->surfaceMesh(), color_surface, highlightEdges());
		visCache.surfacePrimitive->setCullFaces(renderableMesh->backfaceCulling());

        // Get the original surface mesh.
        const SurfaceMesh* surfaceMesh = dynamic_object_cast<SurfaceMesh>(renderableMesh->sourceDataObject().get());

        // Create the pick record that keeps a reference to the original data.
        visCache.pickInfo = createPickInfo(surfaceMesh, renderableMesh);
	}

	// Check if we already have a valid rendering primitive that is up to date.
	if(!visCache.capPrimitive || !visCache.capPrimitive->isValid(renderer)) {
		if(showCap()) {
			visCache.capPrimitive = renderer->createMeshPrimitive();
			visCache.capPrimitive->setMesh(renderableMesh->capPolygonsMesh(), color_cap);
		}
	}

	// Handle picking of triangles.
	renderer->beginPickObject(contextNode, visCache.pickInfo);
	if(visCache.surfacePrimitive) {
		visCache.surfacePrimitive->render(renderer);
	}
	if(showCap()) {
		if(!renderer->isPicking() || cap_alpha >= 1)
			visCache.capPrimitive->render(renderer);
	}
	else {
		visCache.capPrimitive.reset();
	}
	renderer->endPickObject();
}

/******************************************************************************
* Create the viewport picking record for the surface mesh object.
******************************************************************************/
OORef<ObjectPickInfo> SurfaceMeshVis::createPickInfo(const SurfaceMesh* mesh, const RenderableSurfaceMesh* renderableMesh) const
{
    return new SurfaceMeshPickInfo(this, mesh, renderableMesh);
}

/******************************************************************************
* Returns a human-readable string describing the picked object,
* which will be displayed in the status bar by OVITO.
******************************************************************************/
QString SurfaceMeshPickInfo::infoString(PipelineSceneNode* objectNode, quint32 subobjectId)
{
    QString str = surfaceMesh()->objectTitle();

	// List all the properties of the face.
    auto facetIndex = slipFacetIndexFromSubObjectID(subobjectId);
	if(surfaceMesh()->faces() && facetIndex >= 0 && facetIndex < surfaceMesh()->faces()->elementCount()) {
		for(const PropertyObject* property : surfaceMesh()->faces()->properties()) {
	        if(facetIndex >= property->size()) continue;
			if(property->type() == SurfaceMeshFaces::SelectionProperty) continue;
			if(property->type() == SurfaceMeshFaces::ColorProperty) continue;
			if(property->type() == SurfaceMeshFaces::RegionProperty) continue;
			if(property->dataType() != PropertyStorage::Int && property->dataType() != PropertyStorage::Int64 && property->dataType() != PropertyStorage::Float) continue;
			if(!str.isEmpty()) str += QStringLiteral(" | ");
			str += property->name();
			str += QStringLiteral(" ");
			for(size_t component = 0; component < property->componentCount(); component++) {
				if(component != 0) str += QStringLiteral(", ");
				if(property->dataType() == PropertyStorage::Int) {
					str += QString::number(property->getIntComponent(facetIndex, component));
					if(property->elementTypes().empty() == false) {
						if(const ElementType* ptype = property->elementType(property->getIntComponent(facetIndex, component))) {
							if(!ptype->name().isEmpty())
								str += QString(" (%1)").arg(ptype->name());
						}
					}
				}
				else if(property->dataType() == PropertyStorage::Int64) {
					str += QString::number(property->getInt64Component(facetIndex, component));
				}
				else if(property->dataType() == PropertyStorage::Float) {
					str += QString::number(property->getFloatComponent(facetIndex, component));
				}
			}
		}

		// Additionally, list all properties of the region to which the face belongs.
		if(const PropertyObject* regionProperty = surfaceMesh()->faces()->getProperty(SurfaceMeshFaces::RegionProperty)) {
			if(facetIndex < regionProperty->size() && surfaceMesh()->regions()) {
				int regionIndex = regionProperty->getInt(facetIndex);
				if(!str.isEmpty()) str += QStringLiteral(" | ");
				str += QStringLiteral("Region %1").arg(regionIndex);
				for(const PropertyObject* property : surfaceMesh()->regions()->properties()) {
					if(regionIndex < 0 || regionIndex >= property->size()) continue;
					if(property->dataType() != PropertyStorage::Int && property->dataType() != PropertyStorage::Int64 && property->dataType() != PropertyStorage::Float) continue;
					str += QStringLiteral(" | ");
					str += property->name();
					str += QStringLiteral(" ");
					for(size_t component = 0; component < property->componentCount(); component++) {
						if(component != 0) str += QStringLiteral(", ");
						if(property->dataType() == PropertyStorage::Int) {
							str += QString::number(property->getIntComponent(regionIndex, component));
							if(property->elementTypes().empty() == false) {
								if(const ElementType* ptype = property->elementType(property->getIntComponent(regionIndex, component))) {
									if(!ptype->name().isEmpty())
										str += QString(" (%1)").arg(ptype->name());
								}
							}
						}
						else if(property->dataType() == PropertyStorage::Int64) {
							str += QString::number(property->getInt64Component(regionIndex, component));
						}
						else if(property->dataType() == PropertyStorage::Float) {
							str += QString::number(property->getFloatComponent(regionIndex, component));
						}
					}
				}
			}
		}
    }

    return str;
}

/******************************************************************************
* Creates the asynchronous task that builds the non-peridic representation of the input surface mesh.
******************************************************************************/
std::shared_ptr<SurfaceMeshVis::PrepareSurfaceEngine> SurfaceMeshVis::createSurfaceEngine(const SurfaceMesh* mesh) const
{
	return std::make_shared<PrepareSurfaceEngine>(
		mesh,
		reverseOrientation(),
		mesh->cuttingPlanes(),
		smoothShading(),
		surfaceColor(),
		true);
}

/******************************************************************************
* Computes the results and stores them in this object for later retrieval.
******************************************************************************/
void SurfaceMeshVis::PrepareSurfaceEngine::perform()
{
	setProgressText(tr("Preparing mesh for display"));

	determineVisibleFaces();
	if(isCanceled()) return;

	// Determine wheter we can simply use two-sided rendering to display faces.
	// This will be the case case if there is no visible mesh face that has a 
	// corresponding opposite face.
	if(_faceSubset.empty()) {
		_renderFacesTwoSided = std::none_of(inputMesh().topology()->begin_faces(), inputMesh().topology()->end_faces(),
			std::bind(&HalfEdgeMesh::hasOppositeFace, inputMesh().topology().get(), std::placeholders::_1));
	}
	else {
		_renderFacesTwoSided = std::none_of(inputMesh().topology()->begin_faces(), inputMesh().topology()->end_faces(),
			[this](SurfaceMeshData::face_index face) { return _faceSubset[face] && inputMesh().hasOppositeFace(face) && _faceSubset[inputMesh().oppositeFace(face)]; });
	}

	if(!buildSurfaceTriangleMesh() && !isCanceled())
		throw Exception(tr("Failed to build non-periodic representation of periodic surface mesh. Periodic domain might be too small."));

	if(isCanceled()) return;

	determineFaceColors();

	if(_generateCapPolygons) {
		if(isCanceled()) return;
		buildCapTriangleMesh();
	}

	setResult(
		std::move(_surfaceMesh), 
		std::move(_capPolygonsMesh), 
		std::move(_materialColors), 
		std::move(_originalFaceMap), 
		_renderFacesTwoSided);
}

/******************************************************************************
* Transfers face colors from the input to the output mesh.
******************************************************************************/
void SurfaceMeshVis::PrepareSurfaceEngine::determineFaceColors()
{
	ColorA defaultFaceColor(_surfaceColor);
	ColorA selectionColor(1,0,0,1);

	if(PropertyPtr colorProperty = _inputMesh.faceProperty(SurfaceMeshFaces::ColorProperty)) {
		// The "Color" property of mesh faces has the highest priority.
		// If it is present, use its information to color the triangle faces.
		_surfaceMesh.setHasFaceColors(true);
		auto meshFaceColor = _surfaceMesh.faceColors().begin();
		for(size_t originalFace : _originalFaceMap) {
			OVITO_ASSERT(originalFace < colorProperty->size());
			*meshFaceColor++ = ColorA(colorProperty->getColor(originalFace));
		}
	}
	else if(PropertyPtr colorProperty = _inputMesh.regionProperty(SurfaceMeshRegions::ColorProperty)) {
		// If the "Color" property of mesh regions is present, use it information to color the 
		// mesh faces according to the region they belong to.
		if(PropertyPtr regionProperty = _inputMesh.faceProperty(SurfaceMeshFaces::RegionProperty)) {
			_surfaceMesh.setHasFaceColors(true);
			size_t regionCount = colorProperty->size();
			auto meshFaceColor = _surfaceMesh.faceColors().begin();
			for(size_t originalFace : _originalFaceMap) {
				OVITO_ASSERT(originalFace < regionProperty->size());
				SurfaceMeshData::region_index regionIndex = regionProperty->getInt(originalFace);
				if(regionIndex >= 0 && regionIndex < regionCount)
					*meshFaceColor++ = ColorA(colorProperty->getColor(regionIndex));
				else
					*meshFaceColor++ = defaultFaceColor;
			}
		}
	}

	if(PropertyPtr selectionProperty = _inputMesh.faceProperty(SurfaceMeshFaces::SelectionProperty)) {
		// The "Selection" property of mesh faces has the highest priority.
		// If it is present, use it to highlight selected mesh faces.
		if(!_surfaceMesh.hasFaceColors()) {
			_surfaceMesh.setHasFaceColors(true);
			std::fill(_surfaceMesh.faceColors().begin(), _surfaceMesh.faceColors().end(), defaultFaceColor);
		}
		auto meshFaceColor = _surfaceMesh.faceColors().begin();
		for(size_t originalFace : _originalFaceMap) {
			OVITO_ASSERT(originalFace < selectionProperty->size());
			if(selectionProperty->getInt(originalFace))
				*meshFaceColor = selectionColor;
			++meshFaceColor;
		}
	}
	else if(PropertyPtr selectionProperty = _inputMesh.regionProperty(SurfaceMeshRegions::SelectionProperty)) {
		// If the "Selection" property of mesh regions is present, use it information to highlight the 
		// mesh faces that belong to selected regions.
		if(PropertyPtr regionProperty = _inputMesh.faceProperty(SurfaceMeshFaces::RegionProperty)) {
			if(!_surfaceMesh.hasFaceColors()) {
				_surfaceMesh.setHasFaceColors(true);
				std::fill(_surfaceMesh.faceColors().begin(), _surfaceMesh.faceColors().end(), defaultFaceColor);
			}
			size_t regionCount = selectionProperty->size();
			auto meshFaceColor = _surfaceMesh.faceColors().begin();
			for(size_t originalFace : _originalFaceMap) {
				OVITO_ASSERT(originalFace < regionProperty->size());
				SurfaceMeshData::region_index regionIndex = regionProperty->getInt(originalFace);
				if(regionIndex >= 0 && regionIndex < regionCount && selectionProperty->getInt(regionIndex))
					*meshFaceColor = selectionColor;
				++meshFaceColor;
			}
		}
	}
}

/******************************************************************************
* Transfers vertex colors from the input to the output mesh.
******************************************************************************/
void SurfaceMeshVis::PrepareSurfaceEngine::determineVertexColors()
{
	if(PropertyPtr colorProperty = _inputMesh.vertexProperty(SurfaceMeshVertices::ColorProperty)) {
		OVITO_ASSERT(colorProperty->size() == _surfaceMesh.vertexCount());
		if(colorProperty->size() == _surfaceMesh.vertexCount()) {
			_surfaceMesh.setHasVertexColors(true);
			std::transform(colorProperty->constDataColor(), colorProperty->constDataColor() + colorProperty->size(),
				_surfaceMesh.vertexColors().begin(), [](const Color& c) { return ColorA(c); });
		}
	}
}

/******************************************************************************
* Generates the triangle mesh from the periodic surface mesh, which will be rendered.
******************************************************************************/
bool SurfaceMeshVis::PrepareSurfaceEngine::buildSurfaceTriangleMesh()
{
	if(cell().is2D())
		throw Exception(tr("Cannot generate surface triangle mesh when domain is two-dimensional."));

	// Transfer vertices and faces from half-edge mesh structure to triangle mesh structure.
	_inputMesh.convertToTriMesh(_surfaceMesh, _smoothShading, _faceSubset, &_originalFaceMap, !_renderFacesTwoSided);

	// Check for early abortion.
	if(isCanceled())
		return false;

	// Assign mesh vertex colors if available.
	determineVertexColors();

	// Flip orientation of mesh faces if requested.
	if(_reverseOrientation)
		_surfaceMesh.flipFaces();

	// Check for early abortion.
	if(isCanceled())
		return false;

	// Convert vertex positions to reduced coordinates and transfer them to the output mesh.
	SurfaceMeshData::vertex_index vidx = 0;
	for(Point3& p : _surfaceMesh.vertices()) {
		p = cell().absoluteToReduced(_inputMesh.vertexPosition(vidx++));
		OVITO_ASSERT(std::isfinite(p.x()) && std::isfinite(p.y()) && std::isfinite(p.z()));
	}

	// Wrap mesh at periodic boundaries.
	for(size_t dim = 0; dim < 3; dim++) {
		if(cell().pbcFlags()[dim] == false) continue;

		if(isCanceled())
			return false;

		// Make sure all vertices are located inside the periodic box.
		for(Point3& p : _surfaceMesh.vertices()) {
			OVITO_ASSERT(std::isfinite(p[dim]));
			p[dim] -= std::floor(p[dim]);
			OVITO_ASSERT(p[dim] >= FloatType(0) && p[dim] <= FloatType(1));
		}

		// Split triangle faces at periodic boundaries.
		int oldFaceCount = _surfaceMesh.faceCount();
		int oldVertexCount = _surfaceMesh.vertexCount();
		std::vector<Point3> newVertices;
		std::vector<ColorA> newVertexColors;
		std::map<std::pair<int,int>,std::tuple<int,int,FloatType>> newVertexLookupMap;
		for(int findex = 0; findex < oldFaceCount; findex++) {
			if(!splitFace(findex, oldVertexCount, newVertices, newVertexColors, newVertexLookupMap, dim)) {
				return false;
			}
		}

		// Insert newly created vertices into mesh.
		_surfaceMesh.setVertexCount(oldVertexCount + newVertices.size());
		std::copy(newVertices.cbegin(), newVertices.cend(), _surfaceMesh.vertices().begin() + oldVertexCount);
		if(_surfaceMesh.hasVertexColors()) {
			OVITO_ASSERT(newVertexColors.size() == newVertices.size());
			std::copy(newVertexColors.cbegin(), newVertexColors.cend(), _surfaceMesh.vertexColors().begin() + oldVertexCount);
		}
	}
	if(isCanceled())
		return false;

	// Convert vertex positions back from reduced coordinates to absolute coordinates.
	const AffineTransformation cellMatrix = cell().matrix();
	for(Point3& p : _surfaceMesh.vertices())
		p = cellMatrix * p;

	// Clip mesh at cutting planes.
	if(!_cuttingPlanes.empty()) {
		auto of = _originalFaceMap.begin();
		for(TriMeshFace& face : _surfaceMesh.faces())
			face.setMaterialIndex(*of++);

		for(const Plane3& plane : _cuttingPlanes) {
			if(isCanceled())
				return false;

			_surfaceMesh.clipAtPlane(plane);
		}

		_originalFaceMap.resize(_surfaceMesh.faces().size());
		of = _originalFaceMap.begin();
		for(TriMeshFace& face : _surfaceMesh.faces())
			*of++ = face.materialIndex();
	}

	_surfaceMesh.invalidateVertices();
	_surfaceMesh.invalidateFaces();
	OVITO_ASSERT(_originalFaceMap.size() == _surfaceMesh.faces().size());

	return true;
}

/******************************************************************************
* Splits a triangle face at a periodic boundary.
******************************************************************************/
bool SurfaceMeshVis::PrepareSurfaceEngine::splitFace(int faceIndex, int oldVertexCount, std::vector<Point3>& newVertices, std::vector<ColorA>& newVertexColors,
		std::map<std::pair<int,int>,std::tuple<int,int,FloatType>>& newVertexLookupMap, size_t dim)
{
    TriMeshFace& face = _surfaceMesh.face(faceIndex);
	OVITO_ASSERT(face.vertex(0) != face.vertex(1));
	OVITO_ASSERT(face.vertex(1) != face.vertex(2));
	OVITO_ASSERT(face.vertex(2) != face.vertex(0));

	FloatType z[3];
	for(int v = 0; v < 3; v++)
		z[v] = _surfaceMesh.vertex(face.vertex(v))[dim];
	FloatType zd[3] = { z[1] - z[0], z[2] - z[1], z[0] - z[2] };

	OVITO_ASSERT(z[1] - z[0] == -(z[0] - z[1]));
	OVITO_ASSERT(z[2] - z[1] == -(z[1] - z[2]));
	OVITO_ASSERT(z[0] - z[2] == -(z[2] - z[0]));

	if(std::abs(zd[0]) < FloatType(0.5) && std::abs(zd[1]) < FloatType(0.5) && std::abs(zd[2]) < FloatType(0.5))
		return true;	// Face does not cross the periodic boundary.

	// Create four new vertices (or use existing ones created during splitting of adjacent faces).
	int properEdge = -1;
	int newVertexIndices[3][2];
	Vector3 interpolatedNormals[3];
	for(int i = 0; i < 3; i++) {
		if(std::abs(zd[i]) < FloatType(0.5)) {
			if(properEdge != -1)
				return false;		// The simulation box may be too small or invalid.
			properEdge = i;
			continue;
		}
		int vi1 = face.vertex(i);
		int vi2 = face.vertex((i+1)%3);
		int oi1, oi2;
		if(zd[i] <= FloatType(-0.5)) {
			std::swap(vi1, vi2);
			oi1 = 1; oi2 = 0;
		}
		else {
			oi1 = 0; oi2 = 1;
		}
		auto entry = newVertexLookupMap.find(std::make_pair(vi1, vi2));
		if(entry != newVertexLookupMap.end()) {
			newVertexIndices[i][oi1] = std::get<0>(entry->second);
			newVertexIndices[i][oi2] = std::get<1>(entry->second);
		}
		else {
			Vector3 delta = _surfaceMesh.vertex(vi2) - _surfaceMesh.vertex(vi1);
			delta[dim] -= FloatType(1);
			for(size_t d = dim + 1; d < 3; d++) {
				if(cell().pbcFlags()[d])
					delta[d] -= floor(delta[d] + FloatType(0.5));
			}
			FloatType t;
			if(delta[dim] != 0)
				t = _surfaceMesh.vertex(vi1)[dim] / (-delta[dim]);
			else
				t = FloatType(0.5);
			OVITO_ASSERT(std::isfinite(t));
			Point3 p = delta * t + _surfaceMesh.vertex(vi1);
			newVertexIndices[i][oi1] = oldVertexCount + (int)newVertices.size();
			newVertexIndices[i][oi2] = oldVertexCount + (int)newVertices.size() + 1;
			entry = newVertexLookupMap.emplace(std::make_pair(vi1, vi2), std::make_tuple(newVertexIndices[i][oi1], newVertexIndices[i][oi2], t)).first;
			newVertices.push_back(p);
			p[dim] += FloatType(1);
			newVertices.push_back(p);
			// Compute the color at the intersection point by interpolating the colors of the two existing vertices.
			if(_surfaceMesh.hasVertexColors()) {
				const ColorA& color1 = _surfaceMesh.vertexColor(vi1);
				const ColorA& color2 = _surfaceMesh.vertexColor(vi2);
				ColorA interp_color(color1.r() + (color2.r() - color1.r()) * t,
									color1.g() + (color2.g() - color1.g()) * t,
									color1.b() + (color2.b() - color1.b()) * t,
									color1.a() + (color2.a() - color1.a()) * t);
				newVertexColors.push_back(interp_color);
				newVertexColors.push_back(interp_color);
			}
		}
		// Compute interpolated normal vector at intersection point.
		if(_smoothShading) {
			const Vector3& n1 = _surfaceMesh.faceVertexNormal(faceIndex, (i+oi1)%3);
			const Vector3& n2 = _surfaceMesh.faceVertexNormal(faceIndex, (i+oi2)%3);
			FloatType t = std::get<2>(entry->second);
			interpolatedNormals[i] = n1*t + n2*(FloatType(1)-t);
			interpolatedNormals[i].normalizeSafely();
		}
	}
	OVITO_ASSERT(properEdge != -1);

	// Build output triangles.
	int originalVertices[3] = { face.vertex(0), face.vertex(1), face.vertex(2) };
	bool originalEdgeVisibility[3] = { face.edgeVisible(0), face.edgeVisible(1), face.edgeVisible(2) };
	int pe1 = (properEdge+1)%3;
	int pe2 = (properEdge+2)%3;
	face.setVertices(originalVertices[properEdge], originalVertices[pe1], newVertexIndices[pe2][1]);
	face.setEdgeVisibility(originalEdgeVisibility[properEdge], false, originalEdgeVisibility[pe2]);

    int materialIndex = face.materialIndex();
	OVITO_ASSERT(_originalFaceMap.size() == _surfaceMesh.faceCount());
	_surfaceMesh.setFaceCount(_surfaceMesh.faceCount() + 2);
    _originalFaceMap.resize(_originalFaceMap.size() + 2, _originalFaceMap[faceIndex]);
	TriMeshFace& newFace1 = _surfaceMesh.face(_surfaceMesh.faceCount() - 2);
	TriMeshFace& newFace2 = _surfaceMesh.face(_surfaceMesh.faceCount() - 1);
	newFace1.setVertices(originalVertices[pe1], newVertexIndices[pe1][0], newVertexIndices[pe2][1]);
	newFace2.setVertices(newVertexIndices[pe1][1], originalVertices[pe2], newVertexIndices[pe2][0]);
    newFace1.setMaterialIndex(materialIndex);
    newFace2.setMaterialIndex(materialIndex);
	newFace1.setEdgeVisibility(originalEdgeVisibility[pe1], false, false);
	newFace2.setEdgeVisibility(originalEdgeVisibility[pe1], originalEdgeVisibility[pe2], false);
	if(_smoothShading) {
		auto n = _surfaceMesh.normals().end() - 6;
		*n++ = _surfaceMesh.faceVertexNormal(faceIndex, pe1);
		*n++ = interpolatedNormals[pe1];
		*n++ = interpolatedNormals[pe2];
		*n++ = interpolatedNormals[pe1];
		*n++ = _surfaceMesh.faceVertexNormal(faceIndex, pe2);
		*n   = interpolatedNormals[pe2];
		n = _surfaceMesh.normals().begin() + faceIndex*3;
		std::rotate(n, n + properEdge, n + 3);
		_surfaceMesh.setFaceVertexNormal(faceIndex, 2, interpolatedNormals[pe2]);
	}

	return true;
}

/******************************************************************************
* Generates the cap polygons where the surface mesh intersects the
* periodic domain boundaries.
******************************************************************************/
void SurfaceMeshVis::PrepareSurfaceEngine::buildCapTriangleMesh()
{
	bool isCompletelySolid = _inputMesh.spaceFillingRegion() != 0;
	bool flipCapNormal = (cell().matrix().determinant() < 0);

	// Convert vertex positions to reduced coordinates.
	AffineTransformation invCellMatrix = cell().inverseMatrix();
	if(flipCapNormal)
		invCellMatrix.column(0) = -invCellMatrix.column(0);

	std::vector<Point3> reducedPos(_inputMesh.vertexCount());
	SurfaceMeshData::vertex_index vidx = 0;
	for(Point3& p : reducedPos)
		p = invCellMatrix * _inputMesh.vertexPosition(vidx++);

	int isBoxCornerInside3DRegion = -1;

	// Create caps for each periodic boundary.
	for(size_t dim = 0; dim < 3; dim++) {
		if(cell().pbcFlags()[dim] == false) continue;

		if(isCanceled())
			return;

		// Make sure all vertices are located inside the periodic box.
		for(Point3& p : reducedPos) {
			FloatType& c = p[dim];
			OVITO_ASSERT(std::isfinite(c));
			if(FloatType s = std::floor(c))
				c -= s;
			OVITO_ASSERT(std::isfinite(c));
		}

		// Used to keep track of already visited faces during the current pass.
		std::vector<bool> visitedFaces(_inputMesh.faceCount(), false);

		// The lists of 2d contours generated by clipping the 3d surface mesh.
		std::vector<std::vector<Point2>> openContours;
		std::vector<std::vector<Point2>> closedContours;

		// Find a first edge that crosses a periodic cell boundary.
		for(HalfEdgeMesh::face_index face : _originalFaceMap) {
			// Skip faces that have already been visited.
			if(visitedFaces[face]) continue;
			if(isCanceled()) return;
			visitedFaces[face] = true;

			HalfEdgeMesh::edge_index startEdge = _inputMesh.firstFaceEdge(face);
			HalfEdgeMesh::edge_index edge = startEdge;
			do {
				const Point3& v1 = reducedPos[_inputMesh.vertex1(edge)];
				const Point3& v2 = reducedPos[_inputMesh.vertex2(edge)];
				if(v2[dim] - v1[dim] >= FloatType(0.5)) {
					std::vector<Point2> contour = traceContour(edge, reducedPos, visitedFaces, dim);
					if(contour.empty())
						throw Exception(tr("Surface mesh is not a proper manifold."));
					clipContour(contour, std::array<bool,2>{{ cell().pbcFlags()[(dim+1)%3], cell().pbcFlags()[(dim+2)%3] }}, openContours, closedContours);
					break;
				}
				edge = _inputMesh.nextFaceEdge(edge);
			}
			while(edge != startEdge);
		}

		if(_reverseOrientation) {
			for(auto& contour : openContours)
				std::reverse(std::begin(contour), std::end(contour));
		}

		// Feed contours into tessellator to create triangles.
		CapPolygonTessellator tessellator(_capPolygonsMesh, dim);
		tessellator.beginPolygon();
		for(const auto& contour : closedContours) {
			if(isCanceled())
				return;
			tessellator.beginContour();
			for(const Point2& p : contour) {
				tessellator.vertex(p);
			}
			tessellator.endContour();
		}

		auto yxCoord2ArcLength = [](const Point2& p) {
			if(p.x() == 0) return p.y();
			else if(p.y() == 1) return p.x() + FloatType(1);
			else if(p.x() == 1) return FloatType(3) - p.y();
			else return std::fmod(FloatType(4) - p.x(), FloatType(4));
		};

		// Build the outer contour.
		if(!openContours.empty()) {
			boost::dynamic_bitset<> visitedContours(openContours.size());
			for(auto c1 = openContours.begin(); c1 != openContours.end(); ++c1) {
				if(isCanceled())
					return;
				if(!visitedContours.test(c1 - openContours.begin())) {
					tessellator.beginContour();
					auto currentContour = c1;
					do {
						for(const Point2& p : *currentContour) {
							tessellator.vertex(p);
						}
						visitedContours.set(currentContour - openContours.begin());

						FloatType t_exit = yxCoord2ArcLength(currentContour->back());

						// Find the next contour.
						FloatType t_entry;
						FloatType closestDist = FLOATTYPE_MAX;
						for(auto c = openContours.begin(); c != openContours.end(); ++c) {
							FloatType t = yxCoord2ArcLength(c->front());
							FloatType dist = t_exit - t;
							if(dist < 0) dist += FloatType(4);
							if(dist < closestDist) {
								closestDist = dist;
								currentContour = c;
								t_entry = t;
							}
						}
						int exitCorner = (int)std::floor(t_exit);
						int entryCorner = (int)std::floor(t_entry);
						OVITO_ASSERT(exitCorner >= 0 && exitCorner < 4);
						OVITO_ASSERT(entryCorner >= 0 && entryCorner < 4);
						if(exitCorner != entryCorner || t_exit < t_entry) {
							for(int corner = exitCorner;;) {
								switch(corner) {
								case 0: tessellator.vertex(Point2(0,0)); break;
								case 1: tessellator.vertex(Point2(0,1)); break;
								case 2: tessellator.vertex(Point2(1,1)); break;
								case 3: tessellator.vertex(Point2(1,0)); break;
								}
								corner = (corner + 3) % 4;
								if(corner == entryCorner) break;
							}
						}
					}
					while(!visitedContours.test(currentContour - openContours.begin()));
					tessellator.endContour();
				}
			}
		}
		else {
			if(isBoxCornerInside3DRegion == -1) {
				if(closedContours.empty()) {
					isBoxCornerInside3DRegion = (_inputMesh.locatePoint(Point3::Origin() + cell().matrix().translation(), 0, _faceSubset) > 0);
				}
				else {
					isBoxCornerInside3DRegion = isCornerInside2DRegion(closedContours);
				}
				if(_reverseOrientation)
					isBoxCornerInside3DRegion = !isBoxCornerInside3DRegion;
			}
			if(isBoxCornerInside3DRegion) {
				tessellator.beginContour();
				tessellator.vertex(Point2(0,0));
				tessellator.vertex(Point2(1,0));
				tessellator.vertex(Point2(1,1));
				tessellator.vertex(Point2(0,1));
				tessellator.endContour();
			}
		}

		tessellator.endPolygon();
	}

	// Check for early abortion.
	if(isCanceled())
		return;

	// Convert vertex positions back from reduced coordinates to absolute coordinates.
	const AffineTransformation cellMatrix = invCellMatrix.inverse();
	for(Point3& p : _capPolygonsMesh.vertices())
		p = cellMatrix * p;

	// Clip mesh at cutting planes.
	for(const Plane3& plane : _cuttingPlanes) {
		if(isCanceled())
			return;
		_capPolygonsMesh.clipAtPlane(plane);
	}
}

/******************************************************************************
* Traces the closed contour of the surface-boundary intersection.
******************************************************************************/
std::vector<Point2> SurfaceMeshVis::PrepareSurfaceEngine::traceContour(HalfEdgeMesh::edge_index firstEdge, const std::vector<Point3>& reducedPos, std::vector<bool>& visitedFaces, size_t dim) const
{
	size_t dim1 = (dim + 1) % 3;
	size_t dim2 = (dim + 2) % 3;
	std::vector<Point2> contour;
	HalfEdgeMesh::edge_index edge = firstEdge;
	do {
		OVITO_ASSERT(_inputMesh.adjacentFace(edge) != HalfEdgeMesh::InvalidIndex);

		// Mark face as visited.
		visitedFaces[_inputMesh.adjacentFace(edge)] = true;

		// Compute intersection point.
		Point3 v1 = reducedPos[_inputMesh.vertex1(edge)];
		Point3 v2 = reducedPos[_inputMesh.vertex2(edge)];
		Vector3 delta = v2 - v1;
		OVITO_ASSERT(delta[dim] >= FloatType(0.5));

		delta[dim] -= FloatType(1);
		if(cell().pbcFlags()[dim1]) {
			FloatType& c = delta[dim1];
			if(FloatType s = std::floor(c + FloatType(0.5)))
				c -= s;
		}
		if(cell().pbcFlags()[dim2]) {
			FloatType& c = delta[dim2];
			if(FloatType s = std::floor(c + FloatType(0.5)))
				c -= s;
		}
		if(std::abs(delta[dim]) > FloatType(1e-9f)) {
			FloatType t = v1[dim] / delta[dim];
			FloatType x = v1[dim1] - delta[dim1] * t;
			FloatType y = v1[dim2] - delta[dim2] * t;
			OVITO_ASSERT(std::isfinite(x) && std::isfinite(y));
			if(contour.empty() || std::abs(x - contour.back().x()) > FLOATTYPE_EPSILON || std::abs(y - contour.back().y()) > FLOATTYPE_EPSILON) {
				contour.push_back({x,y});
			}
		}
		else {
			FloatType x1 = v1[dim1];
			FloatType y1 = v1[dim2];
			FloatType x2 = v1[dim1] + delta[dim1];
			FloatType y2 = v1[dim2] + delta[dim2];
			if(contour.empty() || std::abs(x1 - contour.back().x()) > FLOATTYPE_EPSILON || std::abs(y1 - contour.back().y()) > FLOATTYPE_EPSILON) {
				contour.push_back({x1,y1});
			}
			else if(contour.empty() || std::abs(x2 - contour.back().x()) > FLOATTYPE_EPSILON || std::abs(y2 - contour.back().y()) > FLOATTYPE_EPSILON) {
				contour.push_back({x2,y2});
			}
		}

		// Find the face edge that crosses the boundary in the reverse direction.
		FloatType v1d = v2[dim];
		for(;;) {
			edge = _inputMesh.nextFaceEdge(edge);
			FloatType v2d = reducedPos[_inputMesh.vertex2(edge)][dim];
			if(v2d - v1d <= FloatType(-0.5))
				break;
			v1d = v2d;
		}

		edge = _inputMesh.oppositeEdge(edge);
		if(edge == HalfEdgeMesh::InvalidIndex) {
			// Mesh is not closed (not a proper manifold).
			contour.clear();
			break;
		}
	}
	while(edge != firstEdge);
	return contour;
}

/******************************************************************************
* Clips a 2d contour at a periodic boundary.
******************************************************************************/
void SurfaceMeshVis::PrepareSurfaceEngine::clipContour(std::vector<Point2>& input, std::array<bool,2> pbcFlags, std::vector<std::vector<Point2>>& openContours, std::vector<std::vector<Point2>>& closedContours)
{
	if(!pbcFlags[0] && !pbcFlags[1]) {
		closedContours.push_back(std::move(input));
		return;
	}

	// Ensure all coordinates are within the primary image.
	if(pbcFlags[0]) {
		for(auto& v : input) {
			OVITO_ASSERT(std::isfinite(v.x()));
			if(FloatType s = floor(v.x())) v.x() -= s;
		}
	}
	if(pbcFlags[1]) {
		for(auto& v : input) {
			OVITO_ASSERT(std::isfinite(v.y()));
			if(FloatType s = floor(v.y())) v.y() -= s;
		}
	}

	std::vector<std::vector<Point2>> contours;
	contours.push_back({});

	auto v1 = input.cend() - 1;
	for(auto v2 = input.cbegin(); v2 != input.cend(); v1 = v2, ++v2) {
		contours.back().push_back(*v1);

		Vector2 delta = (*v2) - (*v1);
		if(std::abs(delta.x()) < FloatType(0.5) && std::abs(delta.y()) < FloatType(0.5))
			continue;

		FloatType t[2] = { 2, 2 };
		Vector2I crossDir(0, 0);
		for(size_t dim = 0; dim < 2; dim++) {
			if(pbcFlags[dim]) {
				if(delta[dim] >= FloatType(0.5)) {
					delta[dim] -= FloatType(1);
					if(std::abs(delta[dim]) > FLOATTYPE_EPSILON)
						t[dim] = std::min((*v1)[dim] / -delta[dim], FloatType(1));
					else
						t[dim] = FloatType(0.5);
					crossDir[dim] = -1;
					OVITO_ASSERT(t[dim] >= 0 && t[dim] <= 1);
				}
				else if(delta[dim] <= FloatType(-0.5)) {
					delta[dim] += FloatType(1);
					if(std::abs(delta[dim]) > FLOATTYPE_EPSILON)
						t[dim] = std::max((FloatType(1) - (*v1)[dim]) / delta[dim], FloatType(0));
					else
						t[dim] = FloatType(0.5);
					crossDir[dim] = +1;
					OVITO_ASSERT(t[dim] >= 0 && t[dim] <= 1);
				}
			}
		}

		Point2 base = *v1;
		if(t[0] < t[1]) {
			OVITO_ASSERT(t[0] <= 1);
			computeContourIntersection(0, t[0], base, delta, crossDir[0], contours);
			if(crossDir[1] != 0) {
				OVITO_ASSERT(t[1] <= 1);
				computeContourIntersection(1, t[1], base, delta, crossDir[1], contours);
			}
		}
		else if(t[1] < t[0]) {
			OVITO_ASSERT(t[1] <= 1);
			computeContourIntersection(1, t[1], base, delta, crossDir[1], contours);
			if(crossDir[0] != 0) {
				OVITO_ASSERT(t[0] <= 1);
				computeContourIntersection(0, t[0], base, delta, crossDir[0], contours);
			}
		}
	}

	if(contours.size() == 1) {
		closedContours.push_back(std::move(contours.back()));
	}
	else {
		auto& firstSegment = contours.front();
		auto& lastSegment = contours.back();
		firstSegment.insert(firstSegment.begin(), lastSegment.begin(), lastSegment.end());
		contours.pop_back();
		for(auto& contour : contours) {
			bool isDegenerate = std::all_of(contour.begin(), contour.end(), [&contour](const Point2& p) {
				return p.equals(contour.front());
			});
			if(!isDegenerate)
				openContours.push_back(std::move(contour));
		}
	}
}

/******************************************************************************
* Computes the intersection point of a 2d contour segment crossing a
* periodic boundary.
******************************************************************************/
void SurfaceMeshVis::PrepareSurfaceEngine::computeContourIntersection(size_t dim, FloatType t, Point2& base, Vector2& delta, int crossDir, std::vector<std::vector<Point2>>& contours)
{
	OVITO_ASSERT(std::isfinite(t));
	Point2 intersection = base + t * delta;
	intersection[dim] = (crossDir == -1) ? 0 : 1;
	contours.back().push_back(intersection);
	intersection[dim] = (crossDir == +1) ? 0 : 1;
	contours.push_back({intersection});
	base = intersection;
	delta *= (FloatType(1) - t);
}

/******************************************************************************
* Determines if the 2D box corner (0,0) is inside the closed region described
* by the 2d polygon.
*
* 2D version of the algorithm:
*
* J. Andreas Baerentzen and Henrik Aanaes
* Signed Distance Computation Using the Angle Weighted Pseudonormal
* IEEE Transactions on Visualization and Computer Graphics 11 (2005), Page 243
******************************************************************************/
bool SurfaceMeshVis::PrepareSurfaceEngine::isCornerInside2DRegion(const std::vector<std::vector<Point2>>& contours)
{
	OVITO_ASSERT(!contours.empty());
	bool isInside = true;

	// Determine which vertex is closest to the test point.
	std::vector<Point2>::const_iterator closestVertex = contours.front().end();
	FloatType closestDistanceSq = FLOATTYPE_MAX;
	for(const auto& contour : contours) {
		auto v1 = contour.end() - 1;
		for(auto v2 = contour.begin(); v2 != contour.end(); v1 = v2++) {
			Vector2 r = (*v1) - Point2::Origin();
			FloatType distanceSq = r.squaredLength();
			if(distanceSq < closestDistanceSq) {
				closestDistanceSq = distanceSq;
				closestVertex = v1;

				// Compute pseuso-normal at vertex.
				auto v0 = (v1 == contour.begin()) ? (contour.end() - 1) : (v1 - 1);
				Vector2 edgeDir = (*v2) - (*v0);
				Vector2 normal(edgeDir.y(), -edgeDir.x());
				isInside = (normal.dot(r) > 0);
			}

			// Check if any edge is closer to the test point.
			Vector2 edgeDir = (*v2) - (*v1);
			FloatType edgeLength = edgeDir.length();
			if(edgeLength <= FLOATTYPE_EPSILON) continue;
			edgeDir /= edgeLength;
			FloatType d = -edgeDir.dot(r);
			if(d <= 0 || d >= edgeLength) continue;
			Vector2 c = r + edgeDir * d;
			distanceSq = c.squaredLength();
			if(distanceSq < closestDistanceSq) {
				closestDistanceSq = distanceSq;

				// Compute normal at edge.
				Vector2 normal(edgeDir.y(), -edgeDir.x());
				isInside = (normal.dot(c) > 0);
			}
		}
	}

	return isInside;
}

}	// End of namespace
}	// End of namespace
