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

#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include <plugins/crystalanalysis/objects/ClusterGraphObject.h>
#include <plugins/crystalanalysis/objects/Microstructure.h>
#include <plugins/crystalanalysis/objects/DislocationVis.h>
#include <plugins/mesh/surface/RenderableSurfaceMesh.h>
#include <core/rendering/SceneRenderer.h>
#include <core/utilities/mesh/TriMesh.h>
#include <plugins/stdobj/simcell/SimulationCellObject.h>
#include <core/dataset/DataSetContainer.h>
#include "SlipSurfaceVis.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

IMPLEMENT_OVITO_CLASS(SlipSurfaceVis);
IMPLEMENT_OVITO_CLASS(SlipSurfacePickInfo);

/******************************************************************************
* Constructor.
******************************************************************************/
SlipSurfaceVis::SlipSurfaceVis(DataSet* dataset) : SurfaceMeshVis(dataset)
{
    // Slip surfaces consist of pairs of opposite faces. Rendere them as one-sided triangles.
    setCullFaces(true);
}

/******************************************************************************
* Constructor.
******************************************************************************/
SlipSurfaceVis::PrepareMeshEngine::PrepareMeshEngine(const SurfaceMesh* microstructure, QVector<Plane3> cuttingPlanes, bool smoothShading) :
        SurfaceMeshVis::PrepareSurfaceEngine(microstructure, false, std::move(cuttingPlanes), smoothShading, false),
        _microstructure(microstructure)
{
    if(const PropertyObject* phaseProperty = microstructure->regions()->getProperty(SurfaceMeshRegions::PhaseProperty)) {
        for(const ElementType* type : phaseProperty->elementTypes()) {
            if(type->name() == ParticleType::getPredefinedStructureTypeName(ParticleType::PredefinedStructureType::BCC))
                _phaseStructureTypes.emplace(type->numericId(), ParticleType::PredefinedStructureType::BCC);
            else if(type->name() == ParticleType::getPredefinedStructureTypeName(ParticleType::PredefinedStructureType::FCC))
                _phaseStructureTypes.emplace(type->numericId(), ParticleType::PredefinedStructureType::FCC);
        }
    }
}

/******************************************************************************
* Determines the set of visible mesh faces,
******************************************************************************/
void SlipSurfaceVis::PrepareMeshEngine::determineVisibleFaces()
{
    // Determine which faces of the input surface mesh should be included in the
    // output triangle mesh.
	HalfEdgeMesh::size_type faceCount = inputMesh().faceCount();
    _faceSubset.resize(faceCount);
	for(HalfEdgeMesh::face_index face = 0; face < faceCount; face++) {
        _faceSubset[face] = _microstructure.isSlipSurfaceFace(face);
    }
}

/******************************************************************************
* Assigns colors to invidual mesh faces.
******************************************************************************/
void SlipSurfaceVis::PrepareMeshEngine::determineFaceColors()
{
    PropertyPtr phaseProperty = _microstructure.regionProperty(SurfaceMeshRegions::PhaseProperty);
    auto originalFace = _originalFaceMap.begin();
    for(TriMeshFace& face : _surfaceMesh.faces()) {
        int materialIndex = 0;
        int region = _microstructure.faceRegion(*originalFace);
        int phaseId = phaseProperty->getInt(region);
        const Vector3& b = _microstructure.burgersVector(*originalFace);
        auto entry = _phaseStructureTypes.find(phaseId);
        ColorA c = MicrostructurePhase::getBurgersVectorColor(entry != _phaseStructureTypes.end() ? entry->second : ParticleType::PredefinedStructureType::OTHER, b);
        auto iter = std::find(_materialColors.begin(), _materialColors.end(), c);
        if(iter == _materialColors.end()) {
            materialIndex = _materialColors.size();
            _materialColors.push_back(c);
        }
        else {
            materialIndex = std::distance(_materialColors.begin(), iter);
        }
        face.setMaterialIndex(materialIndex);
        ++originalFace;
    }

    // Increase brightness of slip surface colors.
    for(ColorA& c : _materialColors) {
        c.r() = std::min(c.r() + FloatType(0.3), FloatType(1));
        c.g() = std::min(c.g() + FloatType(0.3), FloatType(1));
        c.b() = std::min(c.b() + FloatType(0.3), FloatType(1));
    }
}

