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

#include <ovito/crystalanalysis/CrystalAnalysis.h>
#include <ovito/crystalanalysis/objects/ClusterGraphObject.h>
#include <ovito/crystalanalysis/objects/Microstructure.h>
#include <ovito/crystalanalysis/objects/DislocationVis.h>
#include <ovito/mesh/surface/RenderableSurfaceMesh.h>
#include <ovito/core/rendering/SceneRenderer.h>
#include <ovito/core/utilities/mesh/TriMesh.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/core/dataset/DataSetContainer.h>
#include "SlipSurfaceVis.h"

namespace Ovito { namespace CrystalAnalysis {

IMPLEMENT_OVITO_CLASS(SlipSurfaceVis);
IMPLEMENT_OVITO_CLASS(SlipSurfacePickInfo);

/******************************************************************************
* Constructor.
******************************************************************************/
SlipSurfaceVis::SlipSurfaceVis(DataSet* dataset) : SurfaceMeshVis(dataset)
{
    // Do not interpolate facet normals by default.
    setSmoothShading(false);
}

/******************************************************************************
* Constructor.
******************************************************************************/
SlipSurfaceVis::PrepareMeshEngine::PrepareMeshEngine(const SurfaceMesh* microstructure, QVector<Plane3> cuttingPlanes, bool smoothShading) :
        SurfaceMeshVis::PrepareSurfaceEngine(microstructure, false, std::move(cuttingPlanes), smoothShading, Color(1,1,1), false),
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
    ConstPropertyAccess<int> phaseProperty = _microstructure.regionProperty(SurfaceMeshRegions::PhaseProperty);
    auto originalFace = _originalFaceMap.begin();
    for(TriMeshFace& face : _surfaceMesh.faces()) {
        int materialIndex = 0;
        int region = _microstructure.faceRegion(*originalFace);
        int phaseId = phaseProperty[region];
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

    auto facetIndex = slipFacetIndexFromSubObjectID(subobjectId);
    if(ConstPropertyAccess<int> regionProperty = surfaceMesh()->faces()->getProperty(SurfaceMeshFaces::RegionProperty)) {
        if(facetIndex >= 0 && facetIndex < regionProperty.size()) {
            if(ConstPropertyAccess<Vector3> burgersVectorProperty = surfaceMesh()->faces()->getProperty(SurfaceMeshFaces::BurgersVectorProperty)) {
                int region = regionProperty[facetIndex];
                if(const PropertyObject* phaseProperty = surfaceMesh()->regions()->getProperty(SurfaceMeshRegions::PhaseProperty)) {
                    ConstPropertyAccess<int> phaseArray(phaseProperty);
                    if(region >= 0 && region < phaseArray.size()) {
                        int phaseId = phaseArray[region];
                        if(const MicrostructurePhase* phase = dynamic_object_cast<MicrostructurePhase>(phaseProperty->elementType(phaseId))) {
                            QString formattedBurgersVector = DislocationVis::formatBurgersVector(burgersVectorProperty[facetIndex], phase);
                            str = tr("Slip vector: %1").arg(formattedBurgersVector);
                            if(ConstPropertyAccess<Vector3> crystallographicNormalProperty = surfaceMesh()->faces()->getProperty(SurfaceMeshFaces::CrystallographicNormalProperty)) {
                                QString formattedNormalVector = DislocationVis::formatBurgersVector(crystallographicNormalProperty[facetIndex], phase);
                                str += tr(" | Crystallographic normal: %1").arg(formattedNormalVector);
                            }
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
