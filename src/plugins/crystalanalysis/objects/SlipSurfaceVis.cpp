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
    OVITO_ASSERT(originalFace == _originalFaceMap.end());

    // Increase brightness of slip surface colors.
    for(ColorA& c : _materialColors) {
        c.r() = std::min(c.r() + FloatType(0.3), FloatType(1));
        c.g() = std::min(c.g() + FloatType(0.3), FloatType(1));
        c.b() = std::min(c.b() + FloatType(0.3), FloatType(1));
    }
}

/******************************************************************************
* Create the viewport picking record for the surface mesh object.
******************************************************************************/
OORef<ObjectPickInfo> SlipSurfaceVis::createPickInfo(const SurfaceMesh* mesh, const RenderableSurfaceMesh* renderableMesh) const
{
    return new SlipSurfacePickInfo(this, mesh, renderableMesh);
}

/******************************************************************************
* Returns a human-readable string describing the picked object,
* which will be displayed in the status bar by OVITO.
******************************************************************************/
QString SlipSurfacePickInfo::infoString(PipelineSceneNode* objectNode, quint32 subobjectId)
{
    QString str;

    int facetIndex = slipFacetIndexFromSubObjectID(subobjectId);
    if(const PropertyObject* regionProperty = surfaceMesh()->faces()->getProperty(SurfaceMeshFaces::RegionProperty)) {
        if(facetIndex >= 0 && facetIndex < regionProperty->size()) {
            if(const PropertyObject* burgersVectorProperty = surfaceMesh()->faces()->getProperty(SurfaceMeshFaces::BurgersVectorProperty)) {
                int region = regionProperty->getInt(facetIndex);
                if(const PropertyObject* phaseProperty = surfaceMesh()->regions()->getProperty(SurfaceMeshRegions::PhaseProperty)) {
                    if(region >= 0 && region < phaseProperty->size()) {
                        int phaseId = phaseProperty->getInt(region);
                        if(const MicrostructurePhase* phase = dynamic_object_cast<MicrostructurePhase>(phaseProperty->elementType(phaseId))) {
                            QString formattedBurgersVector = DislocationVis::formatBurgersVector(burgersVectorProperty->getVector3(facetIndex), phase);
                            str = tr("Slip vector: %1").arg(formattedBurgersVector);
                            str += tr(" | Crystal region: %1").arg(region);
                            str += tr(" | Crystal structure: %1").arg(phase->name());
                        }
                    }
                }
            }
        }
    }

    return str;
}

}	// End of namespace
}	// End of namespace
}	// End of namespace