#if 0
/******************************************************************************
* Lets the visualization element render the data object.
******************************************************************************/
void SlipSurfaceVis::render(TimePoint time, const std::vector<const DataObject*>& objectStack, const PipelineFlowState& flowState, SceneRenderer* renderer, const PipelineSceneNode* contextNode)
{
    // Ignore render calls for the original Microstructure.
    // We are only interested in the RenderableSurfaceMesh.
    if(dynamic_object_cast<Microstructure>(objectStack.back()) != nullptr)
        return;

    if(renderer->isBoundingBoxPass()) {
        TimeInterval validityInterval;
        renderer->addToLocalBoundingBox(boundingBox(time, objectStack, contextNode, flowState, validityInterval));
        return;
    }

    // Get the rendering colors for the surface.
    FloatType surface_alpha = 1;
    TimeInterval iv;
    if(surfaceTransparencyController()) surface_alpha = FloatType(1) - surfaceTransparencyController()->getFloatValue(time, iv);
    ColorA color_surface(1, 1, 1, surface_alpha);

    // The key type used for caching the rendering primitive:
    using SurfaceCacheKey = std::tuple<
        CompatibleRendererGroup,	// The scene renderer
        VersionedDataObjectRef,		// Data object
        FloatType					// Alpha
    >;

    // The values stored in the vis cache.
    struct CacheValue {
        std::shared_ptr<MeshPrimitive> surfacePrimitive;
        OORef<SlipSurfacePickInfo> pickInfo;
    };

    // Get the renderable mesh.
    const RenderableSurfaceMesh* renderableMesh = dynamic_object_cast<RenderableSurfaceMesh>(objectStack.back());
    if(!renderableMesh) return;

    // Lookup the rendering primitive in the vis cache.
    auto& visCache = dataset()->visCache().get<CacheValue>(SurfaceCacheKey(renderer, objectStack.back(), surface_alpha));

    // Check if we already have a valid rendering primitive that is up to date.
    if(!visCache.surfacePrimitive || !visCache.surfacePrimitive->isValid(renderer)) {
        visCache.surfacePrimitive = renderer->createMeshPrimitive();
        auto materialColors = renderableMesh->materialColors();
        for(ColorA& c : materialColors)
            c.a() = surface_alpha;
        visCache.surfacePrimitive->setMaterialColors(materialColors);
        visCache.surfacePrimitive->setMesh(renderableMesh->surfaceMesh(), color_surface);
		visCache.surfacePrimitive->setCullFaces(true);

        // Get the original microstructure object and the pattern catalog.
        const Microstructure* microstructureObj = dynamic_object_cast<Microstructure>(renderableMesh->sourceDataObject().get());

        // Create the pick record that keeps a reference to the original data.
        visCache.pickInfo = new SlipSurfacePickInfo(this, microstructureObj, renderableMesh);
    }

    // Handle picking of triangles.
    renderer->beginPickObject(contextNode, visCache.pickInfo);
    visCache.surfacePrimitive->render(renderer);
    renderer->endPickObject();
}
#endif

/******************************************************************************
* Returns a human-readable string describing the picked object,
* which will be displayed in the status bar by OVITO.
******************************************************************************/
QString SlipSurfacePickInfo::infoString(PipelineSceneNode* objectNode, quint32 subobjectId)
{
    QString str;
#if 0
    int facetIndex = slipFacetIndexFromSubObjectID(subobjectId);
    if(facetIndex >= 0 && facetIndex < microstructureObj()->storage()->faces().size()) {
        Microstructure::Face* face = microstructureObj()->storage()->faces()[facetIndex];
        StructurePattern* structure = nullptr;
        if(patternCatalog() != nullptr) {
            structure = patternCatalog()->structureById(face->cluster()->structure);
        }
        QString formattedBurgersVector = DislocationVis::formatBurgersVector(face->burgersVector(), structure);
        str = tr("Slip vector: %1").arg(formattedBurgersVector);
        str += tr(" | Cluster Id: %1").arg(face->cluster()->id);
        if(structure) {
            str += tr(" | Crystal structure: %1").arg(structure->name());
        }
    }
#endif
    return str;
}

}	// End of namespace
}	// End of namespace
}	// End of namespace
